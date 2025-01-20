/*
 * Rotary encoder library for Arduino.
 */



#ifndef Rotary_V12_H
#define Rotary_V12_H

/*
 * Note: BOURN encoders have their A/B pins reversed compared to cheaper encoders.
 */

#define BOURN_ENCODERS

// Enable weak pullups
#define ENABLE_PULLUPS


// Enable this to emit codes twice per step.
//#define HALF_STEP

// Values returned by 'process'
// No complete step yet.
#define DIR_NONE 0
// Clockwise step.
#define DIR_CW 1
// Counter-clockwise step.
#define DIR_CCW 2


class Rotary_V12 {
public:
  Rotary_V12(bool reversed);
  void updateA(unsigned char aState);
  void updateB(unsigned char bState);
  int process();

private:
  int aLastState;
  int bLastState;
  int value;
  bool _reversed;
};


#endif
