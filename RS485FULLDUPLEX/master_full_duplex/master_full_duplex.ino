#define DE_PIN 4
#define RE_PIN 5

unsigned long lastRequest = 0;
unsigned long lastSumTime = 0;
float receivedData[3];
float sumData[3] = {0, 0, 0};
int countSamples = 0;

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, 16, 17);
  
  pinMode(DE_PIN, OUTPUT);
  pinMode(RE_PIN, OUTPUT);
  digitalWrite(DE_PIN, LOW);
  digitalWrite(RE_PIN, LOW);
  
  Serial.println("Master siap");
}

void loop() {
  // Request data setiap 1 ms
  if (millis() - lastRequest >= 1) {
    sendRequest();
    receiveData();
    lastRequest = millis();
  }

  // Reset dan cetak jumlah data setiap 1 detik
  if (millis() - lastSumTime >= 1000) {
    Serial.print("Total dalam 1 detik => ");
    // Serial.print("A0: ");
    // Serial.print(sumData[0], 4);
    // Serial.print(" | A1: ");
    // Serial.print(sumData[1], 4);
    // Serial.print(" | A2: ");
    // Serial.print(sumData[2], 4);
    Serial.print(" ============================== Total Samples: ");
    Serial.println(countSamples);

    // Reset jumlah dan hitungan sampel
    sumData[0] = sumData[1] = sumData[2] = 0;
    countSamples = 0;
    lastSumTime = millis();
  }
}

void sendRequest() {
  digitalWrite(DE_PIN, HIGH);
  digitalWrite(RE_PIN, HIGH);
  Serial2.write('R');
  digitalWrite(DE_PIN, LOW);
  digitalWrite(RE_PIN, LOW);
}

void receiveData() {
  static byte buffer[12];
  static byte index = 0;
  unsigned long startTime = millis();
  
  while ((millis() - startTime) < 500) {  // Timeout 500ms
    while (Serial2.available()) {
      buffer[index++] = Serial2.read();
      
      if (index >= 12) {
        processData(buffer);
        index = 0;
        return;
      }
    }
  }
  
  if (index > 0) {
    Serial.println("Error: Data tidak lengkap");
    index = 0;
  }
}

void processData(byte* data) {
  memcpy(&receivedData[0], data, 4);
  memcpy(&receivedData[1], data+4, 4);
  memcpy(&receivedData[2], data+8, 4);

  // Tambahkan data ke total sum
  sumData[0] += receivedData[0];
  sumData[1] += receivedData[1];
  sumData[2] += receivedData[2];
  countSamples++;

  Serial.print("Diterima => ");
  Serial.print("A0: ");
  Serial.print(receivedData[0], 4);
  Serial.print(" | A1: ");
  Serial.print(receivedData[1], 4);
  Serial.print(" | A2: ");
  Serial.println(receivedData[2], 4);
}
