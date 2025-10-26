#include <SPI.h>
#include <ArduinoMqttClient.h>
#include <ArduinoCommon.h>
#include <secrets.h>

const char ssid[] = SECRET_SSID;
const char pass[] = SECRET_PASS;

const char BROKER[] = "test.mosquitto.org";
const int MQTT_PORT = 1883;
const char TOPIC[] = "ufroicc452/tvf";

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

TaskHandle_t handleSubscriberTask;

void setup() {
  initSerial();
  connectWiFi(ssid, pass);
  connectMQTT(mqttClient, BROKER, MQTT_PORT);

  vSetErrorLed(LED_BUILTIN, HIGH);
  vSetErrorSerial(&Serial);

  xTaskCreate(subscriberTask, "Subscriber Task", 256, NULL, tskIDLE_PRIORITY + 1, &handleSubscriberTask);

  vTaskStartScheduler();
}

static void subscriberTask(void *pvParameters) {
    initSubscriber(mqttClient, TOPIC);

    while (true) {
        processMessage(mqttClient);
        delayMs(1); 
    }
}

void initSubscriber(MqttClient& client, const char* topic) {
    Serial.print("Subscribing to topic: ");
    Serial.println(topic);
    client.subscribe(topic);

    Serial.print("Waiting for messages on topic ");
    Serial.print(topic);
    Serial.println("...");
}

void processMessage(MqttClient& client) {
    int messageSize = client.parseMessage();
    if (messageSize == 0) return;

    Serial.print("Received a message! Topic: '");
    Serial.print(client.messageTopic());
    Serial.print("', Length: ");
    Serial.print(messageSize);
    Serial.println(" bytes");

    while (client.available()) {
        Serial.print((char)client.read());
    }

    Serial.println();
    Serial.println();
}

void loop() {}
