#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include "IPLaptop.h"

// Pin Definitions
#define CS 15    // Chip Select
#define RDY 21  // Data Ready

#define SPISPEED 2500000  // SPI speed for communication with ADS1256

const int UDP_PACKET_SIZE = 24;  // UDP time stamp is in the first 48 bytes of the message.
char packetBuffer[UDP_PACKET_SIZE];  // Buffer for both incoming and outgoing packets.
char ReplyBuffer[] = "acknowledged";

EthernetUDP Udp;

IPAddress remoteIP(255, 255, 255, 255);  // Replace with the IP of the laptop or server
const unsigned int remotePort = 5000;   // Port to send data to

void setup() {
  Serial.begin(115200);
  delay(500);

  // ADS1256 Initialization
  pinMode(CS, OUTPUT);
  digitalWrite(CS, HIGH); // Ensure CS is high to start
  pinMode(RDY, INPUT);
  SPI.begin();  // Initialize SPI
  configureADS1256();
  Serial.println("ADS1256 initialized. Starting readings...");

  // Ethernet Initialization
  Serial.println("\n\tUDP Client v1.0\r\n");
  Ethernet.init(CS);
  WizReset();

  Serial.println("Starting ETHERNET connection...");
  Ethernet.begin(eth_MAC, eth_IP, eth_DNS, eth_GW, eth_MASK);

  delay(200);
  Serial.print("Ethernet IP is: ");
  Serial.println(Ethernet.localIP());
  cableConnection();

  Udp.begin(remoteIP);  // Start UDP communication
}

void loop() {
  // Read analog values from A0, A1, and A2 (Potentiometers)
  float voltageA0 = readSingleEndedChannel(0); // Channel 0 corresponds to AIN0
  float voltageA1 = readSingleEndedChannel(1); // Channel 1 corresponds to AIN1
  float voltageA2 = readSingleEndedChannel(2); // Channel 2 corresponds to AIN2

  // Prepare data to send
  char message[64];
  snprintf(message, sizeof(message), "A1: %.6f | A2: %.6f | A3: %.6f", voltageA0, voltageA1, voltageA2);

  // Send a UDP packet
  if (Udp.beginPacket(remoteIP, remotePort)) {
    Udp.write(message);
    if (Udp.endPacket()) {
      Serial.println("UDP packet sent successfully!");
      Serial.println(message);
    } else {
      Serial.println("Error sending UDP packet.");
    }
  } else {
    Serial.println("Failed to start UDP packet.");
  }

  delay(1);  // Minimal delay for sending data
}

// Function to configure ADS1256
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

// Function to write to a register
void writeRegister(byte reg, byte value) {
  SPI.transfer(0x50 | reg); // Write command to register
  SPI.transfer(0x00);       // Write one byte
  SPI.transfer(value);      // Register value
  delayMicroseconds(100);
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
  if (rawValue & 0x800000) {
    rawValue |= 0xFF000000; // Sign extend for negative values
  }

  float voltage = rawValue * (3.3 / (0x7FFFFF)); // Assuming reference voltage is 3.3V
  return voltage;
}

void WizReset() {
  Serial.print("Resetting Wiz W5500 Ethernet Board...  ");
  pinMode(RESET_P, OUTPUT);
  digitalWrite(RESET_P, HIGH);
  delay(250);
  digitalWrite(RESET_P, LOW);
  delay(50);
  digitalWrite(RESET_P, HIGH);
  delay(350);
  Serial.println("Done.");
}

void prt_hwval(uint8_t refval) {
  switch (refval) {
    case 0:
      Serial.println("No hardware detected.");
      break;
    case 1:
      Serial.println("WizNet W5100 detected.");
      break;
    case 2:
      Serial.println("WizNet W5200 detected.");
      break;
    case 3:
      Serial.println("WizNet W5500 detected.");
      break;
    default:
      Serial.println("UNKNOWN - Update espnow_gw.ino to match Ethernet.h");
  }
}

void prt_ethval(uint8_t refval) {
  switch (refval) {
    case 0:
      Serial.println("Unknown status.");
      break;
    case 1:
      Serial.println("Link flagged as UP.");
      break;
    case 2:
      Serial.println("Link flagged as DOWN. Check cable connection.");
      break;
    default:
      Serial.println("UNKNOWN - Update espnow_gw.ino to match Ethernet.h");
  }
}

void cableConnection() {
  Serial.print("Checking connection.");
  bool rdy_flag = false;
  for (uint8_t i = 0; i <= 20; i++) {
    if ((Ethernet.hardwareStatus() == EthernetNoHardware) || (Ethernet.linkStatus() == LinkOFF)) {
      Serial.print(".");
      rdy_flag = false;
      delay(80);
    } else {
      rdy_flag = true;
      break;
    }
  }
  if (rdy_flag == false) {
    Serial.println("\n\r\tHardware fault, or cable problem... cannot continue.");
    Serial.print("Hardware Status: ");
    prt_hwval(Ethernet.hardwareStatus());
    Serial.print("   Cable Status: ");
    prt_ethval(Ethernet.linkStatus());
    while (true) {
      delay(10);
    }
  } else {
    Serial.println(" OK");
  }
}
