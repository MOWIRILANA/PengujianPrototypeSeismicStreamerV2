#include <Ethernet.h>
#include <SPI.h>

// W5500 settings
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; // MAC address
IPAddress ip(0, 0, 0, 0);                      // Static IP address ESP32
IPAddress serverIP(169, 254, 98, 18);                // IP Laptop (server)
IPAddress gateway(192, 168, 43, 1);                   // Gateway
IPAddress subnet(255, 255, 0, 0);                  // Subnet mask

EthernetClient client;

void setup() {
  Serial.begin(115200);

  // Initialize Ethernet
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet. Using static IP...");
    Ethernet.begin(mac, ip, gateway, gateway, subnet);
  }

  Serial.print("ESP32 IP Address: ");
  Serial.println(Ethernet.localIP());

  // Connect to server
  if (client.connect(serverIP, 5000)) {
    Serial.println("Connected to server.");
  } else {
    Serial.println("Connection to server failed.");
  }
}

void loop() {
  if (client.connected()) {
    // Kirim data ke server
    String message = "Hello from ESP32!";
    client.println(message);
    Serial.println("Sent: " + message);

    // Tunggu respon dari server
    if (client.available()) {
      String response = client.readStringUntil('\n');
      Serial.println("Received from server: " + response);
    }
  } else {
    Serial.println("Disconnected from server. Retrying...");
    client.connect(serverIP, 5000);
  }

  delay(2000); // Kirim data setiap 2 detik
}
