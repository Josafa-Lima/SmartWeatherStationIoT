/*
  ============================================================
  SmartWeather Station IoT - ESP32
  Projeto Final - Prof. João Ferreira
  ============================================================
  Componentes:
    - ESP32
    - Sensor DHT22 (temperatura e umidade)
    - LCD 16x2 I2C
    - Display TFT ILI9341
    - LED (verde = OK, vermelho = falha)
    - Buzzer (alerta sonoro)

  API utilizada: Open-Meteo (gratuita, sem autenticação)
  Endpoint:
    https://api.open-meteo.com/v1/forecast?latitude=-8.05&longitude=-34.88&current_weather=true

  IMPORTANTE (Wokwi):
    - O Serial Monitor abre automaticamente ao clicar em "Start Simulation".
    - Se não aparecer, clique no ícone de "terminal" na barra inferior
      da aba de simulação (ao lado do botão de play) ou use o menu
      "View > Serial Monitor".
    - Para requisições HTTPS funcionarem no Wokwi, é necessário usar
      WiFiClientSecure com setInsecure() (sem validação de certificado),
      pois o Wokwi não possui RTC sincronizado para validar TLS.
  ============================================================
*/

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

// ---------------------- CONFIGURAÇÕES ----------------------

// Wi-Fi (no Wokwi, a rede simulada padrão é "Wokwi-GUEST" sem senha)
const char* ssid     = "Wokwi-GUEST";
const char* password = "";

// API Open-Meteo - Recife (PE)
const char* apiHost = "api.open-meteo.com";
const char* apiPath = "/v1/forecast?latitude=-8.05&longitude=-34.88&current_weather=true";

// Pinos
#define DHTPIN   16
#define DHTTYPE  DHT22

#define LED_VERDE   25
#define LED_VERMELHO 26
#define BUZZER_PIN  27

// TFT ILI9341 - pinos SPI
#define TFT_CS   5
#define TFT_DC   2
#define TFT_RST  4

// Limite de temperatura para alerta sonoro
#define TEMP_LIMITE 30.0

// Intervalo de atualização da API (ms)
#define INTERVALO_API 30000

// ---------------------- OBJETOS ----------------------

DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2); // Endereço confirmado via I2C scanner
Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_RST);

// ---------------------- VARIÁVEIS GLOBAIS ----------------------

float tempLocal = 0;
float umidLocal = 0;
float tempAPI = 0;
String horaAtualizacao = "--:--:--";
bool statusConexao = false;

unsigned long ultimaAtualizacaoAPI = 0;

// ---------------------- FUNÇÕES ----------------------

void conectaWiFi() {
  Serial.println();
  Serial.print("Conectando ao Wi-Fi");
  WiFi.begin(ssid, password);

  int tentativas = 0;
  while (WiFi.status() != WL_CONNECTED && tentativas < 30) {
    delay(500);
    Serial.print(".");
    tentativas++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.print("Wi-Fi conectado! IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println();
    Serial.println("Falha ao conectar ao Wi-Fi.");
  }
}

// Faz a requisição HTTPS para a API Open-Meteo e extrai a temperatura atual
bool consultaAPIClima() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[API] Wi-Fi desconectado. Tentando reconectar...");
    conectaWiFi();
  }

  WiFiClientSecure client;
  client.setInsecure(); // Necessário no Wokwi (sem RTC válido para checar certificado)

  HTTPClient http;
  String url = String("https://") + apiHost + apiPath;

  Serial.println("[API] Conectando em: " + url);

  if (!http.begin(client, url)) {
    Serial.println("[API] Erro ao iniciar conexão HTTP.");
    return false;
  }

  int httpCode = http.GET();
  Serial.printf("[API] Codigo de resposta HTTP: %d\n", httpCode);

  if (httpCode != HTTP_CODE_OK) {
    Serial.println("[API] Erro na requisição HTTP.");
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  Serial.println("[API] Resposta JSON recebida:");
  Serial.println(payload);

  // Faz o parse do JSON usando ArduinoJson
  DynamicJsonDocument doc(2048);
  DeserializationError error = deserializeJson(doc, payload);

  if (error) {
    Serial.print("[API] Erro ao interpretar JSON: ");
    Serial.println(error.c_str());
    return false;
  }

  // Extrai dados do clima atual
  tempAPI = doc["current_weather"]["temperature"];
  String tempoISO = doc["current_weather"]["time"]; // formato: AAAA-MM-DDTHH:MM

  // Extrai apenas o horário (HH:MM) da string ISO
  int posT = tempoISO.indexOf('T');
  if (posT != -1) {
    horaAtualizacao = tempoISO.substring(posT + 1) + ":00";
  } else {
    horaAtualizacao = tempoISO;
  }

  Serial.printf("[API] Temperatura API: %.1f C | Hora: %s\n", tempAPI, horaAtualizacao.c_str());

  return true;
}

