#include <SPI.h>
#include <ModbusRTU.h>

// Pin Definitions for ADS1256
#define CS 5    
#define RDY 21  
#define SPISPEED 2500000  

// Pin Definitions for RS485
#define RX_PIN 16
#define TX_PIN 17
#define DE_RE_PIN 4  

ModbusRTU mb;
uint16_t analogValues[4] = {0, 0, 0, 0};  // Array untuk menyimpan nilai A0, A1, A2, A3

// Buffer untuk menyimpan data per detik
#define BUFFER_SIZE 1000  // Ukuran buffer untuk 1000 sampel per detik
uint16_t buffer[4][BUFFER_SIZE];  // Buffer untuk 4 channel
int bufferIndex[4] = {0};  // Indeks untuk setiap channel dalam buffer

unsigned long lastReadMillis = 0;  // Waktu pembacaan terakhir
unsigned long lastPrintMillis = 0;  // Waktu untuk mencetak jumlah data di buffer
unsigned long sampleCount = 0;  // Hitungan sampel per detik

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);

  pinMode(DE_RE_PIN, OUTPUT);
  digitalWrite(DE_RE_PIN, LOW);

  mb.begin(&Serial2, DE_RE_PIN); // Pass DE_RE pin to library
  mb.slave(1);  // Set Modbus slave ID to 2

  // Initialize Modbus holding registers untuk setiap channel
  for (int i = 0; i < 4; i++) {
    mb.addHreg(i);  // Register index matches the channel number
    Serial.printf("Register %d added with initial value: %d\n", i, analogValues[i]);
  }

  // Initialize SPI dan ADS1256
  SPI.begin();
  pinMode(CS, OUTPUT);
  digitalWrite(CS, HIGH);
  pinMode(RDY, INPUT);

  configureADS1256();
  Serial.println("System initialized. Modbus RTU active.");
}

void loop() {
  unsigned long currentMillis = millis();

  // Hanya ambil sampel setiap 1 ms (1000 sampel per detik)
  if (currentMillis - lastReadMillis >= 1) {
    readsensorads();
    sampleCount++;  // Hitung jumlah sampel setiap kali membaca sensor
    lastReadMillis = currentMillis;  // Catat waktu pembacaan terakhir
  }

  // Setiap 1 detik, tampilkan jumlah data dalam buffer dan reset buffer
  if (currentMillis - lastPrintMillis >= 1000) {  // Setiap 1 detik
    printBufferData();  // Fungsi untuk mencetak jumlah data dalam buffer
    clearBuffer();  // Clear buffer setelah setiap detik
    lastPrintMillis = currentMillis;  // Catat waktu print terakhir
  }

  // Update nilai Modbus holding registers dengan pembacaan terbaru
  for (int i = 0; i < 4; i++) {
    mb.Hreg(i, analogValues[i]);
  }

  // Handle komunikasi Modbus
  mb.task();
  yield();
}

// Fungsi untuk membaca sensor ADS1256
void readsensorads() {
  for (int i = 0; i < 4; i++) {
    analogValues[i] = (uint16_t)(readSingleEndedChannel(i) * 1000);  // Convert to mV
    buffer[i][bufferIndex[i]] = analogValues[i];  // Menyimpan data dalam buffer

    // Update indeks untuk channel tersebut
    bufferIndex[i] = (bufferIndex[i] + 1) % BUFFER_SIZE;  // Rotasi buffer jika indeks mencapai BUFFER_SIZE
  }
}

// Fungsi untuk mencetak jumlah data dalam buffer
void printBufferData() {
  for (int i = 0; i < 4; i++) {
    int dataCount = (bufferIndex[i] == 0) ? BUFFER_SIZE : bufferIndex[i];  // Menghitung jumlah data dalam buffer
    Serial.print("Channel ");
    Serial.print(i);
    Serial.print(" - Data count: ");
    Serial.println(dataCount);
  }
}

// Fungsi untuk membersihkan buffer
void clearBuffer() {
  for (int i = 0; i < 4; i++) {
    bufferIndex[i] = 0;  // Reset buffer index untuk setiap channel
  }
}

// Fungsi untuk mengonfigurasi ADS1256
void configureADS1256() {
  SPI.beginTransaction(SPISettings(SPISPEED, MSBFIRST, SPI_MODE1));
  digitalWrite(CS, LOW);

  delay(2);
  SPI.transfer(0xFE); // Reset command
  delay(5);

  // Set STATUS register (most significant bit first, buffer disabled)
  writeRegister(0x00, 0x01);

  // Set ADCON register (gain = 1, clock off)
  writeRegister(0x02, 0x00);

  // Set DRATE register (data rate = 30,000SPS)
  writeRegister(0x03, 0xF0);

  // Perform self-calibration
  SPI.transfer(0xF0);
  delay(400);

  digitalWrite(CS, HIGH);
  SPI.endTransaction();
}

// Fungsi untuk menulis ke register
void writeRegister(byte reg, byte value) {
  SPI.transfer(0x50 | reg); // Write command to register
  SPI.transfer(0x00);       // Write one byte
  SPI.transfer(value);      // Register value
  delayMicroseconds(100);
}

// Fungsi untuk membaca saluran single-ended
float readSingleEndedChannel(byte channel) {
  SPI.beginTransaction(SPISettings(SPISPEED, MSBFIRST, SPI_MODE1));
  digitalWrite(CS, LOW);

  // Tunggu sampai DRDY turun
  while (digitalRead(RDY));

  // Set MUX register untuk memilih saluran yang diinginkan (AINx-AINCOM)
  byte muxValue = (channel << 4) | 0x08; // Channel 0 = AIN0, Channel 1 = AIN1, Channel 2 = AIN2
  writeRegister(0x01, muxValue);

  // Kirim perintah SYNC dan WAKEUP
  SPI.transfer(0xFC); // SYNC command
  delayMicroseconds(2);
  SPI.transfer(0x00); // WAKEUP command
  delay(2);

  // Baca hasil ADC
  SPI.transfer(0x01); // RDATA command
  delayMicroseconds(5);

  long rawValue = 0;
  rawValue |= SPI.transfer(0);
  rawValue <<= 8;
  rawValue |= SPI.transfer(0);
  rawValue <<= 8;
  rawValue |= SPI.transfer(0);

  digitalWrite(CS, HIGH);
  SPI.endTransaction();

  // Konversi nilai mentah ke tegangan
  if (rawValue & 0x800000) {
    rawValue |= 0xFF000000; // Sign extend untuk nilai negatif
  }

  float voltage = rawValue * (3.3 / (0x7FFFFF)); // Asumsikan tegangan referensi adalah 3.3V
  return voltage;
}
