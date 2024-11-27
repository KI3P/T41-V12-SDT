#ifndef BEENHERE
#include "SDT.h"
#endif

#ifdef V12HWR // changes are so extensive for calibration in V12, this file supports only V12

// Updates to DoReceiveCalibration() and DoXmitCalibrate() functions by .  July 20, 2023
// Updated PlotCalSpectrum() function to clean up graphics.   August 3, 2023
// Major clean-up of calibration.   August 16, 2023

#define RX_STATE            0
#define TX_STATE            1
#define TX_STATE_RX_PHASE   2
#define TX_STATE_TX_PHASE   3
#define GAIN_COARSE_MAX       1.3
#define GAIN_COARSE_MIN       0.7
#define PHASE_COARSE_MAX      0.2
#define PHASE_COARSE_MIN      -0.2
#define GAIN_COARSE_STEP2_N   4
#define PHASE_COARSE_STEP2_N  4
#define GAIN_FINE_N           5
#define PHASE_FINE_N          5
static int stateMachine;
static bool FFTupdated;
static int val;
static int corrChange;
static float correctionIncrement;  //AFP 2-7-23
static int userScale, userZoomIndex, userXmtMode;
static int transmitPowerLevelTemp;
static char strBuf[100];
static long userCurrentFreq;
static long userCenterFreq;
static long userTxRxFreq;
static long userNCOFreq;
static float adjdB;

#ifdef QUADFFT
/*****
  Purpose: DrawIQBalancePlotContainer

  Parameter list:
    float IQfreqStart, IQfreqStop, long numIQPoints

  Return value;
    void

 
*****/
static int IQfreqStart_kHz,IQfreqStop_kHz,numIQPoints;
static float frmax;
static uint32_t index_of_max;
static bool corrections_calculated = false;
float r,i;
static int delay_R = 0;
static float scale_R = 1.0;

void DrawIQBalancePlotContainer() {
  tft.writeTo(L1);
  tft.fillRect(0, 0, 799, 479, RA8875_BLACK);
  tft.writeTo(L2);
  tft.fillRect(0, 0, 799, 479, RA8875_BLACK);

  #define xstart   50
  #define ystart   50
  #define xend     750
  #define yend     400
  #define Nxticks  10
  #define Nyticks  10
  #define xpixels_per_tick  ((float)(xend-xstart))/Nxticks
  #define ypixels_per_tick  ((float)(yend-ystart))/Nyticks
  #define xpixels_per_kHz   ((xend-xstart)/((float)(IQfreqStop_kHz-IQfreqStart_kHz))) // -100 kHz to +100 kHz
  #define ypixels_per_deg   ((yend-ystart)/(100.0-(-100.0))) // -100 deg to +100 deg
  #define ypixels_per_unit  ((yend-ystart)/(1.5-(0.5)))      // 1.5 to 0.5
  #define kHz_per_tick      (((float) xpixels_per_tick) / (xpixels_per_kHz))   
  #define deg_per_tick      (((float) ypixels_per_tick) / (ypixels_per_deg))   
  #define unit_per_tick     (((float) ypixels_per_tick) / (ypixels_per_unit))   

  //============== Draw lines on Layer 2
  // Top-most horizontal, includes tick
  tft.drawFastHLine(xstart-10, ystart, xend-xstart+10, RA8875_GREEN);
  // Bottom-most horizontal, includes tick
  tft.drawFastHLine(xstart-10, yend, xend-xstart+10, RA8875_GREEN);
  // Major horizontal gridlines
  tft.setFontScale((enum RA8875tsize)0);
  for (int i=1; i<10; i++){
    // Left ticks
    tft.drawFastHLine(xstart-10, ystart+i*ypixels_per_tick, 10, RA8875_GREEN);
    tft.setCursor(10, ystart+ypixels_per_tick * i - CHAR_HEIGHT/2);
    tft.setTextColor(RA8875_WHITE);
    tft.print(100 - i*deg_per_tick,0);
    // Right ticks
    tft.drawFastHLine(xend, ystart+i*ypixels_per_tick, 10, RA8875_GREEN);
    tft.setCursor(xend + 15, ystart+ypixels_per_tick * i - CHAR_HEIGHT/2);
    tft.setTextColor(RA8875_YELLOW);
    tft.print(1.5 - i*unit_per_tick,1);
    // grid line
    tft.drawFastHLine(xstart, ystart+i*ypixels_per_tick, xend-xstart, tft.Color565(50, 50, 50));
  }
  // Left-most vertical, includes tick
  tft.drawFastVLine(xstart, ystart, yend-ystart+10, RA8875_GREEN); 
  // Right-most vertical, includes tick
  tft.drawFastVLine(xend, ystart, yend-ystart+10, RA8875_GREEN);
  tft.setTextColor(RA8875_WHITE);
  tft.setFontScale((enum RA8875tsize)0);
  for (int k = 1; k < 10; k++) {  // Draw Freq axis tick marks and numbers
    tft.drawFastVLine(xstart + xpixels_per_tick * k, yend, 10, RA8875_GREEN);
    tft.setCursor(xstart-13 + xpixels_per_tick * k, yend+15);
    tft.print(IQfreqStart_kHz + k * kHz_per_tick, 1);
    tft.drawFastVLine(xstart + xpixels_per_tick * k, ystart, yend-ystart, tft.Color565(50, 50, 50));
  }
  tft.setFontScale((enum RA8875tsize)0);
  tft.setCursor(350, 440);
  tft.print("kHz");
  tft.setTextColor(RA8875_YELLOW);
  tft.setCursor(16, 20);
  tft.print("Deg");
  tft.setTextColor(RA8875_YELLOW);
  tft.setCursor(xend+5, 20);
  tft.print("|I/Q|");
  tft.setFontScale((enum RA8875tsize)1);
  tft.setTextColor(RA8875_YELLOW);
  tft.setCursor(270, 15);
  tft.print("T41 IQ Balance");
  tft.writeTo(L1);
}

void CalculateIQCorrectionValues(){
  for (int i=0; i<numIQPoints; i++){
    // The image frequency is in the bin mirrored around the center:
    // Bin 0's image is in bin 2047
    // Bin 1's image is in bin 2046
    // image = 2047 - i
    gCorrIQ[i] = 1.0 / gErrorIQ[numIQPoints-1-i];
    float pcorrect = PI/2;
    if (i >= numIQPoints/2){
      pcorrect = -PI/2;
    }
    float pcorrection = -1*(pcorrect-pErrorIQ[numIQPoints-1-i]);
    pCorrIQr[i] = cos(pcorrection);
    pCorrIQi[i] = sin(pcorrection);
  }
}

void calculate750HzCalibration(){
  // Calculate the values at 750 Hz for the XMit calibration routine to use
  if (bands[currentBand].mode == DEMOD_LSB) {
  Clk2SetFreq = (centerFreq + IFFreq - 750)* SI5351_FREQ_MULT;
  } else {
    Clk2SetFreq = (centerFreq + IFFreq + 750)* SI5351_FREQ_MULT;
  }
  si5351.set_freq(Clk2SetFreq, SI5351_CLK2);
  MyDelay(10);
  Q_in_L.clear();
  Q_in_R.clear();
  MeasureIandQFFT();
  arm_max_f32(Imag,1024,&frmax,&index_of_max);
  scale_R = Imag[index_of_max]/Qmag[index_of_max];
  float perr;
  if (bands[currentBand].mode == DEMOD_LSB) { 
    // Tone is at -750 Hz, so we expect phase difference to be +90 deg
    perr = PI/2.0 - IQphase[index_of_max];
  } else {
    // Tone is at +750 Hz, so we expect phase difference to be -90 deg
    perr = -1*PI/2.0 - IQphase[index_of_max];
  }
  // Calculate the delay in number of samples that is closest to the needed
  // phase shift at 750 Hz
  delay_R = (int)(round(192000.0 * perr / (2*PI*750.0) ));
  sprintf(strBuf,"delay=%d,scale=%4.3f\n",delay_R,scale_R);
  Serial.println(strBuf);
}
#endif

