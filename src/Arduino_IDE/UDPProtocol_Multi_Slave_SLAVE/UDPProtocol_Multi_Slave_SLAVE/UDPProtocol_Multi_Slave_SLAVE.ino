#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include "local_config.h"

// Pin Definitions
#define CS_W5500 15
#define CS_SLAVE 15 // Pin ini digunakan untuk SPI (ditentukan oleh SPI Daisy Chain)
#define SPISPEED 2500000 // SPI Speed for communication

// UDP Setup
const int UDP_PACKET_SIZE = 64;
char packetBuffer[UDP_PACKET_SIZE];
EthernetUDP Udp;

void setup() {
  Serial.begin(115200);
  delay(500);

  // SPI Initialization
  SPI.begin();
  pinMode(CS_W5500, OUTPUT);
  digitalWrite(CS_W5500, HIGH); // Set W5500 CS High

  // Ethernet Initialization
  Ethernet.init(CS_W5500);
  WizReset();
  Ethernet.begin(eth_MAC, eth_IP, eth_DNS, eth_GW, eth_MASK);
  Udp.begin(localPort);

  Serial.println("Slave initialized. Waiting for SPI requests...");
}

void loop() {
  // Simulate sensor data
  float sensorData[3];
  sensorData[0] = generateRandomValue(0.0, 5.0);
  sensorData[1] = generateRandomValue(0.0, 5.0);
  sensorData[2] = generateRandomValue(0.0, 5.0);

  // Respond to SPI requests
  handleSPIRequests(sensorData);
  delay(10); // Reduce CPU usage
}

// Function to generate random float values
float generateRandomValue(float minValue, float maxValue) {
  return minValue + (random(0, 10000) / 10000.0) * (maxValue - minValue);
}

// Handle incoming SPI requests
void handleSPIRequests(float *data) {
  if (digitalRead(CS_SLAVE) == LOW) { // Detect SPI activity
    SPI.beginTransaction(SPISettings(SPISPEED, MSBFIRST, SPI_MODE0));
    digitalWrite(CS_SLAVE, LOW);

    for (int i = 0; i < 3; i++) {
      uint16_t valueToSend = static_cast<uint16_t>(data[i] * 1000); // Convert to integer format
      SPI.transfer16(valueToSend); // Send data
      delayMicroseconds(5); // Small delay to ensure SPI stability
    }

    digitalWrite(CS_SLAVE, HIGH);
    SPI.endTransaction();
  }
}

// Reset W5500 Ethernet Module
void WizReset() {
  Serial.print("Resetting W5500...");
  pinMode(RESET_P, OUTPUT);
  digitalWrite(RESET_P, HIGH);
  delay(250);
  digitalWrite(RESET_P, LOW);
  delay(50);
  digitalWrite(RESET_P, HIGH);
  delay(350);
  Serial.println("Done.");
}
