// Library
#include <WiFi.h>
#include <WiFiUdp.h>

// SSID dan Password WiFi:
const char * ssid = "SSID";
const char * pwd = "mowirilana";

// Mengatur Port
const int udpPort = 9999;

// Objek Kelas WiFi UDP
WiFiUDP udp;

void setup(){
  // Serial Monitor
  Serial.begin(115200);
  
  // Menghubungkan ke WiFi
  WiFi.begin(ssid, pwd);
  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  
  // Jika terhubung maka akan mencetak Alamat IP ESP32
  Serial.println("");
  Serial.print("Ip: ");
  Serial.print(WiFi.localIP());
  
  // Memulai menjadi Server
  udp.begin(WiFi.localIP(), udpPort);
}

void loop(){
  uint8_t buffer_terima[50];
  memset(buffer_terima, 0, 50);
  
  // Parsing nilai yang diterima
  udp.parsePacket();
  if(udp.read(buffer_terima, 50) > 0){
    // Mencetak nilai yang diterima
    Serial.print("Terima Data: ");
    Serial.println((char *) buffer_terima);

    // mengirim ke client
    uint8_t buffer_kirim[12] = "Dari Server";
    udp.beginPacket();
    udp.write(buffer_kirim, 11);
    udp.endPacket();
  }
  
  delay(1000);
}