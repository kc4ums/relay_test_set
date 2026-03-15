# relay_test_set

3-Phase Secondary Relay Test Set

Generates three-phase 60Hz sine waves to simulate motor starting inrush current for
overcurrent relay testing.

## Architecture

```
[BeagleBone Black eHRPWM]
  P9_22 (eHRPWM0A, Phase A, 0°)   --> [RC Filter] --> [X9C104] --> [MOSFET Amp A] --> Relay CT input A
  P9_14 (eHRPWM1A, Phase B, 120°) --> [RC Filter] --> [X9C104] --> [MOSFET Amp B] --> Relay CT input B
  P8_19 (eHRPWM2A, Phase C, 240°) --> [RC Filter] --> [X9C104] --> [MOSFET Amp C] --> Relay CT input C
```

The BBB outputs 100kHz PWM on three eHRPWM pins. Each PWM duty cycle is updated every
~65µs from a 256-sample sine lookup table to produce 60Hz. RC filters remove the PWM
carrier, leaving a clean 60Hz sine. MOSFET push-pull amplifiers (IRF540N + IRF9540N)
drive up to 10A into the relay current coil burden.

An X9C104 digital potentiometer on the signal path controls amplitude, allowing simulation
of different fault current multiples (2×, 6×, 10× rated).

## BeagleBone Black OS (microSD)

Run Debian from a microSD card — no eMMC flashing required.

1. Download **BeagleBone Black Debian 12.12 2025-10-29 IoT (v5.10-ti)** from https://www.beagleboard.org/distros
   — use the v5.10-ti kernel, NOT 6.x (6.x breaks PWM/cape support)
2. Write to a 4GB+ microSD card using **Balena Etcher**
3. Power off the BBB, insert the card
4. **Hold the S2 button** (near the microSD slot) while applying power; release after ~5 seconds
5. SSH in: `ssh debian@192.168.7.2` — password `temppwd`

## Setup

```bash
# Copy files to BBB
scp -r firmware/beaglebone_3phase/ debian@beaglebone.local:~/relay_test_set/

# On the BBB (as root):
sudo bash setup.sh

# Run with real-time priority
sudo chrt -f 50 /opt/relay_venv/bin/python3 beaglebone_3phase.py
```

`setup.sh` installs `Adafruit_BBIO`, configures all three PWM pins via `config-pin`, and
prints instructions for making pin configuration persistent across reboots.

## Verification

Probe P9\_22, P9\_14, and P8\_19 with an oscilloscope — you should see a 100kHz PWM
carrier with a 60Hz sine envelope and 120° phase offsets between channels.
Press Ctrl-C for clean shutdown.

## Files

```
firmware/
  beaglebone_3phase/
    beaglebone_3phase.py        Signal generator (CPython + Adafruit_BBIO)
    setup.sh                    First-time setup (run as root)
hardware/
  bom.md                        Bill of materials
  amp_circuit.md                MOSFET amplifier circuit description and build notes
datasheets/
  ad9833.pdf
  AD5290.pdf
  x9c104.pdf
```

## Power Supply

Meanwell LRS-350-24 (24V / 14.6A) shared across all three amplifier channels.

## Motor Inrush Simulation

- 1A CT secondary relay: target 6–10A output
- 5A CT secondary relay: use a 1:5 step-up CT to reach 30–50A