/*****
  Purpose: Set up prior to IQ calibrations.  Revised function. AFP 07-13-24
  These things need to be saved here and restored in the prologue function:
  Vertical scale in dB  (set to 10 dB during calibration)
  Zoom, set to 1X in receive and 4X in transmit calibrations.
  Transmitter power, set to 5W during both calibrations.
   Parameter List:
      int setZoom   (This parameter should be 0 for receive (1X) and 2 (4X) for transmit)

   Return value:
      void
 *****/
void CalibratePreamble(int setZoom) {
  calOnFlag = 1;
  corrChange = 0;
  correctionIncrement = 0.01;  //AFP 2-7-23
  IQCalType = 0;
  radioState = CW_TRANSMIT_STRAIGHT_STATE;  // 
  userXmtMode = xmtMode;          // Store the user's mode setting.   July 22, 2023
  userZoomIndex = spectrum_zoom;  // Save the zoom index so it can be reset at the conclusion.   August 12, 2023
  spectrum_zoom = setZoom; // spectrum_zoom is used in Process.cpp
  zoomIndex = setZoom - 1; 
  userCurrentFreq = currentFreq;
  userTxRxFreq = TxRxFreq;
  userNCOFreq = NCOFreq;
  userCenterFreq = centerFreq;
  transmitPowerLevelTemp = transmitPowerLevel;
  TxRxFreq = centerFreq;
  currentFreq = TxRxFreq;
  NCOFreq = 0L;
  tft.clearScreen(RA8875_BLACK);  //AFP07-13-24
  ButtonZoom(); // must call ButtonZoom because it configures the FFT!
  ResetTuning(); 
  tft.writeTo(L1);  // Always exit function in L1.   August 15, 2023
  DrawBandWidthIndicatorBar();
  ShowSpectrumdBScale();
  DrawFrequencyBarValue();
  ShowFrequency();
  tft.writeTo(L2);  // Erase the bandwidth bar.   August 16, 2023
  tft.clearMemory();
  tft.writeTo(L1);
  tft.setFontScale((enum RA8875tsize)0);
  tft.setTextColor(RA8875_GREEN);
  tft.setCursor(550, 160);
  tft.print("Dir Freq - Gain/Phase");
  tft.setCursor(550, 175);
  tft.print("User1 - Incr");
  tft.setTextColor(RA8875_CYAN);
  tft.fillRect(350, 125, 100, tft.getFontHeight(), RA8875_BLACK);
  tft.fillRect(0, 272, 517, 399, RA8875_BLACK);  // Erase waterfall.   August 14, 2023
  tft.setCursor(400, 125);
  tft.print("dB");
  tft.setCursor(350, 110);
  tft.print("Incr= ");
  tft.setCursor(400, 110);
  tft.print(correctionIncrement, 3);
  userScale = currentScale;  //  Remember user preference so it can be reset when done.  
  currentScale = 1;          //  Set vertical scale to 10 dB during calibration.  
  updateDisplayFlag = 0;
  xrState = RECEIVE_STATE;
  T41State = CW_RECEIVE;
  modeSelectInR.gain(0, 1);
  modeSelectInL.gain(0, 1);
  modeSelectInExR.gain(0, 0);
  modeSelectInExL.gain(0, 0);
  modeSelectOutL.gain(0, 1);
  modeSelectOutR.gain(0, 1);
  modeSelectOutL.gain(1, 0);
  modeSelectOutR.gain(1, 0);
  modeSelectOutExL.gain(0, 1);
  modeSelectOutExR.gain(0, 1);
  
  xrState = TRANSMIT_STATE;
  digitalWrite(RXTX, HIGH);  // Turn on transmitter.
  ShowTransmitReceiveStatus();
  ShowSpectrumdBScale();
}

/*****
  Purpose: Shut down calibtrate and clean up after IQ calibrations.  Revised function.  AFP 07-13-24

   Parameter List:
      void

   Return value:
      void
 *****/
void CalibratePost() {
  digitalWrite(RXTX, LOW);  // Turn off the transmitter.
  digitalWrite(CAL, CAL_OFF);
  SetRF_InAtten(currentRF_InAtten);
  SetRF_OutAtten(currentRF_OutAtten);
  updateDisplayFlag = 0;
  xrState = RECEIVE_STATE;
  T41State = CW_RECEIVE;
  Q_in_L.clear();
  Q_in_R.clear();
  currentFreq = userCurrentFreq;
  TxRxFreq = userTxRxFreq;
  NCOFreq = userNCOFreq;
  centerFreq = userCenterFreq;
  xrState = RECEIVE_STATE;
  calibrateFlag = 0;  // this was set when the Calibration menu option was selected
  calFreqShift = 0;
  currentScale = userScale;  //  Restore vertical scale to user preference.  
  //ShowSpectrumdBScale();
  xmtMode = userXmtMode;                        // Restore the user's floor setting.   July 27, 2023
  transmitPowerLevel = transmitPowerLevelTemp;  // Restore the user's transmit power level setting.   August 15, 2023
  EEPROMWrite();                                // Save calibration numbers and configuration.   August 12, 2023
  zoomIndex = userZoomIndex - 1;
  spectrum_zoom = userZoomIndex;
  ButtonZoom();     // Restore the user's zoom setting.  Note that this function also modifies spectrum_zoom.
  EEPROMWrite();    // Save calibration numbers and configuration.   August 12, 2023
  tft.writeTo(L2);  // Clear layer 2.   July 31, 2023
  tft.clearMemory();
  tft.writeTo(L1);  // Exit function in layer 1.   August 3, 2023
  RedrawDisplayScreen();
  IQChoice = 5; // this is the secondary menu choice. equivalent to Cancel
  calOnFlag = 0;
  mainMenuWindowActive = 0;
  radioState = CW_RECEIVE_STATE;  // 
  SetFreq();                      // Return Si5351 to normal operation mode.  
  lastState = 1111;               // This is required due to the function deactivating the receiver.  This forces a pass through the receiver set-up code.   October 16, 2023
  return;
}

/*====
  Purpose: Combined input/ output for the purpose of calibrating the receive IQ

   Parameter List:
      void

   Return value:
      void
 *****/

