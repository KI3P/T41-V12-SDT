#ifndef BPF_CONTROL_h
#define BPF_CONTROL_h

#include <Adafruit_MCP23X17.h> 
#include "AD7991.h"

// BPF band control word definitions
#define BPF_BAND_BYPASS 0x0008
#define BPF_BAND_6M     0x0004
#define BPF_BAND_10M    0x0002
#define BPF_BAND_12M    0x0001
#define BPF_BAND_15M    0x8000
#define BPF_BAND_17M    0x4000
#define BPF_BAND_20M    0x2000
#define BPF_BAND_30M    0x1000
#define BPF_BAND_40M    0x0800
#define BPF_BAND_60M    0x0100
#define BPF_BAND_80M    0x0400
#define BPF_BAND_160M   0x0200

void BPFControlInit();
void setBPFBand(int currentBand);

#endif // BPF_CONTROL_h