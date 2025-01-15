#include <SPI.h>

// Pin Definitions
#define CS 5  
#define VSCK 18
#define VMISO 19
#define VMOSI 23
#define RDY 21  

#define SPISPEED 10000000    
// #define SPISPEED 80000000

unsigned long previousMillis = 0;  // Menyimpan waktu sebelumnya
unsigned long sampleCount = 0;     // Hitungan sampel per detik
unsigned long lastReadMillis = 0;  // Menyimpan waktu pembacaan terakhir

const int numChannels = 4;
float voltages[numChannels];

void setup() {
  Serial.begin(115200);
  delay(500);

  // ADS1256 Initialization
  pinMode(CS, OUTPUT);
  digitalWrite(CS, HIGH); // Ensure CS is high to start
  pinMode(RDY, INPUT);
  SPI.begin(VSCK, VMISO, VMOSI, CS);  //SCK, MISO, MOSI, CS
  configureADS1256();
  Serial.println("ADS1256 initialized. Starting readings...");
}

void loop() {
  unsigned long currentMillis = millis();

  // Hanya ambil sampel setiap 5 ms (200 sampel per detik)
  if (currentMillis - lastReadMillis >= 5) {
    readsensorads();
    sampleCount++;  // Hitung jumlah sampel setiap kali membaca sensor
    lastReadMillis = currentMillis;  // Catat waktu pembacaan terakhir
  }

  // Hitung dan tampilkan jumlah sampel per detik
  if (currentMillis - previousMillis >= 100) {  // Setiap 1 detik
    Serial.print("Samples per second: ");
    Serial.println(sampleCount);
    sampleCount = 0;  // Reset hitungan sampel
    previousMillis = currentMillis;
  }
}

void readsensorads() {
  for (int i = 0; i<numChannels; i++){
    voltages[i]= readSingleEndedChannel(i);
  }

  for(int i = 0; i<numChannels; i++){
    Serial.print(voltages[i]);
    if(i<numChannels-1){
      Serial.print(",");
    }else{
      Serial.println();
    }
  }
}

// Function to configure ADS1256
void configureADS1256() {
  SPI.beginTransaction(SPISettings(SPISPEED, MSBFIRST, SPI_MODE1));
  digitalWrite(CS, LOW);

  delay(2);
  SPI.transfer(0xFE); // Reset command
  delayMicroseconds(7);

  // Set STATUS register (most significant bit first, buffer disabled)
  writeRegister(0x00, 0x01);

  // Set ADCON register (gain = 1, clock off)
  writeRegister(0x08, 0x00);

  // Set DRATE register (data rate = 30,000SPS)
  writeRegister(0x03, 0xF0);

  // Perform self-calibration
  SPI.transfer(0xF0);
  delayMicroseconds(7);

  digitalWrite(CS, HIGH);
  SPI.endTransaction();
}

// Function to write to a register
void writeRegister(byte reg, byte value) {
  SPI.transfer(0x50 | reg); // Write command to register
  SPI.transfer(0x00);       // Write one byte
  SPI.transfer(value);      // Register value
  delayMicroseconds(1);
}

// Function to read single-ended channel
float readSingleEndedChannel(byte channel) {
  SPI.beginTransaction(SPISettings(SPISPEED, MSBFIRST, SPI_MODE1));
  digitalWrite(CS, LOW);

  // Wait for DRDY to go low
  while (digitalRead(RDY));

  // Set MUX register to select the desired channel (AINx-AINCOM)
  byte muxValue = (channel << 4) | 0x08; // Channel 0 = AIN0, Channel 1 = AIN1, Channel 2 = AIN2
  writeRegister(0x01, muxValue);

  // Send SYNC and WAKEUP commands
  SPI.transfer(0xFC); // SYNC command
  delayMicroseconds(2);
  SPI.transfer(0x00); // WAKEUP command
  delay(2);

  // Read ADC result
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

  // Convert raw value to voltage
  if (rawValue & 0x800000) {  // Check for negative value
    rawValue -= 16777216;     // Convert to negative using two's complement
  }

  // Convert raw value to voltage with appropriate scaling
  float voltage = (rawValue / 4095.0);  // Assuming reference voltage is 3.3V

  return voltage;
}