void tuneCalParameter(int indexStart, int indexEnd, float increment, float *IQCorrectionFactor, char prompt[]){
  float adjMin = 100;
  int adjMinIndex = 0;
  float correctionFactor = *IQCorrectionFactor;
  for (int i = indexStart; i < indexEnd; i++){
    *IQCorrectionFactor = correctionFactor + i*increment;
    FFTupdated = false;
    while (!FFTupdated){
      adjdB = ShowSpectrum2();
    }
    if ((stateMachine == TX_STATE_TX_PHASE) | (stateMachine == TX_STATE_RX_PHASE)){
      // Get two updated FFTs to avoid the same where the audio samples
      // span a change in the correction factor
      FFTupdated = false;
      while (!FFTupdated){
        adjdB = ShowSpectrum2();
      }
    } else {
        adjdB = ShowSpectrum2();
    }
    //Serial.println(String(i)+","+String(adjdB));
    if (adjdB < adjMin){
      adjMin = adjdB;
      adjMinIndex = i;
    }
    tft.setFontScale((enum RA8875tsize)1);
    tft.setTextColor(RA8875_WHITE);
    tft.fillRect(250, 0, 285, CHAR_HEIGHT, RA8875_BLACK);  // Increased rectangle size to full erase value.  KF5N August 12, 2023
    tft.setCursor(257, 1);
    tft.print(prompt);
    tft.setCursor(440, 1);
    tft.print(*IQCorrectionFactor, 3);
  }
  *IQCorrectionFactor = correctionFactor + adjMinIndex*increment;
  /*sprintf(strBuf,"Min (%d, %4.3f) = %2.1f",
                                  adjMinIndex,
                                  *IQCorrectionFactor,
                                  adjMin);
  Serial.println(strBuf);*/
}

void autotune(float *amp, float *phase, float gain_coarse_max, float gain_coarse_min,
                                        float phase_coarse_max, float phase_coarse_min,
                                        int gain_coarse_step2_N, int phase_coarse_step2_N,
                                        int gain_fine_N, int phase_fine_N, bool phase_first){
  if (phase_first){
    // Step 2: phase changes in 0.01 steps from -0.2 to 0.2. Find the minimum.
    int phaseStepsCoarseN = (int)((phase_coarse_max-phase_coarse_min)/0.01/2);
    *phase = 0.0;
    //Serial.print("Step 2: ");
    tuneCalParameter(-phaseStepsCoarseN, phaseStepsCoarseN+1, 0.01, phase,(char *)"IQ Phase");
    // Step 1: Gain in 0.01 steps from 0.5 to 1.5
    int gainStepsCoarseN = (int)((gain_coarse_max-gain_coarse_min)/0.01/2);
    *amp = 1.0;
    //Serial.print("Step 1: ");
    tuneCalParameter(-gainStepsCoarseN, gainStepsCoarseN+1, 0.01, amp,(char *)"IQ Gain");
  } else {
    // Step 1: Gain in 0.01 steps from 0.5 to 1.5
    int gainStepsCoarseN = (int)((gain_coarse_max-gain_coarse_min)/0.01/2);
    *amp = 1.0;
    //Serial.print("Step 1: ");
    tuneCalParameter(-gainStepsCoarseN, gainStepsCoarseN+1, 0.01, amp,(char *)"IQ Gain");
    // Step 2: phase changes in 0.01 steps from -0.2 to 0.2. Find the minimum.
    int phaseStepsCoarseN = (int)((phase_coarse_max-phase_coarse_min)/0.01/2);
    *phase = 0.0;
    //Serial.print("Step 2: ");
    tuneCalParameter(-phaseStepsCoarseN, phaseStepsCoarseN+1, 0.01, phase,(char *)"IQ Phase");
  }
  // Step 3: Gain in 0.01 steps from 4 steps below previous minimum to 4 steps above          
  //Serial.print("Step 3: ");
  tuneCalParameter(-gain_coarse_step2_N, gain_coarse_step2_N+1, 0.01, amp,(char *)"IQ Gain");
  // Step 4: phase in 0.01 steps from 4 steps below previous minimum to 4 steps above
  //Serial.print("Step 4: ");
  tuneCalParameter(-phase_coarse_step2_N, phase_coarse_step2_N+1, 0.01, phase,(char *)"IQ Phase");
  // Step 5: gain in 0.001 steps 10 steps below to 10 steps above
  //Serial.print("Step 5: ");
  tuneCalParameter(-gain_fine_N, gain_fine_N+1, 0.001, amp,(char *)"IQ Gain");
  // Step 6: phase in 0.001 steps 10 steps below to 10 steps above
  //Serial.print("Step 6: ");
  tuneCalParameter(-phase_fine_N, phase_fine_N+1, 0.001, phase,(char *)"IQ Phase");
}

void DoReceiveCalibrate() {
  stateMachine = RX_STATE;        // what calibration step are we in 
  int task = -1;                  // captures the button presses
  int lastUsedTask = -2;
  tft.setTextColor(RA8875_CYAN);
  CalibratePreamble(0);
  tft.setFontScale((enum RA8875tsize)1);
  tft.setTextColor(RA8875_CYAN);
  tft.setCursor(550, 300);
  tft.print("Receive I/Q ");
  tft.setCursor(550, 350);
  tft.print("Calibrate");
  // CLK0/1 will be set to centerFreq + IFFreq
  SetFreq();
  Clk2SetFreq = (centerFreq +2*IFFreq)* SI5351_FREQ_MULT;
  si5351.output_enable(SI5351_CLK2, 1);
  si5351.set_freq(Clk2SetFreq, SI5351_CLK2);
  digitalWrite(XMIT_MODE, XMIT_CW);
  digitalWrite(CW_ON_OFF, CW_ON);
  digitalWrite(CAL, CAL_ON);
  uint8_t out_atten = 60;
  uint8_t previous_atten = out_atten;
  SetRF_InAtten(out_atten);
  SetRF_OutAtten(out_atten);
  zoomIndex = 0;
  calTypeFlag = 0;  // RX cal
  bool calState = true;

  while (true) {
    val = ReadSelectedPushButton();
    if (val != BOGUS_PIN_READ) {
      val = ProcessButtonPress(val);
      if (val != lastUsedTask && task == -100) task = val;
      else task = BOGUS_PIN_READ;
    }    
    adjdB = ShowSpectrum2(); // this is where the IQ data processing is applied
    val = ReadSelectedPushButton();
    if (val != BOGUS_PIN_READ) {
      val = ProcessButtonPress(val);
      if (val != lastUsedTask && task == -100) task = val;
      else task = BOGUS_PIN_READ;
    }
    switch (task) {
      case (CAL_AUTOCAL):{
        // Run through the autocal routine
        if (stateMachine == RX_STATE){
          autotune(&IQAmpCorrectionFactor[currentBand], &IQPhaseCorrectionFactor[currentBand],
                  GAIN_COARSE_MAX, GAIN_COARSE_MIN,
                  PHASE_COARSE_MAX, PHASE_COARSE_MIN,
                  GAIN_COARSE_STEP2_N, PHASE_COARSE_STEP2_N,
                  GAIN_FINE_N, PHASE_FINE_N, false);
        }
        break;
      }
      case (CAL_TOGGLE_OUTPUT):{
        // Toggle the transmit signal between the CAL line and the RF output 
        if (calState){
          calState = false;
        } else {
          calState = true;
        }
        digitalWrite(CAL, calState);
        break;}
        // Toggle gain and phase
      case CAL_CHANGE_TYPE:{
        IQCalType = !IQCalType;
        break;}
      case CAL_CHANGE_INC: {
        corrChange = !corrChange;
        if (corrChange == 1) {
          correctionIncrement = 0.001;  //AFP 2-7-23
        } else {                        //if (corrChange == 0)                   // corrChange is a toggle, so if not needed JJP 2/5/23
          correctionIncrement = 0.01;   //AFP 2-7-23
        }
        tft.setFontScale((enum RA8875tsize)0);
        tft.fillRect(400, 110, 50, tft.getFontHeight(), RA8875_BLACK);
        tft.setCursor(400, 110);
        tft.print(correctionIncrement, 3);
        break;}

      case MENU_OPTION_SELECT:{
        tft.fillRect(SECONDARY_MENU_X, MENUS_Y, EACH_MENU_WIDTH + 35, CHAR_HEIGHT, RA8875_BLACK);
        EEPROMData.IQAmpCorrectionFactor[currentBand] = IQAmpCorrectionFactor[currentBand];
        EEPROMData.IQPhaseCorrectionFactor[currentBand] = IQPhaseCorrectionFactor[currentBand];
        IQChoice = 6;
        break;}
      default:
        break;
    }                                     // End switch
    if (task != -1) lastUsedTask = task;  //  Save the last used task.
    task = -100;                          // Reset task after it is used.
    if (IQCalType == 0) {                 // AFP 2-11-23
      IQAmpCorrectionFactor[currentBand] = GetEncoderValueLive(-2.0, 2.0, IQAmpCorrectionFactor[currentBand], correctionIncrement, (char *)"IQ Gain");
    } else {
      IQPhaseCorrectionFactor[currentBand] = GetEncoderValueLive(-2.0, 2.0, IQPhaseCorrectionFactor[currentBand], correctionIncrement, (char *)"IQ Phase");
    }
    // Adjust the value of the TX attenuator:
    out_atten = GetFineTuneValueLive(0,63,out_atten,1,(char *)"Out Atten");
    // Update via I2C if the attenuation value changed
    if (out_atten != previous_atten){
        SetRF_OutAtten(out_atten);
        previous_atten = out_atten;
    }
    if (val == MENU_OPTION_SELECT) {
      break;
    }
  }
  si5351.output_enable(SI5351_CLK2, 0);
  CalibratePost();
}


