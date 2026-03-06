import machine, math, time

SAMPLES     = 256
PWM_CARRIER = 100_000   # Hz
SIGNAL_HZ   = 60

# Precompute sine table (0 to 65535)
sine_lut = [int((math.sin(2 * math.pi * i / SAMPLES) + 1) * 32767)
            for i in range(SAMPLES)]

phase_pins    = [0, 2, 4]
phase_offsets = [0, SAMPLES // 3, 2 * SAMPLES // 3]  # 0°, 120°, 240°

pwms = []
for pin in phase_pins:
    p = machine.PWM(machine.Pin(pin))
    p.freq(PWM_CARRIER)
    pwms.append(p)

step_period = 1.0 / (SIGNAL_HZ * SAMPLES)   # ~65µs
idx = 0

while True:
    t0 = time.ticks_us()
    for i, pwm in enumerate(pwms):
        pwm.duty_u16(sine_lut[(idx + phase_offsets[i]) % SAMPLES])
    idx = (idx + 1) % SAMPLES
    elapsed = time.ticks_diff(time.ticks_us(), t0)
    sleep_us = int(step_period * 1_000_000) - elapsed
    if sleep_us > 0:
        time.sleep_us(sleep_us)
