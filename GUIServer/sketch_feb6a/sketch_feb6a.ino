void setup() {
  Serial.begin(115200);
  delay(2000);  // Wait for ESP32 to stabilize
}

void loop() {
  for (int i = 0; i < 4; i++) {
    Serial.print(random(-500, 500) / 100.0, 2);  // Random float with 2 decimal places
    if (i < 3) Serial.print(", ");  // Add comma between values
  }
  Serial.println();  // New line after all values
  delay(50);
}