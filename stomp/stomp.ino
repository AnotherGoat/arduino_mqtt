#include <WiFi101.h>
#include "secrets.h"

const char ssid[] = SECRET_SSID;
const char pass[] = SECRET_PASS;

const char* HOST = "192.168.1.100";
const int STOMP_PORT = 61613;

WiFiClient client;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  if (client.connect(HOST, STOMP_PORT)) {
    Serial.println("Connected to STOMP broker.");

    client.print("CONNECT\n");
    client.print("accept-version:1.2\n");
    client.print("host:/\n\n");
    client.write('\0');

    delay(500);

    client.print("SEND\n");
    client.print("destination:/queue/a\n");
    client.print("content-type:text/plain\n\n");
    client.print("Hello world\n");
    client.write('\0'); // End of frame

    Serial.println("Message sent!");
  } else {
    Serial.println("Connection failed.");
  }
}

void loop() {
  // Do nothing
}
