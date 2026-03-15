# C2000 3-Phase Sine Generator

Generates three-phase 60Hz sine waves on a **LAUNCHXL-F28069M** LaunchPad using
hardware ePWM modules. No OS, no Linux, no Python — bare-metal C on a
purpose-built motor-control DSP.

## Target Hardware

- **Board:** LAUNCHXL-F28069M (~$20, Digi-Key/Mouser P/N: LAUNCHXL-F28069M)
- **MCU:** TMS320F28069M, 90MHz C28x DSP, 8 ePWM channels
- **Toolchain:** TI Code Composer Studio (CCS) 12.x — free download from ti.com
- **SDK:** C2000Ware 5.x — included with CCS or downloadable separately

## Output Pins (LaunchPad J4 header)

| Channel | GPIO | ePWM  | Phase  | J4 Pin |
|---------|------|-------|--------|--------|
| A       | GPIO0 | ePWM1A | 0°   | 40     |
| B       | GPIO2 | ePWM2A | 120° | 39     |
| C       | GPIO4 | ePWM3A | 240° | 38     |

PWM carrier: **100 kHz**. After the RC filter the output is a clean **60 Hz** sine.

## Firmware Parameters

| Parameter        | Value  | Notes                              |
|------------------|--------|------------------------------------|
| SYSCLK           | 90 MHz | PLL × 18 / 2 from 10 MHz XTAL     |
| TBPRD            | 450    | 90e6 / (2 × 100e3), up-down count |
| LUT size         | 256    | samples per sine cycle             |
| LUT update rate  | 15,360 Hz | 256 × 60 Hz, CPU Timer 0 ISR  |
| Phase B offset   | 85     | samples (≈ 120°)                   |
| Phase C offset   | 170    | samples (≈ 240°)                   |

## Setup: CCS Project

1. **Install CCS 12.x**
   - Download from https://www.ti.com/tool/CCSTUDIO
   - During install select the *C2000 real-time MCUs* component.

2. **Install C2000Ware**
   - CCS → Help → Browse Software Packages → install **C2000Ware**.
   - Or download standalone from https://www.ti.com/tool/C2000WARE and point CCS
     at it (Window → Preferences → Code Composer Studio → Products).

3. **Create a new CCS project**
   - File → New → CCS Project
   - Target: *TMS320F28069M*
   - Connection: *Texas Instruments XDS110 USB Debug Probe* (onboard)
   - Template: *Empty Project (with main.c)* — then delete the generated main.c

4. **Add source files**
   - Copy `main.c` and `sine_lut.h` from this directory into the project folder.
   - CCS should pick them up automatically (or right-click project → Add Files).

5. **Add C2000Ware include paths** (Project → Properties → Build → C2000 Compiler → Include Options):
   ```
   ${C2000WARE_ROOT}/device_support/f2806x/v151/DSP2806x_headers/include
   ${C2000WARE_ROOT}/device_support/f2806x/v151/DSP2806x_common/include
   ```
   Replace `${C2000WARE_ROOT}` with your actual C2000Ware installation path.

6. **Add C2000Ware source files** to the project (or link a C2000Ware library):
   Required files from C2000Ware `f2806x` device support:
   - `DSP2806x_SysCtrl.c`
   - `DSP2806x_PieCtrl.c`
   - `DSP2806x_PieVect.c`
   - `DSP2806x_CpuTimers.c`
   - `DSP2806x_GlobalVariableDefs.c`
   - `DSP2806x_usDelay.asm`

7. **Add linker command file**
   - Copy `F28069M.cmd` from C2000Ware:
     `device_support/f2806x/v151/DSP2806x_common/cmd/F28069M.cmd`

8. **Build**
   - Project → Build (Ctrl+B). Should compile with 0 errors.
   - If `DSP28x_Project.h` is not found, verify the include paths in step 5.

## Flashing

1. Plug the LaunchPad into USB. Windows should enumerate the XDS110 debugger.
2. In CCS, click **Run → Debug** (F11). CCS will flash the `.out` file via XDS110.
3. Click **Run → Resume** (F8) to start execution.
4. To reflash: Run → Terminate, rebuild, Run → Debug again.

> **No jumper changes needed.** The LaunchPad boots from flash by default.
> Hold the RESET button and release to restart without reflashing.

## Verification

Probe the output pins with an oscilloscope:

1. **GPIO0 (J4-40)**: 100 kHz PWM with a 60 Hz sine envelope, centered at 50% duty.
2. **GPIO2 (J4-39)**: same, 120° behind GPIO0.
3. **GPIO4 (J4-38)**: same, 240° behind GPIO0.
4. **After RC filter**: clean 60 Hz sine, peak-to-peak ≈ supply/2.
5. **Phase check**: channel B zero-crossing lags A by 1/180 s ≈ 5.56 ms at 60 Hz.

## Troubleshooting

| Symptom | Likely cause | Fix |
|---------|-------------|-----|
| No output on GPIO0 | GPIO mux not set | Check GPAMUX1.GPIO0 = 1 in debugger Registers view |
| All three channels in phase | Wrong LUT index init | Verify idx_b=85, idx_c=170 in main.c |
| Frequency wrong | PLL misconfigured | Check SYSCLK in CCS → Registers → SysCtrlRegs.PLLSTS |
| ISR not firing | PIE not enabled | Ensure PIEIER1.INTx7=1 and IER bit 0 set |
| CCS can't connect | XDS110 driver issue | Install XDS110 driver from ti.com, or try USB cable swap |
