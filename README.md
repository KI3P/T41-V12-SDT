## Introduction

This repository hosts Version 66-9 of the source code for the T41-EP Software Defined Transceiver (SDT). Originally designed by Al Peter-AC8GY and Jack Purdum-W8TEE, the T41-EP is a 20W, HF, 7 band, CW/SSB Software Defined Transceiver (SDT) with features not even found in commercial radios costing ten times as much (e.g., up to 192kHz spectrum display bandwidth, ALP CW decoder, Bode Plots). The T41-EP is a self-contain SDT that does not require an external PC, laptop, or tablet to use. Al and Jack wrote a book, available on [Amazon](https://a.co/d/drLsJlJ) (note: link is to 3rd edition), describing the theory and operation of the T41-EP.

## Version 66-9

This version of the code corresponds to the upcoming 4th edition of "Digital Signal Processing and Software Defined Radio: Theory and Construction of the T41-EP Software Defined Transceiver" by Albert F. Peter, AC8GY and Dr Jack Purdum, W8TEE. It is a point release of the software which contains the features and capabilities described in the book and works on V12 hardware only. Ongoing development of the code base is captured in the companion repository [T41-V12-SDT](https://github.com/KI3P/T41-V12-SDT).

### Configuring the IDE and libraries

Use V2 of the [Arduino IDE](https://www.arduino.cc/en/software). This code has been tested with V2.3.4. Configure your Arduino IDE to use the Teensyduino library following the instructions [here](https://www.pjrc.com/teensy/td_download.html).

Install the following libraries via the Arduino Library Manager:

* Adafruit MCP23017 Arduino Library, by Adafruit (v2.3.2) (note: install with dependencies)
* Adafruit GFX Library, by Adafruit (v1.11.11) (note: install with dependencies)
* ArduinoJson, by Benoit Blanchon (v7.3.0)
* Chrono, by Thomas O. Fredericks (v1.2.0)
* Timer, by Stefan Staub (v1.2.1)

Several libraries need to be installed manually. The manual process is:

1. Go to the provided GitHub link for the library and download the library as a zip by clicking Code -> Download ZIP.
2. Import it into the Arduino 2 IDE by clicking Sketch -> Include Library -> Add .ZIP library, and then selecting the file you just downloaded.

The libraries to install using this process are:

* OpenAudio: [https://github.com/chipaudette/OpenAudio_ArduinoLibrary](https://github.com/chipaudette/OpenAudio_ArduinoLibrary)
* Regressino: [https://github.com/cubiwan/Regressino/tree/master](https://github.com/cubiwan/Regressino/tree/master)
* Arduino Regression: [https://github.com/nkaaf/Arduino-Regression](https://github.com/nkaaf/Arduino-Regression)

### Compiler settings

The file `Config.h` includes several compiler options which add, subtract, or adjust parameters of some features. Please review this file prior to compilation. 

Completing a FLASH erase of the Teensy is strongly recommended before uploading this new version. The instructions for performing a FLASH erase of the Teensy are at [this link](https://www.pjrc.com/store/teensy41.html#programming) under the bullet "Memory Wipe & LED Blink Restore".

Select the Teensy 4.1 board, and select the following build configuration options: 

* Tools->Optimize->Fast with LTO
* Tools->USB Type->Dual Serial
* Tools->CPU Speed->528 MHz

The memory usage after compiling should look like this:

```
   FLASH: code:282444, data:133432, headers:9076   free for files:7701512
   RAM1: variables:195744, code:259848, padding:2296   free for local variables:66400
   RAM2: variables:483744  free for malloc/new:40544
```


## Other web resources

The T41-EP is a fully open-source radio. This repository hosts the transceiver software. The hardware designs are hosted on Bill-K9HZ's [GitHub repository](https://github.com/DRWJSCHMIDT/T41/tree/main/T41_V012_Files_01-15-24). The primary forum for discussions on the T41-EP radio is on [Groups.io](https://groups.io/g/SoftwareControlledHamRadio/topics).

The EP stands for Experimenter's Platform because the T41-EP is designed around 5 small printed circuit boards (100mm x 100mm) that can be easily swapped for boards of your own design. Because the T41-EP project is completely Open Source, you have complete access to the C/C++ source code that controls the T41-EP as well as the KiCad design files, schematics, and Gerber files. 

The hardware design files for the V12 radio modules can be found at the following links:

* [Main board](https://github.com/DRWJSCHMIDT/T41/tree/main/T41_V012_Files_01-15-24/T41_V012_KiCad/T41-main-board-V012)
* [RF board](https://github.com/DRWJSCHMIDT/T41/tree/main/T41_V012_Files_01-15-24/T41_V012_KiCad/T41-RF-board-V012)
* [BPF board](https://github.com/DRWJSCHMIDT/T41/tree/main/T41_V012_Files_01-15-24/T41_V012_KiCad/T41-BPF-filter-board)
* [Front panel switch board](https://github.com/DRWJSCHMIDT/K9HZ/tree/main/K9HZ_Front_Panel_Boards)
* [Front panel encoder boards](https://github.com/DRWJSCHMIDT/K9HZ/tree/main/K9HZ_Encoder_Boards)
* [LPF module](https://github.com/DRWJSCHMIDT/K9HZ/tree/main/K9HZ_LPF_Module)
* [20W amplifier module](https://github.com/DRWJSCHMIDT/K9HZ/tree/main/K9HZ_20W_PA)

## Hardware purchase

The latest version (V12.6) of the bare PCBs are available for less than $5 each on the [discussion forum](https://groups.io/g/SoftwareControlledHamRadio). If you prefer a partially-assembled kit,  Justin AI6YM sells them on his [website](https://ai6ym.radio/t41-ep-sdt/).

Kits for the prior hardware version (V11) were produced and sold by the [4SQRP club](http://www.4sqrp.com/T41main.php).
