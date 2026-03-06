# relay_test_set

3-Phase Secondary Relay Test Set

Generates three-phase 60Hz sine waves to simulate motor starting inrush current for
overcurrent relay testing.

## Architecture

```
[Raspberry Pi Pico]
  Pin 0 (Phase A, 0°)   --> [RC Filter] --> [MOSFET Amp A] --> Relay CT input A
  Pin 2 (Phase B, 120°) --> [RC Filter] --> [MOSFET Amp B] --> Relay CT input B
  Pin 4 (Phase C, 240°) --> [RC Filter] --> [MOSFET Amp C] --> Relay CT input C
```

The Pico outputs 100kHz PWM on three pins. Each PWM duty cycle is updated every ~65µs
from a 256-sample sine lookup table to produce 60Hz. RC filters remove the PWM carrier,
leaving a clean 60Hz sine. MOSFET push-pull amplifiers (IRF540N + IRF9540N) drive up to
10A into the relay current coil burden.

An X9C104 digital potentiometer on the signal path controls amplitude, allowing simulation
of different fault current multiples (2×, 6×, 10× rated).

## Files

```
sketches/
  pico_3phase/
    pico_3phase.py      MicroPython signal generator (runs on Pico)
  ad9833_single/
    ad9833_single.ino   Legacy SPI/AD9833 sketch (superseded, kept for reference)
hardware/
  bom.md                Bill of materials
  amp_circuit.md        MOSFET amplifier circuit description and build notes
datasheets/
  ad9833.pdf
  AD5290.pdf
  x9c104.pdf
docs/
  relay_test_set.org    Project TODO list
```

## Power Supply

Meanwell LRS-350-24 (24V / 14.6A) shared across all three amplifier channels.

## Motor Inrush Simulation

- 1A CT secondary relay: target 6–10A output
- 5A CT secondary relay: use a 1:5 step-up CT to reach 30–50A
