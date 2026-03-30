# relay_test_set

3-Phase Secondary Relay Test Set

Generates three-phase 60Hz sine waves to simulate motor starting inrush current for
overcurrent relay testing. Amplitude, inrush profile, and decay are all controlled
programmatically via a serial console — no external digital potentiometer required.

## Signal Path

```
LAUNCHXL-F28069M
  GPIO0 (ePWM1A, Phase A,   0°) --> [RC Filter] --> [MOSFET Amp A] --> Relay CT input A
  GPIO2 (ePWM2A, Phase B, 120°) --> [RC Filter] --> [MOSFET Amp B] --> Relay CT input B
  GPIO4 (ePWM3A, Phase C, 240°) --> [RC Filter] --> [MOSFET Amp C] --> Relay CT input C

  GPIO28 (SCIRXDA) <-- USB serial console (backchannel UART via XDS100v2)
  GPIO29 (SCITXDA) --> USB serial console
```

The C2000 outputs 100kHz PWM on three ePWM channels. Duty cycle is updated at
15,360 Hz (256 steps × 60Hz) from a sine lookup table. The ISR scales each
channel's amplitude in real time, implementing the inrush profile in firmware.
RC filters remove the PWM carrier, leaving a clean 60Hz sine. MOSFET push-pull
amplifiers (IRF540N + IRF9540N) drive up to 10A into the relay CT burden.

## Platform

**TI LAUNCHXL-F28069M** — TMS320F28069M, 90MHz C28x DSP with FPU.
Bare-metal C, no OS.

- **IDE:** TI Code Composer Studio (CCS Theia)
- **SDK:** C2000Ware 26.x
- **Flash:** USB (XDS100v2 onboard debugger)

## Building and Flashing

```
1. Open CCS Theia, import the relay_test_set project (File → Import → CCS Project)
2. Build: Ctrl+B
3. Run → Debug to flash via USB
4. Run → Resume to start
5. Open a serial terminal at 9600-8-N-1 on the LaunchPad COM port
```

## Serial Console

Connect at **9600 baud, 8-N-1** on the USB COM port assigned to the LaunchPad.
The board boots with output **stopped**. Use the console to configure and run:

| Command | Description |
|---------|-------------|
| `start` | Begin output — triggers inrush from full scale, decays to steady-state |
| `stop` | Hold all outputs at DC midpoint (no sine output) |
| `amp <0.0–1.0>` | Set steady-state amplitude (fraction of full scale) |
| `decay <1–45>` | Set inrush decay time constant in seconds |
| `accel <0–20>` | Set initial drop speed multiplier (0 = pure exponential, 3 = default) |
| `status` | Print current state, amp, decay, accel, and live amplitude |

### Inrush Profile

On `start`, amplitude begins at full scale (1.0) and decays exponentially toward
the configured steady-state amplitude. The decay is **variable-rate**:

```
step = excess × (1 − decay_factor) × (1 + norm × accel)

  norm  = (amplitude − steady_amp) / (1.0 − steady_amp)   [1.0 at peak → 0 at steady]
  accel = speed multiplier at peak (default 3 → 4× faster than base rate at peak)
```

This produces a fast initial current drop (motor accelerating from stall) that
slows smoothly as the motor approaches running speed.

**Example session:**
```
> amp 0.3
amp set
> decay 10
decay set
> accel 5
accel set
> start
Output started (inrush active)
> status
State    : RUNNING
amp      : 0.3000
decay    : 10.0 sec (factor=0.99999350)
accel    : 5.0  (peak rate = 6.0x)
amp_now  : 0.6123
```

## Verification

1. Probe GPIO0/GPIO2/GPIO4 — confirm 100kHz PWM with a 60Hz amplitude envelope
   and 120° phase offsets between channels.
2. Probe after the RC filter — confirm a clean 60Hz sine with no visible PWM ripple.
3. Issue `start` on the console and watch amplitude decay on the oscilloscope.

## Files

```
main.c                   Firmware source (signal generator + serial console)
sine_lut.h               256-sample sine LUT scaled to ePWM period (0–450)
relay_test_set_lnk.cmd   Custom flash linker (stack placed in RAMM1)
.project / .cproject     CCS project configuration
targetConfigs/           Debug target (XDS100v2 + TMS320F28069)
hardware/
  bom.md                 Bill of materials
  amp_circuit.md         MOSFET amplifier circuit description
  schematic.txt          Full ASCII schematic (one channel shown)
datasheets/
  IRF540N, IRF9540N, AD5290, ad9833, x9c104 datasheets
Kicad/                   KiCad PCB design files
```

## Power Supply

Meanwell LRS-350-24 (24V / 14.6A) shared across all three amplifier channels.
A 7812 linear regulator supplies +12V for the op-amp rails.

## Relay CT Notes

- 1A CT secondary relay: target 6–10A output from the amplifier
- 5A CT secondary relay: use a 1:5 step-up CT to reach 30–50A equivalent fault current
