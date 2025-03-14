#ifndef FRONTPANEL_H
#define FRONTPANEL_H

#include <stdint.h>
#include "Rotary_V12.h"

#define AUDIO_VOLUME 0
#define MIC_GAIN 1
#define AGC_GAIN 2
#define SIDETONE_VOLUME 3
#define NOISE_FLOOR_LEVEL 4
#define SQUELCH_LEVEL 5
#define FREQ_OFFSET 6

extern Rotary_V12 volumeEncoder;
extern Rotary_V12 filterEncoder;
extern Rotary_V12 tuneEncoder;
extern Rotary_V12 fineTuneEncoder;

extern int volumeFunction;
extern int my_ptt;


extern void FrontPanelSetLed(int led, uint8_t state);

extern void PTT_Interrupt();

#endif