#include <SPI.h>

// RS485
#define TX_PIN 16
#define RX_PIN 17

#define led 2

// ADS1256 Pin Definitions
#define CS 25    
#define RDY 21  
#define SPISPEED 2500000  

void setup() {
    Serial.begin(115200);
    Serial2.begin(115200, SERIAL_8N1, TX_PIN, RX_PIN);  // RS485 UART

    pinMode(led, OUTPUT);
    // ADS1256 Initialization
    pinMode(CS, OUTPUT);
    digitalWrite(CS, HIGH);
    pinMode(RDY, INPUT);
    SPI.begin(18, 19, 23, CS);
    configureADS1256();
    
    Serial.println("ADS1256 initialized. Waiting for Master request...");
}

void loop() {
    readsensorads();
    digitalWrite(led, LOW);
    delay(100);
}

void readsensorads() {
    digitalWrite(led, HIGH);
    float voltageA0 = readSingleEndedChannel(0);
    float voltageA1 = readSingleEndedChannel(1);
    float voltageA2 = readSingleEndedChannel(2);

    // Kirim data ke Master melalui RS485
    Serial2.print(voltageA0);
    Serial2.print(";");
    Serial2.print(voltageA1);
    Serial2.print(";");
    Serial2.println(voltageA2);

    // Debugging di Serial Monitor Slave
    Serial.print("Sent: ");
    Serial.print(voltageA0);
    Serial.print(";");
    Serial.print(voltageA1);
    Serial.print(";");
    Serial.println(voltageA2);
}

// Fungsi konfigurasi ADS1256
void configureADS1256() {
    SPI.beginTransaction(SPISettings(SPISPEED, MSBFIRST, SPI_MODE1));
    digitalWrite(CS, LOW);

    delay(2);
    SPI.transfer(0xFE); // Reset command
    delay(5);

    // Set STATUS register
    writeRegister(0x00, 0x01);
    // Set ADCON register (gain = 1, clock off)
    writeRegister(0x02, 0x00);
    // Set DRATE register (data rate = 30,000SPS)
    writeRegister(0x03, 0xF0);
    // Self-calibration
    SPI.transfer(0xF0);
    delay(400);

    digitalWrite(CS, HIGH);
    SPI.endTransaction();
}

// Fungsi untuk menulis ke register ADS1256
void writeRegister(byte reg, byte value) {
    SPI.transfer(0x50 | reg); // Write command
    SPI.transfer(0x00);
    SPI.transfer(value);
    delayMicroseconds(100);
}

// Fungsi membaca satu channel ADS1256
float readSingleEndedChannel(byte channel) {
    SPI.beginTransaction(SPISettings(SPISPEED, MSBFIRST, SPI_MODE1));
    digitalWrite(CS, LOW);

    while (digitalRead(RDY));

    byte muxValue = (channel << 4) | 0x08;
    writeRegister(0x01, muxValue);

    SPI.transfer(0xFC); // SYNC
    delayMicroseconds(2);
    SPI.transfer(0x00); // WAKEUP
    delay(2);

    SPI.transfer(0x01); // RDATA
    delayMicroseconds(5);

    long rawValue = 0;
    rawValue |= SPI.transfer(0);
    rawValue <<= 8;
    rawValue |= SPI.transfer(0);
    rawValue <<= 8;
    rawValue |= SPI.transfer(0);

    digitalWrite(CS, HIGH);
    SPI.endTransaction();

    if (rawValue & 0x800000) {
        rawValue |= 0xFF000000;
    }

    float voltage = rawValue * (3.3 / (0x7FFFFF)); // 3.3V reference
    return voltage;
}
