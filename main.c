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
 * SCI-A console: GPIO28=RX, GPIO29=TX, 9600-8-N-1 (USB backchannel via XDS100v2)
 * Commands: start | stop | amp <0.0-1.0> | decay <0.0-1.0> | status
 *
 * Build: TI Code Composer Studio 12.x, C2000Ware 5.x, target F28069M.
 * Flash: USB (XDS110 onboard debugger on LaunchPad).
 */

#include "DSP28x_Project.h"
#include "sine_lut.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* ── Prototypes ─────────────────────────────────────────────────────────── */
static void init_gpio(void);
static void init_scia(void);
static void init_epwm(volatile struct EPWM_REGS *regs);
static void init_timer0(void);
static void scia_putch(Uint16 ch);
static void scia_puts(const char *str);
static void console_poll(void);
interrupt void timer0_isr(void);

/* ── LUT index per channel ──────────────────────────────────────────────── */
static volatile Uint16 idx_a = PHASE_A_OFFSET;
static volatile Uint16 idx_b = PHASE_B_OFFSET;
static volatile Uint16 idx_c = PHASE_C_OFFSET;

/* ── Inrush amplitude profile ───────────────────────────────────────────── *
 * Variable-rate exponential: step scales with normalized excess so the      *
 * drop is fastest at peak and slows smoothly as it nears steady state:      *
 *                                                                           *
 *   norm     = excess / (INRUSH_PEAK - g_steady_amp)   [1.0→0.0]           *
 *   per_tick = excess * (1 - factor) * (1 + norm * accel)                  *
 *   amplitude = amp - per_tick                                              *
 *                                                                           *
 * At peak (norm=1): rate = (1+accel) × base   — fast motor-start drop      *
 * At steady(norm=0): rate = 1 × base           — smooth final settle       *
 *                                                                           *
 * decay <s> sets base τ;  accel <n> sets initial speed multiplier          *
 * ─────────────────────────────────────────────────────────────────────── */
#define INRUSH_PEAK   1.0f   /* starting amplitude (full scale)              */

static volatile float g_steady_amp   = 0.25f;    /* steady-state target     */
static volatile float g_decay_factor = 0.9995f;  /* per-tick base decay     */
static volatile float g_accel        = 3.0f;     /* peak speed-up (0=off)   */
static float          g_decay_secs   = 0.0f;     /* last decay setting in s */
static volatile float amplitude      = INRUSH_PEAK;
static volatile Uint16 g_running     = 0;         /* 1=running, 0=stopped   */

/* ── Console command buffer ─────────────────────────────────────────────── */
#define CMD_BUF_LEN  32
static char cmd_buf[CMD_BUF_LEN];
static Uint16 cmd_idx = 0;

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
    init_scia();

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

    scia_puts("\r\n3-Phase Relay Test Set — SCI Console\r\n");
    scia_puts("Commands: start | stop | amp <0.0-1.0> | decay <1-45 sec> | accel <0-20> | status\r\n> ");

    /* Poll SCI console — all waveform work done in timer0_isr */
    for (;;)
    {
        console_poll();
    }
}

/* ========================================================================
 * timer0_isr — fires at 15,360 Hz, updates ePWM duty cycles from LUT
 * ======================================================================== */
