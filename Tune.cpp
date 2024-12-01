#ifndef BEENHERE
#include "SDT.h"
#endif
// =============== AFP 09-04-23  All fot Tune.cpp file updated for V012

int EvenDivisor(long freq2) {
  // next 6 ifs added by DRM VK3KQT for use by a phase method of time delay described by
  // https://tj-lab.org/2020/08/27/si5351単体で3mhz以下の直交信号を出力する/
  // for below 3.2MHz the ~limit of PLLA @ 400MHz for a 126 divider
  if (freq2 < 100000)
      multiple = 8192;

  if ((freq2 >= 100000) && (freq2 < 200000))   // PLLA 409.6 MHz to 819.2 MHz
     multiple = 4096;
  
  if ((freq2 >= 200000) && (freq2 < 400000))   //   ""          "" 
     multiple = 2048;

  if ((freq2 >= 400000) && (freq2 < 800000))   //    ""          ""
     multiple = 1024;

  if ((freq2 >= 800000) && (freq2 < 1600000))   //    ""         ""
     multiple = 512;

  if ((freq2 >= 1600000) && (freq2 < 3200000))   //    ""        ""
     multiple = 256;
   //==================================================================  
  // the original multiple
  // if (freq2 < 6850000)
  if ((freq2 >= 3200000) && (freq2 < 6850000))   // 403.2 MHz - 863.1 MHz
    multiple = 126;

  if ((freq2 >= 6850000) && (freq2 < 9500000))
    multiple = 88;

  if ((freq2 >= 9500000) && (freq2 < 13600000))
    multiple = 64;

  if ((freq2 >= 13600000) && (freq2 < 17500000))
    multiple = 44;

  if ((freq2 >= 17500000) && (freq2 < 25000000))
    multiple = 34;

  if ((freq2 >= 25000000) && (freq2 < 36000000))
    multiple = 24;

  if ((freq2 >= 36000000) && (freq2 < 45000000))
    multiple = 18;

  if ((freq2 >= 45000000) && (freq2 < 60000000))
    multiple = 14;

  if ((freq2 >= 60000000) && (freq2 < 80000000))
    multiple = 10;

  if ((freq2 >= 80000000) && (freq2 < 100000000))
    multiple = 8;

  if ((freq2 >= 100000000) && (freq2 < 150000000)) // G0ORX changed upper limit
    multiple = 6;

  if ((freq2 >= 150000000) && (freq2 < 220000000))
    multiple = 4;

  // ? G0ORX - for higher bands
  if(freq2>=220000000) {
    multiple = 2;
  }
  return multiple;
}

