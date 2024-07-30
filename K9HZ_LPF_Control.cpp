// K9HZ LPF Board MCP23017 control (added by KI3P)

#ifndef BEENHERE
#include "SDT.h"
#endif

#include "K9HZ_LPF_Control.h"

static Adafruit_MCP23X17 mcpLPF; // connected to Wire2
static AD7991 swrADC; // on K9HZ LPF board
static uint8_t LPF_GPB_state;
static uint8_t LPF_GPA_state;

void K9HZLPFControlInit() {
  /******************************************************************
   * Set up the K9HZ LPF which is connected via the BANDS connector *
   ******************************************************************/
  if (mcpLPF.begin_I2C(K9HZ_LPF_ADDR,&Wire2)){
    Debug("Initializing K9HZ LPF board");
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
  } else {
    Debug("LPF MCP23017 not found at 0x"+String(K9HZ_LPF_ADDR,HEX));
    ShowMessageOnWaterfall("LPF MCP23017 not found at 0x"+String(K9HZ_LPF_ADDR,HEX));
  }
  // This is the ADC module for the SWR meter
  if (!swrADC.begin(AD7991_I2C_ADDR1,&Wire2)){
    Debug("AD7991 not found at 0x"+String(AD7991_I2C_ADDR1,HEX));
    ShowMessageOnWaterfall("AD7991 not found at 0x"+String(AD7991_I2C_ADDR1,HEX));

    if (!swrADC.begin(AD7991_I2C_ADDR2,&Wire2)){
      Debug("AD7991 found at alternative 0x"+String(AD7991_I2C_ADDR2,HEX));
      ShowMessageOnWaterfall("AD7991 found at alternative 0x"+String(AD7991_I2C_ADDR2,HEX));
    }
  }
}

void setLPFBand(int currentBand) {
  switch (currentBand){
    case BAND_160M:
      LPF_GPB_state = LPF_GPB_state & 0b11110000;
      LPF_GPB_state = LPF_GPB_state | LPF_BAND_160M;
      break;
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
