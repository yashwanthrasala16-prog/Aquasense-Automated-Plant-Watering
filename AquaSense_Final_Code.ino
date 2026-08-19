#include <WiFi.h>
#include <ThingSpeak.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

unsigned long channelNumber = YOUR_CHANNEL_NUMBER;
const char* writeAPIKey = "YOUR_WRITE_API_KEY";

WiFiClient client;

const int moisturePin = 34;
const int relayPin = 26;

const int SDA_PIN = 19;
const int SCL_PIN = 22;

const int DRY_VALUE = 4095;
const int WET_VALUE = 1500;

const int PUMP_ON_THRESHOLD = 30;
const int PUMP_OFF_THRESHOLD = 60;

unsigned long lastThingSpeakUpdate = 0;
const unsigned long thingSpeakInterval = 20000;

bool pumpState = false;

void setup() {
  Serial.begin(115200);

  Wire.begin(SDA_PIN, SCL_PIN);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED Failed!");
    while (true);
  }

  display.clearDisplay();
  display.setTextColor(WHITE);

  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, HIGH); // Active LOW relay: OFF

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi Connected!");
  Serial.println(WiFi.localIP());

  ThingSpeak.begin(client);

  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println("AquaSense");
  display.setTextSize(1);
  display.setCursor(0, 30);
  display.println("System Ready");
  display.display();

  delay(1000);
}

void loop() {

  int sensorValue = analogRead(moisturePin);

  int moisturePercent = map(
    sensorValue,
    DRY_VALUE,
    WET_VALUE,
    0,
    100
  );

  moisturePercent = constrain(moisturePercent, 0, 100);

  // Automatic pump control with hysteresis
  if (moisturePercent <= PUMP_ON_THRESHOLD) {
    pumpState = true;
    digitalWrite(relayPin, LOW);
  }
  else if (moisturePercent >= PUMP_OFF_THRESHOLD) {
    pumpState = false;
    digitalWrite(relayPin, HIGH);
  }

  Serial.print("Sensor: ");
  Serial.print(sensorValue);
  Serial.print(" | Moisture: ");
  Serial.print(moisturePercent);
  Serial.print("% | Pump: ");
  Serial.println(pumpState ? "ON" : "OFF");

  // OLED updates immediately
  display.clearDisplay();

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("AquaSense");

  display.setTextSize(2);
  display.setCursor(0, 16);
  display.print(moisturePercent);
  display.print("%");

  display.setTextSize(1);
  display.setCursor(0, 45);
  display.print("Pump: ");
  display.print(pumpState ? "ON" : "OFF");

  display.display();

  // ThingSpeak update every 20 seconds
  if (millis() - lastThingSpeakUpdate >= thingSpeakInterval) {

    if (WiFi.status() == WL_CONNECTED) {

      ThingSpeak.setField(1, moisturePercent);

      int response = ThingSpeak.writeFields(
        channelNumber,
        writeAPIKey
      );

      if (response == 200) {
        Serial.println("ThingSpeak upload successful");
      } else {
        Serial.print("ThingSpeak error: ");
        Serial.println(response);
      }

    } else {
      Serial.println("WiFi disconnected. Reconnecting...");
      WiFi.reconnect();
    }

    lastThingSpeakUpdate = millis();
  }
}
