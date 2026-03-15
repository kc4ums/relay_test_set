# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

3-phase secondary relay test set. Generates 60Hz sine waves (120° apart) to simulate motor inrush current for testing overcurrent relay CT inputs. Target output: up to 10A.

## Firmware Deployment

Runs on BeagleBone Black (Debian 12 Bookworm, v5.10-ti kernel, console). Requires `Adafruit_BBIO` (installed into `/opt/relay_venv`) and `bb-cape-overlays`.

1. Copy `firmware/beaglebone_3phase/` to the BBB (e.g. via `scp`)
2. Run `sudo bash setup.sh` — installs deps and configures PWM pins
3. Run the generator: `sudo chrt -f 50 /opt/relay_venv/bin/python3 beaglebone_3phase.py`
4. Ctrl-C for clean shutdown (outputs return to 50% midpoint)

### First-time BBB setup notes

- **OS image:** BeagleBone Black Debian 12.12 2025-10-29 IoT (v5.10-ti) — use this exact image
  - Download from https://www.beagleboard.org/distros
  - Flash with Balena Etcher; hold S2 button on first boot to boot from SD
  - Use v5.10-ti kernel — newer mainline kernels (6.x) break cape universal and PWM
- `bb-cape-overlays` is available via apt on Bookworm with the BeagleBone repo
- `Adafruit_BBIO` installs fine on Python 3.9 (shipped with Bookworm/v5.10-ti image)
- `Adafruit_BBIO` is installed into a venv at `/opt/relay_venv` (PEP 668)
- `.gitattributes` enforces LF line endings for `.sh`/`.py` — no CRLF issues after scp
- Enable cape universal in `/boot/uEnv.txt`: uncomment `enable_uboot_cape_universal=1`

`SCHED_FIFO` priority 50 (`chrt -f 50`) is required for acceptable timing jitter on a stock Debian kernel.

## Hardware Architecture

Signal path (per channel, ×3):
1. **BBB eHRPWM** (P9\_22/P9\_14/P8\_19, 100kHz carrier, 256-sample sine LUT) →
2. **RC filter** (2-pole, 1kΩ+1kΩ, 2×100nF, ~1.6kHz cutoff) →
3. **X9C104 digital pot** (amplitude control) →
4. **LM358 buffer** →
5. **MOSFET push-pull** (IRF540N + IRF9540N, Class AB, 24V rail) → relay CT input

Power rails: +24V (Meanwell LRS-350-24) for MOSFET drains; +12V (7812 from 24V) for op-amps; +3.3V from BBB internal regulator.

## Key Files

| File | Purpose |
|------|---------|
| `firmware/beaglebone_3phase/beaglebone_3phase.py` | Active firmware (CPython + Adafruit_BBIO) |
| `firmware/beaglebone_3phase/setup.sh` | BBB first-time setup script (run as root) |
| `hardware/schematic.txt` | Full ASCII schematic (one channel + power) |
| `hardware/amp_circuit.md` | MOSFET amplifier design details |
| `hardware/bom.md` | Bill of materials |
| `Kicad/Kicad.kicad_sch` | KiCad schematic |

## Firmware Notes

- **`sine_lut`**: Precomputed 256-sample table, values 0.0–100.0% (Adafruit_BBIO duty cycle is percent)
- **Phase offsets**: 0, 85, 170 samples (≈120° and 240° at 256 samples/cycle)
- **PWM API**: `PWM.start(pin, duty, freq)` then `PWM.set_duty_cycle(pin, val)` per loop step
- **Pins**: P9\_22 (eHRPWM0A, Phase A), P9\_14 (eHRPWM1A, Phase B), P8\_19 (eHRPWM2A, Phase C)
  — one A-channel per eHRPWM module avoids shared-timebase conflicts
- **Timing**: `time.perf_counter()` (monotonic float seconds); ~65µs/step; elapsed write time subtracted before sleep
- **Shutdown**: SIGINT/SIGTERM handler sets all outputs to 50% midpoint, calls `PWM.cleanup()`
- **Pre-shifted LUTs**: Per-phase pre-shifted arrays avoid per-loop modulo arithmetic
- **Real-time priority**: Run with `sudo chrt -f 50` (SCHED\_FIFO) to reduce timing jitter
- **X9C104 control** (not yet implemented): INC/U\_D/CS signals; 100 positions, NVM-capable

## Design Constraints

- The X9C104 sits *after* the RC filter on the pre-amplified signal — amplitude scaling happens on the clean sine, not the PWM
- Class AB bias is set by a 10kΩ trim pot + 2× 1N4148 diodes; adjust for minimal crossover distortion at bench test
- Star-ground topology: SGND (signal) and PGND (power) joined only at the Meanwell return
