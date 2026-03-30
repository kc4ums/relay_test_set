# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

3-phase secondary relay test set. Generates 60Hz sine waves (120° apart) to simulate motor inrush current for testing overcurrent relay CT inputs. Target output: up to 10A.

## Platform

**TI LAUNCHXL-F28069M** — TMS320F28069M, 90MHz C28x DSP. Bare-metal C, no OS.
- **Toolchain:** TI Code Composer Studio (CCS) 12.x + C2000Ware 5.x
- **Flash:** USB via XDS110 onboard debugger (CCS Run → Debug)

### Why C2000 (not BeagleBone Black)

BBB was abandoned due to: `Adafruit_BBIO` broken on Python 3.11+, `bb-cape-overlays`
not in apt (requires source build), cape_universal/pinmux not loading on kernel 6.x,
and Linux timing jitter unsuitable for deterministic PWM. C2000 ePWM is purpose-built
for 3-phase power control with no OS overhead.

## Firmware Deployment

1. Open CCS, create new C2000 project targeting TMS320F28069M
2. Add `firmware/c2000_3phase/main.c` and `sine_lut.h` to the project
3. Add C2000Ware device support files + linker `.cmd` (see `firmware/c2000_3phase/README.md`)
4. Build (Ctrl+B), then Run → Debug to flash via USB
5. See `firmware/c2000_3phase/README.md` for full CCS setup details

## Hardware Architecture

Signal path (per channel, ×3):
1. **C2000 ePWM** (GPIO0/GPIO2/GPIO4, 100kHz carrier, 256-sample sine LUT) →
2. **RC filter** (2-pole, 1kΩ+1kΩ, 2×100nF, ~1.6kHz cutoff) →
3. **X9C104 digital pot** (amplitude control) →
4. **LM358 buffer** →
5. **MOSFET push-pull** (IRF540N + IRF9540N, Class AB, 24V rail) → relay CT input

Power rails: +24V (Meanwell LRS-350-24) for MOSFET drains; +12V (7812 from 24V) for op-amps; +3.3V from LaunchPad internal regulator.

## Key Files

| File | Purpose |
|------|---------|
| `firmware/c2000_3phase/main.c` | Active firmware (bare-metal C, TMS320F28069M) |
| `firmware/c2000_3phase/sine_lut.h` | 256-sample sine LUT (uint16_t, 0–450) |
| `firmware/c2000_3phase/README.md` | CCS project setup and flashing instructions |
| `hardware/schematic.txt` | Full ASCII schematic (one channel + power) |
| `hardware/amp_circuit.md` | MOSFET amplifier design details |
| `hardware/bom.md` | Bill of materials |
| `Kicad/Kicad.kicad_sch` | KiCad schematic |

## Firmware Notes

- **`sine_lut`**: 256-sample table, uint16_t, values 0–450 (EPWM_PERIOD). Generated as `round((sin(2π·i/256)+1)·225)`
- **Phase offsets**: 0, 85, 170 samples (≈0°, 120°, 240° at 256 samples/cycle)
- **ePWM pins**: GPIO0 (ePWM1A, Phase A), GPIO2 (ePWM2A, Phase B), GPIO4 (ePWM3A, Phase C)
- **Carrier**: 100kHz, up-down count, TBPRD=450 at 90MHz SYSCLK
- **Sine update**: CPU Timer 0 ISR at 15,360Hz (= 256 × 60Hz); updates CMPA on ePWM1/2/3
- **SYSCLK**: 90MHz (PLL ×18/÷2 from 10MHz XTAL on LaunchPad)
- **No phase sync hardware**: software index offsets (0/85/170) maintain phase; no ePWM hardware phase register used
- **X9C104 control** (not yet implemented): INC/U_D/CS signals; 100 positions, NVM-capable

## Design Constraints

- The X9C104 sits *after* the RC filter on the pre-amplified signal — amplitude scaling happens on the clean sine, not the PWM
- Class AB bias is set by a 10kΩ trim pot + 2× 1N4148 diodes; adjust for minimal crossover distortion at bench test
- Star-ground topology: SGND (signal) and PGND (power) joined only at the Meanwell return
- C2000Ware version: use 5.x; F28069 device support is under `device_support/f2806x/`
