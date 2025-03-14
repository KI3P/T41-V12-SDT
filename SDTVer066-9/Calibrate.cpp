#ifndef BEENHERE
#include "SDT.h"
#endif

// changes are so extensive for calibration in V12, this file supports only V12
static int stateMachine;
static bool FFTupdated;
static int val;
int corrChange = 0;
//AFP 2-7-23
static int userScale, userZoomIndex, userXmtMode;
static int transmitPowerLevelTemp;

static long userCurrentFreq;
static long userCenterFreq;
static long userTxRxFreq;
static long userNCOFreq;
int xPosCircleOld;
float corrPlotXValue = 300;

Chrono calChrono;
Chrono calChrono2;
Metro calMetro = Metro(15000);
LinearRegression lr = LinearRegression();

double values[2];

Timer CalTimer;

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
void CalibrateFrequency() {

  tft.clearScreen(RA8875_BLACK);
  freqCalFlag = 1;  //AFP 01-30-25
  int setZoom = 0;
  digitalWrite(RXTX, LOW);
  radioState = SSB_RECEIVE_STATE;  //
                                   // userXmtMode = xmtMode;                    // Store the user's mode setting.   July 22, 2023
  userZoomIndex = spectrum_zoom;   // Save the zoom index so it can be reset at the conclusion.   August 12, 2023
  spectrum_zoom = setZoom;         // spectrum_zoom is used in Process.cpp
  zoomIndex = setZoom - 1;
  userCurrentFreq = currentFreq;
  userTxRxFreq = TxRxFreq;
  userNCOFreq = NCOFreq;
  userCenterFreq = centerFreq;
  int corrPlotXValue4;
  int freqAutoPlotFlag = 0;
  int freqTimePlotFlag = 0;
  int timeIncrement = 0;
  int freqCalType = 0;
  int corrPlotYValue4;
  int corrPlotYValue5;
  int corrPlotYValue6;
  float plotElapsedTimeStart = 0;
  float32_t frequencyDiffValue[1250];
  float32_t frequencyDiffValueTen[20];
  float32_t corrFactorStdDev;
  float32_t corrFactorMean;
  float32_t corrFactorAveMean;
  float32_t corrFactorCalc;
  float32_t corrFactorStdDevAve = 0;
  calChrono.restart(0);
  Linear2DRegression *linear2DRegression = new Linear2DRegression();
  Linear2DRegression *linear2DRegression2 = new Linear2DRegression();
  Linear2DRegression *linear2DRegression3 = new Linear2DRegression();
  TxRxFreq = centerFreq;
  currentFreq = TxRxFreq;
  NCOFreq = 0L;
  int plotStartY;
  int plotEndY;
  int plotStartYPointOld;
  int plotEndYPointOld;
  int corrPlotXValue4Old;

  tft.writeTo(L2);  // Erase the bandwidth bar.   August 16, 2023
  tft.clearMemory();
  tft.writeTo(L1);
  tft.setFontScale((enum RA8875tsize)0);

  userScale = currentScale;  //  Remember user preference so it can be reset when done.
  currentScale = 1;          //  Set vertical scale to 10 dB during calibration.
  updateDisplayFlag = 0;

  xrState = RECEIVE_STATE;
  T41State = SSB_RECEIVE;
  modeSelectInR.gain(0, 1);
  modeSelectInL.gain(0, 1);
  modeSelectOutL.gain(0, 1);
  modeSelectOutR.gain(0, 1);
  modeSelectOutL.gain(1, 0);
  modeSelectOutR.gain(1, 0);
  int plotTimeInterval;

  userFilterLowCut = bands[currentBand].FLoCut;  // AFP 01-30-25
  userFilterHiCut = bands[currentBand].FHiCut;
  bands[currentBand].FHiCut = 1000;
  bands[currentBand].FLoCut = -1000;
  userFreqCalMode = bands[currentBand].mode;
  bands[currentBand].mode = DEMOD_SAM;
  SetupMode(bands[currentBand].mode);
  ControlFilterF();
  FilterBandwidth();
  AudioInterrupts();
  SetFreq();

  stateMachine = RX_STATE;  // what calibration step are we in
  int task = -1;            // captures the button presses

  int freqAutoLowSet = -400;
  int freqAutoIncrementSet = 50;
  int freqCalFactorStart = 0;
  adjdB = 0;
  int autoCalOffset = 0;

  tft.drawRect(470, 55, 310, 110, RA8875_GREEN);
  tft.setFontScale((enum RA8875tsize)1);
  tft.setTextColor(RA8875_WHITE);

  si5351.output_enable(SI5351_CLK2, 0);

  digitalWrite(CW_ON_OFF, CW_OFF);
  digitalWrite(CAL, CAL_OFF);
  uint8_t out_Atten = 60;
  uint8_t in_atten = 20;
  SetRF_InAtten(in_atten);
  SetRF_OutAtten(out_Atten);
  zoomIndex = 0;
  // let's you change in_atten when false
  int val;
  int32_t incrementSAM = 10L;
  int plotIntervalIndex = 0;
  int plotScaleIndex = 0;
  int freqCalDirections = 0;
  float plotScaleNumber = 0.;
  tft.fillRect(670, 60, 100, CHAR_HEIGHT, RA8875_BLACK);  //
  tft.setFontScale((enum RA8875tsize)1);
  tft.setCursor(670, 60);
  tft.print(EEPROMData.freqCorrectionFactor, 0);
  tft.drawRect(300, 200, 480, 270, RA8875_GREEN);
  tft.drawFastHLine(340, 440, 400, RA8875_GREEN);
  tft.drawFastVLine(340, 210, 230, RA8875_GREEN);
  tft.setTextColor(RA8875_YELLOW);
  tft.setFontScale((enum RA8875tsize)0);
  tft.setCursor(20, 240);
  tft.print("User1 - Time Plot");
  tft.setCursor(20, 255);
  tft.print("User2 - Auto");
  tft.setCursor(20, 270);
  tft.print("User3 - Corr Factor Increment");
  tft.setCursor(20, 285);
  tft.print("Select - Save/Exit");
  tft.setCursor(20, 320);
  tft.print("For Time Plot only:");
  tft.setCursor(20, 335);
  tft.print("Filter - Time Plot Error Scale");
  tft.setCursor(20, 350);
  tft.print("Decode - Time Plot Duration");
  tft.setCursor(20, 380);
  tft.print("Dir Freq - Directions");

  AMDecodeSAM();
  freqAutoPlotFlag = 0;
  //==============
  currentFreq = centerFreq + NCOFreq;  // Changed currentFreqA to currentFreq.  KF5N August 5, 2023
  NCOFreq = 0L;
  centerFreq = TxRxFreq = currentFreq;
  int autoCount = 0;
  freqAutoPlotFlag = 0;
  freqTimePlotFlag = 0;
  plotTimeInterval = 1200;
  long remainTimeUpdate = 0;
  while (1) {

    if (freqCalType != 1) {
      freqCorrectionFactorOld = EEPROMData.freqCorrectionFactor;
      EEPROMData.freqCorrectionFactor = (int)GetEncoderValueLiveFreq(-200000, 200000, EEPROMData.freqCorrectionFactor, incrementSAM, (char *)"Freq Cal: ", 3);
    }  //==ongoing process
    if (EEPROMData.freqCorrectionFactor != freqCorrectionFactorOld) {
      calChrono.restart(0);
      updateDisplayFlag = 1;
      si5351.init(SI5351_LOAD_CAPACITANCE, Si_5351_crystal, EEPROMData.freqCorrectionFactor);  // KI3P July 27 2024, updated to mirror Setup()
                                                                                               // MyDelay(100L);                                                                           // KI3P July 27 2024, updated to mirror Setup()
      si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_CURRENT);                                // KI3P July 27 2024, updated to mirror Setup()
      si5351.drive_strength(SI5351_CLK1, SI5351_DRIVE_CURRENT);                                // KI3P July 27 2024, updated to mirror Setup()
      si5351.drive_strength(SI5351_CLK2, SI5351_DRIVE_CURRENT);                                // KF5N July 10 2023
      si5351.set_ms_source(SI5351_CLK0, SI5351_PLLA);                                          // KI3P July 27 2024, updated to mirror Setup()
      si5351.set_ms_source(SI5351_CLK1, SI5351_PLLA);
      si5351.output_enable(SI5351_CLK2, 0);  // KI3P July 27 2024, updated to mirror Setup()
      oldMultiple = 0;
      SetFreq();
      MyDelay(10L);
    }
    ProcessIQData();  //AFP 01-30-25
    freqError = (0.20000012146 * SAM_carrier_freq_offset);
    tft.setFontScale((enum RA8875tsize)1);
    tft.setCursor(480, 95);
    tft.print("Freq Error");
    tft.fillRect(670, 60, 100, CHAR_HEIGHT, RA8875_BLACK);
    tft.setCursor(670, 60);
    tft.print(EEPROMData.freqCorrectionFactor, 0);
    tft.setCursor(480, 60);
    tft.print("Freq Cal");
    tft.setCursor(480, 130);
    tft.print("Corr Factor");

    if (calChrono.elapsed() >= 5000) {
      //linear2DRegression->addPoint(0.20024 * SAM_carrier_freq_offset, EEPROMData.freqCorrectionFactor);
      tft.setCursor(670, 95);
      tft.fillRect(670, 95, 100, CHAR_HEIGHT, RA8875_BLACK);
      tft.print(freqError, 3);
      corrPlotXValue4 = map(autoCalOffset, -freqAutoLowSet, freqAutoLowSet, 340, 740);  // AFP 01-30-25                                                                               //corrPlotXValue4 = map(-(freqCalFactorStart - EEPROMData.freqCorrectionFactor), -freqAutoLowSet, freqAutoLowSet, 340, 740);
      corrPlotYValue4 = map((100 * 0.20000012146 * SAM_carrier_freq_offset), -500, 500, 440, 210);
      linear2DRegression2->addPoint(corrPlotXValue4, corrPlotYValue4);
      updateDisplayFlag = 0;
      calChrono.restart(0);
      if (freqAutoPlotFlag == 1) {

        tft.writeTo(L2);
        tft.drawCircle(linear2DRegression3->calculate(325), 325, 5, RA8875_BLACK);
        linear2DRegression3->addPoint(corrPlotYValue4, corrPlotXValue4);
        tft.drawLine(340, plotStartYPointOld, corrPlotXValue4Old, plotEndYPointOld, RA8875_BLACK);
        plotStartY = linear2DRegression2->calculate(340);
        plotEndY = linear2DRegression2->calculate(corrPlotXValue4);
        tft.drawLine(340, plotStartY, corrPlotXValue4, plotEndY, LIGHT_BLUE);
        tft.drawCircle(linear2DRegression3->calculate(325), 325, 5, RA8875_CYAN);
        xPosCircleOld = linear2DRegression3->calculate(325);
        plotStartYPointOld = plotStartY;
        plotEndYPointOld = plotEndY;
        corrPlotXValue4Old = corrPlotXValue4;
        tft.fillRect(340, 0, 799, 209, RA8875_BLACK);
        tft.writeTo(L1);

        if (corrPlotYValue4 < 210 || corrPlotYValue4 > 440) {  // AFP 01-30-25
        } else {
          // AFP 01-30-25

          tft.fillCircle(corrPlotXValue4, corrPlotYValue4, 2, RA8875_YELLOW);

        }  // AFP 01-30-25

        linear2DRegression->addPoint(0.20000012146 * SAM_carrier_freq_offset, EEPROMData.freqCorrectionFactor);
        tft.fillRect(670, 130, 100, CHAR_HEIGHT, RA8875_BLACK);
        tft.setCursor(670, 130);
        tft.print(linear2DRegression->calculate(0), 0);
        corrFactorCalc = linear2DRegression->calculate(0);
        autoCount++;
        freqCalType = 1;
      }
    }
    //=========
    switch (freqCalType) {  //Select the Trequency Cal Function
      case (0):             //Manual Plot

        break;
      case (1):  //Auto Plot Run
        if (autoCount == 0) {

          linear2DRegression2->reset();
          linear2DRegression3->reset();
          tft.writeTo(L2);
          tft.drawCircle(xPosCircleOld, 325, 5, RA8875_BLACK);
          tft.writeTo(L1);
        }
        updateDisplayFlag = 1;
        freqAutoPlotFlag = 1;
        freqAutoLowSet = 500;        //AFP 01-30-25
        freqAutoIncrementSet = 100;  //AFP 01-30-25
        tft.setFontScale((enum RA8875tsize)0);
        tft.setCursor(310, 200);
        tft.print("5");
        tft.setCursor(310, 440);
        tft.print("-5");
        freqCorrectionFactorOld = EEPROMData.freqCorrectionFactor;
        tft.fillRect(340, 165, 250, CHAR_HEIGHT, RA8875_BLACK);
        tft.setFontScale((enum RA8875tsize)1);
        tft.setCursor(340, 165);
        tft.print("Auto Tune On");
        EEPROMData.freqCorrectionFactor = freqCalFactorStart - freqAutoLowSet + autoCount * freqAutoIncrementSet;
        autoCalOffset = -freqAutoLowSet + autoCount * freqAutoIncrementSet;  //AFP 01-30-25
        if (autoCount >= 11) {                                               //AFP 01-30-25
          tft.fillRect(340, 165, 250, CHAR_HEIGHT, RA8875_BLACK);
          tft.setFontScale((enum RA8875tsize)1);
          tft.setCursor(340, 165);
          tft.print("Auto Tune Off");
          MyDelay(3000);
          tft.fillRect(340, 165, 250, CHAR_HEIGHT, RA8875_BLACK);
          freqAutoPlotFlag = 0;
          autoCount = 0;
          freqCalType = -1;
          EEPROMData.freqCorrectionFactor = corrFactorCalc;
        }
        break;

      case (2):  //Time plot Run
        tft.setTextColor(RA8875_YELLOW);
        tft.setFontScale((enum RA8875tsize)0);
        tft.setCursor(10, 50);
        tft.print("Plot Time hrs.");

        tft.setFontScale((enum RA8875tsize)1);
        tft.setCursor(5, 160);
        tft.setTextColor(LIGHT_BLUE);
        tft.print("Ave Mean");
        tft.setTextColor(RA8875_RED);
        tft.setCursor(5, 125);
        tft.print("Run Mean");
        tft.setTextColor(LIGHT_BLUE);
        tft.setFontScale((enum RA8875tsize)0);
        tft.setCursor(10, 103);
        tft.print("#");
        tft.setCursor(130, 103);
        tft.setTextColor(RA8875_WHITE);
        tft.print("Sec");

        tft.setFontScale((enum RA8875tsize)1);
        tft.setCursor(265, 160);
        tft.print("StdDev");

        tft.setFontScale((enum RA8875tsize)0);
        tft.setTextColor(RA8875_YELLOW);
        tft.setFontScale((enum RA8875tsize)0);
        if (millis() - remainTimeUpdate > 1000) {  // 1 second
          tft.setTextColor(RA8875_WHITE);
          tft.fillRect(287, 50, 180, tft.getFontHeight(), RA8875_BLACK);
          if ((float)(plotTimeInterval - timeIncrement * plotTimeInterval / 1200) <= 3600.) {
            tft.setCursor(190, 50);
            tft.print("Remain Time min.");
            tft.setCursor(330, 50);
            tft.print((float)(plotTimeInterval - (float)(timeIncrement * plotTimeInterval / 1200)) / 60, 1);

          } else {
            if ((float)(plotTimeInterval - (float)(timeIncrement * plotTimeInterval / 1200)) > 3600) {
              //tft.fillRect(300, 50, 200, CHAR_HEIGHT, RA8875_BLACK);
              tft.setCursor(190, 50);
              tft.print("Remain Time hrs.");
              tft.setCursor(330, 50);
              tft.print((float)(plotTimeInterval - (timeIncrement * plotTimeInterval / 1200)) / 3600, 2);
            }
          }
          remainTimeUpdate = millis();
        }
        if (calChrono2.elapsed() >= (long unsigned int)plotTimeInterval) {  //next point is 1.2 sec, 3.6 sec, 10.8 sec or 36 sec Chrono counts in MS
          tft.setFontScale((enum RA8875tsize)0);
          tft.setTextColor(LIGHT_BLUE);
          tft.setCursor(20, 103);
          tft.fillRect(20, 103, 50, CHAR_HEIGHT, RA8875_BLACK);
          tft.print(timeIncrement);
          tft.setTextColor(RA8875_WHITE);
          tft.setCursor(70, 103);
          tft.fillRect(70, 103, 70, CHAR_HEIGHT, RA8875_BLACK);
          tft.print((millis() - plotElapsedTimeStart) / 1000, 0);

          corrPlotXValue4 = map(timeIncrement, 0, 1200, 60, 760);
          corrPlotYValue4 = map(1000 * ((0.20000012146 * SAM_carrier_freq_offset)), -plotScaleNumber * 1000, plotScaleNumber * 1000, 440, 210);
          if (corrPlotYValue4 >= 400) corrPlotYValue4 = 439;
          if (corrPlotYValue4 <= 210) corrPlotYValue4 = 211;
          tft.fillCircle(corrPlotXValue4, corrPlotYValue4, 2, RA8875_YELLOW);

          frequencyDiffValue[timeIncrement] = 0.20000012146 * SAM_carrier_freq_offset;
          tft.setFontScale((enum RA8875tsize)1);
          arm_mean_f32(frequencyDiffValue, timeIncrement, &corrFactorAveMean);

          tft.setTextColor(LIGHT_BLUE);
          tft.setCursor(150, 160);
          tft.fillRect(150, 160, 95, CHAR_HEIGHT, RA8875_BLACK);
          tft.print(corrFactorAveMean, 3);

          if (timeIncrement >= 20) {  //Running Average reading for plot timencrement = 1.2, 3.6, 10.8 or 36 sec
            for (int i = 0; i < 20; i++) {
              frequencyDiffValueTen[i] = frequencyDiffValue[timeIncrement - (20 - i)];
            }
            arm_std_f32(frequencyDiffValueTen, 20, &corrFactorStdDev);
            arm_mean_f32(frequencyDiffValueTen, 20, &corrFactorMean);
            tft.setCursor(150, 125);
            tft.fillRect(150, 125, 120, CHAR_HEIGHT, RA8875_BLACK);

            tft.setTextColor(RA8875_RED);
            tft.print(corrFactorMean, 3);
            corrPlotXValue4 = map(timeIncrement, 0, 1200, 60, 760);
            corrPlotYValue6 = map(1000 * ((corrFactorMean)), -plotScaleNumber * 1000, plotScaleNumber * 1000, 440, 210);
            corrPlotYValue5 = map(1000 * ((corrFactorAveMean)), -plotScaleNumber * 1000, plotScaleNumber * 1000, 440, 210);
            if (corrPlotYValue5 >= 400) corrPlotYValue5 = 439;
            if (corrPlotYValue5 <= 210) corrPlotYValue5 = 211;
            if (corrPlotYValue6 >= 400) corrPlotYValue6 = 439;
            if (corrPlotYValue6 <= 210) corrPlotYValue6 = 211;
            //corrPlotYValue4 = map((int)1000 * 0.05, -100, 100, 440, 210);
            tft.fillCircle(corrPlotXValue4, corrPlotYValue6, 3, RA8875_RED);
            tft.fillCircle(corrPlotXValue4, corrPlotYValue5, 3, LIGHT_BLUE);
          }
          // Print values to screen
          tft.setTextColor(RA8875_WHITE);
          corrFactorStdDevAve = (corrFactorStdDevAve + corrFactorStdDev) / timeIncrement;
          tft.setCursor(375, 160);
          tft.fillRect(375, 160, 95, CHAR_HEIGHT, RA8875_BLACK);
          tft.print(corrFactorStdDev, 3);
          tft.setCursor(670, 95);
          tft.fillRect(670, 95, 100, CHAR_HEIGHT, RA8875_BLACK);
          tft.print(freqError, 3);
          timeIncrement++;
          if (timeIncrement >= 1200) {  //Stop the plot after 1200 points
            timeIncrement = 0;
            task = -1;
            //tft.fillRect(0, 95, 799, 385, RA8875_BLACK);
          }
          calChrono2.restart(0);
          calChrono.restart(0);
          freqCalType = 2;
          break;
        }
    }
    val = ReadSelectedPushButton();
    if (val != BOGUS_PIN_READ)  // Any button press??
    {
      val = ProcessButtonPress(val);  // Use ladder value to get menu choice
      if (val == MENU_OPTION_SELECT)  // Yep. Make a choice??
      {
        EEPROMWrite();
        calibrateFlag = 0;
        calOnFlag = 0;
        bands[currentBand].FLoCut = userFilterLowCut;  // AFP 01-30-25
        bands[currentBand].FHiCut = userFilterHiCut;
        bands[currentBand].mode = userFreqCalMode;
        RedrawDisplayScreen();
        tft.writeTo(L2);
        tft.clearMemory();
        tft.writeTo(L1);
        IQChoice = 5;
        freqCalFlag = 0;  //AFP 01-30-25
        return;
      } else {
        task = val;
        switch (task) {


          case (13):  //=11 F Tune Incr button - Select Plot vertical scale
            if (freqCalType == 2) {
              plotScaleIndex++;
              if (plotScaleIndex > 3) plotScaleIndex = 0;
              plotScaleNumber = plotScaleValues[plotScaleIndex];
              tft.setFontScale((enum RA8875tsize)0);
              tft.fillRect(12, 210, 30, CHAR_HEIGHT, RA8875_BLACK);
              tft.setCursor(12, 210);

              tft.setTextColor(RA8875_CYAN);
              tft.print(plotScaleNumber, 1);
              tft.fillRect(12, 440, 30, CHAR_HEIGHT, RA8875_BLACK);
              tft.setCursor(12, 440);
              tft.print(-plotScaleNumber, 1);
              tft.setCursor(12, 330);
              tft.print("  0");
              tft.setCursor(12, 270);
              tft.print("Hz");
            }

            break;
          case (12):  //=12 Filter Button - Select Time plot time interval
            if (freqCalType == 2) {
              plotIntervalIndex++;
              if (plotIntervalIndex > 3) plotIntervalIndex = 0;
              plotTimeInterval = plotIntervalValues[plotIntervalIndex];

              tft.setFontScale((enum RA8875tsize)0);

              tft.fillRect(130, 50, 80, tft.getFontHeight(), RA8875_BLACK);
              tft.setTextColor(RA8875_YELLOW);
              tft.setCursor(130, 50);
              tft.print((float)plotTimeInterval / 3600.);
            }
            break;

          case (DDE):  // 14  Direct Freq Button - Show Directions freqDescrptFlag = !freqDescrptFlag;

            freqCalDirections = !freqCalDirections;
            if (freqCalDirections != 0) {
              tft.fillRect(11, 199, 776, 280, RA8875_BLACK);
              tft.setFontScale((enum RA8875tsize)0);
              tft.setTextColor(RA8875_CYAN);
              tft.setCursor(10, 50);
              tft.print("Freq Cal Directions");
              tft.setCursor(25, 65);
              tft.print("* Input Std Freq Source or tune to WWV or CHU");
              tft.setCursor(25, 80);
              tft.print("  Step 1 Auto Freq Cal");
              tft.setCursor(25, 95);
              tft.print("  Tune to Source frequency");
              tft.setCursor(25, 110);
              tft.print("  Set Mode to SAM");
              tft.setCursor(25, 125);
              tft.print("  In Menu Select: Calibration/Freq Cal");
              tft.setCursor(25, 140);
              tft.print("Press User2 to do Auto Tune");
              tft.setCursor(25, 155);
              tft.print("  For best results do Auto Tune several times");
              tft.setCursor(25, 170);
              tft.print("Option: Time plot");
              tft.setCursor(25, 185);
              tft.print(" Pres User1 for Time Plot");
              tft.setCursor(10, 200);
              tft.print("  Set Plot time with Decode Button");
              tft.setCursor(25, 215);
              tft.print("  Select Vertical scale with Filter Button");
              tft.setCursor(25, 230);
              tft.print("Press User1 to restart");
              tft.setCursor(25, 245);
              tft.print("  When plot is finished - press Select to Sve/Exit");
              tft.setCursor(25, 260);
              tft.print("Press Select to Save/Exit");

            } else {
              tft.fillRect(0, 199, 799, 280, RA8875_BLACK);
              tft.fillRect(0, 50, 290, 321, RA8875_BLACK);
              tft.fillRect(288, 50, 150, 122, RA8875_BLACK);
              val = 16;
            }
            break;

          case (15):  //User1  Time Plot-Start/Reset
            tft.writeTo(L2);
            tft.clearMemory();
            tft.writeTo(L1);
            freqCalFlag = 1;  //AFP 01-30-25
            freqTimePlotFlag = !freqTimePlotFlag;
            tft.fillRect(340, 165, 250, CHAR_HEIGHT, RA8875_BLACK);
            freqCalType = 2;
            timeIncrement = 0;
            task = -1;
            plotElapsedTimeStart = 0;
            plotScaleIndex = 0;
            plotScaleNumber = plotScaleValues[plotScaleIndex];
            calChrono2.restart(0);
            calChrono.restart(0);
            plotIntervalIndex = 0;
            plotTimeInterval = plotIntervalValues[plotIntervalIndex];
            tft.setFontScale((enum RA8875tsize)0);
            tft.fillRect(130, 50, 80, tft.getFontHeight(), RA8875_BLACK);
            tft.setTextColor(RA8875_YELLOW);
            tft.setCursor(130, 50);
            tft.print((float)plotTimeInterval / 3600.);
            tft.fillRect(299, 200, 500, 279, RA8875_BLACK);
            tft.drawRect(10, 200, 778, 270, RA8875_GREEN);
            tft.fillRect(11, 201, 776, 268, RA8875_BLACK);
            tft.drawFastHLine(50, 450, 700, RA8875_GREEN);
            tft.drawFastVLine(50, 220, 230, RA8875_GREEN);
            tft.setFontScale((enum RA8875tsize)0);
            tft.setTextColor(RA8875_CYAN);
            tft.setCursor(12, 210);
            tft.print(plotScaleNumber, 1);
            tft.fillRect(12, 440, 30, CHAR_HEIGHT, RA8875_BLACK);
            tft.setCursor(12, 440);
            tft.print(-plotScaleNumber, 1);
            tft.setCursor(12, 330);
            tft.print("  0");
            tft.setCursor(12, 270);
            tft.print(" Hz");
            tft.drawFastHLine(50, 330, 700, RA8875_CYAN);
            tft.setCursor(400, 425);
            tft.print("Time->");
            break;

          case (16):          //User2 Auto Plot Setup/Staret/Restart
            freqCalFlag = 1;  //AFP 01-30-25
            freqCalFactorStart = EEPROMData.freqCorrectionFactor;

            tft.fillRect(0, 50, 400, 32, RA8875_BLACK);
            tft.fillRect(300, 200, 500, 279, RA8875_BLACK);
            tft.fillRect(0, 200, 799, 280, RA8875_BLACK);
            tft.fillRect(0, 98, 465, 180, RA8875_BLACK);
            tft.fillRect(670, 60, 100, CHAR_HEIGHT, RA8875_BLACK);  //
            tft.fillRect(340, 165, 250, CHAR_HEIGHT, RA8875_BLACK);
            tft.setTextColor(RA8875_YELLOW);
            tft.setFontScale((enum RA8875tsize)0);
            tft.setCursor(20, 240);
            tft.print("User2 - Auto");
            tft.setCursor(20, 255);
            tft.print("User2 - Time Plot");
            tft.setCursor(20, 270);
            tft.print("User3 - Corr Factor Increment");
            tft.setCursor(20, 285);
            tft.print("Select - Save/Exit");
            tft.setCursor(20, 320);
            tft.print("For Time Plot only:");
            tft.setCursor(20, 335);
            tft.print("Filter - Time Plot Error Scale");
            tft.setCursor(20, 350);
            tft.print("Decode - Time Plot Duration");
            tft.setCursor(20, 380);
            tft.print("Dir Freq - Directions");

            tft.setTextColor(RA8875_WHITE);
            tft.setFontScale((enum RA8875tsize)1);
            tft.setCursor(670, 60);
            tft.print(EEPROMData.freqCorrectionFactor, 0);
            tft.drawRect(300, 200, 480, 270, RA8875_GREEN);
            tft.drawFastHLine(340, 440, 400, RA8875_GREEN);
            tft.drawFastVLine(340, 210, 230, RA8875_GREEN);
            tft.setFontScale((enum RA8875tsize)0);
            tft.setCursor(310, 200);
            tft.print("5");
            tft.setCursor(310, 440);
            tft.print("-5");
            tft.setFontScale((enum RA8875tsize)0);
            tft.setCursor(310, 270);
            tft.print("Hz");
            tft.setCursor(480, 443);
            tft.print("Corr.");
            tft.drawFastHLine(340, 210 + 2 * 57.5, 400, RA8875_CYAN);
            for (int k = 0; k < 5; k++) {
              tft.drawFastVLine(340 + k * 100, 440, 7, RA8875_GREEN);
              tft.setCursor(330 + k * 100, 448);
              tft.print(-500 + 250 * k);

              tft.drawFastHLine(333, 210 + k * 57.5, 7, RA8875_GREEN);
              //480, 60
              tft.fillRect(390, 20, 159, CHAR_HEIGHT, RA8875_BLACK);
              tft.fillRect(400, 60, 55, CHAR_HEIGHT, RA8875_BLACK);
            }
            tft.setCursor(480, 443);
            tft.print("Corr.");
            tft.drawFastHLine(340, 210 + 2 * 57.5, 400, RA8875_CYAN);
            tft.setFontScale((enum RA8875tsize)0);

            freqAutoPlotFlag = !freqAutoPlotFlag;
            if (freqAutoPlotFlag == 1) {
              freqCalType = 1;
            } else {
              freqCalType = 0;
            }
            break;

          case (CAL_CHANGE_INC):  // 17 User3 - CAL_CHANGE_INC increment17 User3
            {
              freqCorrIndex++;
              if (freqCorrIndex >= 3) freqCorrIndex = 0;
              incrementSAM = freqCorrIncrementValues[freqCorrIndex];

              tft.setFontScale((enum RA8875tsize)0);

              tft.fillRect(600, 175, 150, tft.getFontHeight(), RA8875_BLACK);
              tft.setTextColor(RA8875_YELLOW);
              tft.setCursor(600, 175);
              tft.print("increment=  ");
              //tft.setCursor(680, 180);
              tft.print(incrementSAM);
              break;
            }


            if (task == MENU_OPTION_SELECT) {

              break;
            }
        }           //switch
      }             //else
    }               //if
  }                 //while
  freqCalFlag = 0;  //AFP 01-30-25
  CalibratePost();
  return;
}

