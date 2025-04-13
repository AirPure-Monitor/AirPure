#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <ThingSpeak.h>
#include <WiFi.h>

Adafruit_BME280 bme;  // Sensor BME280

HardwareSerial mhzSerial(2); // UART2 da ESP32
#define MHZ_RX 16  // GPIO16 - RX do MH-Z14
#define MHZ_TX 17  // GPIO17 - TX do MH-Z14

// Wi-Fi
const char* ssid = "DELTA_FIBRA_BMOS";
const char* password = "bmos161930";

// ThingSpeak
unsigned long myChannelNumber = 2917570;
const char * myWriteAPIKey = "Q58F71BSW629XSHP";
unsigned long lastTSUpdate = 0;
const long intervalTS = 15000; // 15 segundos

WiFiClient client;

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);  // SDA, SCL padrão ESP32
  mhzSerial.begin(9600, SERIAL_8N1, MHZ_RX, MHZ_TX);

  Serial.println("Inicializando sensores...");

  if (!bme.begin(0x76)) {
    Serial.println("Sensor BME280/BMP280 nao encontrado!");
    while (1);
  }

  Serial.println("Sensores prontos!");

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando no Wi-Fi...");
  }
  Serial.println("Wi-Fi conectado!");

  ThingSpeak.begin(client);

}

void loop() {
  Serial.println("----- Leitura dos Sensores -----");

  Serial.print("Temperatura: ");
  Serial.print(bme.readTemperature());
  Serial.println(" *C");

  Serial.print("Pressao: ");
  Serial.print(bme.readPressure() / 100.0F);
  Serial.println(" hPa");

  Serial.print("Umidade: ");
  Serial.print(bme.readHumidity());
  Serial.println(" %");

  float altitude = bme.readAltitude(1013.25); // Pressão ao nível do mar
  Serial.print("Altitude: ");
  Serial.println(altitude);

  // Leitura do MH-Z14
  byte cmd[9] = { 0xFF, 0x01, 0x86, 0, 0, 0, 0, 0, 0x79 };
  mhzSerial.write(cmd, 9);
  delay(10);
  int ppm;

  if (mhzSerial.available() >= 9) {
    byte response[9];
    mhzSerial.readBytes(response, 9);
    if (response[0] == 0xFF && response[1] == 0x86) {
      ppm = (response[2] * 256) + response[3];
      Serial.print("CO2: ");
      Serial.print(ppm);
      Serial.println(" ppm");
    }
  }

  if (millis() - lastTSUpdate > intervalTS) {
    ThingSpeak.setField(1, bme.readTemperature());
    ThingSpeak.setField(2, bme.readPressure() / 100.0F);
    ThingSpeak.setField(3, bme.readHumidity());
    ThingSpeak.setField(4, bme.readAltitude(1013.25));
    ThingSpeak.setField(5, ppm);

    int httpResponseCode  = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);

    if (httpResponseCode == 200) {
      Serial.println("Dados enviados para o ThingSpeak!");
    } else {
      Serial.println("Erro ao enviar. Codigo HTTP: " + String(httpResponseCode));
    }

    lastTSUpdate = millis();
  }

  Serial.println("------------------------------");
  delay(5000); // Atualiza leitura local a cada 1 segundo

}
