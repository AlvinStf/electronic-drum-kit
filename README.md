# Electronic Drum Kit

Hardware & Firmware for an electronic drum kit based on the **ESP32-S3 N16R8**, featuring **USB MIDI**, **polyphonic WAV audio playback through a PCM5102 DAC**, a **master-volume potentiometer on GPIO15**, and an **ST7789 settings menu**.

**New to GitHub?** Start with the [GitHub Getting Started Guide](docs/GITHUB_PERTAMA.md). For setup and operation, read the [Firmware Guide](FIRMWARE_GUIDE.md).

## Features

- USB MIDI for kick, snare, three toms, hi-hat, pedal, crash, and ride.
- A 16-voice audio mixer with 44.1 kHz stereo I2S output.
- Master Volume (GPIO15), Master Trim, Local Audio, and Audio Velocity settings.
- An ST7789 menu for pad parameters, curves, and MIDI notes.
- Settings saved through Preferences/NVS, with validation and CRC checks.
- A local sound test and host-side logic tests using hardware stubs.

WAV samples for each pad are included. The repository also includes a tool to convert WAV files to C arrays and a workflow for replacing the header file to change the sounds played through the PCM5102. Audio is not read from an SD card. Changing a MIDI note does not change the corresponding WAV sample.

## Opening the project in Arduino IDE

Open the following sketch and keep all header files in the same folder:

```text
firmware/src/DrumModule.ino
```

Firmware dependencies include Arduino-ESP32, HelloDrum with the project-specific patch, AudioTools, Adafruit GFX, Adafruit ST7735 and ST7789 Library, and Adafruit BusIO. `Preferences`, `USB`, `USBMIDI`, and `SPI` are provided by Arduino-ESP32.

The exact versions used in the author's installation have not yet been documented. Update the [Build Environment Notes](docs/BUILD_ENVIRONMENT.md) based on a known-working installation rather than assuming the latest versions.

## GPIO mapping in the firmware

| Device | Signal | GPIO |
|---|---|---:|
| PCM5102 | BCK / WS / DIN | 42 / 47 / 21 |
| Master potentiometer | Wiper | 15 |
| ST7789 | SCK / MOSI | 12 / 11 |
| ST7789 | CS / DC / RST | 39 / 40 / 41 |
| Buttons | UP / SELECT / DOWN | 0 / 45 / 48 |
| Kick | Piezo | 10 |
| Snare | Head / rim | 7 / 8 |
| Toms | High / hi-mid / floor | 4 / 5 / 6 |
| Hi-hat / pedal | Sensor | 1 / 2 |
| Crash | Piezo / sensor | 3 / 13 |
| Ride | Piezo / sensor | 9 / 14 |

See the [Firmware Guide](FIRMWARE_GUIDE.md) for notes on power supply, grounding, and strapping pins. Turn off the power before changing any wiring. Connect the potentiometer to 3V3 and GND, **not 5 V**.

## Modified HelloDrum library

`library_patch/hellodrum.cpp` is the modified source file used by this project; it is not the complete library. Back up the existing file, then install this file as `src/hellodrum.cpp` in your active HelloDrum installation, making sure the corresponding headers and supporting files are compatible. If your installation already uses this version of the file, you do not need to replace it again.

**Do not copy the patch into the sketch folder while also compiling the same library separately.** Read the [patch instructions](library_patch/README.txt) and [third-party notices](THIRD_PARTY_NOTICES.md).

## Firmware repository structure

```text
src/                  Main sketch and firmware header files
library_patch/        Modified hellodrum.cpp, instructions, and upstream license
reference/            Examples used during integration; not the main sketch
artifacts/            Results of local packaging checks
tests/                Logic tests and hardware stubs
tools/                WAV PCM16-to-C++-header converter
docs/                 GitHub guide and build notes
```

`tests/stubs` are not a replacement for the original Arduino libraries.

## Testing

The firmware has been tested on the custom PCB included in this repository. All functions and features have worked as expected during testing.

## Hardware

The drum kit interface was designed in SolidWorks. The STL files have been tested on a Bambu Lab P1S printer. STEP files are also included for users who want to inspect or modify the design. Check the STEP files for any standard components required in addition to the 3D-printed parts. A bill of materials (BOM) has not been prepared yet.

## PCB

The PCB has been manufactured and tested, and no issues were found during testing. The schematic and EasyEDA project files are included for troubleshooting or modification.

## License and audio sample status

The upstream HelloDrum MIT license and attribution to Ryo Kosaka are retained.