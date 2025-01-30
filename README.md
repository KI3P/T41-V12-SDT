# Introduction

This repository hosts the code for the T41-EP Software Defined Transceiver (SDT). Originally designed by Al Peter-AC8GY and Jack Purdum-W8TEE, the T41-EP is a 20W, HF, 7 band, CW/SSB Software Defined Transceiver (SDT) with features not even found in commercial radios costing ten times as much (e.g., up to 192kHz spectrum display bandwidth, ALP CW decoder, Bode Plots). The T41-EP is a self-contain SDT that does not require an external PC, laptop, or tablet to use. Al and Jack wrote a book, available on [Amazon](https://a.co/d/drLsJlJ), describing the theory and operation of the T41-EP.

### Home on the web

The T41-EP is a fully open-source radio. This repository hosts the transceiver software. The hardware designs are hosted on Bill-K9HZ's [GitHub repository](https://github.com/DRWJSCHMIDT/T41/tree/main/T41_V012_Files_01-15-24). The primary forum for discussions on the T41-EP radio is on [Groups.io](https://groups.io/g/SoftwareControlledHamRadio/topics).

### Design philosophy

The EP stands for Experimenter's Platform because the T41-EP is designed around 5 small printed circuit boards (100mm x 100mm) that can be easily swapped for boards of your own design. Because the T41-EP project is completely Open Source, you have complete access to the C/C++ source code that controls the T41-EP as well as the KiCad design files, schematics, and Gerber files. 

### Hardware purchase

The latest version (V12.6) of the bare PCBs are available for less than $5 each on the [discussion forum](https://groups.io/g/SoftwareControlledHamRadio). If you prefer a partially-assembled kit,  Justin AI6YM sells them on his [website](https://ai6ym.radio/t41-ep-sdt/).

Kits for the prior hardware version (V11) were produced and sold by the [4SQRP club](http://www.4sqrp.com/T41main.php).

### Major software variants

There are three primary software forks for the T41-EP radio. Greg KF5N has produced the [T41 Extreme Experimenters Edition (T41EEE)](https://github.com/Greg-R/T41EEE) which implements innovative features but currently only supports V11 hardware. [Terrance KN6ZDE](https://github.com/tmr4/T41_SDR) has a fork that implements mouse and keyboard input, a beacon monitor, and has implemented new modes like NFM and some data modes.

This repository hosts the "official" vanilla fork for the [V12.6 hardware](https://github.com/DRWJSCHMIDT/T41/tree/main/T41_V012_Files_01-15-24). It merges the original software written by Jack and Al with the additional features written by [John Melton-G0ORX](https://github.com/g0orx/SDTVer050.0). We aspire to merge in changes and features implemented by others if they are supported by unmodified V12.6 hardware. You are encouraged to fork this repository, experiment, and submit pull requests if you develop a feature others will like! Your help tackling the list of [Issues](https://github.com/KI3P/SDTVer050.0/issues) would also be valuable.

# SDTVer050.2

This version has the following extra software features:

* Bearing plots
* Bode plots
* Kenwood TS-2000 CAT interface
* Built-in-test for I2C errors

The V12.6 hardware features currently supported in this version include:

* Shutdown routine using the ATTiny85 on the main board
* V12.6 BPF board
* K9HZ LPF board (band select only)
* G0ORX / K9HZ MCP23017 front panel and encoders
* IQ and power amp calibration

V12.6 hardware features that still need to be supported in software include:

* K9HZ LPF SWR meter
* K9HZ LPF transverter selection
* K9HZ LPF 100W amp selection
* K9HZ LPF antenna selection

## Build Environment Setup

- Download and install the [Arduino IDE](https://www.arduino.cc/en/software). Versions prior to 2.0.4 are incompatible with the build tools for the Teensy 4.1.

- Download and install [Teensyduino](https://www.pjrc.com/teensy/td_download.html). Note the udev rules step which is required for those using Linux.

- Within the Arduino IDE install the following libraries - Adafruit BusIO, Adafruit GFX Library, Adafruit MCP23017 Arduino Library, Arduinojson.

- Clone the [OpenAudio library repository](https://github.com/chipaudette/OpenAudio_ArduinoLibrary/) and [install in your Arduino libraries directory](https://docs.arduino.cc/software/ide-v1/tutorials/installing-libraries/).

- Likewise clone and install the [Rotary library](https://github.com/brianlow/Rotary).

As of the time of this writing this author is running Arduino IDE 2.3.4, Teensyduino 1.59, Adafruit BusIO 1.16.2, Adafruit GFX Library 1.11.11, Adafruit MCP23017 Arduino Library 2.3.2, Arduinojson 7.2.1, OpenAudio commit 465672a46c1890fad72bf83ee94e673d23e384c8, and Rotary commit 6772bfa57d320e6bb8c7e6101dff0bfb93b211aa. These version numbers will naturally become out of date as time progresses. You should setup your build environment with current versions of each of these packages but refer back to this listing as a known stable setup if any issues occur.

## Compiling

Within the Arduino IDE configure the build as follows:

- Tools -> Board -> Teensy -> Teensy 4.1
- Tools -> Optimize -> Faster with LTO
- Tools -> USB Type -> Dual Serial
- Tools -> CPU Speed -> 528MHz

Configure the software as you need for your radio in *MyConfigurationFile.h*. The memory usage with the options above set, configured for a V12 single receiver build with both `G0ORX_FRONTPANEL` and `G0ORX_CAT` enabled should look something like:

```
   FLASH: code:274516, data:93624, headers:8684   free for files:7749640
   RAM1: variables:200864, code:256936, padding:5208   free for local variables:61280
   RAM2: variables:454272  free for malloc/new:70016
```

## Flashing

In the process of building the T41 radio you've likely cut a trace on the Teensy, this trace connects the USB cable to *Vin*. Doing this ensures that the radio does not draw power over the USB connection to the Teensy but it means that your Teensy cannot be powered by the USB cable. You will need to supply external power to your Teensy (e.g. by installing it in the radio and turning the radio on). Alternatively you may solder the jumper where you've cut that trace.

Connect your Teensy and click *Upload* in the Arduino IDE. If the Teensy is not put in programming mode automatically you will be asked to press the program button. This may be difficult to reach with the audio hat and offset board installed on the Teensy, those can be removed during programming or you may use a **non-conductive** object to reach between the boards and press the button.

Major differences in this software package (e.g. from older versions or other forks) may have incompatible EEPROM data. This may cause flashing to fail or the radio to fail to boot after flashing. If this occurs you may need to [manually clear the EEPROM](https://forum.pjrc.com/index.php?threads/teensy-4-1-reset-eeprom-empty-eeprom-values.74575/).
