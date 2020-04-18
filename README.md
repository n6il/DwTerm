# DwTerm v0.2
A RSDOS DriveWire 4 Terminal Program

DwTerm is a terminal program for RS-DOS on the TRS-80 Color Computer 1/2/3.  This program uses the DriveWire protocol for communication.  A connection to a DriveWire server is _required_.

* (new for v0.2) ANSI Support for CoCo1/2/3 on 32x16 VDG
* (new for v0.2) ANSI Support for CoCo1/2 64x32 on CoCoVGA
* (new for v0.2) ANSI Support for CoCo3 on 80x25 GIME
* (new for v0.2) Break-C to hang up connection
* (new for v0.2) Break-Q to Quit
* (new for v0.2) Break-Break to send Ctrl-C

###Download the latest version from:
[https://github.com/n6il/DwTerm/releases/latest](https://github.com/n6il/DwTerm/releases/latest)

### Documentation & GitHub Site:
[https://github.com/n6il/DwTerm](https://github.com/n6il/DwTerm)

### Using it
At the `DWTERM>` prompt you can type `dw` commands.  Here are some examples:

* Dial out to the internet: `ATD<host>:<port>` or `telnet <host> <port>`
* `dw disk show`
* `dw disk insert <drive> <path>`
* `help` for more commands
* Break-C to hang up connection
* Break-Q to Quit
* Break-Break to send Ctrl-C


## ZIP File Distribution ##
The Zip file Contains both:

1. `DWTERM.dsk` -- Disk image with all the BIN files listed below
2. All of the individual BIN/CAS/WAV versions listed below

## Standard Version for Bit-Banger ##

Standard Versions for all Cocos, Minimum 32k RAM

* `DWTRMBB1.BIN` - Bit-Banger  (CoCo1 38400)
* `DWTRMBB.BIN` - Bit-Banger  (CoCo2 57600/CoCo3 115200)
* `DWTRMB63.BIN` - Bit-Banger HD6309 (CoCo2 57600/CoCo3 115200)

## Lite Version (Minimum 16k RAM)

Lite Version for Cocos with minimum 16k RAM

Bit-Banger (38400)

* `DWTBB1LT.BIN`
* `DWTBB1LT.CAS`
* `DWTBB1LT.WAV`

Becker Port - Emulators, Coco3FPGA

* `DWTBCKLT.BIN`
* `DWTBB1LT.CAS`
* `DWTBB1LT.WAV`

## Other Versions ##
RS-232 Pak

* `DWTRM232.BIN` -- All Cocos Minimum 32k RAM

Becker Port

* `DWTRMBCK.BIN` - Becker Port (32k Coco and up; Emulators; Coco3FPGA)
* `DWTRMWI.BIN` - Coco3FPGA WIFI Module

