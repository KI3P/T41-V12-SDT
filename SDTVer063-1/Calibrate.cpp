#ifndef BEENHERE
#include "SDT.h"
#endif

// changes are so extensive for calibration in V12, this file supports only V12
static int stateMachine;
static bool FFTupdated;
//static int val;
int corrChange = 0;
//AFP 2-7-23
static int userScale, userZoomIndex;//, userXmtMode;
//static int transmitPowerLevelTemp;
//static char strBuf[100];
static long userCurrentFreq;
static long userCenterFreq;
static long userTxRxFreq;
static long userNCOFreq;
//static float adjdB;
float corrPlotXValue = 300;
Timer CalTimer;
Chrono calChrono;
Chrono calChrono2;  //default milliseconds other usage - Chrono calChrono2(Chrono::SECONDS);
Metro calMetro = Metro(15000);
LinearRegression lr = LinearRegression();
double values[2];

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

  //int currentValue;

  tft.clearScreen(RA8875_BLACK);
  freqCalFlag = 1;
  int setZoom = 0;
  //int valuex;
  //IQCalType = 0;
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
  //int freqCorrectionFactor2;
  int corrPlotXValue4;
  int freqAutoPlotFlag = 0;
  int freqTimePlotFlag = 0;
  int updateTimeDisplayFlag = 0;
  //int freqDescrptFlag = 0;
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
  //float32_t corrFactorMean2;
  //float32_t corrFactorMeanNew;
  //float32_t corrFactorMeanOld;
  float32_t corrFactorCalc;
  float32_t corrFactorStdDevAve = 0;
  calChrono.restart(0);
  Linear2DRegression *linear2DRegression = new Linear2DRegression();
  TxRxFreq = centerFreq;
  currentFreq = TxRxFreq;
  NCOFreq = 0L;

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
  //modeSelectInExR.gain(0, 0);
  //modeSelectInExL.gain(0, 0);
  modeSelectOutL.gain(0, 1);
  modeSelectOutR.gain(0, 1);
  modeSelectOutL.gain(1, 0);
  modeSelectOutR.gain(1, 0);
  int plotTimeInterval;
  freqCalmode = DEMOD_SAM;
  stateMachine = RX_STATE;  // what calibration step are we in
  int task = -1;            // captures the button presses
  //int lastUsedTask = -2;
  //int lastUsedTask4 = -2;
  //int recCalDirections = 0;
  //int task4 = -1;
  //int val4;
  int freqAutoLowSet = -400;
  int freqAutoIncrementSet = 50;
  int freqCalFactorStart = 0;
  //float ErrorZeroPoint;
  adjdB = 0;

  tft.drawRect(470, 55, 310, 110, RA8875_GREEN);
  tft.setFontScale((enum RA8875tsize)1);
  tft.setTextColor(RA8875_WHITE);

  si5351.output_enable(SI5351_CLK2, 0);

  digitalWrite(CW_ON_OFF, CW_OFF);
  digitalWrite(CAL, CAL_OFF);
  //uint8_t out_Atten = 60;
  //uint8_t in_atten = 20;
  //uint8_t previous_out_Atten = out_Atten;
  //uint8_t previous_in_atten = in_atten;
  //SetRF_InAtten(in_atten);
  //SetRF_OutAtten(out_Atten);
  zoomIndex = 0;
  // let's you change in_atten when false
  int val;
  int32_t incrementSAM = 10L;
  int plotIntervalIndex = 0;
  int plotScaleIndex = 0;
  int freqCalDirections = 0;
  float plotScaleNumber = 0.;
  // SetFreq();
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
  tft.print("F Tune Incr - Time Plot Duration");
  tft.setCursor(20, 380);
  tft.print("Dir Freq - Directions");

  AMDecodeSAM();
  freqAutoPlotFlag = 0;
  //==============
  currentFreq = centerFreq + NCOFreq;  // Changed currentFreqA to currentFreq.  KF5N August 5, 2023
  NCOFreq = 0L;
  centerFreq = TxRxFreq = currentFreq;
  //EEPROMData.freqCorrectionFactor = 0.0;
  int autoCount = 0;
  freqAutoPlotFlag = 0;
  freqTimePlotFlag = 0;
  updateTimeDisplayFlag = 0;
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
    ProcessIQDataFreq();
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

      corrPlotXValue4 = map(-(freqCalFactorStart - EEPROMData.freqCorrectionFactor), -freqAutoLowSet, freqAutoLowSet, 340, 740);

      corrPlotYValue4 = map((100 * 0.20000012146 * SAM_carrier_freq_offset), -500, 500, 440, 210);
      // tft.fillCircle(corrPlotXValue4, corrPlotYValue4, 2, RA8875_YELLOW);
      //linear2DRegression->addPoint(0.20024 * SAM_carrier_freq_offset, EEPROMData.freqCorrectionFactor);
      updateDisplayFlag = 0;
      calChrono.restart(0);
      if (freqAutoPlotFlag == 1) {
        tft.fillCircle(corrPlotXValue4, corrPlotYValue4, 2, RA8875_YELLOW);
        linear2DRegression->addPoint(0.20000012146 * SAM_carrier_freq_offset, EEPROMData.freqCorrectionFactor);
        tft.fillRect(670, 130, 100, CHAR_HEIGHT, RA8875_BLACK);
        tft.setCursor(670, 130);
        tft.print(linear2DRegression->calculate(0), 0);
        corrFactorCalc = linear2DRegression->calculate(0);
        autoCount++;
        freqCalType = 1;
      }
    }
    // Serial.print("freqCalType= ");Serial.println(freqCalType);
    //=========
    switch (freqCalType) {  //Select the Trequency Cal Function
      case (0):             //Manual Plot

        break;
      case (1):  //Auto Plot Run

        Serial.print("autoCount= ");
        Serial.println(autoCount);
        updateDisplayFlag = 1;
        freqAutoPlotFlag = 1;

#ifdef TCXO_25MHZ
        freqAutoLowSet = 400;
        freqAutoIncrementSet = 50;
#else
        freqAutoLowSet = 500;
        freqAutoIncrementSet = 150;
#endif
        tft.setFontScale((enum RA8875tsize)0);
        tft.setCursor(310, 200);
        tft.print("5");
        tft.setCursor(310, 440);
        tft.print("-5");
        //if (freqAutoPlotFlag == 1) {
        freqCorrectionFactorOld = EEPROMData.freqCorrectionFactor;
        tft.fillRect(340, 165, 250, CHAR_HEIGHT, RA8875_BLACK);
        tft.setFontScale((enum RA8875tsize)1);
        tft.setCursor(340, 165);
        tft.print("Auto Tune On");
        EEPROMData.freqCorrectionFactor = freqCalFactorStart - freqAutoLowSet + autoCount * freqAutoIncrementSet;
        //calChrono.restart(0);
        if (autoCount >= 10) {
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

        /*tft.fillRect(340, 165, 250, CHAR_HEIGHT, RA8875_BLACK);
        tft.setFontScale((enum RA8875tsize)1);
        tft.setCursor(340, 165);
        tft.print("Auto Tune On");*/
        break;

      case (2):  //Time plot Run
        tft.setTextColor(RA8875_YELLOW);
        tft.setFontScale((enum RA8875tsize)0);
        tft.setCursor(10, 50);
        tft.print("Plot Time hrs.");

        updateTimeDisplayFlag = 1;
        tft.setFontScale((enum RA8875tsize)1);
        tft.setCursor(5, 160);
        tft.setTextColor(LIGHT_BLUE);
        tft.print("Ave Mean");
        tft.setTextColor(RA8875_RED);
        tft.setCursor(5, 125);
        tft.print("Run Mean");
        tft.setTextColor(RA8875_WHITE);
        tft.setCursor(10, 90);
        tft.print("#");
        tft.setCursor(350, 90);
        tft.print("Sec");

        tft.setCursor(265, 160);
        tft.print("StdDev");

        tft.setFontScale((enum RA8875tsize)0);
        tft.setTextColor(RA8875_YELLOW);
        tft.setFontScale((enum RA8875tsize)0);
        if (millis() - remainTimeUpdate > 1000) {  // 1 second
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
          tft.setFontScale((enum RA8875tsize)1);
          tft.setTextColor(RA8875_WHITE);
          tft.setCursor(60, 90);
          tft.fillRect(60, 90, 100, CHAR_HEIGHT, RA8875_BLACK);
          tft.print(timeIncrement);
          tft.setCursor(250, 90);
          tft.fillRect(250, 90, 100, CHAR_HEIGHT, RA8875_BLACK);
          tft.print((millis() - plotElapsedTimeStart) / 1000, 0);

          corrPlotXValue4 = map(timeIncrement, 0, 1200, 60, 760);
          corrPlotYValue4 = map(1000 * ((0.20000012146 * SAM_carrier_freq_offset)), -plotScaleNumber * 1000, plotScaleNumber * 1000, 440, 210);
          if (corrPlotYValue4 >= 400) corrPlotYValue4 = 439;
          if (corrPlotYValue4 <= 210) corrPlotYValue4 = 211;
          tft.fillCircle(corrPlotXValue4, corrPlotYValue4, 2, RA8875_YELLOW);

          frequencyDiffValue[timeIncrement] = 0.20000012146 * SAM_carrier_freq_offset;

          arm_mean_f32(frequencyDiffValue, timeIncrement, &corrFactorAveMean);
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
            tft.print(corrFactorMean, 3);
            corrPlotXValue4 = map(timeIncrement, 0, 1200, 60, 760);
            corrPlotYValue6 = map(1000 * ((corrFactorMean)), -plotScaleNumber * 1000, plotScaleNumber * 1000, 440, 210);
            corrPlotYValue5 = map(1000 * ((corrFactorAveMean)), -plotScaleNumber * 1000, plotScaleNumber * 1000, 440, 210);
            if (corrPlotYValue5 >= 400) corrPlotYValue5 = 439;
            if (corrPlotYValue5 <= 210) corrPlotYValue5 = 211;
            if (corrPlotYValue6 >= 400) corrPlotYValue6 = 439;
            if (corrPlotYValue6 <= 210) corrPlotYValue6 = 211;
            //corrPlotYValue4 = map((int)1000 * 0.05, -100, 100, 440, 210);
            tft.fillCircle(corrPlotXValue4, corrPlotYValue5, 3, RA8875_RED);
            tft.fillCircle(corrPlotXValue4, corrPlotYValue6, 3, LIGHT_BLUE);
          }
          // Print values to screen
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
        // tft.fillRect(SECONDARY_MENU_X, MENUS_Y, EACH_MENU_WIDTH + 35, CHAR_HEIGHT, RA8875_BLACK);
        EEPROMWrite();
        calibrateFlag = 0;
        calOnFlag = 0;
        RedrawDisplayScreen();
        IQChoice = 5;
        return;
        updateTimeDisplayFlag = 0;
      } else {
        task = val;
        updateTimeDisplayFlag = 0;
        switch (task) {
          case (13):  //13 Decode

            break;


          case (11):  //=11 F Tune Incr button - Select Plot vertical scale
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
              tft.print("  Set Plot time with F Tune Button");
              tft.setCursor(25, 215);
              tft.print("  Select Vertical scle with Filter Button");
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

          case (16):  //User2 Auto Plot Setup/Staret/Restart
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
            tft.print("F Tune Incr - Time Plot Duration");
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
            {                      //
              switch (incrementSAM){
                case 1:
                  incrementSAM = 10;
                  break;
                case 10:
                  incrementSAM = 100;
                  break;
                case 100:
                  incrementSAM = 1000;
                  break;
                case 1000:
                  incrementSAM = 1;
                  break;
                default:
                  incrementSAM = 1;
                  break;
              }
              //corrChange = !corrChange;
              //if (corrChange == 1) {  // increase increment value
              //  incrementSAM = 100;   // AFP 2-11-23
              //} else {
              //  incrementSAM = 1;  // AFP 2-11-23
              //}

              calOnFlag = 1;
              tft.setFontScale((enum RA8875tsize)0);
              tft.fillRect(10, 200, 50, tft.getFontHeight(), RA8875_BLACK);
              tft.setTextColor(RA8875_YELLOW);
              tft.setCursor(10, 200);
              tft.print(incrementSAM);
              break;
            }


            if (task == MENU_OPTION_SELECT) {

              break;
            }
        }  //switch
      }    //else
    }      //if
  }        //while
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
  recCalOnFlag = 1;
  corrChange = 0;
  updateCalDisplayFlag = 0;
  correctionIncrement = 0.01;  //AFP 2-7-23
  //IQCalType = 0;
  digitalWrite(RXTX, LOW);

  spectrum_zoom = setZoom;  // spectrum_zoom is used in Process.cpp
  zoomIndex = setZoom - 1;
  ButtonZoom();

  tft.fillRect(INFORMATION_WINDOW_X - 8, INFORMATION_WINDOW_Y, 250, 170, RA8875_BLACK);

  tft.fillRect(550, 280, 150, tft.getFontHeight(), RA8875_BLACK);
  tft.setFontScale((enum RA8875tsize)1);
  tft.setTextColor(RA8875_CYAN);
  tft.setCursor(550, 280);
  tft.print("RX Calibrate");
  tft.setTextColor(RA8875_WHITE);
  //tft.fillRect(550, 350, 230, CHAR_HEIGHT, RA8875_BLACK);
  tft.setCursor(550, 320);
  tft.print("IQ Gain");
  // tft.fillRect(550, 380, 230, CHAR_HEIGHT, RA8875_BLACK);
  tft.setCursor(550, 360);
  tft.print("IQ Phase");

  tft.setFontScale((enum RA8875tsize)1);
  tft.setTextColor(RA8875_YELLOW);
  tft.fillRect(700, 320, 150, CHAR_HEIGHT, RA8875_BLACK);
  tft.setCursor(700, 320);
  tft.print(IQAmpCorrectionFactor[currentBand], 3);
  IQAmpCorrectionFactorOld = IQAmpCorrectionFactor[currentBand];
  tft.fillRect(700, 360, 150, CHAR_HEIGHT, RA8875_BLACK);
  tft.setCursor(700, 360);
  tft.print(IQPhaseCorrectionFactor[currentBand], 3);
  IQPhaseCorrectionFactorOld = IQPhaseCorrectionFactor[currentBand];
  tft.setCursor(550, 400);
  tft.print("adjdB= ");
  tft.setFontScale((enum RA8875tsize)0);
  tft.setCursor(550, 450);
  tft.print("Incr= ");
  tft.setFontScale((enum RA8875tsize)1);

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
  updateDisplayFlag = 0;
  radioState = SSB_RECEIVE_STATE;
  xrState = RECEIVE_STATE;
  T41State = SSB_RECEIVE;
  Q_in_L.clear();
  Q_in_R.clear();
 
  calibrateFlag = 0;  // this was set when the Calibration menu option was selected
  calFreqShift = 0;
 
  EEPROMWrite();  // Save calibration numbers and configuration.   August 12, 2023
                  // zoomIndex = userZoomIndex - 1;
  //spectrum_zoom = userZoomIndex;
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
  si5351.output_enable(SI5351_CLK2, 0);
  radioState = SSB_RECEIVE_STATE;  //
  SetFreq();                       // Return Si5351 to normal operation mode.
  lastState = 1111;                // This is required due to the function deactivating the receiver.  This forces a pass through the receiver set-up code.   October 16, 2023
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
    //int XmitCalDirections = 0;
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
    //Serial.println(String(i)+","+String(adjdB));
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
  //*IQCorrectionFactor=-*IQCorrectionFactor
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
    //Serial.print("Step 2: ");
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
  recCalOnFlag = 1;
  updateCalDisplayFlag = 1;
  stateMachine = RX_STATE;
    Clk2SetFreq = (centerFreq ) * SI5351_FREQ_MULT;
  
  si5351.set_freq(Clk2SetFreq, SI5351_CLK2);
   si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_CURRENT);                                // KI3P July 27 2024, updated to mirror Setup()
      si5351.drive_strength(SI5351_CLK1, SI5351_DRIVE_CURRENT);                                // KI3P July 27 2024, updated to mirror Setup()
      si5351.drive_strength(SI5351_CLK2, SI5351_DRIVE_CURRENT);                                // KF5N July 10 2023
      si5351.set_ms_source(SI5351_CLK0, SI5351_PLLA);                                          // KI3P July 27 2024, updated to mirror Setup()
      si5351.set_ms_source(SI5351_CLK1, SI5351_PLLA);
      SetFreq();
      si5351.output_enable(SI5351_CLK2, 1);  
  digitalWrite(XMIT_MODE, XMIT_CW);
  digitalWrite(CW_ON_OFF, CW_ON);
  digitalWrite(CAL, CAL_ON);
  uint8_t out_Atten = 40;
  uint8_t in_atten = 60;
  SetRF_InAtten(in_atten);
  SetRF_OutAtten(out_Atten);

  zoomIndex = 0;
  //ButtonZoom();
  IQChoice = 5;
  return;



  //===========
  
}