//===================

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
  digitalWrite(RXTX, LOW);
  radioState = CW_TRANSMIT_STRAIGHT_STATE;  //
  userXmtMode = xmtMode;                    // Store the user's mode setting.   July 22, 2023
  userZoomIndex = spectrum_zoom;            // Save the zoom index so it can be reset at the conclusion.   August 12, 2023
  spectrum_zoom = setZoom;                  // spectrum_zoom is used in Process.cpp
  zoomIndex = setZoom - 1;
  userCurrentFreq = currentFreq;
  userTxRxFreq = TxRxFreq;
  userNCOFreq = NCOFreq;
  userCenterFreq = centerFreq;
  transmitPowerLevelTemp = transmitPowerLevel;
  TxRxFreq = centerFreq;
  currentFreq = TxRxFreq;
  NCOFreq = 0L;
  //AFP07-13-24
  ButtonZoom();
  tft.clearScreen(RA8875_BLACK);  // must call ButtonZoom because it configures the FFT!

  tft.writeTo(L2);  // Erase the bandwidth bar.   August 16, 2023
  tft.clearMemory();
  tft.writeTo(L1);
  tft.setFontScale((enum RA8875tsize)0);

  tft.setTextColor(RA8875_GREEN);
  tft.setCursor(550, 130);
  tft.print("Attn In/Out - User1");
  tft.setCursor(550, 145);
  tft.print("Auto Cal -    User2");
  tft.setCursor(550, 160);
  tft.print("Incr. -       User3");
  tft.setCursor(550, 175);
  tft.print("Gain Adj -    Filter Encoder");
  tft.setCursor(550, 190);
  tft.print("Phase Adj -   Vol Encoder");
  tft.setCursor(550, 205);
  tft.print("Exit/Save -   Select");

  //tft.fillRect(0, 272, 517, 399, RA8875_BLACK);  // Erase waterfall.   August 14, 2023
  tft.setCursor(530, 90);
  tft.print("Incr= ");
  tft.fillRect(650, 90, 50, tft.getFontHeight(), RA8875_BLACK);
  tft.setCursor(650, 90);
  tft.print(correctionIncrement, 3);

  userScale = currentScale;  //  Remember user preference so it can be reset when done.
  currentScale = 1;          //  Set vertical scale to 10 dB during calibration.
  updateDisplayFlag = 0;

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
  BandInformation();
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
  si5351.output_enable(SI5351_CLK2, 0);
  digitalWrite(CAL, CAL_OFF);
  SetRF_InAtten(currentRF_InAtten);
  SetRF_OutAtten(currentRF_OutAtten);
  bands[currentBand].FLoCut = userFilterLowCut;  // AFP 01-30-25
  bands[currentBand].FHiCut = userFilterHiCut;   // AFP 01-30-25
  updateDisplayFlag = 0;
  radioState = SSB_RECEIVE_STATE;
  xrState = RECEIVE_STATE;
  T41State = SSB_RECEIVE;

  Q_in_L.clear();
  Q_in_R.clear();
  currentFreq = userCurrentFreq;
  TxRxFreq = userTxRxFreq;
  NCOFreq = userNCOFreq;
  centerFreq = userCenterFreq;
  calibrateFlag = 0;  // this was set when the Calibration menu option was selected
  calFreqShift = 0;
  currentScale = userScale;                     //  Restore vertical scale to user preference.
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
  tft.clearScreen(RA8875_BLACK);
  RedrawDisplayScreen();
  IQChoice = 5;  // this is the secondary menu choice. equivalent to Cancel
  calOnFlag = 0;
  mainMenuWindowActive = 0;
  radioState = SSB_RECEIVE_STATE;  //
  SetFreq();                       // Return Si5351 to normal operation mode.
  lastState = 1111;                // This is required due to the function deactivating the receiver.  This forces a pass through the receiver set-up code.   October 16, 2023
  freqCalFlag = 0;
  return;
}