#ifndef V12HWR
/*****
  Purpose: A special variant of SetFreq() used only for calibration.

  Parameter list:
    void

  Return value;
    void

  CAUTION: SI5351_FREQ_MULT is set in the si5253.h header file and is 100UL
*****/
void SetFreqCal(void) {   
  // V12 hardware using Si5351 quadrature clocks.
  Clk1SetFreq = ((centerFreq * SI5351_FREQ_MULT) + IFFreq * SI5351_FREQ_MULT);
  multiple = EvenDivisor(Clk1SetFreq / SI5351_FREQ_MULT);
  pll_freq = freq * multiple;
  pll_freq = Clk1SetFreq * multiple;
  freq = pll_freq / multiple;
  si5351.output_enable(SI5351_CLK0, 1);
  si5351.output_enable(SI5351_CLK1, 1);
  //si5351.output_enable(SI5351_CLK2, 0);
  si5351.set_freq_manual(freq, pll_freq, SI5351_CLK0);
  si5351.set_freq_manual(freq, pll_freq, SI5351_CLK1);
  // freqCal = (centerFreq + 21000) * SI5351_FREQ_MULT;
  // si5351.output_enable(SI5351_CLK2, 1);
  //si5351.set_freq_manual(Clk1SetFreq, pll_freq, SI5351_CLK2);
  si5351.set_phase(SI5351_CLK0, 0);
  //si5351.set_freq_manual(0, pll_freq, SI5351_CLK2);
  si5351.set_phase(SI5351_CLK0, 0);
  si5351.set_phase(SI5351_CLK1, multiple);
  if (multiple != oldMultiple) {
    si5351.pll_reset(SI5351_PLLA);
    si5351.pll_reset(SI5351_PLLB);
    //Serial.println("Reset in SetCalFreq");      // does it get here?
    //if(calOnFlag == 1){
    Clk1SetFreq = ((centerFreq * SI5351_FREQ_MULT) + IFFreq * SI5351_FREQ_MULT);
    multiple = EvenDivisor(Clk1SetFreq / SI5351_FREQ_MULT);
    long pll_freq = Clk1SetFreq * multiple;
   //    freqCal = (centerFreq + 21000) * SI5351_FREQ_MULT;
    si5351.output_enable(SI5351_CLK2, 1);
    si5351.set_freq_manual((centerFreq + 21000) * SI5351_FREQ_MULT, pll_freq, SI5351_CLK2);
    // }
  }
  oldMultiple = multiple;
  si5351.output_enable(SI5351_CLK2, 0);
  DrawFrequencyBarValue();

 
  if (radioState == SSB_TRANSMIT_STATE) {
    Clk1SetFreq = (TxRxFreq * SI5351_FREQ_MULT) * MASTER_CLK_MULT;  // AFP 09-27-22
  } else if (radioState == CW_TRANSMIT_STRAIGHT_STATE || radioState == CW_TRANSMIT_KEYER_STATE) {
    if (bands[currentBand].mode == DEMOD_LSB) {
      Clk1SetFreq = (((TxRxFreq + CWFreqShift + calFreqShift) * SI5351_FREQ_MULT)) * MASTER_CLK_MULT;  // AFP 09-27-22;   flip CWFreqShift, sign originally minus
    } else {
      if (bands[currentBand].mode == DEMOD_USB) {
        Clk1SetFreq = (((TxRxFreq - CWFreqShift - calFreqShift) * SI5351_FREQ_MULT)) * MASTER_CLK_MULT;  // AFP 10-01-22;  flip CWFreqShift, sign originally plus
      }
    }
  }

  //  The receive LO frequency is not dependent on mode or sideband.  CW frequency shift is done in DSP code.
 //Clk2SetFreq = ((centerFreq * SI5351_FREQ_MULT) + IFFreq * SI5351_FREQ_MULT) * MASTER_CLK_MULT;

  if (radioState == SSB_RECEIVE_STATE || radioState == CW_RECEIVE_STATE) {   //  Receive state
    si5351.set_freq(Clk2SetFreq, SI5351_CLK2);
    si5351.output_enable(SI5351_CLK0, 1);
    si5351.output_enable(SI5351_CLK1, 1);  // CLK2 (transmit) off during receive to prevent birdies
    //si5351.output_enable(SI5351_CLK2, 0);
  }

  if (radioState == SSB_TRANSMIT_STATE || radioState == CW_TRANSMIT_STRAIGHT_STATE || radioState == CW_TRANSMIT_KEYER_STATE) {  // Transmit state
    si5351.set_freq(Clk1SetFreq, SI5351_CLK1);
    //si5351.output_enable(SI5351_CLK2, 0);  // CLK2 (receive) off during transmit to prevent spurious outputs
    si5351.output_enable(SI5351_CLK1, 1);
  }
  //=====================  AFP 10-03-22 =================
  DrawFrequencyBarValue();
}
#endif

