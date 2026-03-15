/*
 * sine_lut.h — 256-sample sine lookup table for C2000 ePWM
 *
 * Values scaled to EPWM_PERIOD (450) for 100kHz carrier on a 90MHz F28069M.
 * Formula: val[i] = round((sin(2*pi*i/256) + 1.0) * 225.0)
 * Range: 0 (0% duty) to 450 (100% duty), midpoint 225 (50%).
 *
 * Generated for:
 *   SYSCLK  = 90 MHz
 *   PWM_FREQ = 100 kHz  →  TBPRD = 90e6 / (2 * 100e3) = 450  (up-down count)
 *   SINE_FREQ = 60 Hz   →  update rate = 256 * 60 = 15,360 Hz
 */

#ifndef SINE_LUT_H
#define SINE_LUT_H

#include "DSP28x_Project.h"

#define EPWM_PERIOD       450U   /* TBPRD for 100kHz at 90MHz, up-down count   */
#define LUT_SIZE          256U   /* samples per sine cycle                      */
#define SINE_UPDATE_HZ    15360UL /* 256 * 60 Hz — CPU Timer 0 interrupt rate  */
#define SYSCLK_HZ         90000000UL

/* Phase offsets (samples): 0°, 120°, 240° at 256 samples/cycle */
#define PHASE_A_OFFSET    0U
#define PHASE_B_OFFSET    85U
#define PHASE_C_OFFSET    170U

/* Timer 0 period register value for SINE_UPDATE_HZ */
#define TIMER0_PRD        ((Uint32)(SYSCLK_HZ / SINE_UPDATE_HZ) - 1U)  /* 5858 */

/*
 * Sine LUT — 256 entries, uint16_t, range 0..EPWM_PERIOD (0..450).
 * Defined in sine_lut.h as a static const array; include in exactly one
 * translation unit (main.c). Guard prevents duplicate definition.
 */
#ifndef SINE_LUT_DEFINED
#define SINE_LUT_DEFINED

static const Uint16 sine_lut[256] = {
    /* i=0..7   */ 225, 231, 236, 242, 247, 253, 258, 263,
    /* i=8..15  */ 269, 274, 280, 285, 290, 296, 301, 306,
    /* i=16..23 */ 311, 316, 321, 326, 331, 336, 341, 345,
    /* i=24..31 */ 350, 355, 359, 363, 368, 372, 376, 380,
    /* i=32..39 */ 384, 388, 392, 396, 400, 404, 407, 410,
    /* i=40..47 */ 414, 417, 420, 423, 425, 428, 431, 433,
    /* i=48..55 */ 435, 437, 439, 441, 442, 444, 445, 446,
    /* i=56..63 */ 447, 448, 449, 449, 450, 450, 450, 450,
    /* i=64..71 */ 450, 450, 450, 450, 450, 449, 449, 448,
    /* i=72..79 */ 447, 446, 445, 444, 442, 441, 439, 437,
    /* i=80..87 */ 435, 433, 431, 428, 425, 423, 420, 417,
    /* i=88..95 */ 414, 410, 407, 404, 400, 396, 392, 388,
    /* i=96..103 */ 384, 380, 376, 372, 368, 363, 359, 355,
    /* i=104..111 */ 350, 345, 341, 336, 331, 326, 321, 316,
    /* i=112..119 */ 311, 306, 301, 296, 290, 285, 280, 274,
    /* i=120..127 */ 269, 263, 258, 253, 247, 242, 236, 231,
    /* i=128..135 */ 225, 219, 214, 208, 203, 197, 192, 187,
    /* i=136..143 */ 181, 176, 170, 165, 160, 154, 149, 144,
    /* i=144..151 */ 139, 134, 129, 124, 119, 114, 109, 105,
    /* i=152..159 */ 100,  95,  91,  87,  82,  78,  74,  70,
    /* i=160..167 */  66,  62,  58,  54,  50,  46,  43,  40,
    /* i=168..175 */  36,  33,  30,  27,  25,  22,  19,  17,
    /* i=176..183 */  15,  13,  11,   9,   8,   6,   5,   4,
    /* i=184..191 */   3,   2,   1,   1,   0,   0,   0,   0,
    /* i=192..199 */   0,   0,   0,   0,   0,   1,   1,   2,
    /* i=200..207 */   3,   4,   5,   6,   8,   9,  11,  13,
    /* i=208..215 */  15,  17,  19,  22,  25,  27,  30,  33,
    /* i=216..223 */  36,  40,  43,  46,  50,  54,  58,  62,
    /* i=224..231 */  66,  70,  74,  78,  82,  87,  91,  95,
    /* i=232..239 */ 100, 105, 109, 114, 119, 124, 129, 134,
    /* i=240..247 */ 139, 144, 149, 154, 160, 165, 170, 176,
    /* i=248..255 */ 181, 187, 192, 197, 203, 208, 214, 219
};

#endif /* SINE_LUT_DEFINED */

#endif /* SINE_LUT_H */