/*====
  Purpose: Combined input/ output for the purpose of calibrating the receive IQ

   Parameter List:
      void

   Return value:
      void
 *****/
void tuneCalParameter(int indexStart, int indexEnd, float increment, float *IQCorrectionFactor, char prompt[]) {
  float adjMin = 100;
  int adjMinIndex = 0;

  float correctionFactor = *IQCorrectionFactor;
  for (int i = indexStart; i < indexEnd; i++) {
    *IQCorrectionFactor = correctionFactor + i * increment;
    FFTupdated = false;

    while (!FFTupdated) {
      adjdB = ShowSpectrum2();
    }
    if ((stateMachine == TX_STATE_TX_PHASE) | (stateMachine == TX_STATE_RX_PHASE)) {
      // Get two updated FFTs to avoid the same where the audio samples
      // span a change in the correction factor
      FFTupdated = false;
      while (!FFTupdated) {
        adjdB = ShowSpectrum2();
      }
    } else {
      adjdB = ShowSpectrum2();
    }
    if (adjdB < adjMin) {
      adjMin = adjdB;
      adjMinIndex = i;
    }
    tft.setFontScale((enum RA8875tsize)1);
    tft.setTextColor(RA8875_WHITE);
    tft.setCursor(30, 20);
    tft.print("Auto Tune");
    tft.fillRect(200, 20, 250, CHAR_HEIGHT, RA8875_BLACK);  // Increased rectangle size to full erase value.  KF5N August 12, 2023
    tft.setCursor(200, 20);
    tft.print(prompt);
    tft.setCursor(350, 20);
    tft.print(*IQCorrectionFactor, 3);
  }
  *IQCorrectionFactor = correctionFactor + adjMinIndex * increment;
  tft.fillRect(30, 20, 420, CHAR_HEIGHT, RA8875_BLACK);
}
/*====
  Purpose: Auto Tune calibrate the receive IQ

   Parameter List:
      void

   Return value:
      void
 *****/
