// G0ORX - RF Board MCP23017 control

#ifndef BEENHERE
#include "SDT.h"
#endif

static Adafruit_MCP23X17 mcp;
static bool failed;
static uint8_t GPB_state;
static uint8_t GPA_state;

// Pin mapping:
// GPA0: RX att 0.5
// GPA1: RX att 1
// GPA2: RX att 2
// GPA3: RX att 4
// GPA4: RX att 8
// GPA5: RX att 16
// GPA6: unused
// GPA7: MF/HF (0 means HF)
// GPB0: TX att 0.5
// GPB1: TX att 1
// GPB2: TX att 2
// GPB3: TX att 4
// GPB4: TX att 8
// GPB5: TX att 16
// GPB6: unused
// GPB7: unused

void set_bank_bit(uint8_t* GPIO_bank, uint8_t bit){
  GPIO_bank[0] = GPIO_bank[0] | (1 << bit);
}

void clear_bank_bit(uint8_t* GPIO_bank, uint8_t bit){
  GPIO_bank[0] = GPIO_bank[0] & (0xFF ^ (1 << bit) );
}

void toggle_bit(uint8_t *BANK, uint8_t bit){
    if ((BANK[0] & (1<<bit)) >> bit){
      clear_bank_bit(BANK, bit);
    } else {
      set_bank_bit(BANK, bit);
    }
}

void RFControlInit() {

  failed=false;

  mcp.begin_I2C(RF_MCP23017_ADDR);
  if (!mcp.begin_I2C(RF_MCP23017_ADDR)) {
    bit_results.RF_I2C_present = false;
    //ShowMessageOnWaterfall("RF MCP23017 not found at 0x"+String(RF_MCP23017_ADDR,HEX));
    failed=true;
  } else {
    bit_results.RF_I2C_present = true;
  }
  Debug("Initialising RF board");
  if(!failed) {
    for (int i=0;i<16;i++){
      mcp.pinMode(i, OUTPUT);
    }

    // Set all pins to zero. This means no attenuation and in HF mode
    GPA_state = 0x00;
    GPB_state = 0x00;
    mcp.writeGPIOA(GPA_state); 
    mcp.writeGPIOB(GPB_state); 
  }
}

void RFControl_Enable_Prescaler(bool status) {
  if(!failed) {
    if(status) {
      // MF Mode
      //mcp.digitalWrite(7,HIGH);
      set_bank_bit(&GPA_state, 7);
    } else {
      // HF Mode
      //mcp.digitalWrite(7,LOW);
      clear_bank_bit(&GPA_state, 7);
    }
    mcp.writeGPIOA(GPA_state);
  }
}

/*****
  Purpose: Set RF input Attenuator AFP 04-12-24
  Parameter list:
    void
  Return value;
    void
*****/
void SetRF_InAtten(int attenIn) {
  // GPA0: RX att 0.5
  // GPA1: RX att 1
  // GPA2: RX att 2
  // GPA3: RX att 4
  // GPA4: RX att 8
  // GPA5: RX att 16
  // GPA6: unused
  // GPA7: MF/HF (0 means HF)
  
  if(!failed) {
    // we ignore the last 0.5 dB bit and never set it
    GPA_state = (GPA_state & 0b11000000) | ((attenIn << 1) & 0xFF);
    mcp.writeGPIOA(GPA_state);
    Serial.print("InAtt State: ");
    Serial.println(GPA_state & 0b00111111 ,BIN);
  }
}

/*****
  Purpose: Set RF output Attenuator AFP 04-12-24
  Parameter list:
    int attenOut
  Return value;
    void
*****/
void SetRF_OutAtten(int attenOut) {
  // GPB0: TX att 0.5
  // GPB1: TX att 1
  // GPB2: TX att 2
  // GPB3: TX att 4
  // GPB4: TX att 8
  // GPB5: TX att 16
  // GPB6: unused
  // GPB7: unused
  
  if(!failed) {
    // we ignore the last 0.5 dB bit and never set it
    GPB_state = (GPB_state & 0b11000000) | ((attenOut << 1) & 0xFF);
    mcp.writeGPIOB(GPB_state);
    Serial.print("OutAtt State: ");
    Serial.println(GPB_state & 0b00111111 ,BIN);
  }
}