/*****
  Purpose: Combined input/ output for the purpose of calibrating the transmit IQ

   Parameter List:
      void

   Return value:
      void

 *****/
void DoXmitCalibrate() {
  stateMachine = TX_STATE_RX_PHASE;
  int task = -1;
  int lastUsedTask = -2;
  // Set the frequency of the transmit: remove the IF offset
  CalibratePreamble(4); //16x zoom
  centerFreq = centerFreq - IFFreq;
  SetFreq();
  corrChange = 0;
  tft.setFontScale((enum RA8875tsize)1);
  tft.setTextColor(RA8875_CYAN);
  tft.setCursor(550, 300);
  tft.print("Transmit I/Q ");
  tft.setCursor(550, 350);
  tft.print("Calibrate RX");
  IQChoice = 3;
  uint8_t out_atten = 60;
  uint8_t previous_atten = out_atten;
  SetRF_InAtten(out_atten);
  SetRF_OutAtten(out_atten);
  calTypeFlag = 1;
  IQEXChoice = 0;
  bool calState = true;
  updateDisplayFlag = 1;
  // For the receive chain calibration portion of transmit cal use CLK2
  // CLK0/1 will be set to centerFreq + IFFreq
  SetFreq();
  if (bands[currentBand].mode == DEMOD_LSB) {
    Clk2SetFreq = (centerFreq + IFFreq - 750)* SI5351_FREQ_MULT;
  } else {
    Clk2SetFreq = (centerFreq + IFFreq + 750)* SI5351_FREQ_MULT;
  }
  si5351.output_enable(SI5351_CLK2, 1);
  si5351.set_freq(Clk2SetFreq, SI5351_CLK2);
  digitalWrite(XMIT_MODE, XMIT_CW);
  digitalWrite(CW_ON_OFF, CW_ON);
  digitalWrite(CAL, CAL_ON);
  #ifdef QUADFFT
  calculate750HzCalibration();
  #endif
  ShowTransmitReceiveStatus();
  // Switch to transmit path cal upon entering loop
  task = CAL_TOGGLE_TX_STATE;
  while (true) {
    adjdB = ShowSpectrum2();
    val = ReadSelectedPushButton();
    if (val != BOGUS_PIN_READ) {
      val = ProcessButtonPress(val);
      if (val != lastUsedTask && task == -100) task = val;
      else task = BOGUS_PIN_READ;
    }
    switch (task) {
      case (CAL_TOGGLE_TX_STATE):{
        switch (stateMachine){ 
          case (TX_STATE_RX_PHASE):{
            stateMachine = TX_STATE_TX_PHASE;
            tft.setFontScale((enum RA8875tsize)1);
            tft.setTextColor(RA8875_WHITE);
            tft.fillRect(550-7, 350-1, 800-550, CHAR_HEIGHT, RA8875_BLACK);
            tft.setCursor(550, 350);
            tft.print("Calibrate TX");
            out_atten = 40;
            SetRF_InAtten(30);
            SetRF_OutAtten(out_atten);
            digitalWrite(XMIT_MODE, XMIT_SSB);
            digitalWrite(CW_ON_OFF, CW_OFF);
            si5351.output_enable(SI5351_CLK2, 0);
            break;
          }
          case (TX_STATE_TX_PHASE):{
            stateMachine = TX_STATE_RX_PHASE;
            tft.setFontScale((enum RA8875tsize)1);
            tft.setTextColor(RA8875_WHITE);
            tft.fillRect(550-7, 350-1, 800-550, CHAR_HEIGHT, RA8875_BLACK);
            tft.setCursor(550, 350);
            tft.print("Calibrate RX");
            out_atten = 60;
            SetRF_InAtten(out_atten);
            SetRF_OutAtten(out_atten);
            digitalWrite(XMIT_MODE, XMIT_CW);
            digitalWrite(CW_ON_OFF, CW_ON);
            si5351.output_enable(SI5351_CLK2, 1);
            #ifdef QUADFFT
            calculate750HzCalibration();
            #endif
            break;
          }
        }
        break;
      }
      case (CAL_AUTOCAL):{
        if (stateMachine == TX_STATE_RX_PHASE){
          autotune(&IQXRecAmpCorrectionFactor[currentBand], &IQXRecPhaseCorrectionFactor[currentBand],
                  GAIN_COARSE_MAX, GAIN_COARSE_MIN,
                  1.0, -1.0,
                  8, 8,
                  GAIN_FINE_N, PHASE_FINE_N, true);
        }
        if (stateMachine == TX_STATE_TX_PHASE){
          autotune(&IQXAmpCorrectionFactor[currentBand], &IQXPhaseCorrectionFactor[currentBand],
                  GAIN_COARSE_MAX, GAIN_COARSE_MIN,
                  PHASE_COARSE_MAX, PHASE_COARSE_MIN,
                  GAIN_COARSE_STEP2_N, PHASE_COARSE_STEP2_N,
                  GAIN_FINE_N, PHASE_FINE_N, false);
        }
        break;
      }
      case (CAL_TOGGLE_OUTPUT):{
        // Toggle the transmit signal between the CAL line and the RF output 
        if (calState){
          calState = false;
        } else {
          calState = true;
        }
        digitalWrite(CAL, calState);
        break;}
      // Toggle gain and phase
      case (CAL_CHANGE_TYPE):{
        IQEXChoice = !IQEXChoice;  //IQEXChoice=0, Gain  IQEXChoice=1, Phase
        break;}
      // Toggle increment value
      case (CAL_CHANGE_INC):{  //
        corrChange = !corrChange;
        if (corrChange == 1) {          // Toggle increment value
          correctionIncrement = 0.001;  // AFP 2-11-23
        } else {
          correctionIncrement = 0.01;  // AFP 2-11-23
        }
        tft.setFontScale((enum RA8875tsize)0);
        tft.fillRect(400, 110, 50, tft.getFontHeight(), RA8875_BLACK);
        tft.setCursor(400, 110);
        tft.print(correctionIncrement, 3);
        break;}
      case (MENU_OPTION_SELECT):{  // Save values and exit calibration.
        tft.fillRect(SECONDARY_MENU_X, MENUS_Y, EACH_MENU_WIDTH + 35, CHAR_HEIGHT, RA8875_BLACK);
        EEPROMData.IQXRecAmpCorrectionFactor[currentBand] = IQXRecAmpCorrectionFactor[currentBand];
        EEPROMData.IQXRecPhaseCorrectionFactor[currentBand] = IQXRecPhaseCorrectionFactor[currentBand];
        EEPROMData.IQXAmpCorrectionFactor[currentBand] = IQXAmpCorrectionFactor[currentBand];
        EEPROMData.IQXPhaseCorrectionFactor[currentBand] = IQXPhaseCorrectionFactor[currentBand];
        tft.fillRect(SECONDARY_MENU_X, MENUS_Y, EACH_MENU_WIDTH + 35, CHAR_HEIGHT, RA8875_BLACK);
        IQChoice = 6;  // AFP 2-11-23
        break;}
      default:
        break;
    }  // end switch
    //  Need to remember the last used task;
    if (task != -1) lastUsedTask = task;  //  Save the last used task.
    task = -100;                          // Reset task after it is used.
    //  Read encoder and update values.
    if (IQEXChoice == 0) {
      switch (stateMachine){
        case TX_STATE_TX_PHASE:{
          IQXAmpCorrectionFactor[currentBand] = GetEncoderValueLive(-2.0, 2.0, IQXAmpCorrectionFactor[currentBand], correctionIncrement, (char *)"IQ Gain X");
          break;
        }
        case TX_STATE_RX_PHASE:{
          IQXRecAmpCorrectionFactor[currentBand] = GetEncoderValueLive(-2.0, 2.0, IQXRecAmpCorrectionFactor[currentBand], correctionIncrement, (char *)"IQ Gain R");
          break;
        }
      }
    } else {
      switch (stateMachine){
        case TX_STATE_TX_PHASE:{
          IQXPhaseCorrectionFactor[currentBand] = GetEncoderValueLive(-2.0, 2.0, IQXPhaseCorrectionFactor[currentBand], correctionIncrement, (char *)"IQ Phase X");
          break;
        }
        case TX_STATE_RX_PHASE:{
          IQXRecPhaseCorrectionFactor[currentBand] = GetEncoderValueLive(-2.0, 2.0, IQXRecPhaseCorrectionFactor[currentBand], correctionIncrement, (char *)"IQ Phase R");
          break;
        }
      }
    }
    // Adjust the value of the TX attenuator:
    out_atten = GetFineTuneValueLive(0,63,out_atten,1,(char *)"Out Atten");
    // Update via I2C if the attenuation value changed
    if (out_atten != previous_atten){
        SetRF_OutAtten(out_atten);
        previous_atten = out_atten;
    }
    if (val == MENU_OPTION_SELECT) {
      break;
    }
  }  // end while
  CalibratePost();
  // Clean up and exit to normal operation.
}

