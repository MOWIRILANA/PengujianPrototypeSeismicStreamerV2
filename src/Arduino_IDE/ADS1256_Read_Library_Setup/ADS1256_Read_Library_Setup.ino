#include <ADS1256.h>

// Inisialisasi pin sesuai dengan konfigurasi perangkat keras
ADS1256 A(21, 0, 0, 5, 2.500); // DRDY, RESET, SYNC(PDWN), CS, VREF(float)

// Variabel untuk menghitung jumlah data per detik
unsigned long prevMillis = 0;
unsigned int dataCount = 0;

void setup() {
  Serial.begin(115200); // Pastikan baud rate sesuai dengan kebutuhan komunikasi serial

  // Tunggu hingga serial siap
  while (!Serial) { ; }

  // Inisialisasi ADS1256
  Serial.println("Inisialisasi ADS1256...");
  A.InitializeADC();

  // Konfigurasi awal: set PGA, kanal, dan DRATE
  A.setPGA(PGA_1);             // Penguatan = 1
  A.setMUX(SING_0);          // Membaca input diferensial antara AIN0 dan AIN1
  A.setDRATE(DRATE_500SPS);    // Sampling rate = 100 sampel per detik

  Serial.println("Inisialisasi selesai.");
}

void loop() {
  // Baca data dari ADS1256
  long rawValue = A.readSingle();
  float voltage = A.convertToVoltage(rawValue);
  dataCount++;

  // Cetak nilai mentah dan tegangan
  Serial.print("Nilai mentah: ");
  Serial.print(rawValue);
  Serial.print("\tTegangan: ");
  Serial.println(voltage, 6);

  // Hitung jumlah data per detik
  unsigned long currentMillis = millis();
  if (currentMillis - prevMillis >= 1000) {
    Serial.print("Jumlah data dalam 1 detik: ");
    Serial.println(dataCount);
    dataCount = 0;
    prevMillis = currentMillis;
  }
}