interrupt void timer0_isr(void)
{
    Uint16 half = EPWM_PERIOD / 2U;  /* 225 — DC midpoint */

    if (g_running)
    {
        float amp = amplitude;
        float ss  = g_steady_amp;

        /* Scale each channel: CMPA = 225 + (lut - 225) * amplitude */
        EPwm1Regs.CMPA.half.CMPA = (Uint16)(half + (int16)(((int16)sine_lut[idx_a] - (int16)half) * amp));
        EPwm2Regs.CMPA.half.CMPA = (Uint16)(half + (int16)(((int16)sine_lut[idx_b] - (int16)half) * amp));
        EPwm3Regs.CMPA.half.CMPA = (Uint16)(half + (int16)(((int16)sine_lut[idx_c] - (int16)half) * amp));

        /* Variable-rate decay: faster near peak, slower near steady state   */
        if (amp > ss)
        {
            float excess = amp - ss;
            float norm   = excess / (INRUSH_PEAK - ss);  /* 1.0 peak → 0 steady */
            float step   = excess * (1.0f - g_decay_factor) * (1.0f + norm * g_accel);
            float next   = amp - step;
            amplitude = (next < ss) ? ss : next;
        }

        /* Advance indices, wrap at 256 */
        idx_a = (idx_a + 1U) & 0xFFU;
        idx_b = (idx_b + 1U) & 0xFFU;
        idx_c = (idx_c + 1U) & 0xFFU;
    }
    else
    {
        /* Output stopped — hold all channels at DC midpoint */
        EPwm1Regs.CMPA.half.CMPA = half;
        EPwm2Regs.CMPA.half.CMPA = half;
        EPwm3Regs.CMPA.half.CMPA = half;
    }

    /* Acknowledge PIE group 1 so further group-1 interrupts can fire */
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;
}

/* ========================================================================
 * init_gpio — mux GPIO0/2/4 to ePWM1A/2A/3A
 * (SCI-A GPIO28/29 are configured in init_scia via InitSciaGpio)
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
 * init_scia — SCI-A at 9600-8-N-1; GPIO28=RX, GPIO29=TX
 *
 * LSPCLK = SYSCLK / 4 = 22.5 MHz
 * BRR = LSPCLK / (baud × 8) − 1 = 22,500,000 / (9600 × 8) − 1 ≈ 292
 * SCIHBAUD = 0x01, SCILBAUD = 0x24  (0x0124 = 292)
 * ======================================================================== */
static void init_scia(void)
{
    EALLOW;
    SysCtrlRegs.PCLKCR0.bit.SCIAENCLK = 1;
    EDIS;

    InitSciaGpio();  /* GPIO28 = SCIRXDA, GPIO29 = SCITXDA (from F2806x_Sci.c) */

    SciaRegs.SCICCR.all  = 0x0007;   /* 1 stop bit, no parity, 8-bit char    */
    SciaRegs.SCICTL1.all = 0x0003;   /* TX/RX enable; hold in reset          */
    SciaRegs.SCICTL2.all = 0x0000;   /* TX and RX interrupts disabled        */

    SciaRegs.SCIHBAUD = 0x01;        /* BRR = 292 = 0x0124                   */
    SciaRegs.SCILBAUD = 0x24;

    SciaRegs.SCICTL1.all = 0x0023;   /* Release SCI from reset               */
}

/* ========================================================================
 * scia_putch / scia_puts — blocking SCI-A transmit helpers
 * ======================================================================== */
static void scia_putch(Uint16 ch)
{
    while (SciaRegs.SCICTL2.bit.TXRDY == 0) {}
    SciaRegs.SCITXBUF = ch & 0xFF;
}

static void scia_puts(const char *str)
{
    while (*str)
        scia_putch((Uint16)*str++);
}

/* ========================================================================
 * console_poll — called from main loop; processes one received char
 *
 * Commands (terminated by \r or \n):
 *   start            — begin output with inrush from INRUSH_PEAK
 *   stop             — hold outputs at DC midpoint (no sine)
 *   amp  <f>         — set steady-state amplitude  (0.0 – 1.0)
 *   decay <s>        — set inrush decay time constant (1 – 45 seconds)
 *   status           — print current settings
 * ======================================================================== */