#ifdef QUADFFT
void DoIQCalibrate() {
  CalibratePreamble(0);
  uint8_t out_atten = 60;
  uint8_t previous_atten = out_atten;
  SetRF_InAtten(out_atten);
  SetRF_OutAtten(out_atten);
  SetFreq();

  IQfreqStart_kHz = -96;
  IQfreqStop_kHz = +96;
  numIQPoints = 512;

  digitalWrite(XMIT_MODE, XMIT_CW);
  digitalWrite(CW_ON_OFF, CW_ON);
  digitalWrite(CAL, CAL_ON);

  // Now, step through frequency
  si5351.output_enable(SI5351_CLK2, 1);
  int FREQ_OFFSET = 4688; // 100*( FFT RBW / 2 ) = 100 * (96000/1024 / 2)

  while (true){
    DrawIQBalancePlotContainer();
    Serial.println("-----------------------");
    Serial.println("i,Clk2SetFreq,I-Q phase,I/Q amp");
    for (int i=0; i<numIQPoints; i++){
      int offset_dHz = IQfreqStart_kHz*1000*SI5351_FREQ_MULT
                      +i*(((IQfreqStop_kHz-IQfreqStart_kHz)*1000*SI5351_FREQ_MULT)/numIQPoints);
      Clk2SetFreq = (long long)offset_dHz + (long long)((centerFreq + IFFreq)*SI5351_FREQ_MULT); //+ FREQ_OFFSET; 
      si5351.set_freq(Clk2SetFreq, SI5351_CLK2);
      MyDelay(10); // probably unnecessary
      // Let's get into the correct configuration
      Q_in_L.clear();
      Q_in_R.clear();
      MeasureIandQFFT();
      arm_max_f32(Imag,1024,&frmax,&index_of_max);
      gErrorIQ[i] = Imag[index_of_max]/Qmag[index_of_max];
      pErrorIQ[i] = IQphase[index_of_max];
      float xx_kHz = ((float)offset_dHz)/(100.0*1000.0) ;
      sprintf(strBuf,"%d,%lld,%3.2f,%4.3f\n",
              i,
              Clk2SetFreq,
              pErrorIQ[i]*180.0/PI,
              gErrorIQ[i]);
      Serial.print(strBuf);
      if ((IQphase[index_of_max]*180.0/PI > -100) & (IQphase[index_of_max]*180.0/PI < 100)){
        tft.fillCircle(xstart + (int16_t)(xpixels_per_kHz*(-1*IQfreqStart_kHz + xx_kHz )), 
                      ystart + (int16_t)(ypixels_per_deg*(100.0 - IQphase[index_of_max]*180.0/PI )),
                      1, RA8875_LIGHT_GREY);
      }
      tft.fillCircle(xstart + (int16_t)(xpixels_per_kHz*(-1*IQfreqStart_kHz + xx_kHz )), 
                     ystart + (int16_t)(ypixels_per_unit*(1.5 - Imag[index_of_max]/Qmag[index_of_max] )),
                      1, RA8875_YELLOW);
    }    
    val = ReadSelectedPushButton();
    if (val != BOGUS_PIN_READ) {        // Any button press??
      val = ProcessButtonPress(val);    // Use ladder value to get menu choice
      if (val == MENU_OPTION_SELECT) {  // Yep. Make a choice??
        tft.fillRect(SECONDARY_MENU_X, MENUS_Y, EACH_MENU_WIDTH + 35, CHAR_HEIGHT, RA8875_BLACK);
        tft.fillRect(250-250, 0, 285+250, CHAR_HEIGHT+1, RA8875_BLACK);
        calibrateFlag = 0;
        IQChoice = 6;
        break;
      }        
    }
  }
  si5351.output_enable(SI5351_CLK2, 0);
  CalibratePost();
}
#endif


/*****
  Purpose: Signal processing for the purpose of calibrating the transmit IQ

   Parameter List:
      void

   Return value:
      void
 *****/
