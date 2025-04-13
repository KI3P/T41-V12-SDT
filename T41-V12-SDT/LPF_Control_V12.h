#ifndef LPF_CONTROL_V12_h
#define LPF_CONTROL_V12_h

#include <Adafruit_MCP23X17.h> 
#include "AD7991.h"

// The LPF band control definitions
#define LPF_BAND_NF 0b1111
#define LPF_BAND_6M 0b1010
#define LPF_BAND_10M 0b1001
#define LPF_BAND_12M 0b1000
#define LPF_BAND_15M 0b0111
#define LPF_BAND_17M 0b0110
#define LPF_BAND_20M 0b0101
#define LPF_BAND_30M 0b0100
#define LPF_BAND_40M 0b0011
#define LPF_BAND_60M 0b0000
#define LPF_BAND_80M 0b0010
#define LPF_BAND_160M 0b0001

// Register A pin mapping:
// Bit    Description     Default
// GPA0   TX BPF          0 (BPF in path)
// GPA1   RX BPF          0 (BPF in path)
// GPA2-GPA7   Not used
#define LPF_STARTUP_STATE_A 0x00
#define BPF_IN_RX_PATH   0
#define BPF_IN_TX_PATH   1
#define BPF_NOT_IN_PATH  2

// Register B pin mapping:
// Bit    Description     Default
// GPB0   Band I2C0       1
// GPB1   Band I2C1       1
// GPB2   Band I2C2       1
// GPB3   Band I2C3       1
// GPB4   Antenna I2C0    0
// GPB5   Antenna I2C1    0
// GPB6   XVTR_SEL        1 (1 means no XVTR)
// GPB7   100W_PA_SEL     0 (0 means no 100W)
#define LPF_STARTUP_STATE_B 0b01001111

void V12_LPFControlInit();
void setLPFBand(int currentBand);
void setBPFPath(int pathSelection);
void selectAntenna(int antennaNum);
void select100WPA(bool selection);
void selectXVTR(bool selection);

#endif // V12_LPF_CONTROL_h