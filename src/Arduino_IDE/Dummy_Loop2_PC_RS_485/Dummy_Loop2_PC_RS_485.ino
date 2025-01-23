#include <SPI.h>
#include <ModbusRTU.h>

// Pin Definitions for RS485
#define RX_PIN 16
#define TX_PIN 17
#define DE_RE_PIN 4  

ModbusRTU mb;
uint16_t analogValues[3] = {0, 0, 0};
unsigned long lastDebugTime = 0;
const unsigned long DEBUG_INTERVAL = 1000; // Debug print interval (1 second)

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);

  pinMode(DE_RE_PIN, OUTPUT);
  digitalWrite(DE_RE_PIN, LOW);

  mb.begin(&Serial2, DE_RE_PIN); // Pass DE_RE pin to library
  mb.slave(2);

  // Add Modbus holding registers with debug message
  for (int i = 0; i < 3; i++) {
    mb.addHreg(i);
    Serial.printf("Register %d added with initial value: %d\n", i, analogValues[i]);
  }

  Serial.println("System initialized. Waiting for Modbus commands...");
  randomSeed(analogRead(0)); // Initialize random seed
}

void loop() {
  // Generate random analog values for channels 0, 1, and 2
  analogValues[0] = random(0, 4096); // Simulated 12-bit ADC value
  analogValues[1] = random(0, 4096);
  analogValues[2] = random(0, 4096);

  // Update Modbus registers
  for (int i = 0; i < 3; i++) {
    mb.Hreg(i, analogValues[i]);
  }

  // Periodic debug output
  if (millis() - lastDebugTime >= DEBUG_INTERVAL) {
    Serial.println("\n=== Debug Info ===");
    Serial.printf("Modbus Status: Active on address %d\n", mb.slave());
    Serial.println("Register Values:");
    for (int i = 0; i < 3; i++) {
      Serial.printf("Reg[%d] = %d\n", i, mb.Hreg(i));
    }
    Serial.println("================\n");
    lastDebugTime = millis();
  }

  // Handle Modbus communication
  mb.task();
  yield();
  delay(10);
}
