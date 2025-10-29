#include <WiFi101.h>
#include "secrets.h"

const char ssid[] = SECRET_SSID;
const char pass[] = SECRET_PASS;

const char* host = "192.168.1.100";
const int port = 5672;

WiFiClient client;

void setup() {
  Serial.begin(115200);
  while (!Serial);
  
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  if (client.connect(host, port)) {
    Serial.println("Connected to AMQP broker");

    const uint8_t amqp_header[] = {'A', 'M', 'Q', 'P', 0, 0, 9, 1};
    client.write(amqp_header, sizeof(amqp_header));
    Serial.println("Sent AMQP header.");

    while (client.available() == 0) delay(10);

    Serial.print("Received: ");
    
    while (client.available()) {
      Serial.print(client.read(), HEX);
      Serial.print(" ");
    }
    
    Serial.println();
  } else {
    Serial.println("Connection failed.");
  }
}

void loop() {}