// Atualiza leitura dos sensores locais (DHT22)
void leSensores() {
  float t, h;
  int tentativas = 0;

  do {
    t = dht.readTemperature();
    h = dht.readHumidity();

    if (!isnan(t) && !isnan(h)) {
      tempLocal = t;
      umidLocal = h;
      Serial.printf("[DHT22] Temp: %.1f C | Umidade: %.1f %%\n", tempLocal, umidLocal);
      return;
    }

    tentativas++;
    delay(500); // Aguarda antes de tentar de novo (DHT22 precisa de ~2s entre leituras)
  } while (tentativas < 3);

  Serial.println("[DHT22] Falha na leitura do sensor!");
}

// Atualiza o LCD 16x2 com dados locais
void atualizaLCD() {
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(tempLocal, 1);
  lcd.print((char)223); // símbolo de grau
  lcd.print("C   ");

  lcd.setCursor(0, 1);
  lcd.print("Umid: ");
  lcd.print(umidLocal, 0);
  lcd.print("%      ");
}

// Atualiza o display TFT com todas as informações
void atualizaTFT() {
  tft.fillScreen(ILI9341_BLACK);

  tft.setCursor(10, 10);
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_CYAN);
  tft.println("SmartWeather IoT");

  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE);

  tft.setCursor(10, 50);
  tft.print("Temp Local: ");
  tft.print(tempLocal, 1);
  tft.println(" C");

  tft.setCursor(10, 80);
  tft.print("Umid Local: ");
  tft.print(umidLocal, 0);
  tft.println(" %");

  tft.setCursor(10, 110);
  tft.print("Temp API:   ");
  tft.print(tempAPI, 1);
  tft.println(" C");

  tft.setCursor(10, 140);
  tft.print("Atualizado:");
  tft.setCursor(10, 165);
  tft.println(horaAtualizacao);

  tft.setCursor(10, 200);
  tft.print("Status: ");
  if (statusConexao) {
    tft.setTextColor(ILI9341_GREEN);
    tft.println("ONLINE");
  } else {
    tft.setTextColor(ILI9341_RED);
    tft.println("OFFLINE");
  }
}

// Controla LED indicador de status de conexão
void atualizaLED() {
  if (statusConexao) {
    digitalWrite(LED_VERDE, HIGH);
    digitalWrite(LED_VERMELHO, LOW);
  } else {
    digitalWrite(LED_VERDE, LOW);
    digitalWrite(LED_VERMELHO, HIGH);
  }
}

// Verifica limite de temperatura e aciona buzzer
void verificaAlerta() {
  if (tempLocal > TEMP_LIMITE) {
    digitalWrite(BUZZER_PIN, HIGH);
    Serial.println("[ALERTA] Temperatura acima do limite! Buzzer ativado.");
  } else {
    digitalWrite(BUZZER_PIN, LOW);
  }
}

// ---------------------- SETUP ----------------------

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("=== SmartWeather Station IoT - Iniciando ===");

  // Pinos de saída
  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_VERMELHO, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_VERMELHO, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  // Sensor DHT22
  dht.begin();
  delay(2000); // Tempo de estabilização do sensor (necessário no Wokwi)

  // LCD 16x2 I2C
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("SmartWeather IoT");
  lcd.setCursor(0, 1);
  lcd.print("Iniciando...");

  // TFT ILI9341
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(10, 10);
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_CYAN);
  tft.println("SmartWeather IoT");
  tft.setCursor(10, 40);
  tft.setTextColor(ILI9341_WHITE);
  tft.println("Conectando Wi-Fi...");

  // Conecta ao Wi-Fi
  conectaWiFi();
  statusConexao = (WiFi.status() == WL_CONNECTED);
  atualizaLED();

  // Primeira leitura
  delay(2000);
  leSensores();

  // Primeira consulta à API
  statusConexao = consultaAPIClima();
  atualizaLED();

  atualizaLCD();
  atualizaTFT();

  ultimaAtualizacaoAPI = millis();
}

// ---------------------- LOOP ----------------------

void loop() {
  // Sempre atualiza leitura local e LCD
  leSensores();
  atualizaLCD();
  verificaAlerta();

  // Atualiza API a cada INTERVALO_API (30 segundos)
  if (millis() - ultimaAtualizacaoAPI >= INTERVALO_API) {
    statusConexao = consultaAPIClima();
    atualizaLED();
    atualizaTFT();
    ultimaAtualizacaoAPI = millis();
  }

  delay(2000); // Pequeno intervalo entre leituras locais
}