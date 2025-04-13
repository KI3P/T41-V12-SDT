#ifndef BEENHERE
#include "SDT.h"
#endif
#include <Wire.h>
#include <Adafruit_MCP23X17.h>
#include <math.h>
#include "AD7991.h"
#define LPF_BOARD_MCP23017_ADDR 0x25
#define TXSTATE 1
#define RXSTATE 0
#define TXRX_PIN 22

// The attenuation of the binocular toroid coupler and the attenuation of the pad
#define COUPLER_ATTENUATION_DB 20
#define PAD_ATTENUATION_DB 26
#define SWR_REPETITIONS 1  // 100 repetitions takes roughly 100 ms
#define VREF_MV 5000.0     // the reference voltage on your board




void read_SWR() {

  float adcF_sRaw=0;
  float adcR_sRaw=0;
  // Steo 1. Measure the peak forward and Reverse voltages
  adcF_sRaw = (float)swrADC.readADCsingle(0);
  adcR_sRaw = (float)swrADC.readADCsingle(1)+SWR_R_Offset[currentBand];
   
  //Serial.print("adcF_sRaw = ");
  //Serial.println(adcF_sRaw, 1); 
  //Serial.print("adcR_sRaw = ");
  //Serial.println(adcR_sRaw, 1);
  //Convert ADC reading to mV
  adcF_sRaw = adcF_sRaw * VREF_MV / 4096.;
  adcR_sRaw = adcR_sRaw * VREF_MV / 4096.;
  //Serial.print("adcF_sRaw = ");
  //Serial.println(adcF_sRaw, 1); 
  //Serial.print("adcR_sRaw = ");
  //Serial.println(adcR_sRaw, 1);
  // Convert to input voltage squared as read by ADC converted to before attenuation
  //Pf_W=(float)pow(10,(adcF_sRaw/25 - 84 + PAD_ATTENUATION_DB + COUPLER_ATTENUATION_DB)/10)/1000;
  //Pr_W=(float)pow(10,(adcR_sRaw/25 - 84 + PAD_ATTENUATION_DB + COUPLER_ATTENUATION_DB)/10)/1000;
  adcF_s = (float)pow(10,(adcF_sRaw/(25.+SWRSlopeAdj[currentBand]) - 84 + PAD_ATTENUATION_DB + COUPLER_ATTENUATION_DB+SWR_PowerAdj[currentBand])/10)*50/1000;  //84 is the zero intercept of the AD8307
  adcR_s = (float)pow(10,(adcR_sRaw/(25.+SWRSlopeAdj[currentBand]) - 84 + PAD_ATTENUATION_DB + COUPLER_ATTENUATION_DB+SWR_PowerAdj[currentBand])/10)*50/1000;
  //Serial.print("adcF_s volt sq= ");
  //Serial.println(adcF_s, 1); 
  //Serial.print("adcR_s  volt sq= ");
  //Serial.println(adcR_s, 1);
  adcF_s = 0.1 * adcF_s + 0.9 * adcF_sOld;  //Running average
  adcR_s = 0.1 * adcR_s + 0.9 * adcR_sOld;
  adcF_sOld = adcF_s;
  adcR_sOld = adcR_s;
  //Serial.print("adcF_s averaged = ");
  //Serial.println(adcF_s, 1); 
  //Serial.print("adcR_s averaged= ");
  //Serial.println(adcR_s, 1);

  Pf_W = adcF_s/ 50;
  Pr_W =adcR_s/ 50;
  float A = pow(Pr_W / Pf_W, 0.5);
  swr = (1.0 + A) / (1.0 - A);
  //Serial.print("Pf_W = ");
  //Serial.println(Pf_W,1);
  //Serial.print("Pr_W = ");
  //Serial.println(Pr_W,1);
  //Serial.print("A = ");
  //Serial.println(A, 4);
  //Serial.print("SWR = ");
  //Serial.println(swr, 2);
  //Serial.println("------ ");
  //ShowCurrentPowerSetting();
}


void setupSWR() {

  uint8_t GPB_state;
  uint8_t GPA_state;

  Wire2.begin();
  mcpLPF.begin_I2C(LPF_BOARD_MCP23017_ADDR, &Wire2);

  mcpLPF.enableAddrPins();
  // Set all pins to be outputs
  for (int i = 0; i < 16; i++) {
    mcpLPF.pinMode(i, OUTPUT);
  }
  GPA_state = 0x00;
  mcpLPF.writeGPIOA(GPA_state);
  GPA_state = GPA_state ^ 0b00000001;
  mcpLPF.writeGPIOA(GPA_state);
  GPB_state = 0b01001111;
  mcpLPF.writeGPIOB(GPB_state);

  while (!swrADC.begin(AD7991_I2C_ADDR1, &Wire2)) {
    Serial.println("AD7991 not found at 0x" + String(AD7991_I2C_ADDR1, HEX));

    if (swrADC.begin(AD7991_I2C_ADDR2, &Wire2)) {
      Serial.println("AD7991 found at alternative 0x" + String(AD7991_I2C_ADDR2, HEX));
      break;
    }
    scanner();
  }
}

void scanner() {
  byte error, address;
  int nDevices;

  Serial.println("Scanning...");

  nDevices = 0;
  for (address = 1; address < 127; address++) {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.print(address, HEX);
      Serial.println("  !");

      nDevices++;
    } else if (error == 4) {
      Serial.print("Unknown error at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.println(address, HEX);
    }
  }
  if (nDevices == 0)
    Serial.println("No I2C devices found\n");
  else
    Serial.println("done\n");
}