// V12 BPF Board MCP23017 control (added by KI3P)

#ifndef BEENHERE
#include "SDT.h"
#endif

#include "BPF_Control.h"

static Adafruit_MCP23X17 mcpBPF; // connected to Wire2
static uint16_t BPF_GPAB_state;

void BPFControlInit() {
  /**********************************************************************
   * Set up the BPF which is connected via the BANDS connector on Wire2 *
   **********************************************************************/
  if (mcpBPF.begin_I2C(BPF_MCP23017_ADDR,&Wire2)){
    bit_results.BPF_I2C_present = true;
    Debug("Initialising BPF board");
    mcpBPF.enableAddrPins();
    // Set all pins to be outputs
    for (int i=0;i<16;i++){
      mcpBPF.pinMode(i, OUTPUT);
    }
    // Set to BYPASS for startup.
    BPF_GPAB_state = BPF_BAND_BYPASS;
    mcpBPF.writeGPIOAB(BPF_GPAB_state); 
  } else {
    bit_results.BPF_I2C_present = false;
    Debug("BPF MCP23017 not found at 0x"+String(BPF_MCP23017_ADDR,HEX));
    //ShowMessageOnWaterfall("BPF MCP23017 not found at 0x"+String(BPF_MCP23017_ADDR,HEX));
  }
}

void setBPFBand(int currentBand) {
  switch (currentBand){
    case BAND_160M:
      BPF_GPAB_state = BPF_BAND_160M;
      break;
    case BAND_80M:
      BPF_GPAB_state = BPF_BAND_80M;
      break;
    case BAND_60M:
      BPF_GPAB_state = BPF_BAND_60M;
      break;   
    case BAND_40M:
      BPF_GPAB_state = BPF_BAND_40M;
      break;
    case BAND_30M:
      BPF_GPAB_state = BPF_BAND_30M;
      break;
    case BAND_20M:
      BPF_GPAB_state = BPF_BAND_20M;
      break;
    case BAND_17M:
      BPF_GPAB_state = BPF_BAND_17M;
      break;
    case BAND_15M:
      BPF_GPAB_state = BPF_BAND_15M;
      break;
    case BAND_12M:
      BPF_GPAB_state = BPF_BAND_12M;
      break;
    case BAND_10M:
      BPF_GPAB_state = BPF_BAND_10M;
      break;
    case BAND_6M:
      BPF_GPAB_state = BPF_BAND_6M;
      break;
    default:
      BPF_GPAB_state = BPF_BAND_BYPASS;
      break;
  }
  mcpBPF.writeGPIOAB(BPF_GPAB_state);
  Debug("Set BPF state: "+String(BPF_GPAB_state,HEX));

}