/***** //AFP 10-11-22 all new
  Purpose:  Reset tuning to center

  Parameter list:
  void

  Return value;
  void
*****/
void ResetTuning() {

  //centerFreq = TxRxFreq;

  currentFreq = centerFreq + NCOFreq;  // Changed currentFreqA to currentFreq.  KF5N August 5, 2023
  NCOFreq = 0L;
  centerFreq = TxRxFreq = currentFreq;  // Changed currentFreqA to currentFreq.  KF5N August 5, 2023
  tft.writeTo(L2);                      // Clear layer 2.  KF5N July 31, 2023
  tft.clearMemory();
  SetFreq();  // For new tuning scheme.  KF5N July 22, 2023
  DrawBandWidthIndicatorBar();
  BandInformation();
  ShowFrequency();
  //centerTuneFlag = 1;  Not necessary with new tuning scheme.  KF5N July 22, 2023
}
// ===== End AFP 10-11-22

/*****
  Purpose: SetFrequency

  Parameter list:
    void

  Return value;
    void

  CAUTION: SI5351_FREQ_MULT is set in the si5253.h header file and is 100UL
*****/

void SetFreq() {   // reworked VK3KQT
  #if defined(G0ORX_FRONTPANEL)
  __disable_irq();
  #endif // G0ORX_FRONTPANEL
  #ifdef V12HWR   // V12 hardware Si5351 quadrature clocks 0, 1
  long long f=centerFreq;
  Clk1SetFreq = ((f * SI5351_FREQ_MULT) + IFFreq * SI5351_FREQ_MULT);
  multiple = EvenDivisor(Clk1SetFreq / SI5351_FREQ_MULT);
  pll_freq = Clk1SetFreq * multiple;
  freq = pll_freq / multiple;     // is this equal to Clk1SetFreq?
  
  if ( multiple == oldMultiple) {               // Still within the same multiple range 
      si5351.set_pll(pll_freq, SI5351_PLLA);    // just change PLLA on each frequency change of encoder
                                                // this minimizes I2C data for each frequency change within a 
                                                // multiple range
  }
  else { 
    if ( multiple <= 126) {                                 // this the library setting of phase for freqs
        si5351.set_freq_manual(freq, pll_freq, SI5351_CLK0);  // greater than 3.2MHz where multiple is <= 126
        si5351.set_freq_manual(freq, pll_freq, SI5351_CLK1);   // set both clocks to new frequency
        si5351.set_phase(SI5351_CLK0, 0);                      // CLK0 phase = 0 
        si5351.set_phase(SI5351_CLK1, multiple);               // Clk1 phase = multiple for 90 degrees(digital delay)
        si5351.pll_reset(SI5351_PLLA);                         // reset PLLA to align outputs 
        si5351.output_enable(SI5351_CLK0, 1);                  // set outputs on or off
        si5351.output_enable(SI5351_CLK1, 1);
        //si5351.output_enable(SI5351_CLK2, 0);
    }
    else {        // this is the timed delay technique for frequencies below 3.2MHz as detailed in 
                  // https://tj-lab.org/2020/08/27/si5351単体で3mhz以下の直交信号を出力する/
        cli();                //__disable_irq(); or __enable_irq();     // or cli()/sei() pair; needed to get accurate timing??
        //si5351.output_enable(SI5351_CLK0, 0);  // optional switch off clocks if audio effects are generated
        //si5351.output_enable(SI5351_CLK1, 0);  //  with the change of multiple below 3.2MHz
        si5351.set_freq_manual((freq - 400ULL), pll_freq, SI5351_CLK0);  // set up frequencies of CLK 0/1 4 Hz low
        si5351.set_freq_manual((freq - 400ULL), pll_freq, SI5351_CLK1);  // as per TJ-Labs article 
        si5351.set_phase(SI5351_CLK0, 0);                          // set phase registers to 0 just to be sure
        si5351.set_phase(SI5351_CLK1, 0);                          
        si5351.pll_reset(SI5351_PLLA);                             // align both clockss in phase
        si5351.set_freq_manual(freq, pll_freq, SI5351_CLK0);       // set clock 0  to required freq
        //delayNanoseconds(625000000);       // 62.5 * 1000000      //configured for a 62.5 mSec delay at 4 Hz difference 
        delayMicroseconds(58500);                       //nominally 62500 this figure can be adjusted for a more exact delay which is phase
        si5351.set_freq_manual(freq, pll_freq, SI5351_CLK1);       // set CLK 1 to the required freq after delay
        sei();
        si5351.output_enable(SI5351_CLK0, 1);                      // switch them on to be sure
        si5351.output_enable(SI5351_CLK1, 1);                      //    ""        ""
        //si5351.output_enable(SI5351_CLK2, 0);
        
      }
  }
  oldMultiple = multiple;
  DrawFrequencyBarValue();
  #else                 // Version V11 and earlier

   // NEVER USE AUDIONOINTERRUPTS HERE: that introduces annoying clicking noise with every frequency change
  // SI5351_FREQ_MULT is 100ULL, MASTER_CLK_MULT is 4;

  // The SSB LO frequency is always the same as the displayed transmit frequency.
  // The CW LOT frequency must be shifted by 750 Hz due to the way the CW carrier is generated by a quadrature tone.
  if (radioState == SSB_TRANSMIT_STATE) {
    Clk1SetFreq = (TxRxFreq * SI5351_FREQ_MULT) * MASTER_CLK_MULT;  // AFP 09-27-22
  } else if (radioState == CW_TRANSMIT_STRAIGHT_STATE || radioState == CW_TRANSMIT_KEYER_STATE) {
    if (bands[currentBand].mode == DEMOD_LSB) {
      Clk1SetFreq = (((TxRxFreq + CWFreqShift + calFreqShift) * SI5351_FREQ_MULT)) * MASTER_CLK_MULT;  // AFP 09-27-22;  KF5N flip CWFreqShift, sign originally minus
    } else {
      if (bands[currentBand].mode == DEMOD_USB) {
        Clk1SetFreq = (((TxRxFreq - CWFreqShift - calFreqShift) * SI5351_FREQ_MULT)) * MASTER_CLK_MULT;  // AFP 10-01-22; KF5N flip CWFreqShift, sign originally plus
      }
    }
  }

  //  The receive LO frequency is not dependent on mode or sideband.  CW frequency shift is done in DSP code.
  Clk2SetFreq = ((centerFreq * SI5351_FREQ_MULT) + IFFreq * SI5351_FREQ_MULT) * MASTER_CLK_MULT;

  if (radioState == SSB_RECEIVE_STATE || radioState == CW_RECEIVE_STATE) {   //  Receive state
    si5351.set_freq(Clk2SetFreq, SI5351_CLK2);
    si5351.output_enable(SI5351_CLK0, 1);
    si5351.output_enable(SI5351_CLK1, 1);  // CLK1 (transmit) off during receive to prevent birdies
    si5351.output_enable(SI5351_CLK2, 0);
  }

  if (radioState == SSB_TRANSMIT_STATE || radioState == CW_TRANSMIT_STRAIGHT_STATE || radioState == CW_TRANSMIT_KEYER_STATE) {  // Transmit state
    si5351.set_freq(Clk1SetFreq, SI5351_CLK1);
    si5351.output_enable(SI5351_CLK2, 0);  // CLK2 (receive) off during transmit to prevent spurious outputs
    si5351.output_enable(SI5351_CLK1, 1);
  }
  #endif

  #if defined(G0ORX_FRONTPANEL)
  __enable_irq();
  #endif // G0ORX_FRONTPANEL
  //=====================  AFP 10-03-22 =================
  DrawFrequencyBarValue();
}

