# Introduction

This repository hosts the code for the T41-EP Software Defined Transceiver (SDT). Originally designed by Al Peter-AC8GY and Jack Purdum-W8TEE, the T41-EP is a 20W, HF, 7 band, CW/SSB Software Defined Transceiver (SDT) with features not even found in commercial radios costing ten times as much (e.g., up to 192kHz spectrum display bandwidth, ALP CW decoder, Bode Plots). The T41-EP is a self-contain SDT that does not require an external PC, laptop, or tablet to use. Al and Jack wrote a book, available on [Amazon](https://a.co/d/drLsJlJ), describing the theory and operation of the T41-EP.

### Home on the web

The T41-EP is a fully open-source radio. This repository hosts the transceiver software. The hardware designs are hosted on Bill-K9HZ's [GitHub repository](https://github.com/DRWJSCHMIDT/T41/tree/main/T41_V012_Files_01-15-24). The primary forum for discussions on the T41-EP radio is on [Groups.io](https://groups.io/g/SoftwareControlledHamRadio/topics).

### Design philosophy

The EP stands for Experimenter's Platform because the T41-EP is designed around 5 small printed circuit boards (100mm x 100mm) that can be easily swapped for boards of you own design. Because the T41-EP project is completely Open Source, you have complete access to the C source code that controls the T41-EP as well as the hardware schematics and Gerber files (for making your own PCBs). 

### Hardware purchase

The latest version (V12) of the bare PCBs are available for less than $5 each on the [discussion forum](https://groups.io/g/SoftwareControlledHamRadio). If you prefer a partially-assembled kit,  Justin AI6YM sells them on his [website](https://ai6ym.radio/t41-ep-sdt/).

Kits for the prior hardware version (V11) were produced and sold by the [4SQRP club](http://www.4sqrp.com/T41main.php).

### Major software variants

There are two primary software baselines for the T41-EP radio. Greg KF5N has produced the [T41 Extreme Experimenters Edition (T41EEE)](https://github.com/Greg-R/T41EEE) which implements innovative features but currently only supports V11 hardware. [Terrance KN6ZDE](https://github.com/tmr4/T41_SDR) has a fork that implements mouse and keyboard input, a beacon monitor, and has implemented a new modes like NFM and some data modes.

This repository hosts the "official" software for the [V12 hardware](https://github.com/DRWJSCHMIDT/T41/tree/main/T41_V012_Files_01-15-24). It merges the original software written by Jack and Al with the additional features written by [John Melton-G0ORX](https://github.com/g0orx/SDTVer050.0). You are encouraged to fork this repository, experiment, and submit pull requests!

# SDTVer050.?

This version has the following extra software features:

* Bearing plots
* Bode plots
* Kenwood TS-2000 CAT interface
* Built-in-test for I2C errors

The V12 hardware features currently supported in this version include:

* Shutdown routine using the ATTiny85 on the main board
* V12 BPF board
* K9HZ LPF board (band select only)
* G0ORX / K9HZ MCP23017 front panel and encoders

V12 hardware features that still need to be supported in software include:

* K9HZ LPF SWR meter
* K9HZ LPF transverter selection
* K9HZ LPF 100W amp selection
* K9HZ LPF antenna selection

# Compiling

To build this version configure Arduino IDE to use the [Teensyduino](https://www.pjrc.com/teensy/td_download.html) library, select the Teensy 4.1 board, and select the following build configuration options: 

* Tools->Optimize->Faster with LTO
* Tools->USB Type->Dual Serial

The memory Usage with these options set and both G0ORX_FRONTPANEL and G0ORX_CAT enabled should look something like:

```
   FLASH: code:274516, data:93624, headers:8684   free for files:7749640
   RAM1: variables:200864, code:256936, padding:5208   free for local variables:61280
   RAM2: variables:454272  free for malloc/new:70016
```

