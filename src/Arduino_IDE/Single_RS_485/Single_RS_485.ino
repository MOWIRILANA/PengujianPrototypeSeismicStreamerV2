#include <HardwareSerial.h>

// Konfigurasi pin RS485
#define RS485_TX_PIN 17 // DI
#define RS485_RX_PIN 16 // RO
#define RS485_DE_RE_PIN 4  // DE

// ID perangkat (set berbeda untuk masing-masing ESP32)
#define DEVICE_ID 2  // Ganti 1, 2, 3 sesuai dengan ESP32 yang digunakan

HardwareSerial RS485(1);

void setup() {
  Serial.begin(115200);
  RS485.begin(9600, SERIAL_8N1, RS485_RX_PIN, RS485_TX_PIN);

  pinMode(RS485_DE_RE_PIN, OUTPUT);

  // Mode default: receiver
  digitalWrite(RS485_DE_RE_PIN, LOW);
}

void sendRS485Data(int id, int value) {
  digitalWrite(RS485_DE_RE_PIN, HIGH);

  // Format data: [ID, VALUE]
  RS485.write(id);
  RS485.write(value);
  RS485.flush();

  delay(5); // Tunggu sampai selesai
  digitalWrite(RS485_DE_RE_PIN, LOW);
}

void loop() {
  int randomValue = random(1, 101); // Data acak 1-100

  sendRS485Data(DEVICE_ID, randomValue);

  Serial.print("Sent: ID=");
  Serial.print(DEVICE_ID);
  Serial.print(", Value=");
  Serial.println(randomValue);

  // Jeda waktu berbeda untuk setiap ESP32
  delay(DEVICE_ID * 500); // Contoh: ESP32 ID 1 = 1s, ID 2 = 2s
}
