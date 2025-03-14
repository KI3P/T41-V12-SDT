// V12 LPF Board MCP23017 control (added by KI3P)

#ifndef BEENHERE
#include "SDT.h"
#endif

#include "LPF_Control_V12.h"

//static Adafruit_MCP23X17 mcpLPF; // connected to Wire2
//static AD7991 swrADC; // on V12 LPF board
static uint8_t LPF_GPB_state;
static uint8_t LPF_GPA_state;
static char strBuf[100];
  
/*****
  Purpose: Initialize the V12 LPF control board: begin I2C and set 
  registers to default states.

  Parameter list:
    None
  Return value;
    None
*****/
void V12_LPFControlInit() {
  /******************************************************************
   * Set up the V12 LPF which is connected via the BANDS connector *
   ******************************************************************/
  if (mcpLPF.begin_I2C(V12_LPF_MCP23017_ADDR,&Wire2)){
    Debug("Initializing V12 LPF board");
    mcpLPF.enableAddrPins();
    // Set all pins to be outputs
    for (int i=0;i<16;i++){
      mcpLPF.pinMode(i, OUTPUT);
    }
    LPF_GPA_state = LPF_STARTUP_STATE_A;
    mcpLPF.writeGPIOA(LPF_GPA_state); 
    LPF_GPB_state = LPF_STARTUP_STATE_B;
    mcpLPF.writeGPIOB(LPF_GPB_state); 
    Debug("Startup LPF GPB state: "+String(LPF_GPB_state,DEC));
    bit_results.V12_LPF_I2C_present = true;
  } else {
    Debug("LPF MCP23017 not found at 0x"+String(V12_LPF_MCP23017_ADDR,HEX));
    bit_results.V12_LPF_I2C_present = false;
    //ShowMessageOnWaterfall("LPF MCP23017 not found at 0x"+String(V12_LPF_MCP23017_ADDR,HEX));
  }
  // This is the ADC module for the SWR meter
  if (!swrADC.begin(AD7991_I2C_ADDR1,&Wire2)){
    bit_results.V12_LPF_AD7991_present = false;
    Debug("AD7991 not found at 0x"+String(AD7991_I2C_ADDR1,HEX));
    //ShowMessageOnWaterfall("AD7991 not found at 0x"+String(AD7991_I2C_ADDR1,HEX));

    if (!swrADC.begin(AD7991_I2C_ADDR2,&Wire2)){
      bit_results.V12_LPF_AD7991_present = true;
      bit_results.AD7991_I2C_ADDR = AD7991_I2C_ADDR2;
      Debug("AD7991 found at alternative 0x"+String(AD7991_I2C_ADDR2,HEX));
      //ShowMessageOnWaterfall("AD7991 found at alternative 0x"+String(AD7991_I2C_ADDR2,HEX));
    }
  } else {
    bit_results.V12_LPF_AD7991_present = true;
    bit_results.AD7991_I2C_ADDR = AD7991_I2C_ADDR1;
  }
}

/*****
  Purpose: place the selected LPF band into the signal path

  Parameter list:
    int currentBand             the band number as defined in SDT.h
  Return value;
    None
*****/
void setLPFBand(int currentBand) {
  switch (currentBand){
   // case BAND_160M:
   //   LPF_GPB_state = LPF_GPB_state & 0b11110000;
    //  LPF_GPB_state = LPF_GPB_state | LPF_BAND_160M;
    //  break;
    case BAND_80M:
      LPF_GPB_state = LPF_GPB_state & 0b11110000;
      LPF_GPB_state = LPF_GPB_state | LPF_BAND_80M;
      break;
    case BAND_60M:
      LPF_GPB_state = LPF_GPB_state & 0b11110000;
     LPF_GPB_state = LPF_GPB_state | LPF_BAND_60M;
      break;   
    case BAND_40M:
      LPF_GPB_state = LPF_GPB_state & 0b11110000;
      LPF_GPB_state = LPF_GPB_state | LPF_BAND_40M;
      break;
    case BAND_30M:
      LPF_GPB_state = LPF_GPB_state & 0b11110000;
      LPF_GPB_state = LPF_GPB_state | LPF_BAND_30M;
      break;
    case BAND_20M:
      LPF_GPB_state = LPF_GPB_state & 0b11110000;
      LPF_GPB_state = LPF_GPB_state | LPF_BAND_20M;
      break;
    case BAND_17M:
      LPF_GPB_state = LPF_GPB_state & 0b11110000;
      LPF_GPB_state = LPF_GPB_state | LPF_BAND_17M;
      break;
    case BAND_15M:
      LPF_GPB_state = LPF_GPB_state & 0b11110000;
      LPF_GPB_state = LPF_GPB_state | LPF_BAND_15M;
      break;
    case BAND_12M:
      LPF_GPB_state = LPF_GPB_state & 0b11110000;
      LPF_GPB_state = LPF_GPB_state | LPF_BAND_12M;
      break;
    case BAND_10M:
      LPF_GPB_state = LPF_GPB_state & 0b11110000;
      LPF_GPB_state = LPF_GPB_state | LPF_BAND_10M;
      break;
    case BAND_6M:
      LPF_GPB_state = LPF_GPB_state & 0b11110000;
      LPF_GPB_state = LPF_GPB_state | LPF_BAND_6M;
     break;
    default:
      LPF_GPB_state = LPF_GPB_state & 0b11110000;
      LPF_GPB_state = LPF_GPB_state | LPF_BAND_NF;
      break;
  }
  mcpLPF.writeGPIOA(LPF_GPA_state);
  mcpLPF.writeGPIOB(LPF_GPB_state);
  Debug("Set LPF GPA state: "+String(LPF_GPA_state,DEC));
  Debug("Set LPF GPB state: "+String(LPF_GPB_state,DEC));
}

