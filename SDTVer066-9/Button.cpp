
#ifndef BEENHERE
#include "SDT.h"
#endif


/*****
  Purpose: Determine which UI button was pressed

  Parameter list:
    int valPin            the ADC value from analogRead()

  Return value;
    int                   -1 if not valid push button, index of push button if valid
*****/
int ProcessButtonPress(int valPin) {

  return valPin;
}

/*****
  Purpose: Check for UI button press. If pressed, return the ADC value

  Parameter list:
    int vsl               the value from analogRead in loop()\

  Return value;
    int                   -1 if not valid push button, ADC value if valid
*****/
int ReadSelectedPushButton() {

  __disable_irq();
  int i = ButtonPressed;
  ButtonPressed = BOGUS_PIN_READ;
  __enable_irq();
  return i;
}

/*****
  Purpose: Function is designed to route program control to the proper execution point in response to
           a button press.

  Parameter list:
    int vsl               the value from analogRead in loop()

  Return value;
    void
*****/
void ExecuteButtonPress(int val) 
{

  switch (val) {
    case MENU_OPTION_SELECT:        // 0
      break;

    case MAIN_MENU_UP:                      // 1          11/16/23 JJP                                     Button 1
      DrawMenuDisplay();                    // Draw selection box and primary menu
      SetPrimaryMenuIndex();                // Scroll through primary indexes and select one
      if (mainMenuWindowActive == false) {  // Was Main Menu choice cancelled?}
        mainMenuWindowActive = false;
        EraseMenus();
        RedrawDisplayScreen();
        ShowFrequency();
        DrawFrequencyBarValue();
        break;
      }
      SetSecondaryMenuIndex();  // Use the primary index selection to redraw the secondary menu and set its index
      secondaryMenuChoiceMade = functionPtr[mainMenuIndex]();
      tft.fillRect(1, SPECTRUM_TOP_Y + 1, 513, 379, RA8875_BLACK);  // Erase Menu box

      EraseMenus();
      RedrawDisplayScreen();
      ShowFrequency();
      DrawFrequencyBarValue();

      break;

    case BAND_UP:                   // 2         Now calls ProcessIQData and Encoders calls                    Button 2
      if (currentBand < BAND_12M) digitalWrite(bandswitchPins[currentBand], LOW);  // Added if so unused GPOs will not be touched.  KF5N October 16, 2023.
      ButtonBandIncrease();
      if (currentBand < BAND_12M) digitalWrite(bandswitchPins[currentBand], HIGH);
      BandInformation();
      NCOFreq = 0L;
      DrawBandWidthIndicatorBar();  // AFP 10-20-22
      SetFreq();
      ShowSpectrum();
      break;

    case ZOOM:  // 3
      menuStatus = PRIMARY_MENU_ACTIVE;
      //      EraseMenus();
      ButtonZoom();
      break;

    //case MAIN_MENU_DN:            // 4
    case RESET_TUNING:
      ResetTuning();
      break;

    case BAND_DN:                   // 5
      ShowSpectrum();  //Now calls ProcessIQData and Encoders calls
      if (currentBand < BAND_12M) digitalWrite(bandswitchPins[currentBand], LOW);
      ButtonBandDecrease();
      if (currentBand < BAND_12M) digitalWrite(bandswitchPins[currentBand], HIGH);
      BandInformation();
      NCOFreq = 0L;
      DrawBandWidthIndicatorBar();  //AFP 10-20-22
      UpdateVolumeField();
      break;

    case SET_MODE:                  // 6
      ButtonMode();
      ShowSpectrumdBScale();
      break;

    case DEMODULATION:              // 7
      EraseMenus();
      ButtonDemodMode();
      break;

    case MAIN_TUNE_INCREMENT:       // 8
      ButtonFreqIncrement();
      break;

    case NOISE_REDUCTION:           // 9
      ButtonNR();
      break;

    case NOTCH_FILTER:              // 10
      ButtonNotchFilter();
      UpdateNotchField();
      break;

    case FINE_TUNE_INCREMENT:       // 11
      UpdateIncrementField();
      break;

    case FILTER:                    // 12
      ButtonFilter();
      break;

    case DECODER_TOGGLE:            // 13
      decoderFlag = !decoderFlag;
      UpdateDecoderField();
      break;

    case BEARING:                   // 17
      break;

    case DDE:  // 16
      if (calOnFlag == 0) {
        ButtonFrequencyEntry();
      }
      break;

    case 18:                        // 18 - Encoder 1 SW (Volume)
      switch (volumeFunction) {
        case AUDIO_VOLUME:
          volumeFunction = AGC_GAIN;
          break;
        case AGC_GAIN:
          volumeFunction = MIC_GAIN;
          break;
        case MIC_GAIN:
          volumeFunction = SIDETONE_VOLUME;
          break;
        case SIDETONE_VOLUME:
          volumeFunction = NOISE_FLOOR_LEVEL;
          break;
        case NOISE_FLOOR_LEVEL:
          volumeFunction = AUDIO_VOLUME;
          break;
          
      }
      volumeChangeFlag = true;
      break;

    case 19:                        // 19 - Encoder 2 SW (Filter/Menu)
      AGCMode++;
      if (AGCMode > 4) {
        AGCMode = 0;
      }
      AGCLoadValues();
      EEPROMData.AGCMode = AGCMode;
      EEPROM.put(EEPROM_BASE_ADDRESS, EEPROMData);
      UpdateAGCField();
      break;

    case 20:                        // 20 - Encoder 3 SW (Center Tune)
      if (activeVFO == VFO_A) {
        centerFreq = TxRxFreq = currentFreqB;
        activeVFO = VFO_B;
        currentBand = currentBandB;
        tft.fillRect(FILTER_PARAMETERS_X + 180, FILTER_PARAMETERS_Y, 150, 20, RA8875_BLACK);  // Erase split message
        splitOn = 0;
      } else {  // Must be VFO-B
        centerFreq = TxRxFreq = currentFreqA;
        activeVFO = VFO_A;
        currentBand = currentBandA;
        tft.fillRect(FILTER_PARAMETERS_X + 180, FILTER_PARAMETERS_Y, 150, 20, RA8875_BLACK);  // Erase split message
        splitOn = 0;
      }
      bands[currentBand].freq = TxRxFreq;
      SetBand();           // KF5N July 12, 2023
      SetBandRelay(HIGH);  // Required when switching VFOs. KF5N July 12, 2023
      SetFreq();
      RedrawDisplayScreen();
      BandInformation();
      ShowBandwidth();
      FilterBandwidth();
      EEPROMData.activeVFO = activeVFO;
      tft.fillRect(FREQUENCY_X_SPLIT, FREQUENCY_Y - 12, VFOB_PIXEL_LENGTH, FREQUENCY_PIXEL_HI, RA8875_BLACK);  // delete old digit
      tft.fillRect(FREQUENCY_X, FREQUENCY_Y - 12, VFOA_PIXEL_LENGTH, FREQUENCY_PIXEL_HI, RA8875_BLACK);        // delete old digit  tft.setFontScale( (enum RA8875tsize) 0);
      ShowFrequency();

      tft.writeTo(L2);
      tft.clearMemory();
      if (xmtMode == CW_MODE) BandInformation();
      DrawBandWidthIndicatorBar();
      DrawFrequencyBarValue();
      break;

    case 21:                        // 21 - Encoder 4 SW (Fine Tune)
      {
        long tempFreq = currentFreqA;
        int tempBand = currentBandA;
        currentFreqA = currentFreqB;
        currentBandA = currentBandB;
        currentFreqB = tempFreq;
        currentBandB = tempBand;
        if (activeVFO == VFO_A) {
          centerFreq = TxRxFreq = currentFreqA;
          currentBand = currentBandA;
        } else {  // must be VFO-B
          centerFreq = TxRxFreq = currentFreqB;
          currentBand = currentBandB;
        }
        bands[currentBand].freq = TxRxFreq;
        SetBand();           // KF5N July 12, 2023
        SetBandRelay(HIGH);  // Required when switching VFOs. KF5N July 12, 2023
        SetFreq();
        RedrawDisplayScreen();
        BandInformation();
        ShowBandwidth();
        FilterBandwidth();
        EEPROMData.activeVFO = activeVFO;
        tft.fillRect(FREQUENCY_X_SPLIT, FREQUENCY_Y - 12, VFOB_PIXEL_LENGTH, FREQUENCY_PIXEL_HI, RA8875_BLACK);  // delete old digit
        tft.fillRect(FREQUENCY_X, FREQUENCY_Y - 12, VFOA_PIXEL_LENGTH, FREQUENCY_PIXEL_HI, RA8875_BLACK);        // delete old digit  tft.setFontScale( (enum RA8875tsize) 0);
        ShowFrequency();
        // Draw or not draw CW filter graphics to audio spectrum area.  KF5N July 30, 2023
        tft.writeTo(L2);
        tft.clearMemory();
        if (xmtMode == CW_MODE) BandInformation();
        DrawBandWidthIndicatorBar();
        DrawFrequencyBarValue();
      }
      break;

    default:
      break;
  }
}


/*****
  Purpose: To process a band decrease button push

  Parameter list:
    void

  Return value:
    void
*****/
void ButtonFreqIncrement() {
  tuneIndex--;
  if (tuneIndex < 0)
    tuneIndex = MAX_FREQ_INDEX - 1;
  freqIncrement = incrementValues[tuneIndex];
  DisplayIncrementField();
}


/*****
  Purpose: Error message if Select button pressed with no Menu active

  Parameter list:
    void

  Return value;
    void
*****/
void NoActiveMenu() {
  tft.setFontScale((enum RA8875tsize)1);
  tft.setTextColor(RA8875_RED);
  tft.setCursor(10, 0);
  tft.print("No menu selected");

  menuStatus = NO_MENUS_ACTIVE;
  mainMenuIndex = 0;
  secondaryMenuIndex = 0;
}
