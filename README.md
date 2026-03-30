# relay_test_set

3-Phase Secondary Relay Test Set

Generates three-phase 60Hz sine waves to simulate motor starting inrush current for
overcurrent relay testing.

## Architecture

```
[LAUNCHXL-F28069M ePWM]
  GPIO0 (ePWM1A, Phase A,   0°) --> [RC Filter] --> [X9C104] --> [MOSFET Amp A] --> Relay CT input A
  GPIO2 (ePWM2A, Phase B, 120°) --> [RC Filter] --> [X9C104] --> [MOSFET Amp B] --> Relay CT input B
  GPIO4 (ePWM3A, Phase C, 240°) --> [RC Filter] --> [X9C104] --> [MOSFET Amp C] --> Relay CT input C
```

The C2000 outputs 100kHz PWM on three ePWM channels. Duty cycle is updated at
15,360 Hz from a 256-sample sine lookup table to produce 60Hz. RC filters remove
the PWM carrier, leaving a clean 60Hz sine. MOSFET push-pull amplifiers
(IRF540N + IRF9540N) drive up to 10A into the relay current coil burden.

An X9C104 digital potentiometer on the signal path controls amplitude, allowing
simulation of different fault current multiples (2×, 6×, 10× rated).

## Platform

**TI LAUNCHXL-F28069M** (~$20) — TMS320F28069M, 90MHz C28x DSP.
Purpose-built for 3-phase PWM generation (8 ePWM channels, hardware motor-control
peripherals). Bare-metal C, no OS.

- **IDE:** TI Code Composer Studio 12.x (free) — https://www.ti.com/tool/CCSTUDIO
- **SDK:** C2000Ware 5.x (included with CCS)
- **Flash:** USB (XDS110 onboard debugger on LaunchPad)

## Setup

```
1. Open CCS, create new C2000 project targeting TMS320F28069M
2. Copy firmware/c2000_3phase/main.c and sine_lut.h into the project
3. Add C2000Ware device support files and linker .cmd (see firmware/c2000_3phase/README.md)
4. Build (Ctrl+B), then Run → Debug to flash via USB
5. Run → Resume to start
```

See `firmware/c2000_3phase/README.md` for full CCS setup, include paths, and
required C2000Ware files.

## Verification

Probe GPIO0, GPIO2, GPIO4 with an oscilloscope — you should see a 100kHz PWM
carrier with a 60Hz sine envelope and 120° phase offsets between channels.
After the RC filter, verify a clean 60Hz sine at each stage.

## Files

```
firmware/
  c2000_3phase/
    main.c          Signal generator (bare-metal C, TMS320F28069M)
    sine_lut.h      256-sample sine LUT scaled to ePWM period
    README.md       CCS project setup and flashing instructions
hardware/
  bom.md            Bill of materials
  amp_circuit.md    MOSFET amplifier circuit description and build notes
  schematic.txt     Full ASCII schematic (one channel shown)
datasheets/
  x9c104.pdf
```

## Power Supply

Meanwell LRS-350-24 (24V / 14.6A) shared across all three amplifier channels.

## Motor Inrush Simulation

- 1A CT secondary relay: target 6–10A output
- 5A CT secondary relay: use a 1:5 step-up CT to reach 30–50A
