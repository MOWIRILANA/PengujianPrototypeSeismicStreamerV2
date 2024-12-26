#define cs 5 // chip select
#define rdy 21 // data ready, input

#define SPISPEED 2500000

#include <SPI.h>

void setup()
{
  Serial.begin(115200);
  
  pinMode(cs, OUTPUT);
  digitalWrite(cs, LOW); // tied low is also OK.
  pinMode(rdy, INPUT);
  
  delay(500);
  SPI.begin(); //start the spi-bus
  delay(500);

  //init
  while (digitalRead(rdy)) {}  // wait for ready_line to go low
  SPI.beginTransaction(SPISettings(SPISPEED, MSBFIRST, SPI_MODE1)); // start SPI
  digitalWrite(cs, LOW);
  delayMicroseconds(100);

  //Reset to Power-Up Values (FEh)
  SPI.transfer(0xFE);
  delay(5);

  byte status_reg = 0x00 ;  // address (datasheet p. 30)
  byte status_data = 0x01; // 01h = 0000 0 0 0 1 => status: Most Significant Bit First, Auto-Calibration Disabled, Analog Input Buffer Disabled
  //byte status_data = 0x07; // 01h = 0000 0 1 1 1 => status: Most Significant Bit First, Auto-Calibration Enabled, Analog Input Buffer Enabled
  SPI.transfer(0x50 | status_reg);
  SPI.transfer(0x00);   // 2nd command byte, write one register only
  SPI.transfer(status_data);   // write the databyte to the register
  delayMicroseconds(100);
  

  byte adcon_reg = 0x02; //A/D Control Register (Address 02h)
  //byte adcon_data = 0x20; // 0 01 00 000 => Clock Out Frequency = fCLKIN, Sensor Detect OFF, gain 1
  byte adcon_data = 0x00; // 0 00 00 000 => Clock Out = Off, Sensor Detect OFF, gain 1
  //byte adcon_data = 0x01;   // 0 00 00 001 => Clock Out = Off, Sensor Detect OFF, gain 2
  SPI.transfer(0x50 | adcon_reg);  // 52h = 0101 0010
  SPI.transfer(0x00);              // 2nd command byte, write one register only
  SPI.transfer(adcon_data);        // write the databyte to the register
  delayMicroseconds(100);

  byte drate_reg = 0x03; //DRATE: A/D Data Rate (Address 03h)
  byte drate_data = 0xF0; // F0h = 11110000 = 30,000SPS
  SPI.transfer(0x50 | drate_reg);
  SPI.transfer(0x00);   // 2nd command byte, write one register only
  SPI.transfer(drate_data);   // write the databyte to the register
  delayMicroseconds(100);

  // Perform Offset and Gain Self-Calibration (F0h)
  SPI.transfer(0xF0);     
  delay(400);
  digitalWrite(cs, HIGH);
  SPI.endTransaction();
  
  while (!Serial && (millis ()  <=  5000));  // WAIT UP TO 5000 MILLISECONDS FOR SERIAL OUTPUT CONSOLE
  Serial.println("configured, starting");
  Serial.println("");
  Serial.println("AIN0-AINCOM  AIN1-AINCOM  AIN2-AINCOM  AIN3-AINCOM  AIN4-AINCOM  AIN5-AINCOM  AIN6-AINCOM  AIN7-AINCOM");
}


void loop()
{

  //Single ended Measurements
  unsigned long adc_val[8] = {0,0,0,0,0,0,0,0}; // store readings in array
  byte mux[8] = {0x08,0x18,0x28,0x38,0x48,0x58,0x68,0x78};
  
  int i = 0;

  SPI.beginTransaction(SPISettings(SPISPEED, MSBFIRST, SPI_MODE1)); // start SPI
  digitalWrite(cs, LOW);
  delayMicroseconds(2);
  
  for (i=0; i <= 7; i++){         // read all 8 Single Ended Channels AINx-AINCOM
  byte channel = mux[i];             // analog in channels # 
  
  while (digitalRead(rdy)) {} ;                          

  //byte data = (channel << 4) | (1 << 3); //AIN-channel and AINCOM   // ********** Step 1 **********
  //byte data = (channel << 4) | (1 << 1)| (1); //AIN-channel and AINCOM   // ********** Step 1 **********
  //Serial.println(channel,HEX);
  SPI.transfer(0x50 | 0x01); // 1st Command Byte: 0101 0001  0001 = MUX register address 01h
  SPI.transfer(0x00);     // 2nd Command Byte: 0000 0000  1-1=0 write one byte only
  SPI.transfer(channel);     // Data Byte(s): xxxx 1000  write the databyte to the register(s)
  delayMicroseconds(2);

  //SYNC command 1111 1100                               // ********** Step 2 **********
  SPI.transfer(0xFC);
  delayMicroseconds(2);

  //while (!digitalRead(rdy)) {} ;
  //WAKEUP 0000 0000
  SPI.transfer(0x00);
  delayMicroseconds(250);   // Allow settling time

/*
MUX : Input Multiplexer Control Register (Address 01h)
Reset Value = 01h
BIT 7    BIT 6    BIT 5    BIT 4    BIT 3    BIT 2    BIT 1    BIT 0
PSEL3    PSEL2    PSEL1    PSEL0    NSEL3    NSEL2    NSEL1    NSEL0
Bits 7-4 PSEL3, PSEL2, PSEL1, PSEL0: Positive Input Channel (AINP) Select
0000 = AIN0 (default)
0001 = AIN1
0010 = AIN2 (ADS1256 only)
0011 = AIN3 (ADS1256 only)
0100 = AIN4 (ADS1256 only)
0101 = AIN5 (ADS1256 only)
0110 = AIN6 (ADS1256 only)
0111 = AIN7 (ADS1256 only)
1xxx = AINCOM (when PSEL3 = 1, PSEL2, PSEL1, PSEL0 are “don’t care”)
NOTE: When using an ADS1255 make sure to only select the available inputs.

Bits 3-0 NSEL3, NSEL2, NSEL1, NSEL0: Negative Input Channel (AINN)Select
0000 = AIN0
0001 = AIN1 (default)
0010 = AIN2 (ADS1256 only)
0011 = AIN3 (ADS1256 only)
0100 = AIN4 (ADS1256 only)
0101 = AIN5 (ADS1256 only)
0110 = AIN6 (ADS1256 only)
0111 = AIN7 (ADS1256 only)
1xxx = AINCOM (when NSEL3 = 1, NSEL2, NSEL1, NSEL0 are “don’t care”)
NOTE: When using an ADS1255 make sure to only select the available inputs.
 */

  SPI.transfer(0x01); // Read Data 0000  0001 (01h)       // ********** Step 3 **********
  delayMicroseconds(5);
  
  adc_val[i] = SPI.transfer(0);
  adc_val[i] <<= 8; //shift to left
  adc_val[i] |= SPI.transfer(0);
  adc_val[i] <<= 8;
  adc_val[i] |= SPI.transfer(0);
  delayMicroseconds(2);
  }                                // Repeat for each channel ********** Step 4 **********
  
  digitalWrite(cs, HIGH);
  SPI.endTransaction();

  for (i=0; i <= 7; i++){   // Single ended Measurements 
  if(adc_val[i] > 0x7fffff){   //if MSB == 1
    adc_val[i] = adc_val[i]-16777216; //do 2's complement
  }

  Serial.print(adc_val[i]);   // Raw ADC integer value +/- 23 bits
  Serial.print("      ");
  }
  Serial.println();
  delay(250);

}