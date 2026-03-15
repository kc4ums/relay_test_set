# MOSFET Current Amplifier — Circuit Description

Build three identical channels, one per phase.

## Topology: Complementary Push-Pull (Class AB)

```
         +24V
           |
         [IRF540N]  Q1 — N-channel source follower (top)
           | Source
           +-----------> OUTPUT (to relay CT input)
           | Source
         [IRF9540N] Q2 — P-channel source follower (bottom)
           |
          GND (or -24V for split supply)


INPUT (0–3.3V sine) ──[LM358 buffer]──+──[220Ω]──> Q1 Gate
                                       |
                                      [D1 1N4148] } bias stack
                                      [D2 1N4148] } sets ~0.6–1.2V
                                      [trim pot]  } quiescent
                                       |
                                       +──[220Ω]──> Q2 Gate
```

## Component Roles

| Component | Role |
|-----------|------|
| LM358 op-amp | Buffers the 0–3.3V C2000 signal; provides gate drive current |
| D1, D2 (1N4148) | Forward voltage drop creates gate-gate offset for Class AB bias |
| Trim pot (10kΩ) | Adjusts quiescent current to minimize crossover distortion |
| IRF540N | Sources current to load on positive half-cycle |
| IRF9540N | Sinks current from load on negative half-cycle |
| R_sense (0.05Ω 5W) | Optional: insert in series with output for current measurement |
| Heat sinks | Both MOSFETs dissipate (V_ds × I); mandatory at 10A |

## Operating Conditions

- Supply: 24V single-rail (or ±12V split)
- Output current: up to 10A continuous into 0.1–1Ω burden (relay CT secondary)
- Signal input: 60Hz sine, 0–3.3V from C2000 LaunchPad → RC filter
- Each MOSFET dissipates up to ~24W at 10A with 2.4V drop; use adequate heat sinking

## RC Input Filter (before amplifier input)

Two-pole filter to remove 100kHz PWM carrier:

```
C2000 PWM ──[1kΩ]──+──[1kΩ]──+── to LM358 input
                   |          |
                 [100nF]    [100nF]
                   |          |
                  GND        GND
```

Cutoff ≈ 1.6kHz (each pole). Two poles give ~40dB attenuation at 100kHz.

## Amplitude Control

Insert an X9C104 digital potentiometer as a voltage divider between the RC filter output
and the LM358 input. The C2000 controls the X9C104 via its INC/U-D/CS pins to set the
simulated fault current level (e.g., 2×, 6×, or 10× relay rated current).

## Verification Steps

1. Probe LaunchPad GPIO0/GPIO2/GPIO4 (J4 pins 40/39/38) — confirm 60Hz sine envelope with 120° shifts.
2. Probe after RC filter — confirm clean sine, no visible 100kHz ripple.
3. Bench test amplifier into 1Ω 50W resistor; measure output current with clamp meter.
   Target: 10A peak sine.
4. Connect to relay under test and observe trip characteristic vs. applied current.
