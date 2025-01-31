#ifndef BEENHERE
#include "SDT.h"
#endif

char atom, currentAtom;
int stateMachine;
bool FFTupdated;
  int16_t adjAmplitude = 0;  // Was float; cast to float in dB calculation.
  int16_t refAmplitude = 0;
  uint32_t index_of_max = 0;
/*****
  Purpose: Read audio from Teensy Audio Library
             Calculate FFT for display
             Process audio into SSB signalF
             Output audio to amplifier

   Parameter List:
      void

   Return value:
      void

   CAUTION: Assumes a spaces[] array is defined
 *****/
void ProcessIQData() {
  if (radioState == CW_TRANSMIT_STRAIGHT_STATE || radioState == CW_TRANSMIT_KEYER_STATE) {  //AFP 09-01-22
    return;
  }
  /**********************************************************************************  AFP 12-31-20
        Get samples from queue buffers
        Teensy Audio Library stores ADC data in two buffers size=128, Q_in_L and Q_in_R as initiated from the audio lib.
        Then the buffers are read into two arrays sp_L and sp_R in blocks of 128 up to N_BLOCKS.  The arrarys are
        of size BUFFER_SIZE * N_BLOCKS.  BUFFER_SIZE is 128.
        N_BLOCKS = FFT_LENGTH / 2 / BUFFER_SIZE * (uint32_t)DF; // should be 16 with DF == 8 and FFT_LENGTH = 512
        BUFFER_SIZE*N_BLOCKS = 2024 samples
     **********************************************************************************/
  float32_t audioMaxSquared;
  uint32_t AudioMaxIndex;
  float rfGainValue;
  int cal_bins[2] = { 0, 0 };
  int capture_bins;


  int val = 0;
  int task = 0;
  //int recCalDirections = 0;
  // are there at least N_BLOCKS buffers in each channel available ?
  if ((uint32_t)Q_in_L.available() > N_BLOCKS + 0 && (uint32_t)Q_in_R.available() > N_BLOCKS + 0) {
    usec = 0;
    // get audio samples from the audio  buffers and convert them to float
    // read in 32 blocks á 128 samples in I and Q
    for (unsigned i = 0; i < N_BLOCKS; i++) {
      sp_L1 = Q_in_R.readBuffer();
      sp_R1 = Q_in_L.readBuffer();


      /**********************************************************************************  AFP 12-31-20
          Using arm_Math library, convert to float one buffer_size.
          Float_buffer samples are now standardized from > -1.0 to < 1.0
      **********************************************************************************/
      arm_q15_to_float(sp_L1, &float_buffer_L[BUFFER_SIZE * i], BUFFER_SIZE);  // convert int_buffer to float 32bit
      arm_q15_to_float(sp_R1, &float_buffer_R[BUFFER_SIZE * i], BUFFER_SIZE);  // convert int_buffer to float 32bit
      Q_in_L.freeBuffer();
      Q_in_R.freeBuffer();
    }
    if (radioState == CW_TRANSMIT_STRAIGHT_STATE || radioState == CW_TRANSMIT_KEYER_STATE) {  //AFP 09-01-22
      return;
    }
    // Set frequency here only to minimize interruption to signal stream during tuning
    // This code was unnecessary in the revised tuning scheme.  KF5N July 22, 2023
    if (centerTuneFlag == 1) {  //AFP 10-04-22
      DrawBandWidthIndicatorBar();
      ShowFrequency();

      SetFreq();

      //    SetFreq();            //AFP 10-04-22
      // BandInformation();


    }                    //AFP 10-04-22
    centerTuneFlag = 0;  //AFP 10-04-22
    if (resetTuningFlag == 1) {
      ResetTuning();
    }
    resetTuningFlag = 0;
//=================== AFP 01-25-25 IQ Test Signals
#ifdef IQ_REC_TEST
    arm_scale_f32(sinBuffer2K, -.0002, float_buffer_L, BUFFER_SIZE * N_BLOCKS);  //AFP 01-25-25
    arm_scale_f32(cosBuffer2K, .0002, float_buffer_R, BUFFER_SIZE * N_BLOCKS);
#endif
    //=====================


    /*******************************
            Set RFGain - for all bands
    */
    rfGainValue = pow(10, (float)rfGainAllBands / 20);
    arm_scale_f32(float_buffer_L, rfGainValue, float_buffer_L, BUFFER_SIZE * N_BLOCKS);  //AFP 09-27-22
    arm_scale_f32(float_buffer_R, rfGainValue, float_buffer_R, BUFFER_SIZE * N_BLOCKS);  //AFP 09-27-22
    /**********************************************************************************  AFP 12-31-20
        Remove DC offset to reduce centeral spike.  First read the Mean value of
        left and right channels.  Then fill L and R correction arrays with those Means
        and subtract the Means from the float L and R buffer data arrays.  Again use Arm_Math functions
        to manipulate the arrays.  Arrays are all BUFFER_SIZE * N_BLOCKS long
    **********************************************************************************/
    //
    //================

    /*arm_mean_f32(float_buffer_L, BUFFER_SIZE * N_BLOCKS, &sample_meanL);
    arm_mean_f32(float_buffer_R, BUFFER_SIZE * N_BLOCKS, &sample_meanR);

    for (uint32_t j = 0; j < BUFFER_SIZE * N_BLOCKS  ; j++) {
      L_BufferOffset [j] = -sample_meanL;
      R_BufferOffset [j] = -sample_meanR;
    }
    arm_add_f32(float_buffer_L , L_BufferOffset, float_buffer_L2 , BUFFER_SIZE * N_BLOCKS ) ;
    arm_add_f32(float_buffer_R , R_BufferOffset, float_buffer_R2 , BUFFER_SIZE * N_BLOCKS ) ;

    arm_biquad_cascade_df2T_f32(&s1_Receive, float_buffer_L, float_buffer_L, 2048); //AFP 09-23-22
    arm_biquad_cascade_df2T_f32(&s1_Receive, float_buffer_R, float_buffer_R, 2048); //AFP 09-23-22*/
    //arm_biquad_cascade_df2T_f32(&s1_Receive2, float_buffer_L, float_buffer_L, 2048); //AFP 11-03-22
    //arm_biquad_cascade_df2T_f32(&s1_Receive2, float_buffer_R, float_buffer_R, 2048); //AFP 11-03-22


    //===========================

    /**********************************************************************************  AFP 12-31-20
        Scale the data buffers by the RFgain value defined in bands[currentBand] structure
    **********************************************************************************/
    arm_scale_f32(float_buffer_L, bands[currentBand].RFgain, float_buffer_L, BUFFER_SIZE * N_BLOCKS);  //AFP 09-23-22
    arm_scale_f32(float_buffer_R, bands[currentBand].RFgain, float_buffer_R, BUFFER_SIZE * N_BLOCKS);  //AFP 09-23-22

    /**********************************************************************************  AFP 12-31-20
      Clear Buffers
      This is to prevent overfilled queue buffers during each switching event
      (band change, mode change, frequency change, the audio chain runs and fills the buffers
      if the buffers are full, the Teensy needs much more time
      in that case, we clear the buffers to keep the whole audio chain running smoothly
      **********************************************************************************/
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
    /**********************************************************************************  AFP 12-31-20
      IQ amplitude and phase correction.  For this scaled down version the I an Q chnnels are
      equalized and phase corrected manually. This is done by applying a correction, which is the difference, to
      the L channel only.  The phase is corrected in the IQPhaseCorrection() function.

      IQ amplitude and phase correction
    ***********************************************************************************************/


    if (recCalOnFlag == 1) {
      spectrum_zoom = SPECTRUM_ZOOM_1;
      if (updateCalDisplayFlag == 1) {


        CalibratePreamble(0);
      }
      //tft.fillRect(INFORMATION_WINDOW_X - 8, INFORMATION_WINDOW_Y, 250, 170, RA8875_BLACK);
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
      updateCalDisplayFlag = 0;
      // }
      IQAmpCorrectionFactor[currentBand] += (float)filterEncoderMove * corrChangeIQIncrement;  // Bump up or down...
      filterEncoderMove = 0;
      if (IQAmpCorrectionFactor[currentBand] != IQAmpCorrectionFactorOld || IQPhaseCorrectionFactor[currentBand] != IQPhaseCorrectionFactorOld) {

        //tft.fillRect(INFORMATION_WINDOW_X - 8, INFORMATION_WINDOW_Y, 250, 170, RA8875_BLACK);
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
        // Sets the number of bins to scan for signal peak.
      }
      val = ReadSelectedPushButton();
      if (val != BOGUS_PIN_READ)  // Any button press??
      {
        val = ProcessButtonPress(val);  // Use ladder value to get menu choice
      }

      task = val;
      switch (task) {
        case (CAL_AUTOCAL):  //13 Decode
          {
            // Run through the autocal routine
            Serial.println("in auto cal");
            correctionIncrement = 0.001,
            autotuneRec(&IQAmpCorrectionFactor[currentBand], &IQPhaseCorrectionFactor[currentBand],
                        GAIN_COARSE_MAX, GAIN_COARSE_MIN,
                        PHASE_COARSE_MAX, PHASE_COARSE_MIN,
                        GAIN_COARSE_STEP2_N, PHASE_COARSE_STEP2_N,
                        GAIN_FINE_N, PHASE_FINE_N, false);
            Serial.println("aftr autotune");
            break;
          }

        case (MENU_OPTION_SELECT):
          {
            //tft.fillRect(INFORMATION_WINDOW_X - 8, INFORMATION_WINDOW_Y, 250, 170, RA8875_BLACK);
            // tft.clearScreen(RA8875_BLACK);
            recCalOnFlag = 0;
            ButtonZoom();     // Restore the user's zoom setting.  Note that this function also modifies spectrum_zoom.
            EEPROMWrite();    // Save calibration numbers and configuration.   August 12, 2023
            tft.writeTo(L2);  // Clear layer 2.   July 31, 2023
            tft.clearMemory();
            tft.writeTo(L1);  // Exit function in layer 1.   August 3, 2023     // Restore the user's zoom setting.  Note that this function also modifies spectrum_zoom.
            EEPROMWrite();    // Save calibration numbers and configuration.   August 12, 2023
            tft.writeTo(L2);  // Clear layer 2.   July 31, 2023
            tft.clearMemory();
            tft.writeTo(L1);  // Exit function in layer 1.   August 3, 2023
            tft.clearScreen(RA8875_BLACK);

            RedrawDisplayScreen();
            UpdateInfoWindow();
            si5351.output_enable(SI5351_CLK2, 0);
            // CalibratePost();
            IQChoice = 5;
            return;
            break;
          }


        case (CAL_CHANGE_INC):  //CAL_CHANGE_INC =17 =User3
          {
            recIQIncrementIndex++;
            if (recIQIncrementIndex > 2) recIQIncrementIndex = 0;
            corrChangeIQIncrement = recIQIncrementValues[recIQIncrementIndex];
            break;
          }
        case (CAL_DIRECTIONS):  //Filter
          {

            while (1) {
              tft.setFontScale((enum RA8875tsize)0);
              tft.setTextColor(RA8875_CYAN);
              tft.setCursor(10, 270);
              tft.print("Directions");
              tft.setCursor(25, 285);
              tft.print("* Jumper JP4: Cal Isolation");
              tft.setCursor(25, 300);
               tft.print("Option 1 Manual Adjust");;
              tft.setCursor(25, 315);
             tft.print("* Adjust Gain (Filter Encoder) for min IQ image (Red)");
              tft.setCursor(25, 330);
              tft.print("* Adjust Phase (Vol Encoder) for min IQ image (Red)");
              tft.setCursor(25, 345);
              tft.print("* Alternate between Gain and Phase adjustment");
              tft.setCursor(25, 360);
              tft.print("OPtion 2 - Auto IQ Tune");
              tft.setCursor(25, 375);
             tft.print("* Press Decode - Auto Tune will start");
              tft.setCursor(25, 390);
              tft.print("* User3 to change Incr.");
              tft.setCursor(25, 405);
              tft.print("* Select to Save/Exit");
              //tft.setCursor(25, 420);

              val = ReadSelectedPushButton();
              if (val != BOGUS_PIN_READ) {
                val = ProcessButtonPress(val);

                if (task == CAL_DIRECTIONS) break;
              }
            }

            tft.fillRect(0, 272, 517, 207, RA8875_BLACK);  // Erase waterfall
            break;
          }
      }
      capture_bins = 15;
      cal_bins[0] = 128;
      cal_bins[1] = 384;


      arm_max_q15(&pixelnew[(cal_bins[0] - capture_bins)], capture_bins * 2, &refAmplitude, &index_of_max);
      arm_max_q15(&pixelnew[(cal_bins[1] - capture_bins)], capture_bins * 2, &adjAmplitude, &index_of_max);
      tft.writeTo(L2);
      tft.fillRect(cal_bins[0] - capture_bins, SPECTRUM_TOP_Y + 20, 2 * capture_bins, h - 6, RA8875_BLUE);  // SPECTRUM_TOP_Y = 100

      tft.fillRect(cal_bins[1] - capture_bins, SPECTRUM_TOP_Y + 20, 2 * capture_bins, h - 6, DARK_RED);
      tft.writeTo(L1);
      tft.setFontScale((enum RA8875tsize)1);
      adjdB = ((float)adjAmplitude - (float)refAmplitude) / 1.95;
      tft.fillRect(700, 400, 150, CHAR_HEIGHT, RA8875_BLACK);
      tft.setCursor(700, 400);
      tft.print(adjdB, 1);
      tft.setFontScale((enum RA8875tsize)0);
      tft.fillRect(620, 450, 100, CHAR_HEIGHT, RA8875_BLACK);
      tft.setCursor(620, 450);
      tft.print(corrChangeIQIncrement, 3);

      /* Serial.print("refAmplitude= ");
        Serial.println(refAmplitude);
        Serial.print("adjAmplitude= ");
        Serial.println(adjAmplitude);
               Serial.print("adjdB= ");
        Serial.println(adjdB);*/
    } else {
      recCalOnFlag = 0;
      //UpdateInfoWindow();
    }

    // Manual IQ amplitude and phase correction
    // to be honest: we only correct the amplitude of the I channel ;-)
    if (bands[currentBand].mode == DEMOD_LSB || bands[currentBand].mode == DEMOD_AM || bands[currentBand].mode == DEMOD_SAM) {
      arm_scale_f32(float_buffer_L, -IQAmpCorrectionFactor[currentBand], float_buffer_L, BUFFER_SIZE * N_BLOCKS);  //AFP 04-14-22
      IQPhaseCorrection(float_buffer_L, float_buffer_R, -IQPhaseCorrectionFactor[currentBand], BUFFER_SIZE * N_BLOCKS);
    } else {
      if (bands[currentBand].mode == DEMOD_USB || bands[currentBand].mode == DEMOD_AM || bands[currentBand].mode == DEMOD_SAM) {
        arm_scale_f32(float_buffer_L, -IQAmpCorrectionFactor[currentBand], float_buffer_L, BUFFER_SIZE * N_BLOCKS);  //AFP 04-14-22
        IQPhaseCorrection(float_buffer_L, float_buffer_R, IQPhaseCorrectionFactor[currentBand], BUFFER_SIZE * N_BLOCKS);
      }
    }

    /**********************************************************************************  AFP 12-31-20
        Perform a 256 point FFT for the spectrum display on the basis of the first 256 complex values
        of the raw IQ input data this saves about 3% of processor power compared to calculating
        the magnitudes and means of the 4096 point FFT for the display

        Only go there from here, if magnification == 1
     ***********************************************************************************************/

    if (spectrum_zoom == SPECTRUM_ZOOM_1) {  // && display_S_meter_or_spectrum_state == 1)
      CalcZoom1Magn();
      FFTupdated = true;  //AFP Moved to display function
    }
    display_S_meter_or_spectrum_state++;
    if (radioState == CW_TRANSMIT_STRAIGHT_STATE || radioState == CW_TRANSMIT_KEYER_STATE) {  //AFP 09-01-22
      return;
    }

    /**********************************************************************************  AFP 12-31-20
        Frequency translation by Fs/4 without multiplication from Lyons (2011): chapter 13.1.2 page 646
        together with the savings of not having to shift/rotate the FFT_buffer, this saves
        about 1% of processor use

        This is for +Fs/4 [moves receive frequency to the left in the spectrum display]
           float_buffer_L contains I = real values
           float_buffer_R contains Q = imaginary values
           xnew(0) =  xreal(0) + jximag(0)
               leave first value (DC component) as it is!
           xnew(1) =  - ximag(1) + jxreal(1)
    **********************************************************************************/
    FreqShift1();
    //===

    //===
    /**********************************************************************************  AFP 12-31-20
        SPECTRUM_ZOOM_2 and larger here after frequency conversion!
        Spectrum zoom displays a magnified display of the data around the translated receive frequency.
        Processing is done in the ZoomFFTExe(BUFFER_SIZE * N_BLOCKS) function.  For magnifications of 2x to 8X
        Larger magnification are not needed in practice.

        Spectrum Zoom uses the shifted spectrum, so the center "hump" around DC is shifted by fs/4
    **********************************************************************************/
    if (spectrum_zoom != SPECTRUM_ZOOM_1) {
      //AFP  Used to process Zoom>1 for display
      ZoomFFTExe(BUFFER_SIZE * N_BLOCKS);  // there seems to be a BUG here, because the blocksize has to be adjusted according to magnification,
      // does not work for magnifications > 8
    }

    /**********************************************************************************  AFP 12-31-20
        S-Meter & dBm-display ?? not usually called
     **********************************************************************************/
    //if (calibrateFlag == 1) {
    //  CalibrateOptions(IQChoice);
    // }

    /*************************************************************************************************
        freq_conv2()

        FREQUENCY CONVERSION USING A SOFTWARE QUADRATURE OSCILLATOR
        Creates a new IF frequency to allow the tuning window to be moved anywhere in the current display.
        THIS VERSION calculates the COS AND SIN WAVE on the fly - uses double precision float

        MAJOR ADVANTAGE: frequency conversion can be done for any frequency !

        large parts of the code taken from the mcHF code by Clint, KA7OEI, thank you!
          see here for more info on quadrature oscillators:
        Wheatley, M. (2011): CuteSDR Technical Manual Ver. 1.01. - http://sourceforge.net/projects/cutesdr/
        Lyons, R.G. (2011): Understanding Digital Processing. – Pearson, 3rd edition.
     *************************************************************************************************/

    FreqShift2();  //AFP 12-14-21
    /**********************************************************************************  AFP 12-31-20
        Decimation
        Resample (Decimate) the shifted time signal, first by 4, then by 2.  Each time the
        signal is decimated by an even number, the spectrum is reversed.  Resampling twice
        returns the spectrum to the correct orientation.
        Signal has now been shifted to base band, leaving aliases at higher frequencies,
        which are removed at each decimation step using the Arm combined decimate/filter function.
        If the statring sample rate is 192K SPS after the combined decimation, the sample rate is
        now 192K/8 = 24K SPS.  The array size is also reduced by 8, making FFT calculations much faster.
        The effective bandwidth (up to Nyquist frequency) is 12KHz.
     **********************************************************************************/
    // decimation-by-4 in-place!
    arm_fir_decimate_f32(&FIR_dec1_I, float_buffer_L, float_buffer_L, BUFFER_SIZE * N_BLOCKS);
    arm_fir_decimate_f32(&FIR_dec1_Q, float_buffer_R, float_buffer_R, BUFFER_SIZE * N_BLOCKS);

    // decimation-by-2 in-place
    arm_fir_decimate_f32(&FIR_dec2_I, float_buffer_L, float_buffer_L, BUFFER_SIZE * N_BLOCKS / (uint32_t)DF1);
    arm_fir_decimate_f32(&FIR_dec2_Q, float_buffer_R, float_buffer_R, BUFFER_SIZE * N_BLOCKS / (uint32_t)DF1);



    // =================  AFP 10-21-22 Level Adjust ===========
    float freqKHzFcut;
    float volScaleFactor;
    if (bands[currentBand].mode == DEMOD_LSB) {
      freqKHzFcut = -(float32_t)bands[currentBand].FLoCut * 0.001;
    } else {
      freqKHzFcut = (float32_t)bands[currentBand].FHiCut * 0.001;
    }
    volScaleFactor = 7.0874 * pow(freqKHzFcut, -1.232);
    // sineTone(8);
    //       arm_scale_f32(sinBuffer2, volScaleFactor, float_buffer_L, FFT_length / 2);// use to calibrate SAM
    // arm_scale_f32(cosBuffer2, volScaleFactor, float_buffer_R, FFT_length / 2);


    arm_scale_f32(float_buffer_L, volScaleFactor, float_buffer_L, FFT_length / 2);
    arm_scale_f32(float_buffer_R, volScaleFactor, float_buffer_R, FFT_length / 2);




    //=================  AFP 10-21-22  =================
    /**********************************************************************************  AFP 12-31-20
        Digital FFT convolution
        Filtering is accomplished by combinig (multiplying) spectra in the frequency domain.
         basis for this was Lyons, R. (2011): Understanding Digital Processing.
         "Fast FIR Filtering using the FFT", pages 688 - 694.
         Method used here: overlap-and-save.

        First, Create Complex time signal for CFFT routine.
        Fill first block with Zeros
        Then interleave RE and IM parts to create signal for FFT
     **********************************************************************************/
    // Prepare the audio signal buffers:

    //------------------------------ ONLY FOR the VERY FIRST FFT: fill first samples with zeros

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

    /**********************************************************************************  AFP 12-31-20
       Perform complex FFT on the audio time signals
       calculation is performed in-place the FFT_buffer [re, im, re, im, re, im . . .]
     **********************************************************************************/
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
    if (updateDisplayFlag == 1) {
      for (int k = 0; k < 1024; k++) {
        audioSpectBuffer[1024 - k] = (iFFT_buffer[k] * iFFT_buffer[k]);
      }
      for (int k = 0; k < 256; k++) {
        if (bands[currentBand].mode == DEMOD_USB || bands[currentBand].mode == DEMOD_AM || bands[currentBand].mode == DEMOD_SAM) {  //AFP 10-26-22
          //audioYPixel[k] = 20+  map((int)displayScale[currentScale].dBScale * log10f((audioSpectBuffer[1024 - k] + audioSpectBuffer[1024 - k + 1] + audioSpectBuffer[1024 - k + 2]) / 3), 0, 100, 0, 120);
          audioYPixel[k] = 50 + map(15 * log10f((audioSpectBuffer[1024 - k] + audioSpectBuffer[1024 - k + 1] + audioSpectBuffer[1024 - k + 2]) / 3), 0, 100, 0, 120);
        } else if (bands[currentBand].mode == DEMOD_LSB) {  //AFP 10-26-22
          //audioYPixel[k] = 20+   map((int)displayScale[currentScale].dBScale * log10f((audioSpectBuffer[k] + audioSpectBuffer[k + 1] + audioSpectBuffer[k + 2]) / 3), 0, 100, 0, 120);
          audioYPixel[k] = 50 + map(15 * log10f((audioSpectBuffer[k] + audioSpectBuffer[k + 1] + audioSpectBuffer[k + 2]) / 3), 0, 100, 0, 120);
        }
        if (audioYPixel[k] < 0)
          audioYPixel[k] = 0;
      }
      arm_max_f32(audioSpectBuffer, 1024, &audioMaxSquared, &AudioMaxIndex);  // AFP 09-18-22 Max value of squared abin magnitued in audio
      audioMaxSquaredAve = .5 * audioMaxSquared + .5 * audioMaxSquaredAve;    //AFP 09-18-22Running averaged values
      if(freqCalFlag==0){ //AFP 01-30-25
         DisplaydbM();
      } else {

      }
    }

    /**********************************************************************************
          Additional Convolution Processes:
              // filter by just deleting bins - principle of Linrad
      only works properly when we have the right window function!

        (automatic) notch filter = Tone killer --> the name is stolen from SNR ;-)
        first test, we set a notch filter at 1kHz
        which bin is that?
        positive & negative frequency -1kHz and +1kHz --> delete 2 bins
        we are not deleting one bin, but five bins for the test
        1024 bins in 12ksps = 11.71Hz per bin
        SR[SampleRate].rate / 8.0 / 1024 = bin BW
        1000Hz / 11.71Hz = bin 85.333

     **********************************************************************************/

    /**********************************************************************************  AFP 12-31-20
      After the frequency domain filter mask and other processes are complete, do a
      complex inverse FFT to return to the time domain
        (if sample rate = 192kHz, we are in 24ksps now, because we decimated by 8)
        perform iFFT (in-place)  IFFT is selected by the IFFT flag=1 in the Arm CFFT function.
     **********************************************************************************/

    arm_cfft_f32(iS, iFFT_buffer, 1, 1);

    // Adjust for level alteration because of filters

    /**********************************************************************************  AFP 12-31-20
        AGC - automatic gain control

        we´re back in time domain
        AGC acts upon I & Q before demodulation on the decimated audio data in iFFT_buffer
     **********************************************************************************/
    AGC();  //AGC function works with time domain I and Q data buffers created in the last step

    //============================  Demod  ========================

    /**********************************************************************************
          Demodulation
            our time domain output is a combination of the real part (left channel) AND the imaginary part (right channel) of the second half of the FFT_buffer
            The demod mode is accomplished by selecting/combining the real and imaginary parts of the output of the IFFT process.
       **********************************************************************************/
    //===================== AFP 10-27-22  =========

    switch (bands[currentBand].mode) {
      case DEMOD_LSB:
        for (unsigned i = 0; i < FFT_length / 2; i++) {
          //if (bands[currentBand].mode == DEMOD_USB || bands[currentBand].mode == DEMOD_LSB ) {  // for SSB copy real part in both outputs
          float_buffer_L[i] = iFFT_buffer[FFT_length + (i * 2)];

          float_buffer_R[i] = float_buffer_L[i];
          //}
        }
        break;
      case DEMOD_USB:
        for (unsigned i = 0; i < FFT_length / 2; i++) {
          // if (bands[currentBand].mode == DEMOD_USB || bands[currentBand].mode == DEMOD_LSB ) {  // for SSB copy real part in both outputs
          float_buffer_L[i] = iFFT_buffer[FFT_length + (i * 2)];

          float_buffer_R[i] = float_buffer_L[i];
          audiotmp = AlphaBetaMag(iFFT_buffer[FFT_length + (i * 2)], iFFT_buffer[FFT_length + (i * 2) + 1]);
          //}
        }

        break;
      case DEMOD_AM:
        for (unsigned i = 0; i < FFT_length / 2; i++) {  // Magnitude estimation Lyons (2011): page 652 / libcsdr
          audiotmp = AlphaBetaMag(iFFT_buffer[FFT_length + (i * 2)], iFFT_buffer[FFT_length + (i * 2) + 1]);
          // DC removal filter -----------------------
          w = audiotmp + wold * 0.99f;  // Response to below 200Hz AFP 10-30-22
          float_buffer_L[i] = w - wold;
          wold = w;
        }
        arm_biquad_cascade_df1_f32(&biquad_lowpass1, float_buffer_L, float_buffer_R, FFT_length / 2);
        arm_copy_f32(float_buffer_R, float_buffer_L, FFT_length / 2);

        //===  Alternate AM detection - not quite as good as AlphaBetaMag AFP 10-30-22 ===
        /*   for (unsigned i = 0; i < FFT_length / 2; i++) { //
             audiotmp = sqrtf(iFFT_buffer[FFT_length + (i * 2)] * iFFT_buffer[FFT_length + (i * 2)]
                              + iFFT_buffer[FFT_length + (i * 2) + 1] * iFFT_buffer[FFT_length + (i * 2) + 1]);
             // DC removal filter -------
             w = audiotmp + wold * 0.9999f; // yes, I want a superb bass response ;-)
             float_buffer_L[i] = w - wold;

             wold = w;
           }
           arm_biquad_cascade_df1_f32 (&biquad_lowpass1, float_buffer_L, float_buffer_R, FFT_length / 2);
           arm_copy_f32(float_buffer_R, float_buffer_L, FFT_length / 2);*/
        //  ===========================
        break;
      case DEMOD_SAM:  //AFP 11-03-22
        AMDecodeSAM();
        break;
    }
    // == AFP 10-30-22

    //============================  Receive EQ  ========================  AFP 08-08-22
    if (receiveEQFlag == ON) {
      DoReceiveEQ();
      arm_copy_f32(float_buffer_L, float_buffer_R, FFT_length / 2);
    }
    //============================ End Receive EQ


    /**********************************************************************************
      Noise Reduction
      3 algorithms working 3-15-22
      NR_Kim
      Spectral NR
      LMS variable leak NR
    **********************************************************************************/
    switch (NR_Index) {
      case 0:  // NR Off
        break;
      case 1:  // Kim NR
        Kim1_NR();
        arm_scale_f32(float_buffer_L, 30, float_buffer_L, FFT_length / 2);
        arm_scale_f32(float_buffer_R, 30, float_buffer_R, FFT_length / 2);
        break;
      case 2:  // Spectral NR
        SpectralNoiseReduction();
        break;
      case 3:  // LMS NR
        ANR_notch = 0;
        Xanr();
        arm_scale_f32(float_buffer_L, 1.5, float_buffer_L, FFT_length / 2);
        arm_scale_f32(float_buffer_R, 2, float_buffer_R, FFT_length / 2);
        break;
    }
    //==================  End NR ============================
    // ===========================Automatic Notch ==================
    if (ANR_notchOn == 1) {
      ANR_notch = 1;
      Xanr();
      arm_copy_f32(float_buffer_R, float_buffer_L, FFT_length / 2);  //AFP 10-21-22
    }
    // ====================End notch =================================
    /**********************************************************************************
      EXPERIMENTAL: noise blanker
      by Michael Wild
    **********************************************************************************/

    //=============================================================
    if (NB_on != 0) {

      NoiseBlanker(float_buffer_L, float_buffer_R);
      arm_copy_f32(float_buffer_R, float_buffer_L, FFT_length / 2);
    }


    if (T41State == CW_RECEIVE) {
      DoCWReceiveProcessing();  //AFP 09-19-22

      // ----------------------  CW Narrow band filters  AFP 10-18-22 -------------------------
      if (CWFilterIndex != 5) {
        switch (CWFilterIndex) {
          case 0:                                                                                           // 0.8 KHz
            arm_biquad_cascade_df2T_f32(&S1_CW_AudioFilter1, float_buffer_L, float_buffer_L_AudioCW, 256);  //AFP 10-18-22
            arm_copy_f32(float_buffer_L_AudioCW, float_buffer_L, FFT_length / 2);                           //AFP 10-18-22
            arm_copy_f32(float_buffer_L_AudioCW, float_buffer_R, FFT_length / 2);
            break;
          case 1:                                                                                           // 1.0 KHz
            arm_biquad_cascade_df2T_f32(&S1_CW_AudioFilter2, float_buffer_L, float_buffer_L_AudioCW, 256);  //AFP 10-18-22
            arm_copy_f32(float_buffer_L_AudioCW, float_buffer_L, FFT_length / 2);                           //AFP 10-18-22
            arm_copy_f32(float_buffer_L_AudioCW, float_buffer_R, FFT_length / 2);
            break;
          case 2:                                                                                           // 1.3 KHz
            arm_biquad_cascade_df2T_f32(&S1_CW_AudioFilter3, float_buffer_L, float_buffer_L_AudioCW, 256);  //AFP 10-18-22
            arm_copy_f32(float_buffer_L_AudioCW, float_buffer_L, FFT_length / 2);                           //AFP 10-18-22
            arm_copy_f32(float_buffer_L_AudioCW, float_buffer_R, FFT_length / 2);
            break;
          case 3:                                                                                           // 1.8 KHz
            arm_biquad_cascade_df2T_f32(&S1_CW_AudioFilter4, float_buffer_L, float_buffer_L_AudioCW, 256);  //AFP 10-18-22
            arm_copy_f32(float_buffer_L_AudioCW, float_buffer_L, FFT_length / 2);                           //AFP 10-18-22
            arm_copy_f32(float_buffer_L_AudioCW, float_buffer_R, FFT_length / 2);
            break;
          case 4:                                                                                           // 2.0 KHz
            arm_biquad_cascade_df2T_f32(&S1_CW_AudioFilter5, float_buffer_L, float_buffer_L_AudioCW, 256);  //AFP 10-18-22
            arm_copy_f32(float_buffer_L_AudioCW, float_buffer_L, FFT_length / 2);                           //AFP 10-18-22
            arm_copy_f32(float_buffer_L_AudioCW, float_buffer_R, FFT_length / 2);
            break;
          case 5:  //Off
            break;
        }
      }
    }
    //=========================  AFP 10-18-22 ===================


    // ======================================Interpolation  ================

    arm_fir_interpolate_f32(&FIR_int1_I, float_buffer_L, iFFT_buffer, BUFFER_SIZE * N_BLOCKS / (uint32_t)(DF));  // Interpolatikon
    arm_fir_interpolate_f32(&FIR_int1_Q, float_buffer_R, FFT_buffer, BUFFER_SIZE * N_BLOCKS / (uint32_t)(DF));

    // interpolation-by-4
    arm_fir_interpolate_f32(&FIR_int2_I, iFFT_buffer, float_buffer_L, BUFFER_SIZE * N_BLOCKS / (uint32_t)(DF1));
    arm_fir_interpolate_f32(&FIR_int2_Q, FFT_buffer, float_buffer_R, BUFFER_SIZE * N_BLOCKS / (uint32_t)(DF1));

    /**********************************************************************************  AFP 12-31-20
      Digital Volume Control
    **********************************************************************************/



    if (mute == 1) {
      arm_scale_f32(float_buffer_L, 0.0, float_buffer_L, BUFFER_SIZE * N_BLOCKS);
      arm_scale_f32(float_buffer_R, 0.0, float_buffer_R, BUFFER_SIZE * N_BLOCKS);
    } else if (mute == 0) {
      arm_scale_f32(float_buffer_L, DF * VolumeToAmplification(audioVolume), float_buffer_L, BUFFER_SIZE * N_BLOCKS);
      arm_scale_f32(float_buffer_R, DF * VolumeToAmplification(audioVolume), float_buffer_R, BUFFER_SIZE * N_BLOCKS);
    }
    /**********************************************************************************  AFP 12-31-20
      CONVERT TO INTEGER AND PLAY AUDIO
    **********************************************************************************/

    for (unsigned i = 0; i < N_BLOCKS; i++) {
      sp_L1 = Q_out_L.getBuffer();
      sp_R1 = Q_out_R.getBuffer();
      arm_float_to_q15(&float_buffer_L[BUFFER_SIZE * i], sp_L1, BUFFER_SIZE);
      arm_float_to_q15(&float_buffer_R[BUFFER_SIZE * i], sp_R1, BUFFER_SIZE);
      Q_out_L.playBuffer();  // play it !
      Q_out_R.playBuffer();  // play it !
    }

    if (auto_codec_gain == 1) {
      Codec_gain();
    }
    elapsed_micros_sum = elapsed_micros_sum + usec;
    elapsed_micros_idx_t++;
  }                         // end of if(audio blocks available)
  if (ms_500.check() == 1 && freqCalFlag==0)  // For clock updates AFP 10-26-22
  {
    //wait_flag = 0;
    DisplayClock();
  }
}
/*====
  Purpose: Auto Tune calibrate the receive IQ

   Parameter List:
      void

   Return value:
      void
 *****/