static void console_poll(void)
{
    char tmp[56];
    char ch;

    if (!SciaRegs.SCIRXST.bit.RXRDY)
        return;

    ch = (char)(SciaRegs.SCIRXBUF.all & 0xFF);

    /* Backspace / DEL */
    if (ch == '\b' || ch == 0x7F)
    {
        if (cmd_idx > 0)
        {
            cmd_idx--;
            scia_puts("\b \b");
        }
        return;
    }

    /* Echo character */
    scia_putch((Uint16)ch);

    if (ch == '\r' || ch == '\n')
    {
        scia_puts("\r\n");
        cmd_buf[cmd_idx] = '\0';
        cmd_idx = 0;

        /* ── Parse ──────────────────────────────────────────────────────── */
        if (strcmp(cmd_buf, "start") == 0)
        {
            amplitude = INRUSH_PEAK;
            idx_a = PHASE_A_OFFSET;
            idx_b = PHASE_B_OFFSET;
            idx_c = PHASE_C_OFFSET;
            g_running = 1;
            scia_puts("Output started (inrush active)\r\n");
        }
        else if (strcmp(cmd_buf, "stop") == 0)
        {
            g_running = 0;
            scia_puts("Output stopped\r\n");
        }
        else if (strncmp(cmd_buf, "amp ", 4) == 0)
        {
            float val = (float)atof(cmd_buf + 4);
            if (val >= 0.0f && val <= 1.0f)
            {
                g_steady_amp = val;
                scia_puts("amp set\r\n");
            }
            else
            {
                scia_puts("Error: amp must be 0.0 to 1.0\r\n");
            }
        }
        else if (strncmp(cmd_buf, "decay ", 6) == 0)
        {
            float val = (float)atof(cmd_buf + 6);
            if (val >= 1.0f && val <= 45.0f)
            {
                g_decay_secs   = val;
                /* exp(-x) ≈ 1-x for small x; error < 0.0001% for τ ≥ 1s   */
                g_decay_factor = 1.0f - 1.0f / (val * (float)SINE_UPDATE_HZ);
                scia_puts("decay set\r\n");
            }
            else
            {
                scia_puts("Error: decay must be 1 to 45 seconds\r\n");
            }
        }
        else if (strncmp(cmd_buf, "accel ", 6) == 0)
        {
            float val = (float)atof(cmd_buf + 6);
            if (val >= 0.0f && val <= 20.0f)
            {
                g_accel = val;
                scia_puts("accel set\r\n");
            }
            else
            {
                scia_puts("Error: accel must be 0 to 20\r\n");
            }
        }
        else if (strcmp(cmd_buf, "status") == 0)
        {
            /* Use %d only — %f causes stack overflow on C2000 */
            int w, f;

            scia_puts("State    : ");
            scia_puts(g_running ? "RUNNING\r\n" : "STOPPED\r\n");

            /* amp (4 dp) */
            w = (int)g_steady_amp;
            f = (int)((g_steady_amp - (float)w) * 10000.0f + 0.5f);
            sprintf(tmp, "amp      : %d.%04d\r\n", w, f);
            scia_puts(tmp);

            /* decay */
            if (g_decay_secs > 0.0f)
            {
                int sw = (int)g_decay_secs;
                int sf = (int)((g_decay_secs - (float)sw) * 10.0f + 0.5f);
                w = (int)g_decay_factor;
                f = (int)((g_decay_factor - (float)w) * 100000000.0f + 0.5f);
                sprintf(tmp, "decay    : %d.%d sec (factor=%d.%08d)\r\n", sw, sf, w, f);
            }
            else
            {
                w = (int)g_decay_factor;
                f = (int)((g_decay_factor - (float)w) * 100000000.0f + 0.5f);
                sprintf(tmp, "decay    : (default factor=%d.%08d)\r\n", w, f);
            }
            scia_puts(tmp);

            /* accel (1 dp) */
            w = (int)g_accel;
            f = (int)((g_accel - (float)w) * 10.0f + 0.5f);
            int pw = (int)(1.0f + g_accel);
            int pf = (int)((1.0f + g_accel - (float)pw) * 10.0f + 0.5f);
            sprintf(tmp, "accel    : %d.%d  (peak rate = %d.%dx)\r\n", w, f, pw, pf);
            scia_puts(tmp);

            /* amp_now (4 dp) */
            w = (int)amplitude;
            f = (int)((amplitude - (float)w) * 10000.0f + 0.5f);
            sprintf(tmp, "amp_now  : %d.%04d\r\n", w, f);
            scia_puts(tmp);
        }
        else if (cmd_buf[0] != '\0')
        {
            scia_puts("Unknown. Try: start | stop | amp <f> | decay <s> | accel <n> | status\r\n");
        }

        scia_puts("> ");
    }
    else if (cmd_idx < CMD_BUF_LEN - 1)
    {
        cmd_buf[cmd_idx++] = ch;
    }
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
