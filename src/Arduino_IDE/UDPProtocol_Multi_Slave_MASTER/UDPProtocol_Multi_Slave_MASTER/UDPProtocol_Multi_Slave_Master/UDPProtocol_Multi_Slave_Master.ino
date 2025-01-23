#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include "local_config.h"

// Pin Definitions
#define CS_ADS1256 5   // Chip Select for ADS1256
#define CS_W5500 14    // Chip Select for W5500 (Slave Ethernet)
#define RDY 21         // Data Ready for ADS1256

#define SPISPEED 2500000  // SPI speed for communication with ADS1256

const int UDP_PACKET_SIZE = 64;
char packetBuffer[UDP_PACKET_SIZE];
EthernetUDP Udp;

IPAddress remoteIP(255, 255, 255, 255);  // Replace with the IP of the laptop or server
const unsigned int remotePort = 5000;   // Port to send data to

// Variables
enum State { READ_MASTER, READ_SLAVE, SEND_DATA };
State currentState = READ_MASTER;

unsigned long lastStateChange = 0;
unsigned long stateInterval = 10;  // Time to wait before transitioning states

float masterData[3] = {0.0, 0.0, 0.0};
float slaveData[3] = {0.0, 0.0, 0.0};

void setup() {
  Serial.begin(115200);
  delay(500);

  // Initialize SPI and Pins
  pinMode(CS_ADS1256, OUTPUT);
  digitalWrite(CS_ADS1256, HIGH);
  pinMode(CS_W5500, OUTPUT);
  digitalWrite(CS_W5500, HIGH);
  pinMode(RDY, INPUT);
  SPI.begin();

  // Initialize ADS1256
  configureADS1256();
  Serial.println("ADS1256 initialized. Starting readings...");

  // Initialize Ethernet
  Ethernet.init(15);
  WizReset();
  Ethernet.begin(eth_MAC, eth_IP, eth_DNS, eth_GW, eth_MASK);
  Udp.begin(remotePort);
  Serial.println("Ethernet initialized.");
}

void loop() {
  unsigned long currentMillis = millis();

  // Non-blocking state machine
  if (currentMillis - lastStateChange >= stateInterval) {
    switch (currentState) {
      case READ_MASTER:
        readMasterData();
        currentState = READ_SLAVE;
        break;

      case READ_SLAVE:
        requestDataFromSlave();
        currentState = SEND_DATA;
        break;

      case SEND_DATA:
        sendDataToPC();
        currentState = READ_MASTER;
        break;
    }
    lastStateChange = currentMillis;
  }
}

// Configure ADS1256
void configureADS1256() {
  SPI.beginTransaction(SPISettings(SPISPEED, MSBFIRST, SPI_MODE1));
  digitalWrite(CS_ADS1256, LOW);
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

  digitalWrite(CS_ADS1256, HIGH);
  SPI.endTransaction();
}

// Write to ADS1256 Register
void writeRegister(byte reg, byte value) {
  SPI.transfer(0x50 | reg);
  SPI.transfer(0x00);
  SPI.transfer(value);
  delayMicroseconds(100);
}

// Read single-ended channel from ADS1256
float readSingleEndedChannel(byte channel) {
  SPI.beginTransaction(SPISettings(SPISPEED, MSBFIRST, SPI_MODE1));
  digitalWrite(CS_ADS1256, LOW);

  // Wait for DRDY to go low
  while (digitalRead(RDY));

  // Set MUX register to select the desired channel
  byte muxValue = (channel << 4) | 0x08;
  writeRegister(0x01, muxValue);

  // Send SYNC and WAKEUP commands
  SPI.transfer(0xFC);
  delayMicroseconds(2);
  SPI.transfer(0x00);
  delay(2);

  // Read ADC result
  SPI.transfer(0x01);
  delayMicroseconds(5);

  long rawValue = 0;
  rawValue |= SPI.transfer(0);
  rawValue <<= 8;
  rawValue |= SPI.transfer(0);
  rawValue <<= 8;
  rawValue |= SPI.transfer(0);

  digitalWrite(CS_ADS1256, HIGH);
  SPI.endTransaction();

  // Convert raw value to voltage
  if (rawValue & 0x800000) {
    rawValue |= 0xFF000000;
  }

  return rawValue * (3.3 / (0x7FFFFF));
}

// Read data from Master
void readMasterData() {
  masterData[0] = readSingleEndedChannel(0);
  masterData[1] = readSingleEndedChannel(1);
  masterData[2] = readSingleEndedChannel(2);
  Serial.println("Master data read.");
}

// Request data from Slave via SPI
void requestDataFromSlave() {
  SPI.beginTransaction(SPISettings(SPISPEED, MSBFIRST, SPI_MODE0));
  digitalWrite(CS_W5500, LOW);

  for (int i = 0; i < 3; i++) {
    uint16_t rawData = SPI.transfer16(0x00);
    slaveData[i] = rawData / 1000.0;
    delayMicroseconds(10);
  }

  digitalWrite(CS_W5500, HIGH);
  SPI.endTransaction();
  Serial.println("Slave data received.");
}

// Send data to PC via UDP
void sendDataToPC() {
  char message[128];
  snprintf(message, sizeof(message),
           "Master A0: %.6f | A1: %.6f | A2: %.6f | Slave A0: %.6f | A1: %.6f | A2: %.6f",
           masterData[0], masterData[1], masterData[2],
           slaveData[0], slaveData[1], slaveData[2]);

  if (Udp.beginPacket(remoteIP, remotePort)) {
    Udp.write(message);
    if (Udp.endPacket()) {
      Serial.println("UDP packet sent:");
      Serial.println(message);
    } else {
      Serial.println("Error sending UDP packet.");
    }
  } else {
    Serial.println("Failed to start UDP packet.");
  }
}

// Reset W5500
void WizReset() {
  pinMode(RESET_P, OUTPUT);
  digitalWrite(RESET_P, HIGH);
  delay(250);
  digitalWrite(RESET_P, LOW);
  delay(50);
  digitalWrite(RESET_P, HIGH);
  delay(350);
  Serial.println("W5500 Reset done.");
}