/*====
  Purpose: Combined input/ output for the purpose of calibrating the receive IQ

   Parameter List:
      void

   Return value:
      void
 *****/
void tuneCalParameterRec(int indexStart, int indexEnd, float increment, float *IQCorrectionFactor, char prompt[]) {
  Serial.println("in tuneCalParameterRec");
  float adjMin = 100;
  int adjMinIndex = 0;
  int cal_bins[2] = { 0, 0 };
  int capture_bins;
  capture_bins = 10;
  cal_bins[0] = 128;
  cal_bins[1] = 384;
  float correctionFactor = *IQCorrectionFactor;
  for (int i = indexStart; i < indexEnd; i++) {
    *IQCorrectionFactor = correctionFactor + i * increment;
    FFTupdated = false;
    //int XmitCalDirections = 0;
    while (!FFTupdated) {
      //===============
      ShowSpectrum();
      arm_max_q15(&pixelnew[(cal_bins[0] - capture_bins)], capture_bins * 2, &refAmplitude, &index_of_max);
      arm_max_q15(&pixelnew[(cal_bins[1] - capture_bins)], capture_bins * 2, &adjAmplitude, &index_of_max);
      adjdB = ((float)adjAmplitude - (float)refAmplitude) / 1.95;
      //==============
    }
    if ((stateMachine == TX_STATE_TX_PHASE) | (stateMachine == TX_STATE_RX_PHASE)) {
      // Get two updated FFTs to avoid the same where the audio samples
      // span a change in the correction factor
      FFTupdated = false;
      while (!FFTupdated) {
        ShowSpectrum();
        arm_max_q15(&pixelnew[(cal_bins[0] - capture_bins)], capture_bins * 2, &refAmplitude, &index_of_max);
        arm_max_q15(&pixelnew[(cal_bins[1] - capture_bins)], capture_bins * 2, &adjAmplitude, &index_of_max);
        adjdB = ((float)adjAmplitude - (float)refAmplitude) / 1.95;
        //ShowSpectrum();
      }
    } else {
      ShowSpectrum();
      arm_max_q15(&pixelnew[(cal_bins[0] - capture_bins)], capture_bins * 2, &refAmplitude, &index_of_max);
      arm_max_q15(&pixelnew[(cal_bins[1] - capture_bins)], capture_bins * 2, &adjAmplitude, &index_of_max);
      adjdB = ((float)adjAmplitude - (float)refAmplitude) / 1.95;
      //ShowSpectrum();
    }
    //Serial.println(String(i)+","+String(adjdB));
    if (adjdB < adjMin) {
      adjMin = adjdB;
      adjMinIndex = i;
    }
    tft.setFontScale((enum RA8875tsize)1);
    tft.setTextColor(RA8875_WHITE);
    tft.setCursor(145, 160);
    tft.print("Auto Tune On");
    tft.fillRect(145, 200, 230, CHAR_HEIGHT, RA8875_BLACK);  // Increased rectangle size to full erase value.  KF5N August 12, 2023
    tft.setCursor(145, 200);
    tft.print(prompt);
    tft.setCursor(280, 200);
    tft.print(*IQCorrectionFactor, 3);
  }
  *IQCorrectionFactor = correctionFactor + adjMinIndex * increment;
  //*IQCorrectionFactor=-*IQCorrectionFactor
  tft.setTextColor(RA8875_WHITE);
  //tft.setCursor(160, 160);
  // tft.print("Auto Complete");
  // MyDelay(3000);
  tft.fillRect(145, 160, 230, 100, RA8875_BLACK);
}

