#include "secrets.h"
#include <WiFiClientSecure.h>
#include <MQTTClient.h>
#include <ArduinoJson.h>
#include "WiFi.h"

// The MQTT topics that this device should publish/subscribe
#define AWS_IOT_PUBLISH_TOPIC "esp32/pub"
#define AWS_IOT_SUBSCRIBE_TOPIC "esp32/sub"

WiFiClientSecure net = WiFiClientSecure();
MQTTClient client = MQTTClient(256);

#define TEMP_SENSOR_PIN 34  // Analog pin for Temp36 sensor
#define VOLTAGE 3000  // Voltage in mV provided to the sensor (5V)

void connectAWS() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.println("Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Configure WiFiClientSecure to use the AWS IoT device credentials
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);


  client.begin(AWS_IOT_ENDPOINT, 8883, net);

  // Create a message handler
  client.onMessage(messageHandler);

  Serial.print("Connecting to AWS IOT");

  while (!client.connect(THINGNAME)) {
    Serial.print(".");
    delay(100);
  }

  if (!client.connected()) {
    Serial.println("AWS IoT Timeout!");
    return;
  }

  // Subscribe to a topic
  client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);

  Serial.println("AWS IoT Connected!");
}

float readTemperature() {
  int sensorValue = analogRead(TEMP_SENSOR_PIN);
  float voltage = sensorValue / 1023.0;
  float temperature = (voltage - 0.5) * 100;   
  return temperature;
}

void publishMessage() {
  float temperature = readTemperature();
  float roundedTemperature = round(temperature * 100.0) / 100.0;  // Round to two decimal places
  Serial.print("Temperature: ");
  Serial.println(roundedTemperature);


  StaticJsonDocument<200> doc;
  doc["Temperature"] = temperature;
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer);

  client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
}

void messageHandler(String &topic, String &payload) {
  Serial.println("incoming: " + topic + " - " + payload);

  StaticJsonDocument<200> doc;
  deserializeJson(doc, payload);
  const char *message = doc["message"];

  Serial.println(message);
}

void setup() {
  Serial.begin(115200);
  connectAWS();
}

void loop() {
  connectAWS();
  publishMessage();
  client.loop();
  delay(3600000);
}

