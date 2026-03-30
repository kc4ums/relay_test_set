/*
 * main.c — 3-phase 60Hz sine generator for TMS320F28069M (LAUNCHXL-F28069M)
 *
 * Signal path:
 *   ePWM1A (GPIO0) → RC filter → X9C104 → MOSFET amp → relay CT input A (0°)
 *   ePWM2A (GPIO2) → RC filter → X9C104 → MOSFET amp → relay CT input B (120°)
 *   ePWM3A (GPIO4) → RC filter → X9C104 → MOSFET amp → relay CT input C (240°)
 *
 * PWM carrier:  100 kHz (TBPRD = 450, up-down count, 90 MHz SYSCLK)
 * Sine output:  60 Hz (256-sample LUT updated at 15,360 Hz via CPU Timer 0)
 * Phase offsets: 0, 85, 170 LUT samples (0°, 120°, 240°)
 *
 * Build: TI Code Composer Studio 12.x, C2000Ware 5.x, target F28069M.
 * Flash: USB (XDS110 onboard debugger on LaunchPad).
 */

#include "DSP28x_Project.h"
#include "sine_lut.h"

/* ── Prototypes ─────────────────────────────────────────────────────────── */
static void init_sysclk(void);
static void init_gpio(void);
static void init_epwm(volatile struct EPWM_REGS *regs);
static void init_timer0(void);
interrupt void timer0_isr(void);

/* ── LUT index per channel ──────────────────────────────────────────────── */
static volatile Uint16 idx_a = PHASE_A_OFFSET;
static volatile Uint16 idx_b = PHASE_B_OFFSET;
static volatile Uint16 idx_c = PHASE_C_OFFSET;

/* ========================================================================
 * main
 * ======================================================================== */
void main(void)
{
    /* C2000Ware boot sequence */
    InitSysCtrl();          /* sets SYSCLK to 90 MHz via PLL (10 MHz XTAL)  */
    DINT;                   /* disable global interrupts during init          */
    InitPieCtrl();          /* clear PIE control regs, disable all PIE ints  */
    IER = 0x0000;
    IFR = 0x0000;
    InitPieVectTable();     /* populate PIE vector table with default ISRs   */

    /* Wire our ISR into the PIE vector table (must be after InitPieVectTable) */
    EALLOW;
    PieVectTable.TINT0 = &timer0_isr;  /* CPU Timer 0 → PIE group 1, INT7   */
    EDIS;

    /* Peripheral init */
    init_gpio();

    EALLOW;
    SysCtrlRegs.PCLKCR1.bit.EPWM1ENCLK = 1;
    SysCtrlRegs.PCLKCR1.bit.EPWM2ENCLK = 1;
    SysCtrlRegs.PCLKCR1.bit.EPWM3ENCLK = 1;
    /* Stagger ePWM clock enable — let TBCLK sync before enabling modules */
    SysCtrlRegs.PCLKCR0.bit.TBCLKSYNC = 0;
    EDIS;

    init_epwm(&EPwm1Regs);
    init_epwm(&EPwm2Regs);
    init_epwm(&EPwm3Regs);

    EALLOW;
    SysCtrlRegs.PCLKCR0.bit.TBCLKSYNC = 1;  /* start all ePWM time-bases    */
    EDIS;

    init_timer0();

    /* Enable PIE group 1 INT7 (TINT0) and CPU INT1 */
    PieCtrlRegs.PIEIER1.bit.INTx7 = 1;
    IER |= M_INT1;

    EINT;   /* enable global interrupts */
    ERTM;   /* enable real-time debug   */

    /* Spin forever — all work done in timer0_isr */
    for (;;)
    {
        __asm(" NOP");
    }
}

/* ========================================================================
 * timer0_isr — fires at 15,360 Hz, updates ePWM duty cycles from LUT
 * ======================================================================== */
