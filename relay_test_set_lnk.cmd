/*
 * relay_test_set_lnk.cmd — Flash linker command file for relay_test_set
 *
 * Based on F28069M.cmd from C2000Ware.
 * Change from original: .stack moved from RAMM0 (0x3B0 words) to RAMM1
 * (0x400 words) to accommodate the default 0x400-word stack with sprintf.
 */

MEMORY
{
PAGE 0 :   /* Program Memory */
   RAML0       : origin = 0x008000, length = 0x000800
   RAML1       : origin = 0x008800, length = 0x000400
   OTP         : origin = 0x3D7800, length = 0x000400

   FLASHH      : origin = 0x3D8000, length = 0x004000
   FLASHG      : origin = 0x3DC000, length = 0x004000
   FLASHF      : origin = 0x3E0000, length = 0x004000
   FLASHE      : origin = 0x3E4000, length = 0x004000
   FLASHD      : origin = 0x3E8000, length = 0x004000
   FLASHC      : origin = 0x3EC000, length = 0x004000
   FLASHA_B    : origin = 0x3F0000, length = 0x007F80
   CSM_RSVD    : origin = 0x3F7F80, length = 0x000076
   BEGIN       : origin = 0x3F7FF6, length = 0x000002
   CSM_PWL_P0  : origin = 0x3F7FF8, length = 0x000008

   FPUTABLES   : origin = 0x3FD590, length = 0x0006A0
   IQTABLES    : origin = 0x3FDC30, length = 0x000B50
   IQTABLES2   : origin = 0x3FE780, length = 0x00008C
   IQTABLES3   : origin = 0x3FE80C, length = 0x0000AA

   ROM         : origin = 0x3FF3B0, length = 0x000C10
   RESET       : origin = 0x3FFFC0, length = 0x000002
   VECTORS     : origin = 0x3FFFC2, length = 0x00003E

PAGE 1 :   /* Data Memory */
   BOOT_RSVD   : origin = 0x000000, length = 0x000050
   RAMM0       : origin = 0x000050, length = 0x0003B0   /* boot/data use */
   RAMM1       : origin = 0x000400, length = 0x000400   /* stack lives here */
   RAML2_3     : origin = 0x008C00, length = 0x001400
   RAML4       : origin = 0x00A000, length = 0x002000
   RAML5       : origin = 0x00C000, length = 0x002000
   RAML6       : origin = 0x00E000, length = 0x002000
   RAML7       : origin = 0x010000, length = 0x002000
   RAML8       : origin = 0x012000, length = 0x001800
   USB_RAM     : origin = 0x040000, length = 0x000800
}

SECTIONS
{
   /* Program sections → Flash */
   .cinit              : > FLASHA_B,   PAGE = 0
   .pinit              : > FLASHA_B,   PAGE = 0
   .text               : > FLASHA_B,   PAGE = 0
   codestart           : > BEGIN,      PAGE = 0
   ramfuncs            : LOAD = FLASHD,
                         RUN  = RAML0,
                         LOAD_START(_RamfuncsLoadStart),
                         LOAD_END(_RamfuncsLoadEnd),
                         RUN_START(_RamfuncsRunStart),
                         PAGE = 0

   csmpasswds          : > CSM_PWL_P0, PAGE = 0
   csm_rsvd            : > CSM_RSVD,   PAGE = 0

   /* Uninitialized data → RAM */
   .stack              : > RAMM1,      PAGE = 1   /* moved: RAMM1=0x400 words */
   .ebss               : > RAML2_3,    PAGE = 1
   .esysmem            : > RAML2_3,    PAGE = 1

   /* Initialized data → Flash */
   .econst             : > FLASHA_B,   PAGE = 0
   .switch             : > FLASHA_B,   PAGE = 0

   /* Math tables */
   IQmath              : > FLASHA_B,   PAGE = 0
   IQmathTables        : > IQTABLES,   PAGE = 0, TYPE = NOLOAD
   FPUmathTables       : > FPUTABLES,  PAGE = 0, TYPE = NOLOAD

   DMARAML5            : > RAML5,      PAGE = 1
   DMARAML6            : > RAML6,      PAGE = 1
   DMARAML7            : > RAML7,      PAGE = 1
   DMARAML8            : > RAML8,      PAGE = 1
}
