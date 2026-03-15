import math
import signal
import sys
import time

import Adafruit_BBIO.PWM as PWM

SAMPLES     = 256
PWM_CARRIER = 100_000   # Hz
SIGNAL_HZ   = 60

# Sine LUT: 0.0–100.0% duty cycle (Adafruit_BBIO uses percent, not raw counts)
sine_lut = [(math.sin(2 * math.pi * i / SAMPLES) + 1) / 2 * 100.0
            for i in range(SAMPLES)]

# eHRPWM pins — one A-channel per module to avoid shared-timebase conflicts
#   P9_22 = eHRPWM0A  Phase A  0°
#   P9_14 = eHRPWM1A  Phase B  120°
#   P8_19 = eHRPWM2A  Phase C  240°
PHASE_PINS    = ["P9_22", "P9_14", "P8_19"]
PHASE_OFFSETS = [0, SAMPLES // 3, 2 * SAMPLES // 3]  # 0, 85, 170 samples

# Pre-shifted LUT per phase — avoids per-loop modulo arithmetic (mirrors pico_3phase_pio.py)
luts = [
    [sine_lut[(j + PHASE_OFFSETS[i]) % SAMPLES] for j in range(SAMPLES)]
    for i in range(3)
]

# Start PWM on all three pins (initial duty 50%, frequency 100kHz)
for pin in PHASE_PINS:
    PWM.start(pin, 50.0, PWM_CARRIER)


def shutdown(signum, frame):
    """Return outputs to midpoint (50%) and release PWM on SIGINT/SIGTERM."""
    for pin in PHASE_PINS:
        PWM.set_duty_cycle(pin, 50.0)
    PWM.cleanup()
    sys.exit(0)


signal.signal(signal.SIGINT, shutdown)
signal.signal(signal.SIGTERM, shutdown)

step_period = 1.0 / (SIGNAL_HZ * SAMPLES)   # ~65.1 µs per sample
idx = 0

while True:
    t0 = time.perf_counter()
    PWM.set_duty_cycle(PHASE_PINS[0], luts[0][idx])
    PWM.set_duty_cycle(PHASE_PINS[1], luts[1][idx])
    PWM.set_duty_cycle(PHASE_PINS[2], luts[2][idx])
    idx = (idx + 1) % SAMPLES
    elapsed = time.perf_counter() - t0
    sleep_s = step_period - elapsed
    if sleep_s > 0:
        time.sleep(sleep_s)
