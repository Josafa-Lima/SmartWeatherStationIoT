# SmartWeather Station IoT

Projeto Final desenvolvido para a disciplina, com ESP32 simulado na plataforma **Wokwi**.

## Descrição do Projeto

Sistema IoT de monitoramento ambiental que:
- Lê temperatura e umidade locais via sensor **DHT22**
- Consome a API pública **Open-Meteo** (sem autenticação) via HTTPS
- Exibe dados locais no **LCD 16x2 I2C**
- Exibe dados completos (locais + API + status) no **Display TFT ILI9341**
- Indica status da conexão via **LED Verde/Vermelho**
- Emite alerta sonoro via **Buzzer** quando a temperatura excede o limite (30°C)
- Atualiza dados da API automaticamente a cada **30 segundos**

## Arquitetura da Solução

```
[DHT22] --> [ESP32] --> [LCD 16x2 I2C]
                |  \--> [TFT ILI9341]
                |  \--> [LED Verde/Vermelho]
                |  \--> [Buzzer]
                |
            [Wi-Fi / HTTPS]
                |
        [API Open-Meteo - JSON]
```

## Componentes Utilizados

| Componente        | Função                                  | Pino ESP32 (dados) |
|-------------------|------------------------------------------|---------------------|
| ESP32             | Microcontrolador principal               | -                   |
| DHT22             | Leitura de temperatura e umidade locais  | GPIO 16 (RX2)       |
| LCD 16x2 I2C      | Exibição rápida de dados locais          | GPIO 21 (SDA) / 22 (SCL) |
| TFT ILI9341       | Exibição completa (locais + API + status)| CS:5 DC:2 RST:4 SCK:18 MOSI:23 MISO:19 |
| LED Verde/Vermelho| Indicador de status da conexão           | GPIO 25 / 26        |
| Buzzer            | Alerta sonoro de temperatura             | GPIO 27             |

## APIs Utilizadas

- **Open-Meteo** (https://open-meteo.com/) — dados meteorológicos gratuitos, sem autenticação, retorno em JSON.
  Endpoint utilizado:
  ```
  https://api.open-meteo.com/v1/forecast?latitude=-8.05&longitude=-34.88&current_weather=true
  ```

## Configuração do LCD I2C

O endereço I2C do LCD 16x2 utilizado neste projeto é **0x27** (confirmado via scanner I2C, disponível em `i2c_scanner.ino`). Caso utilize outro módulo I2C, rode o scanner para confirmar o endereço correto e ajuste a linha no `sketch.ino`:
```cpp
LiquidCrystal_I2C lcd(0x27, 16, 2);
```

## Bibliotecas Necessárias

Instale via Library Manager do Wokwi/Arduino:
- `ArduinoJson`
- `DHT sensor library` (Adafruit) + `Adafruit Unified Sensor`
- `LiquidCrystal I2C`
- `Adafruit GFX Library`
- `Adafruit ILI9341`

## Instruções de Execução (Wokwi)

1. Acesse https://wokwi.com/ e crie um novo projeto **ESP32**.
2. Substitua o conteúdo de `sketch.ino` pelo código deste repositório.
3. Crie/substitua o arquivo `diagram.json` pelo deste repositório (define as conexões do circuito).
4. Adicione as bibliotecas listadas em `libraries.txt` (clique no ícone de bibliotecas na lateral esquerda, busque cada uma e adicione).
5. Clique em **"Start Simulation"** (botão de play verde).
6. **Para visualizar as mensagens (Serial Monitor):**
   - Ele deve abrir automaticamente na parte inferior da tela ao iniciar a simulação.
   - Se não aparecer, clique no ícone de **terminal** localizado na barra inferior da janela de simulação, ou vá em **"View" > "Serial Monitor"** no menu superior.
   - Certifique-se de que a velocidade (baud rate) está em **115200**.
7. Aguarde a conexão Wi-Fi (rede simulada `Wokwi-GUEST`, sem senha) e a primeira resposta da API no Serial Monitor.

## Funcionamento

- O **LCD 16x2** exibe continuamente: `Temp: XX.XC` e `Umid: XX%`.
- O **TFT** exibe temperatura/umidade local, temperatura da API, horário da última atualização e status da conexão (ONLINE/OFFLINE).
- O **LED verde** acende quando a última consulta à API foi bem-sucedida; o **LED vermelho** acende em caso de falha.
- O **buzzer** soa quando a temperatura local ultrapassa 30°C.
- A API é consultada automaticamente a cada **30 segundos**.

## Tratamento de Erros

- Falhas na leitura do DHT22 são detectadas (`isnan`) e reportadas no Serial.
- Falhas de conexão Wi-Fi tentam reconexão automática antes de cada requisição.
- Códigos HTTP diferentes de 200 e erros de parsing JSON são tratados e logados, acionando o LED vermelho.

## Link da Simulação


> `https://wokwi.com/projects/466834444149711873`

## Vídeo Demonstrativo

https://youtu.be/sXIjulYpAgQ?si=XsOlgJ5YTOzEQBOk.



