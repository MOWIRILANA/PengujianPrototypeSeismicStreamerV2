#include <SPI.h>
#include "Ethernet_Generic.h"

// Alamat MAC dan IP
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x02 };
IPAddress ip(10, 14, 1, 21);
IPAddress subnet(255, 255, 0, 0);

EthernetServer server(80);
const float offset = 2.55;
const float changeThreshold = 0.01;  // Ambang batas perubahan untuk mengirim data

// Variabel untuk caching
float lastVoltage = 0;  // Menyimpan pembacaan voltase sebelumnya

void setup() {
  Serial.begin(115200);
  SPI.begin();
  
  Ethernet.init(5);
  Ethernet.begin(mac, ip, subnet);
  Serial.print("Connected! IP address: ");
  Serial.println(Ethernet.localIP());

  server.begin();
}

float readFilteredVoltage() {
    float total = 0;
    for (int i = 0; i < 10; i++) {
        total += analogRead(A3);
        delay(1);  // Sedikit jeda antar pembacaan
    }
    return ((total / 10) / 4095.0) * 3.3 - offset;
}

void loop() {
  // Membaca voltase baru
  float currentVoltage = readFilteredVoltage();

  // Cek apakah perbedaan antara voltase saat ini dan terakhir melebihi ambang batas
  if (abs(currentVoltage - lastVoltage) > changeThreshold) {
      lastVoltage = currentVoltage;  // Update nilai terakhir

      // Jika ada perubahan yang signifikan, tunggu client untuk mengirim data
      EthernetClient client = server.available();
      
      if (client) {
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: application/json");
        client.println("Access-Control-Allow-Origin: *");
        client.println("Connection: close");
        client.println();

        // Mengirimkan data JSON
        client.print("{\"voltage\": ");
        client.print(currentVoltage, 6);
        client.println("}");
        
        delay(100);
        client.stop();
      }
  }
}
