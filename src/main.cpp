#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <ThingSpeak.h>
#include <WiFi.h>
#include <WiFiManager.h>  // Biblioteca para configurar Wi-Fi via Portal Captive

Adafruit_BME280 bme;  // Sensor BME280
HardwareSerial mhzSerial(2); // UART2 da ESP32
#define MHZ_RX 16  // GPIO16 - RX do MH-Z14
#define MHZ_TX 17  // GPIO17 - TX do MH-Z14

// ThingSpeak
unsigned long myChannelNumber = 2917570;
const char * myWriteAPIKey = "Q58F71BSW629XSHP";
unsigned long lastTSUpdate = 0;
const long intervalTS = 300000; // 5 minutos

// Redes salvas manualmente
const char* ssid1 = "DELTA_FIBRA_BMOS";
const char* pass1 = "bmos161930";
const char* ssid2 = "iPhone de Bruna Michelly";
const char* pass2 = "16193016";

bool conectarRede(const char* ssid, const char* pass) {
  WiFi.begin(ssid, pass);
  Serial.printf("Tentando conectar em: %s...\n", ssid);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  return WiFi.status() == WL_CONNECTED;
}

WiFiClient client;
int lastValidCO2 = 400; // Valor inicial razoável

void setup() {

  Serial.begin(115200);
  Wire.begin(21, 22);  // SDA, SCL padrão ESP32
  mhzSerial.begin(9600, SERIAL_8N1, MHZ_RX, MHZ_TX);

  Serial.println("Inicializando sensores...");
  if (!bme.begin(0x76)) {
    Serial.println("Sensor BME280/BMP280 nao encontrado!");
    while (1); // Fica travado se não encontrar o sensor
  }
  Serial.println("Sensores prontos!");

  // ---------- Configuração de Wi-Fi com WiFiManager ----------
  bool conectado = false;

  // Tenta rede 1
  if (conectarRede(ssid1, pass1)) {
    Serial.println("Conectado na rede 1");
    conectado = true;
  }

  // Se falhar, tenta rede 2
  if (!conectado && conectarRede(ssid2, pass2)) {
    Serial.println("Conectado na rede 2");
    conectado = true;
  }

  // Se nenhuma funcionar, entra no modo configuração
  if (!conectado) {
    Serial.println("Nenhuma rede conectada. Abrindo modo configuração...");
    WiFiManager wm;
    wm.autoConnect("AirPure-Setup", "12345678");
    Serial.println("Conectado após configuração!");
  }

  Serial.println("IP: " + WiFi.localIP().toString());
  // -----------------------------------------------------------

  ThingSpeak.begin(client);
}


void loop() {
  Serial.println("----- Leitura dos Sensores -----");

  float temperatura = bme.readTemperature();
  float pressao = bme.readPressure() / 100.0F;
  float umidade = bme.readHumidity();
  float altitude = bme.readAltitude(1013.25);

  Serial.printf("Temp.: %.2f *C\n", temperatura);
  Serial.printf("Pressao: %.2f hPa\n", pressao);
  Serial.printf("Umidade: %.2f %%\n", umidade);
  Serial.printf("Altitude: %.2f m\n", altitude);

  // Leitura do MH-Z14
  byte cmd[9] = { 0xFF, 0x01, 0x86, 0, 0, 0, 0, 0, 0x79 };
  mhzSerial.write(cmd, 9);
  delay(10);

  int ppm = lastValidCO2;
  if (mhzSerial.available() >= 9) {
    byte response[9];
    mhzSerial.readBytes(response, 9);
    if (response[0] == 0xFF && response[1] == 0x86) {
      int readCO2 = (response[2] * 256) + response[3];
      if (readCO2 >= 300 && readCO2 <= 5000) {
        ppm = readCO2;
        lastValidCO2 = ppm;
      } else {
        Serial.println("Valor de CO2 fora da faixa. Mantendo última leitura válida.");
      }
    }
  }

  Serial.printf("CO2: %d ppm\n", ppm);

  // Envio para ThingSpeak a cada 5 minutos
  if (millis() - lastTSUpdate > intervalTS) {
    ThingSpeak.setField(1, temperatura);
    ThingSpeak.setField(2, pressao);
    ThingSpeak.setField(3, umidade);
    ThingSpeak.setField(4, altitude);
    ThingSpeak.setField(5, ppm);

    int httpResponseCode = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
    if (httpResponseCode == 200) {
      Serial.println("Dados enviados para o ThingSpeak!");
    } else {
      Serial.println("Erro ao enviar. Codigo HTTP: " + String(httpResponseCode));
    }
    lastTSUpdate = millis();
  }

  Serial.println("------------------------------");
  delay(5000); // Atualiza leitura local a cada 5 segundos
}
