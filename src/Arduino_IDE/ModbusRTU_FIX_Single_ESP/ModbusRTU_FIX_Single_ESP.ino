#include <ModbusRTU.h>

// Pin untuk komunikasi RS485
#define RX_PIN 16
#define TX_PIN 17
#define DE_RE_PIN 4  // Pin untuk mengontrol mode TX/RX pada RS485 driver

ModbusRTU mb;  // Inisialisasi Modbus object
uint16_t analogValue = 0;  // Data register untuk menyimpan nilai potensiometer

// Callback jika ada permintaan dari master
bool cb(Modbus::ResultCode event, uint16_t transactionId, void* data) {
  Serial.printf("Request result: 0x%02X\n", event);  // Logging
  return true;
}

void setup() {
  Serial.begin(115200);  // Debugging
  Serial2.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);  // UART untuk komunikasi Modbus RTU

  // Atur pin untuk kontrol DE/RE pada RS485 driver
  pinMode(DE_RE_PIN, OUTPUT);
  digitalWrite(DE_RE_PIN, LOW);  // Default: Receive mode

  // Inisialisasi Modbus slave
  mb.begin(&Serial2, DE_RE_PIN);  // Gunakan Serial2 dengan kontrol DE/RE
  mb.slave(1);  // Slave address = 1

  // Atur register untuk nilai analog
  mb.addHreg(0, analogValue);  // Holding Register 0
}

void loop() {
  // Baca nilai dari potensiometer (di pin GPIO 26)
  analogValue = analogRead(25);
  Serial.printf("Analog Value: %d\n", analogValue);
  // Perbarui register holding dengan nilai analog
  mb.Hreg(0, analogValue);

  // Jalankan tugas Modbus
  mb.task();
  delay(10);  // Delay kecil untuk menghindari beban CPU
}