void autotune(float *amp, float *phase, float gain_coarse_max, float gain_coarse_min,
              float phase_coarse_max, float phase_coarse_min,
              int gain_coarse_step2_N, int phase_coarse_step2_N,
              int gain_fine_N, int phase_fine_N, bool phase_first) {

  if (phase_first) {
    // Step 2: phase changes in 0.01 steps from -0.2 to 0.2. Find the minimum.
    int phaseStepsCoarseN = (int)((phase_coarse_max - phase_coarse_min) / 0.01 / 2);
    *phase = 0.0;
    tuneCalParameter(-phaseStepsCoarseN, phaseStepsCoarseN + 1, 0.01, phase, (char *)"IQ Phase");
    // Step 1: Gain in 0.01 steps from 0.5 to 1.5
    int gainStepsCoarseN = (int)((gain_coarse_max - gain_coarse_min) / 0.01 / 2);
    *amp = 1.0;
    //Serial.print("Step 1: ");
    tuneCalParameter(-gainStepsCoarseN, gainStepsCoarseN + 1, 0.01, amp, (char *)"IQ Gain");
  } else {
    // Step 1: Gain in 0.01 steps from 0.5 to 1.5
    int gainStepsCoarseN = (int)((gain_coarse_max - gain_coarse_min) / 0.01 / 2);
    *amp = 1.0;
    //Serial.print("Step 1: ");
    tuneCalParameter(-gainStepsCoarseN, gainStepsCoarseN + 1, 0.01, amp, (char *)"IQ Gain");
    // Step 2: phase changes in 0.01 steps from -0.2 to 0.2. Find the minimum.
    int phaseStepsCoarseN = (int)((phase_coarse_max - phase_coarse_min) / 0.01 / 2);
    *phase = 0.0;
    //Serial.print("Step 2: ");
    tuneCalParameter(-phaseStepsCoarseN, phaseStepsCoarseN + 1, 0.01, phase, (char *)"IQ Phase");
  }
  // Step 3: Gain in 0.01 steps from 4 steps below previous minimum to 4 steps above
  //Serial.print("Step 3: ");
  tuneCalParameter(-gain_coarse_step2_N, gain_coarse_step2_N + 1, 0.01, amp, (char *)"IQ Gain");
  // Step 4: phase in 0.01 steps from 4 steps below previous minimum to 4 steps above
  //Serial.print("Step 4: ");
  tuneCalParameter(-phase_coarse_step2_N, phase_coarse_step2_N + 1, 0.01, phase, (char *)"IQ Phase");
  // Step 5: gain in 0.001 steps 10 steps below to 10 steps above
  //Serial.print("Step 5: ");
  tuneCalParameter(-gain_fine_N, gain_fine_N + 1, 0.001, amp, (char *)"IQ Gain");
  // Step 6: phase in 0.001 steps 10 steps below to 10 steps above
  //Serial.print("Step 6: ");
  tuneCalParameter(-phase_fine_N, phase_fine_N + 1, 0.001, phase, (char *)"IQ Phase");
}
/*====
  Purpose: DoReceiveCalibrate

   Parameter List:
      void

   Return value:
      void
 *****/
void DoReceiveCalibrate() {
  stateMachine = RX_STATE;  // what calibration step are we in
  int task = -1;            // captures the button presses
  int lastUsedTask = -2;
  adjdB = 0;
  recCalOnFlag = 1;
  int task1;

  tft.setTextColor(RA8875_CYAN);
  CalibratePreamble(0);
  DrawSpectrumDisplayContainer();
  tft.setFontScale((enum RA8875tsize)1);
  tft.setTextColor(RA8875_YELLOW);
  tft.setCursor(530, 320);
  tft.print("IQ Gain");
  tft.fillRect(680, 320, 150, CHAR_HEIGHT, RA8875_BLACK);
  tft.setCursor(680, 320);
  tft.print(IQAmpCorrectionFactor[currentBand], 3);
  tft.setTextColor(RA8875_WHITE);
  tft.setCursor(530, 360);
  tft.print("IQ Phase");
  tft.fillRect(680, 360, 150, CHAR_HEIGHT, RA8875_BLACK);
  tft.setCursor(680, 360);
  tft.print(IQPhaseCorrectionFactor[currentBand], 3);

  SetFreq();
  Clk2SetFreq = (centerFreq + calFreqOffset) * SI5351_FREQ_MULT;
  si5351.output_enable(SI5351_CLK2, 1);
  si5351.set_freq(Clk2SetFreq, SI5351_CLK2);
  digitalWrite(XMIT_MODE, XMIT_CW);
  digitalWrite(CW_ON_OFF, CW_ON);
  digitalWrite(CAL, CAL_ON);
  uint8_t out_Atten = 60;
  uint8_t in_atten = 60;
  uint8_t previous_out_Atten = out_Atten;
  uint8_t previous_in_atten = in_atten;
  SetRF_InAtten(in_atten);
  SetRF_OutAtten(out_Atten);
  zoomIndex = 0;
  calTypeFlag = 0;  // RX cal
  IQPhaseCorrectionFactorOld = IQPhaseCorrectionFactor[currentBand];
  tft.setFontScale((enum RA8875tsize)1);
  tft.setCursor(530, 400);
  tft.print("adjdB= ");
  tft.setCursor(680, 400);
  tft.print(aveAdjdB2, 1);

  tft.setFontScale((enum RA8875tsize)0);
  tft.setTextColor(RA8875_CYAN);
  tft.setCursor(10, 290);
  tft.print("Directions ");
  tft.setCursor(25, 305);
  tft.print("* Jumper JP4: Cal Isolation");
  tft.setCursor(25, 320);
  tft.print("Option 1 Manual Adjust");
  tft.setCursor(25, 335);
  tft.print("* Adjust Gain (Filter Encoder) for min IQ image (Red)");
  tft.setCursor(25, 350);
  tft.print("* Adjust Phase (Vol Encoder) for min IQ image (Red)");
  tft.setCursor(25, 365);
  tft.print("* Alternate between Gain and Phase adjustment");
  tft.setCursor(25, 380);
  tft.print("OPtion 2 - Auto IQ Tune");
  tft.setCursor(25, 395);
  tft.print("* Press User2 - Auto Tune will start");
  tft.setCursor(25, 410);
  tft.print("* User3 to change Incr.");
  tft.setCursor(25, 425);
  tft.print("* Select to Save/Exit");
  bool changeOutAtten = true;  // let's you change in_atten when false
  digitalWrite(RXTX, LOW);
  radioState = CW_TRANSMIT_STRAIGHT_STATE;
  //===========
  while (true) {

    val = ReadSelectedPushButton();
    if (val != BOGUS_PIN_READ) {
      val = ProcessButtonPress(val);
      if (val != lastUsedTask) task1 = val;
      lastUsedTask = val;
    } else {
      task1 = BOGUS_PIN_READ;
    }

    task1 = val;
    if (task1 != -1) lastUsedTask = task1;  //  Save the last used task.
    task1 = -100;
    adjdB = ShowSpectrum2();              // this is where the IQ data processing is applied
    if (task != -1) lastUsedTask = task;  //  Save the last used task.
    task = -100;                          // Reset task after it is used.

    // Adjust the value of the TX attenuator:
    if (changeOutAtten) {
      out_Atten = GetFineTuneValueLive(0, 63, out_Atten, 1, (char *)"Out Atten");
    } else {
      in_atten = GetFineTuneValueLive(0, 63, in_atten, 1, (char *)"In Atten");
    }
    // Update via I2C if the attenuation value changed
    if (out_Atten != previous_out_Atten) {
      SetRF_OutAtten(out_Atten);
      previous_out_Atten = out_Atten;
    }
    if (in_atten != previous_in_atten) {
      SetRF_InAtten(in_atten);
      previous_in_atten = in_atten;
    }

    val = ReadSelectedPushButton();
    if (val != BOGUS_PIN_READ) {
      val = ProcessButtonPress(val);
      if (val != lastUsedTask) task1 = val;
      lastUsedTask = val;
    } else {
      task1 = BOGUS_PIN_READ;
    }

    switch (task1) {
      case (MENU_OPTION_SELECT):  //Exit and save
        recCalOnFlag = 0;
        //tft.fillRect(SECONDARY_MENU_X, MENUS_Y, EACH_MENU_WIDTH + 35, CHAR_HEIGHT, RA8875_BLACK);
        EEPROMData.IQAmpCorrectionFactor[currentBand] = IQAmpCorrectionFactor[currentBand];
        EEPROMData.IQPhaseCorrectionFactor[currentBand] = IQPhaseCorrectionFactor[currentBand];
        ButtonZoom();     // Restore the user's zoom setting.  Note that this function also modifies spectrum_zoom.
        EEPROMWrite();    // Save calibration numbers and configuration.   August 12, 2023
        tft.writeTo(L2);  // Clear layer 2.   July 31, 2023
        tft.clearMemory();
        tft.writeTo(L1);  // Exit function in layer 1.   August 3, 2023     // Restore the user's zoom setting.  Note that this function also modifies spectrum_zoom.
                          // EEPROMWrite();    // Save calibration numbers and configuration.   August 12, 2023
                          // tft.clearScreen(RA8875_BLACK);
                          // RedrawDisplayScreen();
                          // UpdateInfoWindow();
        si5351.output_enable(SI5351_CLK2, 0);
        CalibratePost();
        return;
        break;

      case (CAL_TOGGLE_ATTENUATOR):  //CAL_TOGGLE_ATTENUATOR = 15 User1

        changeOutAtten = !changeOutAtten;
        break;

      case (16):  //16 User2 Auto Tune - Run through the autocal routine

        correctionIncrement = 0.001,
        autotuneRec(&IQAmpCorrectionFactor[currentBand], &IQPhaseCorrectionFactor[currentBand],
                    GAIN_COARSE_MAX, GAIN_COARSE_MIN,
                    PHASE_COARSE_MAX, PHASE_COARSE_MIN,
                    GAIN_COARSE_STEP2_N, PHASE_COARSE_STEP2_N,
                    GAIN_FINE_N, PHASE_FINE_N, false);
        tft.setFontScale((enum RA8875tsize)1);
        tft.setTextColor(RA8875_WHITE);
        tft.setCursor(145, 150);
        tft.print("Auto Complete");
        //MyDelay(3000);
        break;

      case CAL_CHANGE_INC:  //CAL_CHANGE_INC =17 =User3 Change Increment
        recIQIncrementIndex++;
        if (recIQIncrementIndex >= 3) recIQIncrementIndex = 0;
        corrChangeIQIncrement = recIQIncrementValues[recIQIncrementIndex];
        tft.setFontScale((enum RA8875tsize)0);
        tft.setTextColor(RA8875_CYAN);
        tft.fillRect(650, 90, 50, tft.getFontHeight(), RA8875_BLACK);
        tft.setCursor(650, 90);
        tft.print(corrChangeIQIncrement, 3);
        break;
    }
    if (task1 != -1) lastUsedTask = task1;  //  Save the last used task.
    task1 = -100;
  }
  return;
}

/*****
  Purpose: Combined input/ output for the purpose of calibrating the transmit IQ

   Parameter List:
      void

   Return value:
      void

 *****/