interrupt void timer0_isr(void)
{
    /* Update CMPA registers directly — no shadow register latency at 15 kHz */
    EPwm1Regs.CMPA.half.CMPA = sine_lut[idx_a];
    EPwm2Regs.CMPA.half.CMPA = sine_lut[idx_b];
    EPwm3Regs.CMPA.half.CMPA = sine_lut[idx_c];

    /* Advance indices, wrap at 256 */
    idx_a = (idx_a + 1U) & 0xFFU;
    idx_b = (idx_b + 1U) & 0xFFU;
    idx_c = (idx_c + 1U) & 0xFFU;

    /* Acknowledge PIE group 1 so further group-1 interrupts can fire */
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;
}

/* ========================================================================
 * init_gpio — mux GPIO0/2/4 to ePWM1A/2A/3A
 * ======================================================================== */
static void init_gpio(void)
{
    EALLOW;
    /* GPIO0 → ePWM1A (MUX value 1) */
    GpioCtrlRegs.GPAMUX1.bit.GPIO0 = 1;
    GpioCtrlRegs.GPADIR.bit.GPIO0  = 1;

    /* GPIO2 → ePWM2A */
    GpioCtrlRegs.GPAMUX1.bit.GPIO2 = 1;
    GpioCtrlRegs.GPADIR.bit.GPIO2  = 1;

    /* GPIO4 → ePWM3A */
    GpioCtrlRegs.GPAMUX1.bit.GPIO4 = 1;
    GpioCtrlRegs.GPADIR.bit.GPIO4  = 1;
    EDIS;
}

/* ========================================================================
 * init_epwm — configure one ePWM module for 100kHz up-down PWM
 *
 * CMPA = EPWM_PERIOD/2 (50% duty) at start; ISR will update each cycle.
 * Action qualifier: set on CMPA count-up, clear on CMPA count-down →
 *   duty cycle = CMPA / TBPRD (0% at CMPA=0, 100% at CMPA=TBPRD).
 * ======================================================================== */
static void init_epwm(volatile struct EPWM_REGS *regs)
{
    /* Time-base */
    regs->TBPRD             = EPWM_PERIOD;          /* 450 → 100 kHz         */
    regs->TBPHS.half.TBPHS  = 0;                    /* no hardware phase shift */
    regs->TBCTR             = 0;                    /* clear counter          */
    regs->TBCTL.bit.CTRMODE = TB_COUNT_UPDOWN;
    regs->TBCTL.bit.PHSEN   = TB_DISABLE;
    regs->TBCTL.bit.HSPCLKDIV = TB_DIV1;
    regs->TBCTL.bit.CLKDIV    = TB_DIV1;
    regs->TBCTL.bit.SYNCOSEL  = TB_SYNC_DISABLE;

    /* Compare — start at midpoint (50%) */
    regs->CMPA.half.CMPA = EPWM_PERIOD / 2U;       /* 225                    */

    /* Action qualifier for output A */
    regs->AQCTLA.bit.CAU = AQ_SET;    /* set high when counting up, CTR=CMPA */
    regs->AQCTLA.bit.CAD = AQ_CLEAR;  /* set low when counting down, CTR=CMPA */

    /* Dead-band, chopper, trip-zone: all disabled (defaults after reset) */
    regs->DBCTL.bit.OUT_MODE = DB_DISABLE;
}

/* ========================================================================
 * init_timer0 — CPU Timer 0 at SINE_UPDATE_HZ (15,360 Hz)
 * ======================================================================== */
static void init_timer0(void)
{
    InitCpuTimers();  /* C2000Ware helper: zeros all timer registers          */

    /* Period = SYSCLK / f_int = 90,000,000 / 15,360 = 5859 cycles          */
    /* PRD+1 cycles per interrupt, so PRD = 5858                             */
    CpuTimer0Regs.PRD.all  = TIMER0_PRD;   /* 5858                          */
    CpuTimer0Regs.TPR.all  = 0;            /* prescaler = /1                 */
    CpuTimer0Regs.TPRH.all = 0;

    CpuTimer0Regs.TCR.bit.TIE  = 1;        /* enable timer interrupt         */
    CpuTimer0Regs.TCR.bit.TSS  = 0;        /* start timer (0 = run)          */
    CpuTimer0Regs.TCR.bit.TRB  = 1;        /* reload from PRD                */
    CpuTimer0Regs.TCR.bit.FREE = 1;        /* free-run during debug halt     */
}
