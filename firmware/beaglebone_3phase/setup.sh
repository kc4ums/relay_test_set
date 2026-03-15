#!/usr/bin/env bash
# setup.sh — BeagleBone Black first-time setup for relay test set
# Must be run as root (sudo bash setup.sh) before first use and after each reboot.

set -euo pipefail

if [[ $EUID -ne 0 ]]; then
    echo "ERROR: This script must be run as root (sudo bash setup.sh)" >&2
    exit 1
fi

# Check for config-pin (part of bb-cape-overlays / bone-universal-io)
if ! command -v config-pin &>/dev/null; then
    echo "config-pin not found — installing prerequisites..."
    apt-get update -q
    apt-get install -y python3-pip python3-dev bb-cape-overlays
fi

# Install Adafruit_BBIO if not already present
if ! python3 -c "import Adafruit_BBIO" &>/dev/null 2>&1; then
    echo "Installing Adafruit_BBIO..."
    pip3 install Adafruit_BBIO
else
    echo "Adafruit_BBIO already installed."
fi

# Configure PWM pins
echo "Configuring PWM pins..."
config-pin P9_22 pwm   # eHRPWM0A — Phase A (0°)
config-pin P9_14 pwm   # eHRPWM1A — Phase B (120°)
config-pin P8_19 pwm   # eHRPWM2A — Phase C (240°)

# Verify
echo "Verifying pin modes..."
for pin in P9_22 P9_14 P8_19; do
    mode=$(config-pin -q "$pin" 2>&1 | tail -1)
    echo "  $pin: $mode"
done

echo ""
echo "Setup complete."
echo ""
echo "To make pin configuration persistent across reboots, add the following"
echo "to /boot/uEnv.txt (edit manually — this script will NOT modify it):"
echo ""
echo "  cape_universal=enable"
echo ""
echo "Then add a udev rule or rc.local entry to re-run 'config-pin' at boot,"
echo "or use the systemd service approach described in the Adafruit_BBIO docs."
echo ""
echo "Run the signal generator with:"
echo "  sudo chrt -f 50 python3 beaglebone_3phase.py"