void autotuneRec(float *amp, float *phase, float gain_coarse_max, float gain_coarse_min,
                 float phase_coarse_max, float phase_coarse_min,
                 int gain_coarse_step2_N, int phase_coarse_step2_N,
                 int gain_fine_N, int phase_fine_N, bool phase_first) {

  if (phase_first) {

    // Step 2: phase changes in 0.01 steps from -0.2 to 0.2. Find the minimum.
    int phaseStepsCoarseN = (int)((phase_coarse_max - phase_coarse_min) / 0.01 / 2);
    *phase = 0.0;
    //Serial.print("Step 2: ");
    tuneCalParameterRec(-phaseStepsCoarseN, phaseStepsCoarseN + 1, 0.01, phase, (char *)"IQ Phase");
    // Step 1: Gain in 0.01 steps from 0.5 to 1.5
    int gainStepsCoarseN = (int)((gain_coarse_max - gain_coarse_min) / 0.01 / 2);
    *amp = 1.0;
    //Serial.print("Step 1: ");
    tuneCalParameterRec(-gainStepsCoarseN, gainStepsCoarseN + 1, 0.01, amp, (char *)"IQ Gain");
  } else {
    // Step 1: Gain in 0.01 steps from 0.5 to 1.5
    int gainStepsCoarseN = (int)((gain_coarse_max - gain_coarse_min) / 0.01 / 2);
    *amp = 1.0;
    //Serial.print("Step 1: ");
    tuneCalParameterRec(-gainStepsCoarseN, gainStepsCoarseN + 1, 0.01, amp, (char *)"IQ Gain");
    // Step 2: phase changes in 0.01 steps from -0.2 to 0.2. Find the minimum.
    int phaseStepsCoarseN = (int)((phase_coarse_max - phase_coarse_min) / 0.01 / 2);
    *phase = 0.0;
    //Serial.print("Step 2: ");
    tuneCalParameterRec(-phaseStepsCoarseN, phaseStepsCoarseN + 1, 0.01, phase, (char *)"IQ Phase");
  }
  // Step 3: Gain in 0.01 steps from 4 steps below previous minimum to 4 steps above
  //Serial.print("Step 3: ");
  tuneCalParameterRec(-gain_coarse_step2_N, gain_coarse_step2_N + 1, 0.01, amp, (char *)"IQ Gain");
  // Step 4: phase in 0.01 steps from 4 steps below previous minimum to 4 steps above
  //Serial.print("Step 4: ");
  tuneCalParameterRec(-phase_coarse_step2_N, phase_coarse_step2_N + 1, 0.01, phase, (char *)"IQ Phase");
  // Step 5: gain in 0.001 steps 10 steps below to 10 steps above
  //Serial.print("Step 5: ");
  tuneCalParameterRec(-gain_fine_N, gain_fine_N + 1, 0.001, amp, (char *)"IQ Gain");
  // Step 6: phase in 0.001 steps 10 steps below to 10 steps above
  //Serial.print("Step 6: ");
  tuneCalParameterRec(-phase_fine_N, phase_fine_N + 1, 0.001, phase, (char *)"IQ Phase");
}