void ProcessIQData2() {
  float bandOutputFactor = 0.5;
  float rfGainValue;                                                    // AFP 2-11-23
  float recBandFactor[NUMBER_OF_BANDS] = { 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0 };
 
  /******************************************************
   * Configure the audio transmit queue
   ******************************************************/
  // Generate I and Q for the transmit calibration.
  // Only configure it if it hasn't been configured before
  if ((stateMachine == TX_STATE_TX_PHASE) ){
    // We only have to configure the DAC output if we're in the TX phase of the TX configuration
    // Use pre-calculated sin & cos instead of Hilbert
    // Sidetone = 750 Hz. Sampled at 24 kHz
    arm_scale_f32(cosBuffer2, bandOutputFactor, float_buffer_L_EX, 256);  
    arm_scale_f32(sinBuffer2, bandOutputFactor, float_buffer_R_EX, 256);
  
    // We apply the correction factor with a varying sign here because this is how we 
    // select the sideband to be transmitted
    if (bands[currentBand].mode == DEMOD_LSB) {
      arm_scale_f32(float_buffer_L_EX, + IQXAmpCorrectionFactor[currentBand], float_buffer_L_EX, 256);
      IQXPhaseCorrection(float_buffer_L_EX, float_buffer_R_EX, IQXPhaseCorrectionFactor[currentBand], 256);
    } else {
      if (bands[currentBand].mode == DEMOD_USB) {
        arm_scale_f32(float_buffer_L_EX, - IQXAmpCorrectionFactor[currentBand], float_buffer_L_EX, 256);
        IQXPhaseCorrection(float_buffer_L_EX, float_buffer_R_EX, IQXPhaseCorrectionFactor[currentBand], 256);
      }
    }

    // interpolation-by-2, 24KHz effective sample rate coming in, 48 kHz going out
    arm_fir_interpolate_f32(&FIR_int1_EX_I, float_buffer_L_EX, float_buffer_LTemp, 256);
    arm_fir_interpolate_f32(&FIR_int1_EX_Q, float_buffer_R_EX, float_buffer_RTemp, 256);

    // interpolation-by-4,  48KHz effective sample rate coming in, 192 kHz going out
    arm_fir_interpolate_f32(&FIR_int2_EX_I, float_buffer_LTemp, float_buffer_L_EX, 512);
    arm_fir_interpolate_f32(&FIR_int2_EX_Q, float_buffer_RTemp, float_buffer_R_EX, 512);
  }
  /******************************************************
   * Load transmit values into output buffers and read in data from the receive channel buffers.

   * Teensy Audio Library stores ADC data in two buffers size=128, Q_in_L and Q_in_R as initiated from the audio lib.
   * Then the buffers are read into two arrays sp_L and sp_R in blocks of 128 up to N_BLOCKS.  The arrarys are
   * of size BUFFER_SIZE * N_BLOCKS.  BUFFER_SIZE is 128.
   * N_BLOCKS = FFT_LENGTH / 2 / BUFFER_SIZE * (uint32_t)DF; // should be 16 with DF == 8 and FFT_LENGTH = 512
   * BUFFER_SIZE*N_BLOCKS = 2048 samples
   ******************************************************/
  // are there at least N_BLOCKS buffers in each channel available ?
  if ((uint32_t)Q_in_L.available() > N_BLOCKS + 0 && (uint32_t)Q_in_R.available() > N_BLOCKS + 0) {
    if ((stateMachine == TX_STATE_TX_PHASE) ){
      // Revised I and Q calibration signal generation using large buffers. 
      q15_t q15_buffer_LTemp[2048];
      q15_t q15_buffer_RTemp[2048];
      Q_out_L_Ex.setBehaviour(AudioPlayQueue::NON_STALLING);
      Q_out_R_Ex.setBehaviour(AudioPlayQueue::NON_STALLING);
      arm_float_to_q15(float_buffer_L_EX, q15_buffer_LTemp, 2048);
      arm_float_to_q15(float_buffer_R_EX, q15_buffer_RTemp, 2048);
      Q_out_L_Ex.play(q15_buffer_LTemp, 2048);
      Q_out_R_Ex.play(q15_buffer_RTemp, 2048);
      Q_out_L_Ex.setBehaviour(AudioPlayQueue::ORIGINAL);
      Q_out_R_Ex.setBehaviour(AudioPlayQueue::ORIGINAL);
    }
    for (unsigned i = 0; i < N_BLOCKS; i++) {
      sp_L1 = Q_in_R.readBuffer(); // this is not a mistake. L and R must have got switched at 
      sp_R1 = Q_in_L.readBuffer(); // some point and the calibration process incorporates it

      /**********************************************************************************  AFP 12-31-20
          Using arm_Math library, convert to float one buffer_size.
          Float_buffer samples are now standardized from > -1.0 to < 1.0
      **********************************************************************************/
      arm_q15_to_float(sp_L1, &float_buffer_L[BUFFER_SIZE * i], BUFFER_SIZE);  // convert int_buffer to float 32bit
      arm_q15_to_float(sp_R1, &float_buffer_R[BUFFER_SIZE * i], BUFFER_SIZE);  // convert int_buffer to float 32bit
      Q_in_L.freeBuffer();
      Q_in_R.freeBuffer();
    }
    rfGainValue = pow(10, (float)rfGainAllBands / 20);                                   //AFP 2-11-23
    arm_scale_f32(float_buffer_L, rfGainValue, float_buffer_L, BUFFER_SIZE * N_BLOCKS);  //AFP 2-11-23
    arm_scale_f32(float_buffer_R, rfGainValue, float_buffer_R, BUFFER_SIZE * N_BLOCKS);  //AFP 2-11-23

    #ifdef QUADFFT
    // Apply an initial phase delay and scaling derived from the IQ balance measurement
    // when in transmit mode
    if ((stateMachine == TX_STATE_TX_PHASE) | (stateMachine == TX_STATE_RX_PHASE)){
      arm_scale_f32(float_buffer_R, scale_R, float_buffer_R, BUFFER_SIZE * N_BLOCKS);
      // Apply a shift-by-N to float_buffer_R
      if (delay_R > 0){
        for (int k=0;k<delay_R;k++){
          float_buffer_RTemp[k] = float_buffer_R[BUFFER_SIZE * N_BLOCKS - (delay_R-k)];
        }
        for (int k=0;k<(BUFFER_SIZE * N_BLOCKS - delay_R);k++){
          float_buffer_RTemp[delay_R+k] = float_buffer_R[k];
        }
        for (int k=0;k<(BUFFER_SIZE * N_BLOCKS);k++){
        float_buffer_R[k] = float_buffer_RTemp[k];
        }
      }
      if (delay_R < 0){
        for (int k=0;k<(-1*delay_R);k++){
          float_buffer_RTemp[BUFFER_SIZE * N_BLOCKS - (-1*delay_R-k)] = float_buffer_R[k];
        }
        for (int k=0;k<(BUFFER_SIZE * N_BLOCKS + delay_R);k++){
          float_buffer_RTemp[k] = float_buffer_R[k-delay_R];
        }
        for (int k=0;k<(BUFFER_SIZE * N_BLOCKS);k++){
        float_buffer_R[k] = float_buffer_RTemp[k];
        }
      } 
    }
    #endif

    /**********************************************************************************  AFP 12-31-20
      Scale the data buffers by the RFgain value defined in bands[currentBand] structure
    **********************************************************************************/
    arm_scale_f32(float_buffer_L, recBandFactor[currentBand], float_buffer_L, BUFFER_SIZE * N_BLOCKS);  //AFP 2-11-23
    arm_scale_f32(float_buffer_R, recBandFactor[currentBand], float_buffer_R, BUFFER_SIZE * N_BLOCKS);  //AFP 2-11-23

    // Receive amplitude correction
    if ((stateMachine == TX_STATE_TX_PHASE) | (stateMachine == TX_STATE_RX_PHASE)){
      // Apply the special calibration parameters that apply only here
      arm_scale_f32(float_buffer_L, -IQXRecAmpCorrectionFactor[currentBand], float_buffer_L, BUFFER_SIZE * N_BLOCKS);
      IQPhaseCorrection(float_buffer_L, float_buffer_R, IQXRecPhaseCorrectionFactor[currentBand], BUFFER_SIZE * N_BLOCKS);
    } else {
      arm_scale_f32(float_buffer_L, -IQAmpCorrectionFactor[currentBand], float_buffer_L, BUFFER_SIZE * N_BLOCKS);  //AFP 04-14-22
      IQPhaseCorrection(float_buffer_L, float_buffer_R, IQPhaseCorrectionFactor[currentBand], BUFFER_SIZE * N_BLOCKS);
    }
    
    if (spectrum_zoom == SPECTRUM_ZOOM_1) {
      // This is executed during receive cal
      CalcZoom1Magn();
      FFTupdated = true;
    } else {
      // This is used during transmit cal
      ZoomFFTExe(BUFFER_SIZE * N_BLOCKS);
      if (zoom_sample_ptr == 0) {
        FFTupdated = true;
        /*for (int i=0; i<SPECTRUM_RES; i++){
          sprintf(strBuf,"%d,%5.4f,%5.4f\n",
                  i,
                  FFT_ring_buffer_x[i],
                  FFT_ring_buffer_y[i]);
          Serial.print(strBuf);
        }*/
      }
    }
  }
}

