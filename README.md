# Arduino-VCO-Thing

A simple digital oscillator for Eurorack, built with an Arduino Uno and the [Mozzi](https://github.com/sensorium/Mozzi) library. It reads a 1V/octave CV input and a tuning pot, can quantize notes to a scale, and offers eight waveforms selected with a button.

<img width="600" height="797" alt="arduino-vco-thing" src="https://github.com/user-attachments/assets/e78d6d7b-96ce-42d5-8eeb-fbcd4678941c" />

It's an experimental project, built from scrap parts and a 3d printed front panel, meant more for learning and playing. 

First I noticed is that I got the buttons on the wrong side of the panel, I noticed that when I wired the jacks wrong too. I fixed that on the STL and on TinkerCAD. I also added stiffeners to the panel and changed the sockets for the LEDs. My plans include reprinting the panel with an updated design (already on the STL available here) in translucent grey PETG (LEDs will shine through the PETG, no holes for the LEDs anymore) and make another module with more care towards soldering and wire positioning.

## Demo 

[![Arduino-VCO-Thing demo](https://img.youtube.com/vi/dJxtJrnOvVs/maxresdefault.jpg)](https://www.youtube.com/watch?v=dJxtJrnOvVs)

## Features

- 1V/octave CV input (0 to 5V, about five octaves)
- Note pot that works as a four-octave transpose, added to the CV
- Quantization on/off, with hysteresis so notes don't flicker at the boundary between two notes
- Scale configurable in the code (minor pentatonic by default)
- Eight waveforms, each with its own indicator LED
- Continuous pitch with quantization off, for vibrato and glides from an LFO
- Two sketches: the original single-pin audio output, and a 2-pin version (Mozzi's former "HIFI" mode) with a much cleaner sound

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
| D4–D8, D10–D12 | Waveform LEDs, in the order of the table above (single-pin version) |
| D4–D8, A3, D11, D12 | Waveform LEDs, in the order of the table above (2-pin version) |
| D9 | Audio output (single-pin), or high bits (2-pin) |
| D10 | Audio output low bits (2-pin version only) |
| D13 | Quantize LED (lights together with the on-board LED) |

The 2-pin version needs D10 for audio, so the sixth waveform LED moves to A3, which works as a normal digital output on the Uno.

The buttons use the Uno's internal pull-ups, so no resistors are needed. Each LED gets a 1k series resistor (470Ω if they're too dim).

## Circuits

### CV input

- Jack tip → 1k resistor → A1
- 100k resistor from the jack tip to GND (the input reads 0V when nothing is plugged in)
- Schottky diode (BAT85 or similar) from A1 to 5V, with the band (cathode) toward 5V
- Schottky diode from GND to A1, with the band toward A1
- 100nF capacitor from A1 to GND

The diodes protect the pin from voltages above 5V and below 0V. Use Schottky diodes: a regular silicon diode would let the pin go slightly past the ATmega's limits.

### Audio output, single-pin version (`Arduino_VCO_Thing`)

- D9 → 1k resistor → capacitor to GND (low-pass filter) → 10µF series capacitor → output jack tip
- Values between 33nF and 56nF work well in the filter: larger values darken the sound and better suppress the PWM carrier

### Audio output, 2-pin version (`Arduino_VCO_Thing_2pin`)

Two pins carry 7 bits each, summed by two resistors into a 14-bit signal with the PWM carrier at 125kHz, far above the audible range. Compared with the single-pin version, the noise floor is much lower and the filter no longer has to dull the treble. The difference is very noticeable.

- D9 → 3.9k resistor → summing point
- D10 → 499k resistor → same summing point
- Summing point → capacitor to GND (low-pass filter, about 4.7nF; up to 10nF if it sounds harsh) → 10µF series capacitor → output jack tip

The ratio between the two resistors (499 / 3.9 ≈ 128) is what matters, so use 1% resistors or measure a few and pick the closest pair. 470k + 27k in series is a workable substitute for 499k. The single-pin circuit sounds badly distorted with the 2-pin sketch, so the resistors have to change together with the code.

The mode is selected at the top of the sketch, before `#include <Mozzi.h>`:

```cpp
#include <MozziConfigValues.h>
#define MOZZI_AUDIO_MODE MOZZI_OUTPUT_2PIN_PWM
```

The output level is a few volts, below the usual 10V peak-to-peak of Eurorack. An op-amp gain stage is a possible improvement.

### Power

Decided on feeding +12V from the bus to the VIN pin and let the Uno's regulator do the work. If you power it with 5V directly on the 5V pin, don't connect USB at the same time.

## Front panel

The 3D-printable front panel is in [`Arduino-VCO-Thing-Panel.stl`](Arduino-VCO-Thing-Panel.stl). The photos of the prototype show an earlier version.

The original design is on Tinkercad, if you want to export it in another format or tinker with it: [Arduino-VCO-Thing-Panel](https://www.tinkercad.com/things/0LKR8gH9oFq-arduino-vco-thing-panel)

## Building

1. Install the Mozzi library (version 2.x) from the Arduino IDE library manager.
2. Open the sketch for your output circuit, `Arduino_VCO_Thing/Arduino_VCO_Thing.ino` (single-pin) or `Arduino_VCO_Thing_2pin/Arduino_VCO_Thing_2pin.ino` (2-pin), select the Arduino Uno board and upload.

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
- 8-bit audio through PWM in the single-pin version: the character is digital and a bit gritty, which is part of the charm.
- In both versions the wavetables are 8-bit and the oscillator runs at 16384Hz, so high notes have some aliasing (a slightly metallic edge). The 2-pin output cleans up the noise, but not this.

## Acknowledgements

- [Mozzi](https://github.com/sensorium/Mozzi), by Tim Barrass and contributors, which does all the heavy lifting of the synthesis.