/*****
  Purpose: Combined input/ output for the purpose of calibrating the transmit IQ

   Parameter List:
      void

   Return value:
      void

 *****/
void DoXmitCalibrate() {

  //IQCalFlag = !IQCalFlag;
  IQCalFlag = 1;
  IQChoice = 5;
  int task = -100;
  //int task1 = -100;
  //int directionsOn = 0;
  int lastUsedTask = -2;
  static int val;
  static int corrChange;
  //int corrIQChange = 0;
  //float corrPlotYValue2;
  float corrPlotYValue3;
  //float result;
  int lastCorrPlotYValue;
  int setCorrPlotDecimalFlag = 1;
  int XmitCalDirections = 0;
  float IQCorrFactorPlotData[30];
  float IQLevelPlotData[30];
  int plotDataPointNum = 0;
  //float IQLevelPlotValuePlotOld;
  float IQCorrFactorPlotValueMin;
  float IQLevelPlotValueMin = 20;
  float IQLevelPlotValue;
  float IQCorrFactorPlotYValue;
  //float IQCorrFactorPlotYValueOld;
  float IQLevelPlotValueMinOld;

  Q_in_L.end();  //Set up input Queues for transmit
  Q_in_R.end();
  Q_in_L_Ex.begin();
  Q_in_R_Ex.begin();
  comp1.setPreGain_dB(currentMicGain);
  comp2.setPreGain_dB(currentMicGain);
  setBPFPath(BPF_IN_TX_PATH);

  xrState = TRANSMIT_STATE;
  // Set the frequency of the transmit: remove the IF offset, add the fine tune
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
  modeSelectOutExL.gain(0, powerOutSSB[currentBand]);  //AFP 10-21-22
  modeSelectOutExR.gain(0, powerOutSSB[currentBand]);  //AFP 10-21-22
  ShowTransmitReceiveStatus();
  //========== Screen print directons
  tft.clearScreen(RA8875_BLACK);
  tft.writeTo(L2);  // Erase the bandwidth bar.   August 16, 2023
  tft.clearMemory();
  tft.writeTo(L1);
  tft.setTextColor(RA8875_WHITE);
  tft.setCursor(100, 10);
  tft.print("Calibrate TX");
  if (IQEXChoice == 0) {  // AFP 2-11-23
    IQXAmpCorrectionFactor[currentBand] = GetEncoderValueLiveXCal(-2.0, 2.0, IQXAmpCorrectionFactor[currentBand], correctionIncrement, (char *)"IQ Gain", 3, IQEXChoice);
  } else {
    IQXPhaseCorrectionFactor[currentBand] = GetEncoderValueLiveXCal(-2.0, 2.0, IQXPhaseCorrectionFactor[currentBand], correctionIncrement, (char *)"IQ Phase", 3, IQEXChoice);
  }

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
            tft.print("* Attach switch to PTT");
            tft.setCursor(25, 95);
            tft.print("* Press and hold PTT");
            tft.setCursor(25, 110);
            tft.print("* Read IQ image level on SA or S meter on receiver");
            tft.setCursor(25, 125);
            tft.print("* Adjustments can only be made during Xmit");
            tft.setCursor(25, 140);
            tft.print("* Use Filter encoder to minimize IQ image");
            tft.setCursor(25, 155);
            tft.print("* Alternate between Gain and Phase adjustment");
            tft.setCursor(25, 170);
            tft.print("* Toggle Increment as needed");
            tft.setCursor(25, 185);
            tft.print("* Press Select to exit");
            tft.setCursor(10, 200);
            tft.print("Optional Plot");
            tft.setCursor(25, 215);
            tft.print("* To Plot S-units vs Gain Factor:");
            tft.setCursor(25, 230);
            tft.print("* Input S-Unit values");
            tft.setCursor(25, 245);
            tft.print("  using Vol Encoder");
            tft.setCursor(25, 260);
            tft.print("* Press Filter Encoder ");
            tft.setCursor(25, 275);
            tft.print("  SW to plot point");
            tft.setCursor(25, 290);
            tft.print("* Press User1 to change");
            tft.setCursor(25, 305);
            tft.print("  S-Unit input increment");
            tft.setCursor(25, 325);
            tft.print("Readout in Lower Right");
            tft.setCursor(25, 340);
            tft.print("shows the Gain for lowest");
            tft.setCursor(25, 355);
            tft.print("IQ Image");
          } else {
            tft.fillRect(0, 50, 290, 321, RA8875_BLACK);
            tft.fillRect(288, 50, 150, 122, RA8875_BLACK);
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
    tft.print("User2 - Gain/Phase");
    tft.setCursor(465, 153);
    tft.print("User3 - Gain/Phase increment");

    //==============
    while (digitalRead(PTT) == LOW) {
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
      }

      switch (task) {
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
        case (19):  //    corrPlotYValue2=GetEncoderValueLiveIQLevel(0.0, 20.0, 10.0, 1, "IQ Level")
          {
            corrPlotXValue = map(IQXAmpCorrectionFactor[currentBand], 0.8, 1.2, 340, 740);

            corrPlotYValue3 = map((int)(10 * corrPlotYValue), 0, 200, 450, 210);
            tft.fillCircle(corrPlotXValue, corrPlotYValue3, 2, RA8875_YELLOW);
            IQCorrFactorPlotData[plotDataPointNum] = corrPlotYValue;
            IQLevelPlotData[plotDataPointNum] = IQXAmpCorrectionFactor[currentBand];
            IQCorrFactorPlotYValue = IQXAmpCorrectionFactor[currentBand];
            IQCorrFactorPlotYValue = IQXAmpCorrectionFactor[currentBand];
            IQLevelPlotValue = corrPlotYValue;
            if (corrPlotYValue <= IQLevelPlotValueMin) {
              IQCorrFactorPlotValueMin = IQXAmpCorrectionFactor[currentBand];
              IQLevelPlotValueMin = corrPlotYValue;
              IQLevelPlotValueMinOld = IQLevelPlotValueMin;
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
      break;
    }  //
  }    //end while(1)

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
  // CalibratePost();
  IQCalFlag = 0;
}


/*****
  Purpose: Signal processing for the purpose of calibrating the transmit IQ

   Parameter List:
      void

   Return value:
      void
 *****/
void ProcessIQData2() {
  float bandOutputFactor = 0.5;
  float rfGainValue;  // AFP 2-11-23
  float recBandFactor[NUMBER_OF_BANDS] = { 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0 };

  /******************************************************
   * Configure the audio transmit queue
   ******************************************************/
  // Generate I and Q for the transmit calibration.
  // Only configure it if it hasn't been configured before
  if ((stateMachine == TX_STATE_TX_PHASE)) {
    // We only have to configure the DAC output if we're in the TX phase of the TX configuration
    // Use pre-calculated sin & cos instead of Hilbert
    // Sidetone = 750 Hz. Sampled at 24 kHz
    //================

    arm_scale_f32(cosBuffer2, bandOutputFactor, float_buffer_L_EX, 256);
    arm_scale_f32(sinBuffer2, bandOutputFactor, float_buffer_R_EX, 256);

    //=========
    // We apply the correction factor with a varying sign here because this is how we
    // select the sideband to be transmitted
    if (bands[currentBand].mode == DEMOD_LSB) {
      arm_scale_f32(float_buffer_L_EX, +IQXAmpCorrectionFactor[currentBand], float_buffer_L_EX, 256);
      IQXPhaseCorrection(float_buffer_L_EX, float_buffer_R_EX, IQXPhaseCorrectionFactor[currentBand], 256);
    } else {
      if (bands[currentBand].mode == DEMOD_USB) {
        arm_scale_f32(float_buffer_L_EX, -IQXAmpCorrectionFactor[currentBand], float_buffer_L_EX, 256);
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
    if ((stateMachine == TX_STATE_TX_PHASE)) {
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
      sp_L1 = Q_in_R.readBuffer();  // this is not a mistake. L and R must have got switched at
      sp_R1 = Q_in_L.readBuffer();  // some point and the calibration process incorporates it

      /**********************************************************************************  AFP 12-31-20
          Using arm_Math library, convert to float one buffer_size.
          Float_buffer samples are now standardized from > -1.0 to < 1.0
      **********************************************************************************/
      arm_q15_to_float(sp_L1, &float_buffer_L[BUFFER_SIZE * i], BUFFER_SIZE);  // convert int_buffer to float 32bit
      arm_q15_to_float(sp_R1, &float_buffer_R[BUFFER_SIZE * i], BUFFER_SIZE);  // convert int_buffer to float 32bit
      Q_in_L.freeBuffer();
      Q_in_R.freeBuffer();
    }
    //=============== AFP 01-25-25  IQ Test Signals
    rfGainValue = pow(10, (float)rfGainAllBands / 20);
//==============
#ifdef IQ_REC_TEST
    arm_scale_f32(sinBuffer2K, .015, float_buffer_L, BUFFER_SIZE * N_BLOCKS);  //AFP 01-25-25
    arm_scale_f32(cosBuffer2K, .015, float_buffer_R, BUFFER_SIZE * N_BLOCKS);
#endif


    //AFP 2-11-23
    rfGainValue = pow(10, (float)rfGainAllBands / 20);
    arm_scale_f32(float_buffer_L, rfGainValue, float_buffer_L, BUFFER_SIZE * N_BLOCKS);  //AFP 2-11-23
    arm_scale_f32(float_buffer_R, rfGainValue, float_buffer_R, BUFFER_SIZE * N_BLOCKS);  //AFP 2-11-23


    arm_scale_f32(float_buffer_L, recBandFactor[currentBand], float_buffer_L, BUFFER_SIZE * N_BLOCKS);  //AFP 2-11-23
    arm_scale_f32(float_buffer_R, recBandFactor[currentBand], float_buffer_R, BUFFER_SIZE * N_BLOCKS);  //AFP 2-11-23

    // Receive amplitude correction
    if ((stateMachine == TX_STATE_TX_PHASE) | (stateMachine == TX_STATE_RX_PHASE)) {
      // Apply the special calibration parameters that apply only here
      arm_scale_f32(float_buffer_L, -IQXRecAmpCorrectionFactor[currentBand], float_buffer_L, BUFFER_SIZE * N_BLOCKS);
      IQPhaseCorrection(float_buffer_L, float_buffer_R, IQXRecPhaseCorrectionFactor[currentBand], BUFFER_SIZE * N_BLOCKS);
    } else {
      arm_scale_f32(float_buffer_L, -IQAmpCorrectionFactor[currentBand], float_buffer_L, BUFFER_SIZE * N_BLOCKS);  //AFP 04-14-22
      IQPhaseCorrection(float_buffer_L, float_buffer_R, IQPhaseCorrectionFactor[currentBand], BUFFER_SIZE * N_BLOCKS);
    }
    FreqShift1();
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
          //Serial.print(strBuf);
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
    capture_bins = 50;
    cal_bins[0] = 384;
    cal_bins[1] = 128;
  }  // Receive calibration

  /****************************** Transmit cal
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


  tft.setFontScale((enum RA8875tsize)0);
  tft.fillRect(650, 115, 50, tft.getFontHeight(), RA8875_BLACK);
  tft.setCursor(650, 115);  // 350, 125
  adjdB2 = adjdBIQ;
  aveAdjdB2 = 0.9 * aveAdjdB2 + 0.1 * adjdB2;
  tft.print(aveAdjdB2, 1);
  MyDelay(30);
  //tft.BTE_move(WATERFALL_LEFT_X, FIRST_WATERFALL_LINE, MAX_WATERFALL_WIDTH, MAX_WATERFALL_ROWS - 2, WATERFALL_LEFT_X, FIRST_WATERFALL_LINE + 1, 1, 2);
  // while (tft.readStatus())
  //  ;
  // tft.BTE_move(WATERFALL_LEFT_X, FIRST_WATERFALL_LINE + 1, MAX_WATERFALL_WIDTH, MAX_WATERFALL_ROWS - 2, WATERFALL_LEFT_X, FIRST_WATERFALL_LINE + 1, 2);
  // while (tft.readStatus())
  //  ;  // Make sure it's done.
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
  int16_t adjAmplitude = 0;                  // Was float; cast to float in dB calculation.
  int16_t refAmplitude = 0;                  // Was float; cast to float in dB calculation.
  uint32_t index_of_max;                     // This variable is not currently used, but it is required by the ARM max function.
  if (x1 == (cal_bins[0] - capture_bins)) {  // Set flag at revised beginning.
    updateDisplayFlag = 1;                   //Set flag so the display data are saved only once during each display refresh cycle at the start of the cycle, not 512 times
  } else updateDisplayFlag = 0;              //  Do not save the the display data for the remainder of the

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
  tft.setFontScale((enum RA8875tsize)0);

  tft.setTextColor(RA8875_WHITE);
  tft.setCursor(cal_bins[1] - capture_bins - 75, 164);  // 350, 125
  tft.print("IQ Image");
  tft.setCursor(cal_bins[0] - capture_bins - 88, 164);  // 350, 125
  tft.print("Ref Level");
  tft.drawFastHLine(cal_bins[0] - capture_bins - 15, 174, 15, RA8875_GREEN);
  tft.drawFastHLine(cal_bins[0] - capture_bins + 22, 174, 15, RA8875_GREEN);
  tft.fillRect(cal_bins[0] - capture_bins, SPECTRUM_TOP_Y + 20, 2 * capture_bins, h - 6, RA8875_BLUE);  // SPECTRUM_TOP_Y = 100

  tft.fillRect(cal_bins[1] - capture_bins, SPECTRUM_TOP_Y + 20, 2 * capture_bins, h - 6, DARK_RED);  // h = SPECTRUM_HEIGHT + 3
  tft.writeTo(L1);
  return adjdB;
}
//========= The following functions are for the Frequency calibration option AFP 01-12-25

/*****
  Purpose: Signal processing for the purpose of calibrating the frequency

   Parameter List:
      void

   Return value:
      void
 *****/
void ProcessIQDataFreq() {


  //float32_t audioMaxSquared;

  //uint32_t AudioMaxIndex;
  float rfGainValue;


  if ((uint32_t)Q_in_L.available() > N_BLOCKS + 0 && (uint32_t)Q_in_R.available() > N_BLOCKS + 0) {
    usec = 0;
    // get audio samples from the audio  buffers and convert them to float
    // read in 32 blocks á 128 samples in I and Q
    for (unsigned i = 0; i < N_BLOCKS; i++) {
      sp_L1 = Q_in_R.readBuffer();
      sp_R1 = Q_in_L.readBuffer();

      arm_q15_to_float(sp_L1, &float_buffer_L[BUFFER_SIZE * i], BUFFER_SIZE);  // convert int_buffer to float 32bit
      arm_q15_to_float(sp_R1, &float_buffer_R[BUFFER_SIZE * i], BUFFER_SIZE);  // convert int_buffer to float 32bit
      Q_in_L.freeBuffer();
      Q_in_R.freeBuffer();
    }

    rfGainValue = pow(10, (float)rfGainAllBands / 20);
    arm_scale_f32(float_buffer_L, rfGainValue, float_buffer_L, BUFFER_SIZE * N_BLOCKS);  //AFP 09-27-22
    arm_scale_f32(float_buffer_R, rfGainValue, float_buffer_R, BUFFER_SIZE * N_BLOCKS);  //AFP 09-27-22

    arm_scale_f32(float_buffer_L, bands[currentBand].RFgain, float_buffer_L, BUFFER_SIZE * N_BLOCKS);  //AFP 09-23-22
    arm_scale_f32(float_buffer_R, bands[currentBand].RFgain, float_buffer_R, BUFFER_SIZE * N_BLOCKS);  //AFP 09-23-22

    if (Q_in_L.available() > 25) {
      Q_in_L.clear();
      n_clear++;  // just for debugging to check how often this occurs
      AudioInterrupts();
    }
    if (Q_in_R.available() > 25) {
      Q_in_R.clear();
      n_clear++;  // just for debugging to check how often this occurs
      AudioInterrupts();
    }

    arm_scale_f32(float_buffer_L, -IQAmpCorrectionFactor[currentBand], float_buffer_L, BUFFER_SIZE * N_BLOCKS);  //AFP 04-14-22
    IQPhaseCorrection(float_buffer_L, float_buffer_R, IQPhaseCorrectionFactor[currentBand], BUFFER_SIZE * N_BLOCKS);


    if (spectrum_zoom == SPECTRUM_ZOOM_1) {  // && display_S_meter_or_spectrum_state == 1)
      CalcZoom1Magn();                       //AFP Moved to display function
    }
    display_S_meter_or_spectrum_state++;
    if (radioState == CW_TRANSMIT_STRAIGHT_STATE || radioState == CW_TRANSMIT_KEYER_STATE) {  //AFP 09-01-22
      return;
    }

    //FreqShift1();
    //FreqShift2();

    if (spectrum_zoom != SPECTRUM_ZOOM_1) {
      //AFP  Used to process Zoom>1 for display
      ZoomFFTExe(BUFFER_SIZE * N_BLOCKS);  // there seems to be a BUG here, because the blocksize has to be adjusted according to magnification,
      // does not work for magnifications > 8
    }

    // decimation-by-4 in-place!
    arm_fir_decimate_f32(&FIR_dec1_I, float_buffer_L, float_buffer_L, BUFFER_SIZE * N_BLOCKS);
    arm_fir_decimate_f32(&FIR_dec1_Q, float_buffer_R, float_buffer_R, BUFFER_SIZE * N_BLOCKS);

    // decimation-by-2 in-place
    arm_fir_decimate_f32(&FIR_dec2_I, float_buffer_L, float_buffer_L, BUFFER_SIZE * N_BLOCKS / (uint32_t)DF1);
    arm_fir_decimate_f32(&FIR_dec2_Q, float_buffer_R, float_buffer_R, BUFFER_SIZE * N_BLOCKS / (uint32_t)DF1);

    //sineTone(8);
    // arm_scale_f32(sinBuffer2, 1, float_buffer_L, FFT_length / 2);  // use to calibrate SAM
    // arm_scale_f32(cosBuffer2, 1, float_buffer_R, FFT_length / 2);
    // =================  AFP 10-21-22 Level Adjust ===========
    float freqKHzFcut;
    float volScaleFactor;
    if (bands[currentBand].mode == DEMOD_LSB) {
      freqKHzFcut = -(float32_t)bands[currentBand].FLoCut * 0.001;
    } else {
      freqKHzFcut = (float32_t)bands[currentBand].FHiCut * 0.001;
    }
    volScaleFactor = 7.0874 * pow(freqKHzFcut, -1.232);
    arm_scale_f32(float_buffer_L, volScaleFactor, float_buffer_L, FFT_length / 2);
    arm_scale_f32(float_buffer_R, volScaleFactor, float_buffer_R, FFT_length / 2);


    if (first_block) {  // fill real & imaginaries with zeros for the first BLOCKSIZE samples
      for (unsigned i = 0; i < BUFFER_SIZE * N_BLOCKS / (uint32_t)(DF / 2.0); i++) {
        FFT_buffer[i] = 0.0;
      }
      first_block = 0;
    } else  // All other FFTs

      //------------------------------ fill FFT_buffer with last events audio samples for all other FFT instances
      for (unsigned i = 0; i < BUFFER_SIZE * N_BLOCKS / (uint32_t)(DF); i++) {
        FFT_buffer[i * 2] = last_sample_buffer_L[i];      // real
        FFT_buffer[i * 2 + 1] = last_sample_buffer_R[i];  // imaginary
      }

    for (unsigned i = 0; i < BUFFER_SIZE * N_BLOCKS / (uint32_t)(DF); i++) {  // copy recent samples to last_sample_buffer for next time!
      last_sample_buffer_L[i] = float_buffer_L[i];
      last_sample_buffer_R[i] = float_buffer_R[i];
    }

    //------------------------------ now fill recent audio samples into FFT_buffer (left channel: re, right channel: im)
    for (unsigned i = 0; i < BUFFER_SIZE * N_BLOCKS / (uint32_t)(DF); i++) {
      FFT_buffer[FFT_length + i * 2] = float_buffer_L[i];      // real
      FFT_buffer[FFT_length + i * 2 + 1] = float_buffer_R[i];  // imaginary
    }

    arm_cfft_f32(S, FFT_buffer, 0, 1);

    /**********************************************************************************  AFP 12-31-20
      Continuing FFT Convolution
          Next, prepare the filter mask (done in the Filter.cpp file).  Only need to do this once for each filter setting.
          Allows efficient real-time variable LP and HP audio filters, without the overhead of time-domain convolution filtering.

          After the Filter mask in the frequency domain is created, complex multiply  filter mask with the frequency domain audio data.
          Filter mask previously calculated in setup Array of filter mask coefficients:
          FIR_filter_mask[]
     **********************************************************************************/

    arm_cmplx_mult_cmplx_f32(FFT_buffer, FIR_filter_mask, iFFT_buffer, FFT_length);

    arm_cfft_f32(iS, iFFT_buffer, 1, 1);

    for (int k = 0; k < 1024; k++) {
      audioSpectBuffer[1024 - k] = (iFFT_buffer[k]);
    }
    //==============

    AMDecodeSAM();
  }
}

/*****
  Purpose: Show Spectrum display modified for Frequency calibration.
           This is similar to the function used for normal reception, however, it has
           been simplified and streamlined for calibration.

  Parameter list:
    void

  Return value;
    void
*****/
float ShowSpectrumFreq() {
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
    capture_bins = 30;
    cal_bins[0] = 128;
    //cal_bins[1] = 256;
  }  // Receive calibration

  /******************************
   * The same LO clock is used for transmit and receive, so the bin tone and image are
   * found symmetric around the center of the FFT. This offset is 750 Hz (see sineTone() 
   * in Utility.cpp). We have zoom of x16, so the bin size is 375/16 = 23.4 Hz. So the 
   * bin numbers are 256 + 750/(375/16) = 256+32 = 288 and 256-32 = 224
   ******************************/
  /* if (calTypeFlag == 1 && bands[currentBand].mode == DEMOD_SAM) {
    capture_bins = 10;  // scans 2*capture_bins
                        //cal_bins[0] = 257 - 32;
                        //cal_bins[1] = 257 + 32;
    cal_bins[0] = 257 - 32;
    cal_bins[1] = 257 + 32;
  }  // Transmit calibration, LSB.
  if (calTypeFlag == 1 && bands[currentBand].mode == DEMOD_SAM) {
    capture_bins = 10;  // scans 2*capture_bins
    cal_bins[0] = 257 + 32;
    cal_bins[1] = 257 - 32;
  }  // Transmit calibration, USB.*/

  //  There are 2 for-loops, one for the reference signal and another for the undesired sideband.
  for (x1 = cal_bins[0] - capture_bins; x1 < cal_bins[0] + capture_bins; x1++) adjdBIQ = PlotCalSpectrumFreq(x1, cal_bins, capture_bins);
  //for (x1 = cal_bins[1] - capture_bins; x1 < cal_bins[1] + capture_bins; x1++) adjdBIQ = PlotCalSpectrumFreq(x1, cal_bins, capture_bins);

  // Finish up: AFP 2-11-23


  tft.setFontScale((enum RA8875tsize)0);
  tft.fillRect(650, 115, 50, tft.getFontHeight(), RA8875_BLACK);
  tft.setCursor(650, 115);  // 350, 125
  adjdB2 = adjdBIQ;
  aveAdjdB2 = 0.9 * aveAdjdB2 + 0.1 * adjdB2;
  //tft.print(aveAdjdB2, 1);
  // MyDelay(30);
  //tft.BTE_move(WATERFALL_LEFT_X, FIRST_WATERFALL_LINE, MAX_WATERFALL_WIDTH, MAX_WATERFALL_ROWS - 2, WATERFALL_LEFT_X, FIRST_WATERFALL_LINE + 1, 1, 2);
  // while (tft.readStatus())
  //  ;
  // tft.BTE_move(WATERFALL_LEFT_X, FIRST_WATERFALL_LINE + 1, MAX_WATERFALL_WIDTH, MAX_WATERFALL_ROWS - 2, WATERFALL_LEFT_X, FIRST_WATERFALL_LINE + 1, 2);
  // while (tft.readStatus())
  //  ;  // Make sure it's done.
  return adjdBIQ;
}

/*****
  Purpose:  Plot Frequency Calibration Spectrum   //   7/2/2023
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

float PlotCalSpectrumFreq(int x1, int cal_bins[2], int capture_bins) {
  adjdB = 0.0;
  int16_t adjAmplitude = 0;                  // Was float; cast to float in dB calculation.
  int16_t refAmplitude = 0;                  // Was float; cast to float in dB calculation.
  uint32_t index_of_max;                     // This variable is not currently used, but it is required by the ARM max function.
  if (x1 == (cal_bins[0] - capture_bins)) {  // Set flag at revised beginning.
    updateDisplayFlag = 1;                   //Set flag so the display data are saved only once during each display refresh cycle at the start of the cycle, not 512 times
  } else updateDisplayFlag = 0;              //  Do not save the the display data for the remainder of the

  //-------------------------------------------------------
  // This block of code, which calculates the latest FFT and finds the maxima of the tone
  // and its image product, does not technically need to be run every time we plot a pixel
  // on the screen. However, according to the comments below this is needed to eliminate
  // conflicts.
  ProcessIQDataFreq();
  //ProcessIQData();
  // Call the Audio process from within the display routine to eliminate conflicts with drawing the spectrum and waterfall displays
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
  tft.setCursor(cal_bins[1] - capture_bins - 75, 164);  // 350, 125
  tft.print("IQ Image");
  tft.setCursor(cal_bins[0] - capture_bins - 88, 164);  // 350, 125
  tft.print("Ref Level");
  tft.drawFastHLine(cal_bins[0] - capture_bins - 15, 174, 15, RA8875_GREEN);
  tft.drawFastHLine(cal_bins[0] - capture_bins + 22, 174, 15, RA8875_GREEN);
  tft.fillRect(cal_bins[0] - capture_bins, SPECTRUM_TOP_Y + 20, 2 * capture_bins, h - 6, RA8875_BLUE);  // SPECTRUM_TOP_Y = 100

  tft.fillRect(cal_bins[1] - capture_bins, SPECTRUM_TOP_Y + 20, 2 * capture_bins, h - 6, DARK_RED);  // h = SPECTRUM_HEIGHT + 3
  tft.writeTo(L1);
  return adjdB;
}
