#include <SPI.h>
#include <Ethernet.h>

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; // Unique MAC address
IPAddress ip(192, 168, 43, 157); // Static IP address
EthernetServer server(80);

void setup() {
  Ethernet.begin(mac, ip);
  server.begin();
  Serial.begin(115200);
  Serial.println("Server is up at 192.168.43.157");
}

void loop() {
  EthernetClient client = server.available();
  if (client) {
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c); // Echo data
        client.println("Hello from W5500!");
        delay(10);
      }
    }
    client.stop();
  }
}