/*****
  Purpose: Show Spectrum display modified for IQ calibration.
           This is similar to the function used for normal reception, however, it has
           been simplified and streamlined for calibration.

  Parameter list:
    void

  Return value;
    void
*****/
float ShowSpectrum2()
{
  int x1 = 0;
  float adjdB = 0.0;

  pixelnew[0] = 0;
  pixelnew[1] = 0;
  pixelold[0] = 0;
  pixelold[1] = 0;

  //  This is the "spectra scanning" for loop.  During calibration, only small areas of the spectrum need to be examined.
  //  If the entire 512 wide spectrum is used, the calibration loop will be slow and unresponsive.
  //  The scanning areas are determined by receive versus transmit calibration, and LSB or USB.  Thus there are 4 different scanning zones.
  //  All calibrations use a 0 dB reference signal and an "undesired sideband" signal which is to be minimized relative to the reference.
  //  Thus there is a target "bin" for the reference signal and another "bin" for the undesired sideband.
  //  The target bin locations are used by the for-loop to sweep a small range in the FFT.  A maximum finding function finds the peak signal strength.

  /*************************************
  ProcessIQData2 performs an N-point (SPECTRUM_RES = 512) FFT on the data in float_buffer_L and 
  float_buffer_R when they fill up. The data in float_buffer_L and float_buffer_R are sampled at 
  192000 Hz. The length of the float_buffer_L and float_buffer_R buffers is BUFFER_SIZE * N_BLOCKS 
  = 128*16 = 2048, of which the FFT only uses the first 512 points.
    N_BLOCKS = FFT_LENGTH / 2 / BUFFER_SIZE * (uint32_t)DF
             = 512 / 2 / 128 * 8
             = 16
  Therefore the bin width of each FFT bin is SAMPLE_RATE / FFT_LEN = 192000 / 512 = 375 Hz.
  The frequency of the middle bin is centerFreq + IFFreq and our spectrum spans 
  (centerFreq + IFFreq - SAMPLE_RATE/2) to (centerFreq + IFFreq + SAMPLE_RATE/2).

  So the equation for bin number n given frequency Clk2SetFreq is:
    n = (Clk2SetFreq - Clk1SetFreq)/375 + 256
      = (Clk2SetFreq - (centerFreq + IFFreq))/375 + 256

  In receive cal mode, we set Clk2SetFreq to centerFreq + 2*IFFreq
  So we expect the desired tone to appear in bin
    n_tone = IFFreq/375 + 256
  while the undesired image product will be at 
    n_image= -IFFreq/375 + 256

  Which are, given IFFreq = 48000:
    n_tone = 384
    n_image= 128
  *********************************************/

  int cal_bins[2] = { 0, 0 };
  int capture_bins; // Sets the number of bins to scan for signal peak.
  if (calTypeFlag == 0) {
    capture_bins = 10;
    cal_bins[0] = 384;
    cal_bins[1] = 128;
  }  // Receive calibration
  
  /******************************
   * The same LO clock is used for transmit and receive, so the bin tone and image are
   * found symmetric around the center of the FFT. This offset is 750 Hz (see sineTone() 
   * in Utility.cpp). We have zoom of x16, so the bin size is 375/16 = 23.4 Hz. So the 
   * bin numbers are 256 + 750/(375/16) = 256+32 = 288 and 256-32 = 224
   ******************************/
  if (calTypeFlag == 1 && bands[currentBand].mode == DEMOD_LSB) {
    capture_bins = 10; // scans 2*capture_bins
    cal_bins[0] = 257 - 32;
    cal_bins[1] = 257 + 32;
  }  // Transmit calibration, LSB.  
  if (calTypeFlag == 1 && bands[currentBand].mode == DEMOD_USB) {
    capture_bins = 10; // scans 2*capture_bins
    cal_bins[0] = 257 + 32;
    cal_bins[1] = 257 - 32;
  }  // Transmit calibration, USB.  

  //  There are 2 for-loops, one for the reference signal and another for the undesired sideband.  
  for (x1 = cal_bins[0] - capture_bins; x1 < cal_bins[0] + capture_bins; x1++) adjdB = PlotCalSpectrum(x1, cal_bins, capture_bins);
  for (x1 = cal_bins[1] - capture_bins; x1 < cal_bins[1] + capture_bins; x1++) adjdB = PlotCalSpectrum(x1, cal_bins, capture_bins);

  // Finish up: AFP 2-11-23
  tft.fillRect(350, 125, 50, tft.getFontHeight(), RA8875_BLACK);
  tft.setFontScale((enum RA8875tsize)0);
  tft.setCursor(350, 125);  // 350, 125
  tft.print(adjdB, 1);

  tft.BTE_move(WATERFALL_LEFT_X, FIRST_WATERFALL_LINE, MAX_WATERFALL_WIDTH, MAX_WATERFALL_ROWS - 2, WATERFALL_LEFT_X, FIRST_WATERFALL_LINE + 1, 1, 2);
  while (tft.readStatus())
    ;
  tft.BTE_move(WATERFALL_LEFT_X, FIRST_WATERFALL_LINE + 1, MAX_WATERFALL_WIDTH, MAX_WATERFALL_ROWS - 2, WATERFALL_LEFT_X, FIRST_WATERFALL_LINE + 1, 2);
  while (tft.readStatus())
    ;  // Make sure it's done.
  return adjdB;
}

