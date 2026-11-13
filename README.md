# Arduino-VCO-Thing

A simple digital oscillator for Eurorack, built with an Arduino Uno and the [Mozzi](https://github.com/sensorium/Mozzi) library. It reads a 1V/octave CV input and a tuning pot, can quantize notes to a scale, and offers eight waveforms selected with a button.

<img width="600" height="797" alt="arduino-vco-thing" src="https://github.com/user-attachments/assets/e78d6d7b-96ce-42d5-8eeb-fbcd4678941c" />

It's an experimental project, built from scrap parts and a 3d printed front panel, meant more for learning and playing.

## Demo 

[![Arduino-VCO-Thing demo](https://img.youtube.com/vi/dJxtJrnOvVs/maxresdefault.jpg)](https://www.youtube.com/watch?v=dJxtJrnOvVs)

## Features

- 1V/octave CV input (0 to 5V, about five octaves)
- Note pot that works as a four-octave transpose, added to the CV
- Quantization on/off, with hysteresis so notes don't flicker at the boundary between two notes
- Scale configurable in the code (minor pentatonic by default)
- Eight waveforms, each with its own indicator LED
- Continuous pitch with quantization off, for vibrato and glides from an LFO

## Waveforms

| # | Mode | Description |
|---|------|-------------|
| 1 | Saw | Classic sawtooth |
| 2 | Triangle | Triangle, softer sound |
| 3 | Square | Square with reduced aliasing |
| 4 | Sine | Pure sine |
| 5 | Analog saw | Sawtooth with the imperfections of an analog oscillator |
| 6 | Supersaw | Three slightly detuned sawtooths |
| 7 | Bell | FM at a 3.5:1 ratio; the metallic brightness retriggers on each new note and decays |
| 8 | Glass | Sine through a Chebyshev polynomial (fundamental + 3rd harmonic) |

## Pinout

| Pin | Function |
|-----|----------|
| A0 | Note pot |
| A1 | CV input |
| D2 | Waveform button (to GND) |
| D3 | Quantize button (to GND) |
| D4–D8, D10–D12 | Waveform LEDs, in the order of the table above |
| D9 | Audio output (Mozzi PWM) |
| D13 | Quantize LED (lights together with the on-board LED) |

The buttons use the Uno's internal pull-ups, so no resistors are needed. Each LED gets a 1k series resistor (470Ω if they're too dim).

## Circuits

### CV input

- Jack tip → 1k resistor → A1
- 100k resistor from the jack tip to GND (the input reads 0V when nothing is plugged in)
- Schottky diode (BAT85 or similar) from A1 to 5V, with the band (cathode) toward 5V
- Schottky diode from GND to A1, with the band toward A1
- 100nF capacitor from A1 to GND

The diodes protect the pin from voltages above 5V and below 0V. Use Schottky diodes: a regular silicon diode would let the pin go slightly past the ATmega's limits.

### Audio output

- D9 → 1k resistor → capacitor to GND (low-pass filter) → 10µF series capacitor → output jack tip
- Values between 33nF and 56nF work well in the filter: larger values darken the sound and better suppress the PWM carrier

The output level is a few volts, below the usual 10V peak-to-peak of Eurorack. An op-amp gain stage is a possible improvement.

### Power

Decided on feeding +12V from the bus to the VIN pin and let the Uno's regulator do the work. If you power it with 5V directly on the 5V pin, don't connect USB at the same time.

## Front panel

The 3D-printable front panel is in [`panel/Arduino-VCO-Thing-Panel.stl`](Arduino-VCO-Thing-Panel.stl). The photos of the prototype show an earlier version.

The original design is on Tinkercad, if you want to export it in another format or tinker with it: [Arduino-VCO-Thing-Panel](https://www.tinkercad.com/things/0LKR8gH9oFq-arduino-vco-thing-panel)

## Building

1. Install the Mozzi library (version 2.x) from the Arduino IDE library manager.
2. Open the ino file, select the Arduino Uno board and upload.

### Compile error with the Linux distro Arduino IDE

The Arduino IDE installed from Debian/Ubuntu packages uses the system's avr-libc, which is newer than the one bundled with the official IDE, and Mozzi may fail to compile with:

```
error: cannot convert 'const double*' to 'const float*'
```

The fix is one line in `mozzi_pgmspace.h`, inside the `double` function:

```cpp
return pgm_read_float_near((const float*) address);
```

The change is lost when the library is updated, so it needs to be reapplied after each update.

## Calibration and settings

At the top of the sketch:

- `VREF`: actual voltage of the 5V pin, measured with a multimeter. It's the ADC reference, so it directly affects CV tracking.
- `VOLTS_POR_OITAVA`: fine adjustment of the CV scale, if octaves are stretching or shrinking.
- `NOTA_BASE`: lowest MIDI note (24 = C1).
- `FAIXA_POT`: how many semitones the pot covers.
- `ESCALA`: scale degrees, in semitones from the root.

To calibrate with a keyboard such as the KeyStep: set `VREF` first, play the same note in different octaves with a tuner, and if they still differ, correct `VOLTS_POR_OITAVA`.

## Limitations

- The Uno's ADC is 10-bit: about 17 steps per semitone on the CV input. With quantization on this doesn't show; with it off, very slow glides can sound slightly stepped.
- The CV range is 0 to 5V. Negative voltages read as 0V, and anything above 5V as the highest note.
- 8-bit audio through PWM: the character is digital and a bit gritty, which is part of the charm.

## Acknowledgements

- [Mozzi](https://github.com/sensorium/Mozzi), by Tim Barrass and contributors, which does all the heavy lifting of the synthesis.
