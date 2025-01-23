#include <SPI.h>
#include <ModbusRTU.h>

// Pin Definitions for RS485
#define RX_PIN 16
#define TX_PIN 17
#define DE_RE_PIN 4  

ModbusRTU mb;
float hydrophoneValue = 0.0; // Dummy dB value for hydrophone
unsigned long lastDebugTime = 0;
const unsigned long DEBUG_INTERVAL = 1000; // Debug print interval (1 second)

// Hydrophone simulation constants
const float BASE_DB = 100.0;   // Base dB value for simulation
const float FLUCTUATION = 5.0; // Max fluctuation in dB

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);

  pinMode(DE_RE_PIN, OUTPUT);
  digitalWrite(DE_RE_PIN, LOW);

  mb.begin(&Serial2, DE_RE_PIN); // Pass DE_RE pin to library
  mb.slave(2);

  // Add Modbus holding register
  mb.addHreg(0);
  Serial.printf("Register 0 added with initial value: %.2f dB\n", hydrophoneValue);

  Serial.println("System initialized. Waiting for Modbus commands...");
  randomSeed(analogRead(0)); // Initialize random seed
}

void loop() {
  // Generate dummy hydrophone value
  hydrophoneValue = BASE_DB + random(-FLUCTUATION * 100, FLUCTUATION * 100) / 100.0;

  // Update Modbus register
  uint16_t modbusValue = (uint16_t)(hydrophoneValue * 10); // Scale to integer for Modbus
  mb.Hreg(0, modbusValue);

  // Periodic debug output
  if (millis() - lastDebugTime >= DEBUG_INTERVAL) {
    Serial.println("\n=== Debug Info ===");
    Serial.printf("Modbus Status: Active on address %d\n", mb.slave());
    Serial.printf("Hydrophone Value (dB re 1 µPa): %.2f dB\n", hydrophoneValue);
    Serial.printf("Register[0]: %d\n", modbusValue);
    Serial.println("================\n");
    lastDebugTime = millis();
  }

  // Handle Modbus communication
  mb.task();
  yield();
  delay(10);
}
