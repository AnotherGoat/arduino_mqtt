#include <SPI.h>
#include <WiFi101.h>
#include <ArduinoCommon.h>
#include <secrets.h>

const char ssid[] = SECRET_SSID;
const char pass[] = SECRET_PASS;

const char BROKER[] = "test.mosquitto.org";
const int MQTT_PORT = 1883;
const char TOPIC[] = "ufroicc452/tvf";
const long INTERVAL = 250;
const int POTENTIOMETER_PIN = A1;

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

TaskHandle_t handlePotentiometerTask;

void setup() {
  initSerial();
  connectWiFi(ssid, pass);
  connectMQTT(mqttClient, BROKER, MQTT_PORT);

  vSetErrorLed(LED_BUILTIN, HIGH);
  vSetErrorSerial(&Serial);

  xTaskCreate(potentiometerTask, "Potentiometer Task", 256, NULL, tskIDLE_PRIORITY + 1, &handlePotentiometerTask);

  vTaskStartScheduler();
}

static void potentiometerTask(void *pvParameters) {
  pinMode(POTENTIOMETER_PIN, INPUT);
  Serial.println("Potentiometer task started");

  while (true) {
    mqttClient.poll();

    Serial.print("Sending a message! Topic: '");
    Serial.print(TOPIC);
    Serial.println("'");

    int value = analogRead(POTENTIOMETER_PIN);
    int angle = map(value, 0, 1023, 0, 180);

    Serial.print("Angle: ");
    Serial.println(angle);

    mqttClient.beginMessage(TOPIC);
    mqttClient.print("Angle: ");
    mqttClient.print(angle);
    mqttClient.endMessage();

    Serial.println();
    Serial.println();

    delayMs(INTERVAL);
  }
}

void loop() {}
