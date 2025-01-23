#include <SPI.h>
#include <digitalWriteFast.h> // BE SURE YOU HAVE INSTALLED THE DIGITAL WRITE FAST LIBRARY FROM HERE: https://github.com/NicksonYap/digitalWriteFast

// IF YOU ARE USING TEENSY, CHECK OUT MY OTHER LIBRARY ON MY GITHUB: https://github.com/mbilsky/TeensyADS1256

// (other stuff for getting the ADS1526 to work is in the next tab
// IF YOU CHANGE THESE, YOU NEED TO UPDATE THE CODE ON THE NEXT TAB...
// ALL THE DIGITAL WRITE FAST NUMBERS NEED TO BE UPDATED TO MATCH YOUR DESIRED CS PIN
// THE DATA READY PIN NEEDS TO BE AN INTERRUPT PIN: https://www.arduino.cc/reference/en/language/functions/external-interrupts/attachinterrupt/

#define ADS_RST_PIN    17 //ADS1256 reset pin (ESP32 GPIO16)
#define ADS_RDY_PIN    16  //ADS1256 data ready (ESP32 GPIO4)
#define ADS_CS_PIN     5  //ADS1256 chip select (ESP32 GPIO5)

/* Keep this commented...
 *  THESE ARE THE PINS ON THE ESP32 YOU SHOULD WIRE THESE TO
    CLK  - SCK (GPIO18)
    DIN  - MOSI (GPIO23)
    DOUT - MISO (GPIO19)
*/

// put the ADC constants here

double resolution = 8388608.; // 2^23-1

// this needs to match the setting in the ADC init function in the library tab
double Gain = 64.; // be sure to have a period

double vRef = 3.3; // reference voltage

// we'll calculate this in setup
double bitToVolt = 0.;

// Variables for data counting
unsigned long lastTime = 0;
unsigned long dataCount = 0;

void setup() {
  delay(1000);
  Serial.begin(115200);
  Serial.println("booting");

  // initialize the ADS
  pinMode(ADS_CS_PIN, OUTPUT);
  pinMode(ADS_RDY_PIN, INPUT);
  pinMode(ADS_RST_PIN, OUTPUT);

  SPI.begin(18, 19, 23); // Initialize SPI with SCK=GPIO18, MISO=GPIO19, MOSI=GPIO23

  initADS();
  Serial.println("done init");

  // determine the conversion factor
  // do some calculations for the constants
  bitToVolt = resolution * Gain / vRef;

  lastTime = millis(); // Initialize timing
}

int32_t val1;
int32_t val2;
int32_t val3;

void loop() {
  // Read three values from the ADS1256
  read_three_values();
  // read_two_values();
  // read_Value();

  // Print data in "data 1 | data 2 | ..." format
  Serial.print(val1);
  Serial.print(" | ");
  Serial.print(val2);
  Serial.print(" | ");
  Serial.println(val3);

  // Serial.print(val1);
  // Serial.print(" | ");
  // Serial.println(val2);

  // Serial.println(val1);

  // Increment data count
  dataCount++;

  // Calculate and print data rate every second
  if (millis() - lastTime >= 1000) {  
    Serial.print("================================== Data rate: ");
    Serial.print(dataCount);
    Serial.println(" data/s");
    dataCount = 0; // Reset data count
    lastTime = millis(); // Reset timer
  }
}