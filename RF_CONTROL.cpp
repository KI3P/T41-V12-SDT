// G0ORX - RF Board MCP23017 control

#ifndef BEENHERE
#include "SDT.h"
#endif

#define MCP23017_ADDR 0x27

static Adafruit_MCP23X17 mcp;
static bool failed;


void RFControlInit() {

  failed=false;

  mcp.begin_I2C(MCP23017_ADDR);
  if (!mcp.begin_I2C(MCP23017_ADDR)) {
    ShowMessageOnWaterfall("RF MCP23017 not found at 0x"+String(MCP23017_ADDR,HEX));
    failed=true;
  }
  Debug("Initialising RF board");
  if(!failed) {
    mcp.pinMode(0, OUTPUT);
    mcp.pinMode(1, OUTPUT);
    mcp.pinMode(2, OUTPUT);
    mcp.pinMode(3, OUTPUT);
    mcp.pinMode(4, OUTPUT);
    mcp.pinMode(5, OUTPUT);
    mcp.pinMode(6, OUTPUT);
    mcp.pinMode(7, OUTPUT);
    mcp.pinMode(8, OUTPUT);
    mcp.pinMode(9, OUTPUT);
    mcp.pinMode(10, OUTPUT);
    mcp.pinMode(11, OUTPUT);
    mcp.pinMode(12, OUTPUT);
    mcp.pinMode(13, OUTPUT);
    mcp.pinMode(14, OUTPUT);
    mcp.pinMode(15, OUTPUT);

    mcp.digitalWrite(0,LOW);
    mcp.digitalWrite(1,LOW);
    mcp.digitalWrite(2,LOW);
    mcp.digitalWrite(3,LOW);
    mcp.digitalWrite(4,LOW);
    mcp.digitalWrite(5,LOW);
    mcp.digitalWrite(6,LOW);
    mcp.digitalWrite(7,LOW);
    mcp.digitalWrite(8,LOW);
    mcp.digitalWrite(9,LOW);
    mcp.digitalWrite(10,LOW);
    mcp.digitalWrite(11,LOW);
    mcp.digitalWrite(12,LOW);
    mcp.digitalWrite(13,LOW);
    mcp.digitalWrite(14,LOW);

  }
}

void RFControl_Enable_Prescaler(bool status) {
  if(!failed) {
    if(status) {
      mcp.digitalWrite(7,HIGH);
    } else {
      mcp.digitalWrite(7,LOW);
    }
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
  int nQ;
  int rem;
  //mcp.begin_I2C(0x27);

  if(!failed) {
    for (int i = 0; i < 6; i++) {
      nQ = (int)attenIn / 2;
      rem = attenIn - 2 * nQ;
      mcp.digitalWrite(i + 1, rem);
      attenIn = nQ;
    }
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
  int nQ;
  int rem;
  //mcp.begin_I2C(0x27);

  if(!failed) {
    for (int i = 0; i < 6; i++) {
      nQ = (int)attenOut / 2;
      rem = attenOut - 2 * nQ;
      mcp.digitalWrite(i + 8, rem);
      attenOut = nQ;
    }
  }
}
