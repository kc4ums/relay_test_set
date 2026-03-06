# Bill of Materials

## Signal Generation

| Qty | Part | Description |
|-----|------|-------------|
| 1 | Raspberry Pi Pico | Microcontroller, 3-phase PWM output |
| 3 | 1kΩ resistor | RC low-pass filter (per channel) |
| 3 | 100nF capacitor | RC low-pass filter (per channel) |

## MOSFET Current Amplifier (×3, one per phase)

| Qty | Part | Value/Part# | Purpose |
|-----|------|-------------|---------|
| 3 | Q1 | IRF540N | N-channel MOSFET, top switch, 100V 28A |
| 3 | Q2 | IRF9540N | P-channel MOSFET, bottom switch, 100V 19A |
| 6 | R_bias | 220Ω | Class AB quiescent bias resistors |
| 3 | R_bias trim | 10kΩ trim pot | Quiescent current adjustment |
| 3 | R_sense | 0.05Ω 5W | Current sense (optional feedback) |
| 3 | U1 | LM358 | Input buffer + bias driver |
| 6 | D1, D2 | 1N4148 | Bias diodes for thermal tracking |
| 3 | — | TO-220 heat sink | Required for 10A+ dissipation |

## Amplitude Control

| Qty | Part | Description |
|-----|------|-------------|
| 1 | X9C104 | Digital potentiometer, signal amplitude control |

## Power Supply

| Qty | Part | Description |
|-----|------|-------------|
| 1 | Meanwell LRS-350-24 | 24V / 14.6A supply (shared across all 3 channels) |

## Notes

- For relay CT secondaries rated at 5A (needing 30–50A fault simulation), add a 1:5 or 1:10
  step-up current transformer on each channel output.
- A ±12V split supply may be substituted for the 24V single-rail to improve output symmetry
  and eliminate the need for an output coupling capacitor.
