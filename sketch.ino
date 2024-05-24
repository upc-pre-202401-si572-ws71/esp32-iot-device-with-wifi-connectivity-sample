#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include "time.h"

// WiFi Simulator Credentials
#define WIFI_SSID "Wokwi-GUEST"
#define WIFI_PASSWORD ""

// Firebase Device Status Database URL
#define DATABASE_URL "https://upcpre202401si572ws71iotstatus-default-rtdb.firebaseio.com/.json"

// Create HTTP Cient
HTTPClient client;

// Distance Simulator Configuration
#define triggerPin 5
#define echoPin 18
#define safeLed 2
#define unsafeLed 4

// Display Configuration
LiquidCrystal_I2C LCD = LiquidCrystal_I2C(0x27, 16, 2);

// Operation variables
int previousValue = 0;
String tempText = "";
String payload = "";

String sensorID = "HC001";
char timeStringBuffer[20];

String safeStatus;
String unsafeStatus;
String distanceStatus;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("ESP32-base IoT Distance Sensor Device");

  // LCD Setup
  LCD.init();
  LCD.backlight();
  LCD.setCursor(0, 0);
  LCD.print("Connecting to ");
  LCD.setCursor(0, 1);
  LCD.print("WiFi ");

  // WiFi Setup
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  } 

  Serial.println();
  Serial.print("Connected: ");
  Serial.println(WiFi.localIP());

  // Status Database Connectivity

  // Step 1. Display status
  LCD.clear();
  LCD.setCursor(0, 0);
  LCD.println("Online");
  delay(500);
  LCD.clear();
  LCD.setCursor(0,0);
  LCD.println("Connecting to");
  LCD.setCursor(0, 1);
  LCD.println("Firebase...");
  Serial.println("Connecting...");

  // Step 2. Perform connection
  client.begin(DATABASE_URL);
  int httpResponseCode = client.GET();

  if (httpResponseCode > 0) {
    LCD.clear();
    LCD.setCursor(0, 0);
    LCD.println("Connected");
    Serial.print("Connected.");
    Serial.println("Firebase payload: ");
    payload = client.getString();
    Serial.println(payload);
    Serial.println();
  }
  // Sensor Pin Configuration
  pinMode(triggerPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(safeLed, OUTPUT);
  pinMode(unsafeLed, OUTPUT);

  // Data transmit
  for (int i = 0; i < 5; i++) {
    digitalWrite(safeLed, HIGH);
    digitalWrite(unsafeLed, HIGH);
    delay(200);
    digitalWrite(safeLed, LOW);
    digitalWrite(unsafeLed, LOW);
    delay(200);
  }

  // Time Client Configuration
  configTime(-9000, -9000, "1.south-america.pool.ntp.org");
}

void loop() {
  // Process init
  long sensorUltrasoundValue;
  // Start ultrasound
  digitalWrite(triggerPin, LOW);
  delayMicroseconds(2);
  digitalWrite(triggerPin, HIGH);
  // Stop ultrasound
  delayMicroseconds(2);
  digitalWrite(triggerPin, LOW); 
  // Read ultrasound distance 
  sensorUltrasoundValue = pulseIn(echoPin, HIGH);
  long sensorMetricValue = (sensorUltrasoundValue/2)/29.1;

  if (previousValue != sensorMetricValue) {
    if (sensorMetricValue > 200 || sensorMetricValue < 0) {
      digitalWrite(safeLed, HIGH);
      digitalWrite(unsafeLed, LOW);
      tempText = "Safe distance: ";
      safeStatus = "true";
      unsafeStatus = "false";
    } else {
      digitalWrite(safeLed, LOW);
      digitalWrite(unsafeLed, HIGH);
      tempText = "Unsafe distance: ";
      safeStatus = "false";
      unsafeStatus = "true";
    }
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
      Serial.println("Time Error");
    }
    strftime(timeStringBuffer, sizeof(timeStringBuffer), "%d/%m/%Y %H:%M", &timeinfo);
    Serial.println(String(timeStringBuffer));

    // Send information to Database
    client.PATCH("{\"Status/Sensors/time\":\"" + String(timeStringBuffer) + "\"}");
    client.PATCH("{\"Status/Led/safeLed\":" + safeStatus + "}");
    client.PATCH("{\"Status/Led/unsafeLed\":" + unsafeStatus + "}");
    client.PATCH("{\"Status/Sensors/Distance\":" + String(sensorMetricValue) + "}");

    LCD.clear();
    LCD.setCursor(0, 0);
    LCD.print(tempText);
    LCD.print(sensorMetricValue);
    LCD.setCursor(0, 1);
    LCD.println(" cm");
  }

  previousValue = sensorMetricValue;
  delay(500);

}