/*****
  Purpose:  Plot Receive Calibration Spectrum   //   7/2/2023
            This function plots a partial spectrum during calibration only.
            This is intended to increase the efficiency and therefore the responsiveness of the calibration encoder.
            This function is called by ShowSpectrum2() in two for-loops.  One for-loop is for the refenence signal,
            and the other for-loop is for the undesired sideband.
  Parameter list:
    int x1, where x1 is the FFT bin.
    cal_bins[2] locations of the desired and undesired signals
    capture_bins width of the bins used to display the signals
  Return value;
    float returns the adjusted value in dB
*****/
float PlotCalSpectrum(int x1, int cal_bins[2], int capture_bins) {
  float adjdB = 0.0;
  int16_t adjAmplitude = 0;  // Was float; cast to float in dB calculation.  
  int16_t refAmplitude = 0;  // Was float; cast to float in dB calculation.  
  uint32_t index_of_max;     // This variable is not currently used, but it is required by the ARM max function.  
  if (x1 == (cal_bins[0] - capture_bins)) {  // Set flag at revised beginning.  
    updateDisplayFlag = 1;                   //Set flag so the display data are saved only once during each display refresh cycle at the start of the cycle, not 512 times 
  } else updateDisplayFlag = 0;  //  Do not save the the display data for the remainder of the

  //-------------------------------------------------------
  // This block of code, which calculates the latest FFT and finds the maxima of the tone
  // and its image product, does not technically need to be run every time we plot a pixel 
  // on the screen. However, according to the comments below this is needed to eliminate
  // conflicts.
  ProcessIQData2();  // Call the Audio process from within the display routine to eliminate conflicts with drawing the spectrum and waterfall displays
  // Find the maximums of the desired and undesired signals.
  arm_max_q15(&pixelnew[(cal_bins[0] - capture_bins)], capture_bins * 2, &refAmplitude, &index_of_max);
  arm_max_q15(&pixelnew[(cal_bins[1] - capture_bins)], capture_bins * 2, &adjAmplitude, &index_of_max);

  y_new = pixelnew[x1];
  y1_new = pixelnew[x1 - 1];
  y_old = pixelold[x1];
  y_old2 = pixelold[x1 - 1];

  //=== // AFP 2-11-23
  if (y_new > base_y) y_new = base_y;
  if (y_old > base_y) y_old = base_y;
  if (y_old2 > base_y) y_old2 = base_y;
  if (y1_new > base_y) y1_new = base_y;

  if (y_new < 0) y_new = 0;
  if (y_old < 0) y_old = 0;
  if (y_old2 < 0) y_old2 = 0;
  if (y1_new < 0) y1_new = 0;

  // Erase the old spectrum and draw the new spectrum.
  tft.drawLine(x1, spectrumNoiseFloor - y_old2, x1, spectrumNoiseFloor - y_old, RA8875_BLACK);   // Erase old...
  tft.drawLine(x1, spectrumNoiseFloor - y1_new, x1, spectrumNoiseFloor - y_new, RA8875_YELLOW);  // Draw new
  pixelCurrent[x1] = pixelnew[x1];                                                               //  This is the actual "old" spectrum!  This is required due to CW interrupts.  Copied to pixelold by the FFT function.

  adjdB = ((float)adjAmplitude - (float)refAmplitude) / 1.95;
  tft.writeTo(L2);
  tft.fillRect(cal_bins[0] - capture_bins, SPECTRUM_TOP_Y + 20, 2*capture_bins, h - 6, RA8875_BLUE);     // SPECTRUM_TOP_Y = 100
  tft.fillRect(cal_bins[1] - capture_bins, SPECTRUM_TOP_Y + 20, 2*capture_bins, h - 6, DARK_RED);  // h = SPECTRUM_HEIGHT + 3
  tft.writeTo(L1);
  return adjdB;
}

#ifdef QUADFFT
/*****
  Purpose: Acquire samples from the I and Q channels, calculate FFTs on each channel
           independently, and calculate the PSD of the bins in each channel and the 
           phase angle between them. This is a blocking function -- it waits until the
           necessary data is available before proceeding.

  Parameter list:
    void

  Return value;
    void
*****/
void MeasureIandQFFT(){
  /******************************************************
   * Teensy Audio Library stores ADC data in two buffers size=128, Q_in_L and Q_in_R.
   * Read into two arrays sp_L and sp_R in blocks of 128 up to N_BLOCKS.  
   * The arrays are of size BUFFER_SIZE * N_BLOCKS.  BUFFER_SIZE is 128.
   * N_BLOCKS = FFT_LENGTH / 2 / BUFFER_SIZE * (uint32_t)DF = 16 with DF = 8 and FFT_LENGTH = 512
   * BUFFER_SIZE*N_BLOCKS = 2048 samples
   ******************************************************/
  // Wait until there at least N_BLOCKS buffers in each channel available
  while (!((uint32_t)Q_in_L.available() > N_BLOCKS + 0 && 
          (uint32_t)Q_in_R.available() > N_BLOCKS + 0)) {;}
  
  for (unsigned i = 0; i < N_BLOCKS; i++) {
    sp_L1 = Q_in_R.readBuffer(); // I
    sp_R1 = Q_in_L.readBuffer(); // Q
    // Convert to float one buffer_size, standardized from > -1.0 to < 1.0 
    arm_q15_to_float(sp_L1, &float_buffer_L[BUFFER_SIZE * i], BUFFER_SIZE);
    arm_q15_to_float(sp_R1, &float_buffer_R[BUFFER_SIZE * i], BUFFER_SIZE);
    Q_in_L.freeBuffer();
    Q_in_R.freeBuffer();
  }

  //if(corrections_calculated){
  //  applyIQcorrections();
  //}

  // perform windowing on 2048 real samples in the buffers
  for (int idx = 0; idx < 2048; idx++)  {  
    // Hann window
    float32_t temp_sample = 0.5 * (float32_t)(1.0 - (cosf(PI * 2.0 * (float32_t)idx / (float32_t)(2048 - 1))));
    float_buffer_L[idx] *= temp_sample;
    float_buffer_R[idx] *= temp_sample;
    //FFTbuf[2*idx] = float_buffer_L[idx];
    //FFTbuf[2*idx+1] = 0.0;
  }
  // Perform real FFT on L and R (I and Q)
  arm_rfft_fast_f32( &Sreal, float_buffer_L, float_buffer_LTemp, 0); 	// I
  arm_rfft_fast_f32( &Sreal, float_buffer_R, float_buffer_RTemp, 0); 	// Q
  // Bin 0,1 is Real{DC},Real{alias}. Thereafter complex samples 

  // calculate magnitudes of L and R and phase angle between them
  for (int i = 0; i < 1024; i++) {
    Imag[i] = sqrt(float_buffer_LTemp[2*i]*float_buffer_LTemp[2*i] + 
              float_buffer_LTemp[2*i+1]*float_buffer_LTemp[2*i+1] );
    Qmag[i] = sqrt(float_buffer_RTemp[2*i]*float_buffer_RTemp[2*i] + 
              float_buffer_RTemp[2*i+1]*float_buffer_RTemp[2*i+1] );
    // What is the phase angle?
    float phiI = atan2f(float_buffer_LTemp[2*i+1],float_buffer_LTemp[2*i]);
    float phiQ = atan2f(float_buffer_RTemp[2*i+1],float_buffer_RTemp[2*i]);
    IQphase[i] = phiI - phiQ;
    if (IQphase[i] > PI){
      IQphase[i] -= PI*2.0;
    } else {
      if (IQphase[i] < (-1*PI)){
        IQphase[i] += PI*2.0;
      }
    }
  }
  // Now let's see what would happen if we applied a correction based on these values
  //arm_cfft_f32(&arm_cfft_sR_f32_len4096, FFTbuf, 0, 1);
  //for (int i = 0; i < 2048; i++) {
//
  //}
}

#endif

#endif // V12HWR