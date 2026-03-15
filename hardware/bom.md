# Bill of Materials

## Signal Generation

| Qty | Part | Description |
|-----|------|-------------|
| 1 | LAUNCHXL-F28069M | TI C2000 LaunchPad, 3-phase ePWM output (GPIO0/2/4) |

## RC Input Filter (×3, one per phase — 2-pole per channel)

| Qty | Part | Value | Purpose |
|-----|------|-------|---------|
| 6 | R | 1kΩ | RC low-pass filter (2 per channel) |
| 6 | C | 100nF | RC low-pass filter (2 per channel) |

Cutoff ≈ 1.6kHz per pole; two poles give ~40dB attenuation at 100kHz carrier.

## MOSFET Current Amplifier (×3, one per phase)

| Qty | Part | Value/Part# | Purpose |
|-----|------|-------------|---------|
| 3 | Q1 | IRF540N | N-channel MOSFET, top switch, 100V 28A |
| 3 | Q2 | IRF9540N | P-channel MOSFET, bottom switch, 100V 19A |
| 6 | R_bias | 220Ω | Gate series resistors (Class AB bias network) |
| 3 | R_bias trim | 10kΩ trim pot | Quiescent current adjustment |
| 3 | R_sense | 0.05Ω 5W | Current sense (optional feedback) |
| 3 | U1 | LM358 | Input buffer + bias driver |
| 6 | D1, D2 | 1N4148 | Bias diodes for Class AB thermal tracking |
| 3 | — | TO-220 heat sink | Required for 10A+ dissipation |

## Amplitude Control

| Qty | Part | Description |
|-----|------|-------------|
| 3 | X9C104 | Digital potentiometer, one per phase, independent CS lines |

## Power Supply

| Qty | Part | Description |
|-----|------|-------------|
| 1 | Meanwell LRS-350-24 | 24V / 14.6A supply (shared across all 3 channels) |
| 1 | 7812 | +12V linear regulator for op-amp rail (from 24V input) |

## Notes

- For relay CT secondaries rated at 5A (needing 30–50A fault simulation), add a 1:5 or 1:10
  step-up current transformer on each channel output.
- A ±12V split supply may be substituted for the 24V single-rail to improve output symmetry
  and eliminate the need for an output coupling capacitor.