/*****
  Purpose: control whether the BPF is placed in the TX signal path, the
  RX signal path, or neither.

  Parameter list:
    int pathSelection           Valid values are BPF_IN_RX_PATH, BPF_IN_TX_PATH,
                                or BPF_NOT_IN_PATH
  Return value;
    None
*****/
void setBPFPath(int pathSelection){
  switch (pathSelection){
    case BPF_IN_RX_PATH:
      LPF_GPA_state = (LPF_GPA_state & 0b11111100) | 0b00000010;
      break;
    case BPF_IN_TX_PATH:
      LPF_GPA_state = (LPF_GPA_state & 0b11111100) | 0b00000001;
      break;
    case BPF_NOT_IN_PATH:
      LPF_GPA_state = (LPF_GPA_state & 0b11111100) | 0b00000000;
      break;
    default:
      sprintf(strBuf, "V12 LPF Control: Invalid BPF path selection! %d [0,2]",pathSelection);
      Debug(strBuf);
      break;
  }
  mcpLPF.writeGPIOA(LPF_GPA_state); 
}

/*****
  Purpose: select which antenna is connected to the T/R switch

  Parameter list:
    int antennaNum           Valid values are 0 to 3
  Return value;
    None
*****/
void selectAntenna(int antennaNum){
  //antennaNum=0;
  // GPB4   Antenna I2C0    0
  // GPB5   Antenna I2C1    0
  if ((antennaNum >= 0) & (antennaNum <=3)){
    LPF_GPB_state = LPF_GPB_state & 0b11001111;
    LPF_GPB_state = LPF_GPB_state | (antennaNum << 4);
    mcpLPF.writeGPIOB(LPF_GPB_state);
  } else {
    sprintf(strBuf, "V12 LPF Control: Invalid antenna selection! %d [0,3]",antennaNum);
    Debug(strBuf);
  }
}

/*****
  Purpose: select whether 100W PA is placed in transmit path or not

  Parameter list:
    bool selection           true if 100W PA is in path, false if not
  Return value;
    None
*****/
void select100WPA(bool selection){
  // GPB7   100W_PA_SEL     0 (0 means no 100W)
  if (selection){
    // Place 100W in path
    LPF_GPB_state = LPF_GPB_state | 0b10000000;
  } else {
    // Do not place 100W in path
    LPF_GPB_state = LPF_GPB_state & 0b01111111;
  }
  mcpLPF.writeGPIOB(LPF_GPB_state);
}

/*****
  Purpose: select whether RF transmit is routed to transverter or not

  Parameter list:
    bool selection           true if transmit is routed to transverter, false if not
  Return value;
    None
*****/
void selectXVTR(bool selection){
  //GPB6   XVTR_SEL        1 (1 means no XVTR)
  if (selection){
    // Place XVTR in path
    LPF_GPB_state = LPF_GPB_state & 0b10111111;
  } else {
    // Do not place XVTR in path
    LPF_GPB_state = LPF_GPB_state | 0b01000000;
  }
  mcpLPF.writeGPIOB(LPF_GPB_state);
}