/*****
  Purpose: Places the Fast Tune cursor in the center of the spectrum display

  Parameter list:

  Return value;
    void
*****/
void CenterFastTune() {
  tft.drawFastVLine(oldCursorPosition, SPECTRUM_TOP_Y + 20, SPECTRUM_HEIGHT - 27, RA8875_BLACK);
  tft.drawFastVLine(newCursorPosition, SPECTRUM_TOP_Y + 20, SPECTRUM_HEIGHT - 27, RA8875_RED);
}

/*****
  Purpose: Purpose is to sety VFOa to receive frequency and VFOb to the transmit frequency

  Parameter list:
    void

  Return value;
    int           the offset as an int

  CAUTION: SI5351_FREQ_MULT is set in the si5253.h header file and is 100UL
*****/
int DoSplitVFO() {
  char freqBuffer[15];
  int val;
  long chunk = SPLIT_INCREMENT;
  long splitOffset;

  tft.drawRect(INFORMATION_WINDOW_X - 10, INFORMATION_WINDOW_Y - 2, 260, 200, RA8875_MAGENTA);
  tft.fillRect(INFORMATION_WINDOW_X - 8, INFORMATION_WINDOW_Y, 250, 185, RA8875_BLACK);  // Clear volume field
  tft.setFontScale((enum RA8875tsize)1);
  tft.setCursor(INFORMATION_WINDOW_X + 10, INFORMATION_WINDOW_Y + 5);
  tft.print("xmit offset: ");

  splitOffset = chunk;  // Set starting offset to 500Hz
  tft.setTextColor(RA8875_GREEN);
  tft.setCursor(INFORMATION_WINDOW_X + 60, INFORMATION_WINDOW_Y + 90);
  tft.print(splitOffset);
  tft.print("Hz  ");

  while (true) {
    if (filterEncoderMove != 0) {  // Changed encoder?
      splitOffset += filterEncoderMove * chunk;
      tft.fillRect(INFORMATION_WINDOW_X + 60, INFORMATION_WINDOW_Y + 90, 150, 50, RA8875_BLACK);
      tft.setCursor(INFORMATION_WINDOW_X + 60, INFORMATION_WINDOW_Y + 90);
      tft.print(splitOffset);
      tft.print("Hz  ");
    }
    filterEncoderMove = 0L;

    val = ReadSelectedPushButton();  // Read pin that controls all switches
    val = ProcessButtonPress(val);
    MyDelay(150L);
    if (val == MENU_OPTION_SELECT) {  // Make a choice??
      Clk1SetFreq += splitOffset;     // New transmit frequency // AFP 09-27-22
      UpdateInfoWindow();
      filterEncoderMove = 0L;
      break;
    }
  }
  currentFreqB = currentFreqA + splitOffset;
  FormatFrequency(currentFreqB, freqBuffer);
  tft.fillRect(FREQUENCY_X_SPLIT, FREQUENCY_Y - 12, VFOB_PIXEL_LENGTH, FREQUENCY_PIXEL_HI, RA8875_BLACK);
  tft.setCursor(FREQUENCY_X_SPLIT, FREQUENCY_Y);
  tft.setFont(&FreeMonoBold24pt7b);
  tft.setTextColor(RA8875_GREEN);
  tft.print(freqBuffer);  // Show VFO_A

  tft.setFont(&FreeMonoBold18pt7b);
  FormatFrequency(currentFreqA, freqBuffer);
  tft.setTextColor(RA8875_LIGHT_GREY);
  tft.setCursor(FREQUENCY_X, FREQUENCY_Y + 6);
  tft.print(freqBuffer);  // Show VFO_A

  tft.useLayers(1);  //mainly used to turn on layers!
  tft.layerEffect(OR);
  tft.writeTo(L2);
  tft.clearMemory();
  tft.writeTo(L1);

  tft.setFont(&FreeMono9pt7b);
  tft.setTextColor(RA8875_RED);
  tft.setCursor(FILTER_PARAMETERS_X + 180, FILTER_PARAMETERS_Y + 6);
  tft.print("Split Active");

  tft.setFontDefault();
  return (int)splitOffset;  // Can be +/-
}