void DoXmitIQCalibrate() {

  twoToneFlag = 0;
  SSB_PA_CalFlag = 0;
  IQCalFlag = 1;
  IQChoice = 2;
  int task = -100;
  int lastUsedTask = -2;
  static int val;
  static int corrChange;
  float corrPlotYValue3;

  int lastCorrPlotYValue = 0;
  int setCorrPlotDecimalFlag = 1;
  int XmitCalDirections = 0;
  int plotDataPointNum = 0;

  float IQCorrFactorPlotValueMin = 0.0;
  float IQLevelPlotValueMin = 20;
  Q_in_L.end();  //Set up input Queues for transmit
  Q_in_R.end();
  Q_in_L_Ex.begin();
  Q_in_R_Ex.begin();
  sgtl5000_1.micGain(currentMicGain);
  setBPFPath(BPF_IN_TX_PATH);
  xrState = TRANSMIT_STATE;
  // Set the frequency of the transmit: remove the IF offset, add the fine tune
  centerFreq = centerFreq - IFFreq + NCOFreq;
  SetFreq();
  digitalWrite(XMIT_MODE, XMIT_SSB);
  currentRF_OutAtten = 3;
  if (currentRF_OutAtten > 63) currentRF_OutAtten = 63;
  if (currentRF_OutAtten < 0) currentRF_OutAtten = 0;
  SetRF_OutAtten(currentRF_OutAtten);
  xrState = TRANSMIT_STATE;
  modeSelectInR.gain(0, 0);
  modeSelectInL.gain(0, 0);
  modeSelectInExR.gain(0, 1);
  modeSelectInExL.gain(0, 1);
  modeSelectOutL.gain(0, 0);
  modeSelectOutR.gain(0, 0);
  modeSelectOutExL.gain(0, 1);  //AFP 10-21-22
  modeSelectOutExR.gain(0, 1);  //AFP 10-21-22
  ShowTransmitReceiveStatus();
  //========== Screen print directons
  tft.clearScreen(RA8875_BLACK);
  tft.writeTo(L2);  // Erase the bandwidth bar.   August 16, 2023
  tft.clearMemory();
  tft.writeTo(L1);
  tft.setTextColor(RA8875_WHITE);
  tft.setCursor(100, 10);
  tft.print("Calibrate TX IQ");

  //========
  tft.setTextColor(RA8875_YELLOW);

  tft.setCursor(610, 90);
  tft.setFontScale((enum RA8875tsize)0);
  tft.print("Incr ");
  tft.fillRect(650, 900, 50, tft.getFontHeight(), RA8875_BLACK);
  tft.setCursor(650, 90);
  tft.print(correctionIncrement, 3);
  tft.setFontScale((enum RA8875tsize)1);
  tft.setTextColor(RA8875_WHITE);
  //tft.fillRect(550 - 7, 350 - 1, 800 - 550, CHAR_HEIGHT, RA8875_BLACK);
  tft.drawRect(455, 10, 265, 110, RA8875_GREEN);
  //================= Gain Correction plot
  tft.drawRect(300, 200, 480, 270, RA8875_GREEN);
  tft.drawFastHLine(340, 440, 400, RA8875_GREEN);
  tft.drawFastVLine(340, 210, 230, RA8875_GREEN);
  tft.setFontScale((enum RA8875tsize)0);
  tft.setCursor(313, 285);
  tft.print("S");
  tft.setCursor(480, 446);
  tft.print("Gain");
  for (int k = 0; k < 5; k++) {
    tft.drawFastVLine(340 + k * 100, 440, 7, RA8875_GREEN);

    tft.setCursor(330 + k * 100, 448);
    tft.setCursor(330 + k * 100, 448);
    tft.print(.8 + 0.1 * (float)k, 2);
    tft.setCursor(310, 200 + k * 57.5);
    tft.print(20 - 5 * (float)k, 0);
    tft.drawFastHLine(333, 210 + k * 57.5, 7, RA8875_GREEN);
  }

  tft.setFontScale((enum RA8875tsize)1);
  tft.setCursor(400, 165);
  tft.print("IQ Image Level=");
  corrChangeIQIncrement = 1.0;

  //====================
  while (1) {
    //=================
    val = ReadSelectedPushButton();
    if (val != BOGUS_PIN_READ) {
      val = ProcessButtonPress(val);
      if (val != lastUsedTask && task == -100) task = val;
      else task = BOGUS_PIN_READ;
    }
    switch (task) {
      case (CAL_DIRECTIONS):  //Filter
        {
          XmitCalDirections = !XmitCalDirections;
          if (XmitCalDirections != 0) {
            tft.setFontScale((enum RA8875tsize)0);
            tft.setTextColor(RA8875_CYAN);
            tft.setCursor(10, 50);
            tft.print("Directions");
            tft.setCursor(25, 65);
            tft.print("* Attach receiver or SA to output thru attenuator");
            tft.setCursor(25, 80);
            tft.print("* Select: Menu/Calibrate/Xmit IQCal");
            tft.setCursor(25, 95);
            tft.print("* Attach switch to PTT");
            tft.setCursor(25, 110);
            tft.print("* Press and hold PTT");
            tft.setCursor(25, 125);
            tft.print("* Read IQ image level on SA or S meter on receiver");
            tft.setCursor(25, 140);
            tft.print("* Adjustments can only be made during Xmit");
            tft.setCursor(25, 155);
            tft.print("* Minimize IQ image:");
            tft.setCursor(25, 170);
            tft.print("* Filter encoder -> IQ Gain");
            tft.setCursor(25, 185);
            tft.print("* Volume encoder -> IQ Phase");
            tft.setCursor(25, 200);
            tft.print("* Alternate between ");
            tft.setCursor(25, 215);
            tft.print("  Gain and Phase adjustment");
            tft.setCursor(25, 230);
            tft.print("* Toggle Increment as needed");
            tft.setCursor(25, 245);
            tft.print("* Press Select to exit");
            tft.setCursor(10, 260);
            tft.print("Optional Plot");
            tft.setCursor(25, 275);
            tft.print("* To Plot S-units vs Gain Factor:");
            tft.setCursor(25, 290);
            tft.print("* Input S-Unit values");
            tft.setCursor(25, 305);
            tft.print("  using F Tune Encoder");
            tft.setCursor(25, 320);
            tft.print("* Press Filter Encoder");
            tft.setCursor(25, 335);
            tft.print("  SW to plot point");
            tft.setCursor(25, 350);
            tft.print("* Press User1 to change");
            tft.setCursor(25, 365);
            tft.print("  S-Unit input increment");
            tft.setCursor(25, 380);
            tft.print("Readout in Lower Right");
            tft.setCursor(25, 395);
            tft.print("shows the Gain for lowest");
            tft.setCursor(25, 410);
            tft.print("IQ Image");
          } else {
            tft.fillRect(0, 50, 290, 321, RA8875_BLACK);
            tft.fillRect(288, 50, 150, 192, RA8875_BLACK);
          }
          break;
        }
      default:
        break;
    }                                     // End switch
    if (task != -1) lastUsedTask = task;  //  Save the last used task.
    task = -100;

    tft.setFontScale((enum RA8875tsize)0);
    tft.setTextColor(RA8875_GREEN);
    tft.setCursor(465, 123);
    tft.print("User1 - IQ level input increment");
    tft.setCursor(465, 138);
    tft.print("User3 - Gain/Phase increment");
    //==============
    while (digitalRead(PTT) == LOW) {
      IQCalFlag = 1;
      digitalWrite(RXTX, HIGH);

      ExciterIQData();
      //=============
      if (corrPlotYValue != lastCorrPlotYValue) {

        tft.fillRect(670, 165, 50, tft.getFontHeight(), RA8875_BLACK);
        tft.setCursor(670, 165);
        tft.print(corrPlotYValue, 1);
        //}
      }
      lastCorrPlotYValue = corrPlotYValue;
      val = ReadSelectedPushButton();
      if (val != BOGUS_PIN_READ) {
        val = ProcessButtonPress(val);
        if (val != lastUsedTask) {
          task = val;
          lastUsedTask = val;
        } else {
          task = BOGUS_PIN_READ;
        }
      };
      switch (task) {

        case (CAL_CHANGE_INC):  //CAL_CHANGE_INC=17 User3
          {                     //
            corrChange = !corrChange;
            if (corrChange == 1) {          // Toggle increment value
              correctionIncrement = 0.001;  // AFP 2-11-23
            } else {
              correctionIncrement = 0.01;  // AFP 2-11-23
            }
            tft.setFontScale((enum RA8875tsize)0);

            tft.fillRect(650, 90, 50, tft.getFontHeight(), RA8875_BLACK);
            tft.setTextColor(RA8875_YELLOW);
            tft.setCursor(650, 90);
            tft.print(correctionIncrement, 3);
            break;
          }
        case (19):  //
          {
            corrPlotXValue = map(IQXAmpCorrectionFactor[currentBand], 0.8, 1.2, 340, 740);

            corrPlotYValue3 = map((int)(10 * corrPlotYValue), 0, 200, 450, 210);
            tft.fillCircle(corrPlotXValue, corrPlotYValue3, 2, RA8875_YELLOW);

            if (corrPlotYValue <= IQLevelPlotValueMin) {
              IQCorrFactorPlotValueMin = IQXAmpCorrectionFactor[currentBand];
              IQLevelPlotValueMin = corrPlotYValue;
              //              IQLevelPlotValueMinOld = IQLevelPlotValueMin;
            }
            tft.setFontScale((enum RA8875tsize)0);
            tft.fillRect(720, 380, 40, tft.getFontHeight(), RA8875_BLACK);
            tft.fillRect(720, 400, 40, tft.getFontHeight(), RA8875_BLACK);
            tft.setTextColor(RA8875_WHITE);
            tft.setTextColor(RA8875_GREEN);
            tft.setCursor(650, 370);
            tft.print("Min Gain Factor");
            tft.setCursor(650, 390);
            tft.print("Gain =");
            tft.setCursor(650, 410);
            tft.print("S-Unit=");
            tft.setCursor(720, 390);
            tft.print(IQCorrFactorPlotValueMin, 3);
            tft.setCursor(720, 410);
            tft.print(IQLevelPlotValueMin, 1);

            plotDataPointNum += 1;
            break;
          }
        case (15):  //Toggle Image Level decimal change 0= 0.1 change, 1=1.0 change
          {
            setCorrPlotDecimalFlag = !setCorrPlotDecimalFlag;
            if (setCorrPlotDecimalFlag == 1) {  // Toggle increment value
              corrChangeIQIncrement = 1.0;      // AFP 2-11-23
            } else {
              corrChangeIQIncrement = 0.1;  // AFP 2-11-23
            }

            break;
          }
      }

      if (task != -1) lastUsedTask = task;  //  Save the last used task.
      task = -100;
    }  // end while (digitalRead(PTT) == LOW)
    digitalWrite(RXTX, LOW);
    if (val == MENU_OPTION_SELECT) {
      tft.fillRect(SECONDARY_MENU_X, MENUS_Y, EACH_MENU_WIDTH + 35, CHAR_HEIGHT, RA8875_BLACK);
      EEPROMData.IQXRecAmpCorrectionFactor[currentBand] = IQXRecAmpCorrectionFactor[currentBand];
      EEPROMData.IQXRecPhaseCorrectionFactor[currentBand] = IQXRecPhaseCorrectionFactor[currentBand];
      EEPROMData.IQXAmpCorrectionFactor[currentBand] = IQXAmpCorrectionFactor[currentBand];
      EEPROMData.IQXPhaseCorrectionFactor[currentBand] = IQXPhaseCorrectionFactor[currentBand];
      tft.fillRect(SECONDARY_MENU_X, MENUS_Y, EACH_MENU_WIDTH + 35, CHAR_HEIGHT, RA8875_BLACK);
      EEPROMWrite();
      IQCalFlag = 0;
      break;
    }  //
  }    //end while(1)

  //RedrawDisplayScreen();
  lastState = SSB_TRANSMIT_STATE;
  // restore the centerFreq
  centerFreq = centerFreq + IFFreq - NCOFreq;
  SetFreq();

  Q_in_L_Ex.end();  // End Transmit Queue
  Q_in_R_Ex.end();
  Q_in_L.begin();  // Start Receive Queue
  Q_in_R.begin();
  xrState = RECEIVE_STATE;
  radioState = SSB_RECEIVE_STATE;
  // CalibratePost();
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
float ShowSpectrum2() {
  int x1 = 0;
  adjdB = 0.0;

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
  int capture_bins;  // Sets the number of bins to scan for signal peak.
  if (calTypeFlag == 0) {
    capture_bins = 10;
    cal_bins[0] = 128 + calFreqOffset / 375;
    cal_bins[1] = 384 - calFreqOffset / 375;
  }  // Receive calibration

  /******************************
   * The same LO clock is used for transmit and receive, so the bin tone and image are
   * found symmetric around the center of the FFT. This offset is 750 Hz (see sineTone() 
   * in Utility.cpp). We have zoom of x16, so the bin size is 375/16 = 23.4 Hz. So the 
   * bin numbers are 256 + 750/(375/16) = 256+32 = 288 and 256-32 = 224
   ******************************/
  if (calTypeFlag == 1 && bands[currentBand].mode == DEMOD_LSB) {
    capture_bins = 10;  // scans 2*capture_bins
    cal_bins[0] = 257 - 32;
    cal_bins[1] = 257 + 32;
  }  // Transmit calibration, LSB.
  if (calTypeFlag == 1 && bands[currentBand].mode == DEMOD_USB) {
    capture_bins = 10;  // scans 2*capture_bins
    cal_bins[0] = 257 + 32;
    cal_bins[1] = 257 - 32;
  }  // Transmit calibration, USB.

  //  There are 2 for-loops, one for the reference signal and another for the undesired sideband.
  for (x1 = cal_bins[0] - capture_bins; x1 < cal_bins[0] + capture_bins; x1++) adjdBIQ = PlotCalSpectrum(x1, cal_bins, capture_bins);
  for (x1 = cal_bins[1] - capture_bins; x1 < cal_bins[1] + capture_bins; x1++) adjdBIQ = PlotCalSpectrum(x1, cal_bins, capture_bins);

  // Finish up: AFP 2-11-23

  tft.setFontScale((enum RA8875tsize)1);
  //tft.fillRect(650, 115, 50, tft.getFontHeight(), RA8875_BLACK);
  //tft.setCursor(650, 115);  // 350, 125
  tft.fillRect(680, 400, 100, tft.getFontHeight(), RA8875_BLACK);
  tft.setCursor(680, 400);  // 350, 125
  adjdB2 = adjdBIQ;
  aveAdjdB2 = 0.9 * aveAdjdB2 + 0.1 * adjdB2;
  tft.print(aveAdjdB2, 1);
  MyDelay(10);

  return adjdBIQ;
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
  adjdB = 0.0;
  int16_t adjAmplitude = 0;  // Was float; cast to float in dB calculation.
  int16_t refAmplitude = 0;  // Was float; cast to float in dB calculation.
  uint32_t index_of_max;     // This variable is not currently used, but it is required by the ARM max function.
  if (x1 == (cal_bins[0] - capture_bins)) {  // Set flag at revised beginning.
    updateDisplayFlag = 1;                   //Set flag so the display data are saved only once during each display refresh cycle at the start of the cycle, not 512 times
  } else updateDisplayFlag = 0;              //  Do not save the the display data for the remainder of the

  //-------------------------------------------------------
  // This block of code, which calculates the latest FFT and finds the maxima of the tone
  // and its image product, does not technically need to be run every time we plot a pixel
  // on the screen. However, according to the comments below this is needed to eliminate
  // conflicts.
  ProcessIQData();  // Call the Audio process from within the display routine to eliminate conflicts with drawing the spectrum and waterfall displays
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
  tft.setFontScale((enum RA8875tsize)0);

  tft.setTextColor(RA8875_WHITE);
  tft.setCursor(cal_bins[1] - capture_bins + 20, 144);  // 350, 125
  tft.print("IQ Image");
  tft.setCursor(cal_bins[0] - capture_bins - 88, 144);  // 350, 125
  tft.print("Ref Level");
  tft.drawFastHLine(cal_bins[0] - capture_bins - 15, 152, 15, RA8875_GREEN);
  tft.drawFastHLine(cal_bins[0] - capture_bins + 22, 152, 15, RA8875_GREEN);
  tft.fillRect(cal_bins[0] - capture_bins, SPECTRUM_TOP_Y + 20, 2 * capture_bins, h - 6, RA8875_BLUE);  // SPECTRUM_TOP_Y = 100
  tft.fillRect(cal_bins[1] - capture_bins, SPECTRUM_TOP_Y + 20, 2 * capture_bins, h - 6, DARK_RED);     // h = SPECTRUM_HEIGHT + 3

  tft.writeTo(L1);
  return adjdB;
}

/*****
  Purpose: Two Tone test from digital signals 750Hz 1875 Hz

   Parameter List:
      void

   Return value:
      void

 *****/
void TwoToneTest() {  //AFP 02-01-25

  twoToneFlag = 1;
  IQCalFlag = 0;
  IQChoice = 5;
  int task = -100;
  int lastUsedTask = -2;
  static int val;
  static int corrChange;
  float secondFreq;
  float firstFreq;

  Q_in_L.end();  //Set up input Queues for transmit
  Q_in_R.end();
  Q_in_L_Ex.begin();
  Q_in_R_Ex.begin();
  comp1.setPreGain_dB(currentMicGain);
  comp2.setPreGain_dB(currentMicGain);
  setBPFPath(BPF_IN_TX_PATH);

  xrState = TRANSMIT_STATE;
  // Set the frequency of the transmit
  centerFreq = centerFreq - IFFreq + NCOFreq;
  SetFreq();
  digitalWrite(XMIT_MODE, XMIT_SSB);
  currentRF_OutAtten = XAttenSSB[currentBand] + getPowerLevelAdjustmentDB();
  if (currentRF_OutAtten > 63) currentRF_OutAtten = 63;
  if (currentRF_OutAtten < 0) currentRF_OutAtten = 0;
  SetRF_OutAtten(currentRF_OutAtten);

  //xmit on
  xrState = TRANSMIT_STATE;
  modeSelectInR.gain(0, 0);
  modeSelectInL.gain(0, 0);
  modeSelectInExR.gain(0, 1);
  modeSelectInExL.gain(0, 1);
  modeSelectOutL.gain(0, 0);
  modeSelectOutR.gain(0, 0);
  modeSelectOutExL.gain(0, 1);  //AFP 10-21-22
  modeSelectOutExR.gain(0, 1);  //AFP 10-21-22
  ShowTransmitReceiveStatus();
  //========== Screen print directons
  tft.clearScreen(RA8875_BLACK);
  tft.writeTo(L2);  // Erase the bandwidth bar.   August 16, 2023
  tft.clearMemory();
  tft.writeTo(L1);
  tft.setTextColor(RA8875_WHITE);
  tft.setCursor(100, 10);
  tft.print("Two Tone Test");
  tft.setFontScale((enum RA8875tsize)0);
  tft.setTextColor(RA8875_CYAN);
  tft.setCursor(10, 50);
  tft.print("Directions");
  tft.setCursor(10, 65);
  tft.print("* Calibrate t41 xmit IQ");
  tft.setCursor(25, 80);
  tft.print("* Attach T41 output to Dummy load/Attenuator");
  tft.setCursor(25, 95);
  tft.print("* Attach switch to PTT");
  tft.setCursor(25, 110);
  tft.print("* Set T41 to desired frequency");
  tft.setCursor(25, 125);
  tft.print("* Set T41 SSA PA Power");
  tft.setCursor(25, 140);
  tft.print("* Attach Spectrum Analyzer to T41 thru Attenuator");
  tft.setCursor(25, 155);
  tft.print("* Set SA to center freq = T41 freq");
  tft.setCursor(25, 170);
  tft.print("* Set SA span to 20KHz or 50KHz");
  tft.setCursor(25, 185);
  tft.print("* Set SA attenuation or input level");
  tft.setCursor(25, 200);
  tft.print("* Select Calibrate/Two Tone Test from T41 Menu");
  tft.setCursor(25, 215);
  tft.print("* Select Tone Freq");
  tft.setCursor(25, 230);
  tft.print("* Press PTT switch to Measure");
  tft.setCursor(25, 245);
  tft.print("* Read T41 output on Spectrum Analyzer");
  tft.setCursor(25, 260);
  tft.print("* Press Select to exit");
  tft.setCursor(10, 290);
  tft.print(" Optional - Adjust Xmit IQ Gain/Phase");
  tft.setCursor(25, 305);
  tft.print("* Adjustments can only be made during Xmit");
  tft.setCursor(25, 320);
  tft.print("* Use Filter encoder to minimize IQ image");
  tft.setCursor(25, 335);
  tft.print("* Press User 2 to Alternate between Gain and Phase adjustment");
  tft.setCursor(25, 350);
  tft.print("* User 3 to Toggle Increment as needed");
  tft.setCursor(25, 365);
  tft.print("* Press Select to exit");

  tft.setFontScale((enum RA8875tsize)0);
  tft.setFontScale((enum RA8875tsize)0);
  tft.setTextColor(RA8875_GREEN);
  tft.setCursor(465, 130);
  tft.print("Filter - Change 2nd Freq");
  tft.setCursor(465, 145);
  tft.print("User1 - Change 2nd Freq");

  tft.setCursor(465, 160);
  tft.print("User2 - Gain/Phase");
  tft.setCursor(465, 175);
  tft.print("User3 - Gain/Phase increment");

  tft.setTextColor(RA8875_YELLOW);
  tft.setCursor(465, 90);
  tft.print("First Freq =");
  tft.setCursor(465, 105);
  tft.print("Second Freq =");

  if (IQEXChoice == 0) {  // AFP 2-11-23
    IQXAmpCorrectionFactor[currentBand] = GetEncoderValueLiveXCal(-2.0, 2.0, IQXAmpCorrectionFactor[currentBand], correctionIncrement, (char *)"IQ Gain", 3, IQEXChoice);
  } else {
    IQXPhaseCorrectionFactor[currentBand] = GetEncoderValueLiveXCal(-2.0, 2.0, IQXPhaseCorrectionFactor[currentBand], correctionIncrement, (char *)"IQ Phase", 3, IQEXChoice);
  }

  //====================
  while (1) {
    //=================
    val = ReadSelectedPushButton();

    sineTone(numTwoToneCycles1);
    sineTone2(numTwoToneCycles2);
    if (numTwoToneCycles1 > 12) numTwoToneCycles1 = 4;
    if (numTwoToneCycles2 > 36) numTwoToneCycles2 = 12;
    firstFreq = numTwoToneCycles1 * 24000 / 256;
    secondFreq = numTwoToneCycles2 * 24000 / 256;

    tft.setFontScale((enum RA8875tsize)0);
    tft.fillRect(600, 90, 50, tft.getFontHeight(), RA8875_BLACK);
    tft.fillRect(600, 105, 50, tft.getFontHeight(), RA8875_BLACK);
    tft.setTextColor(RA8875_YELLOW);
    tft.setCursor(600, 90);
    tft.print(firstFreq, 0);
    tft.setCursor(600, 105);
    tft.print(secondFreq, 0);

    //==============
    while (digitalRead(PTT) == LOW) {
      digitalWrite(RXTX, HIGH);
      ExciterIQData();

      val = ReadSelectedPushButton();
      if (val != BOGUS_PIN_READ) {
        val = ProcessButtonPress(val);
        if (val != lastUsedTask) {
          task = val;
          lastUsedTask = val;
        } else {
          task = BOGUS_PIN_READ;
        }
      }
      switch (task) {
        case (15):  //Change second Frequency
        {
          numTwoToneCycles2 = numTwoToneCycles2 + 4;
          break;
        }
        case (12):  //Change First Frequency
        {
          numTwoToneCycles1 = numTwoToneCycles1 + 4;
          break;
        }
        // Toggle gain and phase
        case (CAL_CHANGE_TYPE):  //CAL_CHANGE_TYPE=16 User2
        {
          IQEXChoice = !IQEXChoice;  //IQEXChoice=0, Gain  IQEXChoice=1, Phase
          break;
        }
        // Toggle increment value
        case (CAL_CHANGE_INC):  //CAL_CHANGE_INC=17 User3
        {                     //
          corrChange = !corrChange;
          if (corrChange == 1) {          // Toggle increment value
            correctionIncrement = 0.001;  // AFP 2-11-23
          } else {
            correctionIncrement = 0.01;  // AFP 2-11-23
          }
          tft.setFontScale((enum RA8875tsize)0);
          tft.fillRect(650, 90, 50, tft.getFontHeight(), RA8875_BLACK);
          tft.setTextColor(RA8875_YELLOW);
          tft.setCursor(650, 90);
          tft.print(correctionIncrement, 3);
          break;
        }
        case (19):  //
        {
          break;
        }
      }
    }  // end while (digitalRead(PTT) == LOW)

    if (task != -1) lastUsedTask = task;  //  Save the last used task.

    digitalWrite(RXTX, LOW);
    if (val == MENU_OPTION_SELECT) {
      tft.fillRect(SECONDARY_MENU_X, MENUS_Y, EACH_MENU_WIDTH + 35, CHAR_HEIGHT, RA8875_BLACK);
      EEPROMData.IQXRecAmpCorrectionFactor[currentBand] = IQXRecAmpCorrectionFactor[currentBand];
      EEPROMData.IQXRecPhaseCorrectionFactor[currentBand] = IQXRecPhaseCorrectionFactor[currentBand];
      EEPROMData.IQXAmpCorrectionFactor[currentBand] = IQXAmpCorrectionFactor[currentBand];
      EEPROMData.IQXPhaseCorrectionFactor[currentBand] = IQXPhaseCorrectionFactor[currentBand];
      tft.fillRect(SECONDARY_MENU_X, MENUS_Y, EACH_MENU_WIDTH + 35, CHAR_HEIGHT, RA8875_BLACK);
      break;
    }

  }  //end while(1)

  RedrawDisplayScreen();
  lastState = SSB_TRANSMIT_STATE;
  // restore the centerFreq
  centerFreq = centerFreq + IFFreq - NCOFreq;
  SetFreq();

  Q_in_L_Ex.end();  // End Transmit Queue
  Q_in_R_Ex.end();
  Q_in_L.begin();  // Start Receive Queue
  Q_in_R.begin();
  xrState = RECEIVE_STATE;
  ShowTransmitReceiveStatus();
  radioState = SSB_RECEIVE_STATE;
  IQCalFlag = 0;
  twoToneFlag = 0;
  //============
  recCalOnFlag = 0;
  tft.fillRect(SECONDARY_MENU_X, MENUS_Y, EACH_MENU_WIDTH + 35, CHAR_HEIGHT, RA8875_BLACK);
  EEPROMData.IQAmpCorrectionFactor[currentBand] = IQAmpCorrectionFactor[currentBand];
  EEPROMData.IQPhaseCorrectionFactor[currentBand] = IQPhaseCorrectionFactor[currentBand];
  ButtonZoom();     // Restore the user's zoom setting.  Note that this function also modifies spectrum_zoom.
  EEPROMWrite();    // Save calibration numbers and configuration.   August 12, 2023
  tft.writeTo(L2);  // Clear layer 2.   July 31, 2023
  tft.clearMemory();
  tft.writeTo(L1);  // Exit function in layer 1.   August 3, 2023     // Restore the user's zoom setting.  Note that this function also modifies spectrum_zoom.
  tft.clearScreen(RA8875_BLACK);
  RedrawDisplayScreen();
  UpdateInfoWindow();
  si5351.output_enable(SI5351_CLK2, 0);
  CalibratePost();
  return;
}

/*****
  Purpose: Calibrate SSB PA Power output
   Parameter List:
      void
   Return value:
      void
 *****/
void CW_PA_Calibrate() {
  int val = 0;
  Clk2SetFreq = (centerFreq + NCOFreq + CWToneOffsetsHz[EEPROMData.CWToneIndex]) * SI5351_FREQ_MULT;
  tft.clearScreen(RA8875_BLACK);
  si5351.set_freq(Clk2SetFreq, SI5351_CLK2);
  digitalWrite(CW_ON_OFF, CW_OFF);   // LOW = CW off, HIGH = CW on
  digitalWrite(XMIT_MODE, XMIT_CW);  //
  setBPFPath(BPF_IN_TX_PATH);
  modeSelectInR.gain(0, 0);
  modeSelectInL.gain(0, 0);
  modeSelectInExR.gain(0, 0);
  modeSelectOutL.gain(0, 0);
  modeSelectOutR.gain(0, 0);
  modeSelectOutExL.gain(0, 0);
  modeSelectOutExR.gain(0, 0);
  tft.setFontScale((enum RA8875tsize)1);
  tft.setTextColor(RA8875_GREEN);
  tft.setCursor(150, 70);
  tft.print("Calibrate CW PA Output Level");
  tft.setTextColor(RA8875_CYAN);
  tft.setCursor(50, 130);
  tft.print("CW Power Set Point");
  tft.setCursor(50, 160);
  tft.print("Atten Setting");
  tft.setCursor(50, 190);
  tft.print("Adjust Factor");
  //===========
  tft.setFontScale((enum RA8875tsize)0);
  tft.setTextColor(RA8875_CYAN);
  tft.setCursor(10, 270);
  tft.print("Directions ");
  tft.setCursor(25, 290);
  tft.print("* Connect T41 Antenna to Power Meter & DL");
  tft.setCursor(25, 305);
  tft.print("* Attach Key");
  tft.setCursor(25, 320);
  tft.print("* Adjust Power Level for Set Point - Menu: RF Set/Power level");
  tft.setCursor(25, 335);
  tft.print("* Select: Calibrate/CW PA Cal from menu");
  tft.setCursor(25, 350);
  tft.print("* Press Key to start CW output");
  tft.setCursor(25, 365);
  tft.print("* Adjust output level on Power Meter to match Set Point");
  tft.setCursor(25, 380);
  tft.print("    using Filter Encoder");
  tft.setCursor(25, 395);
  tft.print("* Release Key");
  tft.setCursor(25, 410);
  tft.print("* Press Select to Save/Exit");

  //================

  while (1) {
    tft.setFontScale((enum RA8875tsize)1);
    tft.setTextColor(RA8875_CYAN);
    tft.setCursor(450, 130);
    powerOutCW[currentBand] = -0.017 * pow(transmitPowerLevelCW, 3) + 0.4501 * pow(transmitPowerLevelCW, 2) - 5.095 * (transmitPowerLevelCW) + 51.086;
    tft.setCursor(450, 130);
    tft.print(transmitPowerLevelCW);
    tft.print(" W");
    if (filterEncoderMove != 0) {
      XAttenCW[currentBand] += filterEncoderMove;
      filterEncoderMove = 0.;
    }
    tft.fillRect(450, 160, 150, 35, RA8875_BLACK);
    tft.setCursor(450, 160);
    tft.print(-powerOutCW[currentBand] / 2, 1);
    tft.print(" dB");
    tft.setCursor(450, 190);
    tft.fillRect(450, 190, 100, 35, RA8875_BLACK);
    tft.print(-(float)XAttenCW[currentBand] / 2, 1);
    tft.print(" dB");
    SetRF_OutAtten(XAttenCW[currentBand] + powerOutCW[currentBand]);
    EEPROMWrite();
    if (digitalRead(paddleDit) == LOW) {
      digitalWrite(RXTX, HIGH);  //Turns on relay
      si5351.output_enable(SI5351_CLK2, 1);
      digitalWrite(CW_ON_OFF, CW_ON);
      digitalWrite(CAL, CAL_OFF);  // Route signal to TX output
      radioState = CW_TRANSMIT_STRAIGHT_STATE;
      ShowTransmitReceiveStatus();
    } else {
      if (digitalRead(paddleDit) == HIGH) {
        digitalWrite(RXTX, LOW);  //Turns on relay
        digitalWrite(CW_ON_OFF, CW_OFF);
        si5351.output_enable(SI5351_CLK2, 0);
        radioState = CW_RECEIVE;
        ShowTransmitReceiveStatus();
      }
    }

    val = ReadSelectedPushButton();
    if (val != BOGUS_PIN_READ)  // Any button press??
    {
      val = ProcessButtonPress(val);  // Use ladder value to get menu choice
      if (val == MENU_OPTION_SELECT)  // Yep. Make a choice??
      {
        lastState = CW_TRANSMIT_STRAIGHT_STATE;;
       // centerFreq = centerFreq + IFFreq - NCOFreq;
       // SetFreq();
        twoToneFlag = 0;
        IQCalFlag = 0;
        SSB_PA_CalFlag = 0;
        Q_in_L.begin();  // Start Receive Queue
        Q_in_R.begin();
        xrState = RECEIVE_STATE;
        radioState = SSB_RECEIVE_STATE;
        digitalWrite(RXTX, LOW); 
        digitalWrite(CAL, CAL_OFF);
        EEPROMWrite();
        //RedrawDisplayScreen();
        tft.writeTo(L2);
        tft.clearMemory();
        tft.writeTo(L1);
        //UpdateInfoWindow();
        break;
      }
      break;
      
    }
  }
}
/*****
  Purpose: Calibrate CW PA Power output

   Parameter List:
      void

   Return value:
      void

 *****/
void SSB_PA_Calibrate() {
  tft.writeTo(L1);
  Q_in_L.end();  //Set up input Queues for transmit
  Q_in_R.end();
  Q_in_L_Ex.begin();
  Q_in_R_Ex.begin();
  setBPFPath(BPF_IN_TX_PATH);
  SetRF_OutAtten(powerOutSSB[currentBand]);

  twoToneFlag = 0;
  IQCalFlag = 0;
  SSB_PA_CalFlag = 1;  //Internal Source
  float transmitPowerLevelSSBTemp;

  xrState = TRANSMIT_STATE;
  radioState = SSB_TRANSMIT_STATE;
  centerFreq = centerFreq - IFFreq + NCOFreq;
  SetFreq();
  tft.clearScreen(RA8875_BLACK);
  tft.writeTo(L2);
  tft.clearMemory();
  digitalWrite(XMIT_MODE, XMIT_SSB);  // KI3P, July 28, 2024

  tft.setFontScale((enum RA8875tsize)1);
  tft.setTextColor(RA8875_GREEN);
  tft.setCursor(160, 40);
  tft.print("Calibrate SSB PA Output Level");
  tft.setTextColor(RA8875_CYAN);
  tft.setCursor(50, 95);
  tft.print("SSB Power Set Point");
  tft.setCursor(50, 125);
  tft.print("Correction Factor");
  tft.setCursor(50, 155);
  tft.print("Attnenuator Setting");
  //===========
  tft.setFontScale((enum RA8875tsize)0);
  tft.setTextColor(RA8875_CYAN);
  tft.setCursor(10, 205);
  tft.print("Directions ");
  tft.setCursor(25, 220);
  tft.print("* First - Calibrate Xmit IQ");
  tft.setCursor(25, 235);
  tft.print("* Connect T41 Antenna to Power Meter & DL");
  tft.setCursor(25, 250);
  tft.print("* Set Mode to SSB & Attach switch to PPT");
  tft.setCursor(25, 265);
  tft.print("* Adjust SSB Power Level to 10W - Menu: RF Set/Power level");
  tft.setCursor(25, 280);
  tft.print("* Select: Calibrate/SSB PA Cal from menu");
  tft.setCursor(25, 295);
  tft.print("* Uses Internal Source, press Key/switch");
  tft.setCursor(25, 310);
  tft.print("* Set level to 10W on External Power Meter");
  tft.setCursor(25, 325);
  tft.print("* Press Select to Save/Exit");
  tft.setCursor(25, 340);
  tft.print("* Attach Mic to input");
  tft.setCursor(25, 355);
  tft.print("* Press PTT & speak into Mic");
  tft.setCursor(25, 370);
  tft.print("* Adjust Mic Gain fo achieve desired output");
  tft.setCursor(25, 385);
  tft.print("  using Vol switch adjustment: Mic option");
  tft.setCursor(25, 400);
  tft.print("* Use RF /Power set to new output level");

  modeSelectInR.gain(0, 0);
  modeSelectInL.gain(0, 0);
  modeSelectInExR.gain(0, 1);
  modeSelectInExL.gain(0, 1);
  modeSelectOutL.gain(0, 0);
  modeSelectOutR.gain(0, 0);
  modeSelectOutExL.gain(0, 1);  //AFP 10-21-22
  modeSelectOutExR.gain(0, 1);

  while (1) {
    tft.setFontScale((enum RA8875tsize)1);
    tft.setTextColor(RA8875_CYAN);
    tft.setCursor(450, 95);
    transmitPowerLevelSSBTemp = (0.0022 * pow((int)powerOutSSB[currentBand], 3) - 0.0598 * pow((int)powerOutSSB[currentBand], 2) - 0.2644 * ((int)powerOutSSB[currentBand]) + 13.7);
    tft.print(round(transmitPowerLevelSSBTemp), 0);
    tft.print(" W");
    while (digitalRead(PTT) == LOW) {

      if (filterEncoderMove != 0) {
        XAttenSSB[currentBand] += filterEncoderMove;
        filterEncoderMove = 0.;
      }
      tft.fillRect(450, 125, 100, 35, RA8875_BLACK);
      tft.setCursor(450, 125);
      tft.print(20 * log((float)XAttenSSB[currentBand] / 10), 1);
      tft.print(" dB");
      tft.fillRect(450, 155, 100, 35, RA8875_BLACK);
      tft.setCursor(450, 155);
      tft.print(-powerOutSSB[currentBand] / 2);
      tft.print(" dB");
      digitalWrite(RXTX, HIGH);  //xmit on
      xrState = TRANSMIT_STATE;
      ExciterIQData();
    }
    digitalWrite(RXTX, LOW);

    val = ReadSelectedPushButton();
    if (val != BOGUS_PIN_READ)  // Any button press??
    {
      val = ProcessButtonPress(val);  // Use ladder value to get menu choice
      if (val == MENU_OPTION_SELECT)  // Yep. Make a choice??
      {
        lastState = SSB_TRANSMIT_STATE;
        centerFreq = centerFreq + IFFreq - NCOFreq;
        SetFreq();
        twoToneFlag = 0;
        IQCalFlag = 0;
        SSB_PA_CalFlag = 0;
        Q_in_L.begin();  // Start Receive Queue
        Q_in_R.begin();
        xrState = RECEIVE_STATE;
        radioState = SSB_RECEIVE_STATE;
        digitalWrite(CAL, CAL_OFF);
        EEPROMWrite();
        //RedrawDisplayScreen();
        tft.writeTo(L2);
        tft.clearMemory();
        tft.writeTo(L1);
        //UpdateInfoWindow();
        val = -100;
        break;
      }
      if (val == 17) {  // 17 User3
        tft.clearScreen(RA8875_BLACK);
        tft.setFontScale((enum RA8875tsize)1);
        if (SSB_PA_CalFlag == 0) {
          twoToneFlag = 0;
          IQCalFlag = 0;
          tft.fillRect(450, 215, 120, 35, RA8875_BLACK);
          tft.setCursor(450, 215);
          tft.print("Internal");
          SSB_PA_CalFlag = 1;
        } else {
          if (SSB_PA_CalFlag == 1) {
            tft.fillRect(450, 215, 120, 35, RA8875_BLACK);
            tft.setCursor(450, 215);
            tft.print("External");
            SSB_PA_CalFlag = 0;
          }
        }
        val = -100;
      }
    }
  }
}

/*****
  Purpose: Calibrate CW PA Power output
   Parameter List:
      void
   Return value:
      void
 *****/
void DoSWRCal() {
  twoToneFlag = 0;
  SSB_PA_CalFlag = 0;
  IQCalFlag = 1;
  int task = -100;
  int lastUsedTask = -2;
  static int val;
  static int corrChange;
  int XmitCalDirections = 0;
  float correctionIncrement2 = 0.1;
  float lastSWR_PowerAdj;
  float lastSWRGainAdj;
  int lastSWR_R_Offset;
  SWRCalFlag = 1;

  Q_in_L.end();  //Set up input Queues for transmit
  Q_in_R.end();
  Q_in_L_Ex.begin();
  Q_in_R_Ex.begin();
  comp1.setPreGain_dB(10);
  comp2.setPreGain_dB(10);
  sgtl5000_1.micGain(currentMicGain);
  setBPFPath(BPF_IN_TX_PATH);
  SSB_PA_CalFlag = 0;
  twoToneFlag = 0;
  IQCalFlag = 0;
  /*if (compressorFlag == 1) {
          SetupMyCompressors(use_HP_filter, (float)currentMicThreshold, comp_ratio, attack_sec, release_sec);  // Cast currentMicThreshold to float.  KF5N, October 31, 2023
        } else {
          if (compressorFlag == 0) {
            SetupMyCompressors(use_HP_filter, 0.0, comp_ratio, 0.01, 0.01);
          }
        }*/
  xrState = TRANSMIT_STATE;
  centerFreq = centerFreq - IFFreq + NCOFreq;
  SetFreq();
  digitalWrite(XMIT_MODE, XMIT_SSB);  // KI3P, July 28, 2024
  //setBPFPath(BPF_IN_TX_PATH);
  SetRF_OutAtten(powerOutSSB[currentBand]);
  digitalWrite(RXTX, HIGH);  //xmit on
  xrState = TRANSMIT_STATE;
  modeSelectInR.gain(0, 0);
  modeSelectInL.gain(0, 0);
  modeSelectInExR.gain(0, 1);
  modeSelectInExL.gain(0, 1);
  modeSelectOutL.gain(0, 0);
  modeSelectOutR.gain(0, 0);
  //modeSelectOutExL.gain(0, (float)XAttenSSB[currentBand] / 10.0);  //AFP 10-21-22
  //modeSelectOutExR.gain(0, (float)XAttenSSB[currentBand] / 10.0);

  modeSelectOutExL.gain(0, 1);  //AFP 10-21-22
  modeSelectOutExR.gain(0, 1);
  //ShowTransmitReceiveStatus();

  //========== Screen print directons
  tft.clearScreen(RA8875_BLACK);
  tft.writeTo(L2);  // Erase the bandwidth bar.   August 16, 2023
  tft.clearMemory();
  tft.writeTo(L1);
  tft.setTextColor(RA8875_WHITE);
  tft.setFontScale((enum RA8875tsize)1);
  tft.setCursor(100, 10);
  tft.print("Calibrate SWR/Power");

  //========
  tft.setTextColor(RA8875_YELLOW);

  tft.setCursor(465, 80);
  tft.setFontScale((enum RA8875tsize)0);
  tft.print("Incr ");
  tft.fillRect(550, 80, 50, tft.getFontHeight(), RA8875_BLACK);
  tft.setCursor(550, 80);
  tft.print(correctionIncrement2, 3);
  tft.setFontScale((enum RA8875tsize)1);
  tft.setTextColor(RA8875_WHITE);

  //====================
  while (1) {
    //=================
    val = ReadSelectedPushButton();
    if (val != BOGUS_PIN_READ) {
      val = ProcessButtonPress(val);
      if (val != lastUsedTask && task == -100) task = val;
      else task = BOGUS_PIN_READ;
    }
    switch (task) {
      case (CAL_DIRECTIONS):  //Filter
        {
          XmitCalDirections = !XmitCalDirections;
          if (XmitCalDirections != 0) {
            tft.setFontScale((enum RA8875tsize)0);
            tft.setTextColor(RA8875_CYAN);
            tft.setCursor(10, 100);
            tft.print("Directions ");
            tft.setCursor(25, 115);
            tft.print("* Connect T41 Antenna to Power Meter & DL");
            tft.setCursor(25, 130);
            tft.print("* Input signal to Mic Jack");
            tft.setCursor(25, 145);
            tft.print("* Attach Key/SW to PTT");
            tft.setCursor(25, 160);
            tft.print("* Select: Calibrate/SWR Cal from menu");
            tft.setCursor(25, 175);
            tft.print("* Press PTT SW to start output");
            tft.setCursor(25, 190);
            tft.print("* Adjust output level on Power Meter to 10W");
            tft.setCursor(25, 205);
            tft.print("* User Filter En to set Pwr reading to = output");
            tft.setCursor(25, 220);
            tft.print("8 Adjust output level on Power Meter to 2W");
            tft.setCursor(25, 235);
            tft.print("* Using Vol En set Pwr reading to 2Wr");
            tft.setCursor(25, 250);
            tft.print("* Repeat last 2 steps as needed");
            tft.setCursor(25, 265);
            tft.print("8 Set DL resistance to 100 ohms");
            tft.setCursor(25, 280);
            tft.print("* Press User to switch Vol to Offset");
            tft.setCursor(25, 295);
            tft.print("*Adj Offset for SWR = 2.0");
            tft.setCursor(25, 310);
            tft.print("* Press Select to Save/Exit");
          } else {
            tft.fillRect(0, 100, 408, 321, RA8875_BLACK);
            //tft.fillRect(288, 50, 150, 192, RA8875_BLACK);
          }
          break;
        }
      default:
        break;
    }                                     // End switch
    if (task != -1) lastUsedTask = task;  //  Save the last used task.
    task = -100;

    tft.setFontScale((enum RA8875tsize)0);
    tft.setTextColor(RA8875_GREEN);
    tft.setCursor(465, 100);
    tft.print("User2 -Toggle");
    tft.setCursor(465, 115);
    tft.print("    Vol EN PowerAdj/SWR Gain Adj");
    tft.setCursor(465, 135);
    tft.print("User3 - Adj increment");
    tft.setCursor(465, 155);
    tft.print("DirFreq - Directions");
    tft.setFontScale((enum RA8875tsize)1);

    tft.setCursor(410, 200);
    tft.print("Power ");
    tft.setCursor(410, 230);
    tft.print("SWR ");
    tft.setCursor(410, 260);
    tft.print("Power Adj ");
    tft.setCursor(410, 290);
    tft.print("SWR Slope Adj ");
    tft.setCursor(410, 320);
    tft.print("SWR R Offset ");
    tft.fillRect(650, 200, 100, tft.getFontHeight(), RA8875_BLACK);
    tft.setCursor(650, 200);
    tft.print(Pf_W, 1);
    tft.fillRect(650, 230, 100, tft.getFontHeight(), RA8875_BLACK);
    tft.setCursor(650, 230);
    tft.print(swr, 2);
    tft.fillRect(650, 260, 100, tft.getFontHeight(), RA8875_BLACK);
    tft.setCursor(650, 260);
    tft.print(SWR_PowerAdj[currentBand], 3);
    tft.fillRect(650, 290, 130, tft.getFontHeight(), RA8875_BLACK);
    tft.setCursor(650, 290);
    tft.print(SWRSlopeAdj[currentBand], 3);
    tft.fillRect(650, 320, 100, tft.getFontHeight(), RA8875_BLACK);
    tft.setCursor(650, 320);
    tft.print(SWR_R_Offset[currentBand]);

    //==============
    while (digitalRead(PTT) == LOW) {
      IQCalFlag = 2;
      val = ReadSelectedPushButton();
      if (val != BOGUS_PIN_READ) {
        val = ProcessButtonPress(val);
        if (val != lastUsedTask) {
          task = val;
          lastUsedTask = val;
        } else {
          task = BOGUS_PIN_READ;
        }
      };

      switch (task) {
        case (16):  //User2
          SWROffsetChange = !SWROffsetChange;
          tft.setCursor(410, 350);
          tft.fillRect(600, 350, 150, tft.getFontHeight(), RA8875_BLACK);
          tft.print("SWR Offset Change= ");
          tft.print(SWROffsetChange);
          break;

        case (CAL_CHANGE_INC):  //CAL_CHANGE_INC=17 User3

          {  //
            corrChange = !corrChange;
            if (corrChange == 1) {         // Toggle increment value
              correctionIncrement2 = 1.0;  // AFP 2-11-23
            } else {
              correctionIncrement2 = 0.1;  // AFP 2-11-23
            }
            tft.setFontScale((enum RA8875tsize)0);

            tft.fillRect(550, 80, 50, tft.getFontHeight(), RA8875_BLACK);
            tft.setTextColor(RA8875_YELLOW);
            tft.setCursor(550, 80);
            tft.print(correctionIncrement2, 3);
            break;
          }
      }
      digitalWrite(RXTX, HIGH);
      SWR_PowerAdj[currentBand] = GetEncoderValueLiveSWRAdj(-20.0, 20.0, SWR_PowerAdj[currentBand], correctionIncrement2);
      if (SWROffsetChange == 0) {
        SWRSlopeAdj[currentBand] = GetEncoderValueLiveXSWRSlope(-50, 50, SWRSlopeAdj[currentBand], 0.1);

      } else {
        if (SWROffsetChange == 1) {
          SWR_R_Offset[currentBand] = GetEncoderValueLiveXSWROffset(-200, 200, SWR_R_Offset[currentBand], 1);
        }
      }

      Serial.print("SWR_R_Offset= ");
      Serial.println(SWR_R_Offset[currentBand]);
      ExciterIQData();
      read_SWR();
      //=============
      if (SWR_PowerAdj[currentBand] != lastSWR_PowerAdj || SWRSlopeAdj[currentBand] != lastSWRGainAdj || SWR_R_Offset[currentBand] != lastSWR_R_Offset) {
        tft.setFontScale((enum RA8875tsize)1);
        tft.setTextColor(RA8875_CYAN);
        tft.fillRect(650, 200, 100, tft.getFontHeight(), RA8875_BLACK);
        tft.setCursor(650, 200);
        tft.print(Pf_W, 1);
        tft.fillRect(650, 230, 100, tft.getFontHeight(), RA8875_BLACK);
        tft.setCursor(650, 230);
        tft.print(swr, 2);
        tft.fillRect(650, 260, 100, tft.getFontHeight(), RA8875_BLACK);
        tft.setCursor(650, 260);
        tft.print(SWR_PowerAdj[currentBand], 3);
        tft.fillRect(650, 290, 100, tft.getFontHeight(), RA8875_BLACK);
        tft.setCursor(650, 290);
        tft.print(25 + SWRSlopeAdj[currentBand], 4);
        tft.fillRect(650, 320, 100, tft.getFontHeight(), RA8875_BLACK);
        tft.setCursor(650, 320);
        tft.print(SWR_R_Offset[currentBand]);
      }
      lastSWR_PowerAdj = SWR_PowerAdj[currentBand];
      lastSWRGainAdj = SWRSlopeAdj[currentBand];
      lastSWR_R_Offset = SWR_R_Offset[currentBand];


      if (task != -1) lastUsedTask = task;  //  Save the last used task.
      task = -100;
    }  // end while (digitalRead(PTT) == LOW)
    digitalWrite(RXTX, LOW);
    if (val == MENU_OPTION_SELECT) {
      tft.fillRect(SECONDARY_MENU_X, MENUS_Y, EACH_MENU_WIDTH + 35, CHAR_HEIGHT, RA8875_BLACK);
      EEPROMData.SWR_PowerAdj[currentBand] = SWR_PowerAdj[currentBand];
      EEPROMData.SWRSlopeAdj[currentBand] = SWRSlopeAdj[currentBand];
      EEPROMData.SWR_R_Offset[currentBand] = SWR_R_Offset[currentBand];

      tft.fillRect(SECONDARY_MENU_X, MENUS_Y, EACH_MENU_WIDTH + 35, CHAR_HEIGHT, RA8875_BLACK);
      EEPROMWrite();
      IQCalFlag = 0;
      SWRCalFlag = 0;
      break;
    }  //
  }    //end while(1)

  //RedrawDisplayScreen();
  lastState = SSB_TRANSMIT_STATE;
  // restore the centerFreq
  centerFreq = centerFreq + IFFreq - NCOFreq;
  SetFreq();

  Q_in_L_Ex.end();  // End Transmit Queue
  Q_in_R_Ex.end();
  Q_in_L.begin();  // Start Receive Queue
  Q_in_R.begin();
  xrState = RECEIVE_STATE;
  //ShowTransmitReceiveStatus();
  radioState = SSB_RECEIVE_STATE;
  SWRCalFlag = 0;
}
