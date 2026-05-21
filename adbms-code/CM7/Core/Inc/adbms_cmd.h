#include <stdint.h>


enum ADBMS_CMDS : uint16_t {
    /* configuration registers commands */
    WRCFGA        = 0x0001,
    WRCFGB        = 0x0024,
    RDCFGA        = 0x0002,
    RDCFGB        = 0x0026,

    /* Read cell voltage result registers commands */
    RDCVA         = 0x0004,
    RDCVB         = 0x0006,
    RDCVC         = 0x0008,
    RDCVD         = 0x000A,
    RDCVE         = 0x0009,
    RDCVF         = 0x000B,

    /* Read average cell voltage result registers commands commands */
    RDACA         = 0x0044,
    RDACB         = 0x0046,
    RDACC         = 0x0048,
    RDACD         = 0x004A,
    RDACE         = 0x0049,
    RDACF         = 0x004B,

    /* Read s voltage result registers commands */
    RDSVA         = 0x0003,
    RDSVB         = 0x0005,
    RDSVC         = 0x0007,
    RDSVD         = 0x000D,
    RDSVE         = 0x000E,
    RDSVF         = 0x000F,

    /* Read filtered cell voltage result registers*/
    RDFCA         = 0x0012,
    RDFCB         = 0x0013,
    RDFCC         = 0x0014,
    RDFCD         = 0x0015,
    RDFCE         = 0x0016,
    RDFCF         = 0x0017,

    /* Read aux results */
    RDAUXA        = 0x0019,
    RDAUXB        = 0x001A,
    RDAUXC        = 0x001B,
    RDAUXD        = 0x001F,

    /* Read redundant aux results */
    RDRAXA        = 0x001C,
    RDRAXB        = 0x001D,
    RDRAXC        = 0x001E,
    RDRAXD        = 0x0025,

    /* Read status registers */
    RDSTATA       = 0x0030,
    RDSTATB       = 0x0031,
    RDSTATC       = 0x0032,
    RDSTATD       = 0x0033,
    RDSTATE       = 0x0034,

    /* Pwm registers commands */
    WRPWM1        = 0x0020,
    RDPWM1        = 0x0022,
    WRPWM2        = 0x0021,
    RDPWM2        = 0x0023,

    /* Clear commands */
    CLRCELL       = 0x0711,
    CLRAUX        = 0x0712,
    CLRSPIN       = 0x0716,
    CLRFLAG       = 0x0717,
    CLRFC         = 0x0714,
    CLOVUV        = 0x0715,

    /* Poll adc command */
    PLADC         = 0x0718,
    PLAUT         = 0x0719,
    PLCADC        = 0x071C,
    PLSADC        = 0x071D,
    PLAUX1        = 0x071E,
    PLAUX2        = 0x071F,

    /* GPIOs Comm commands */
    WRCOMM        = 0x0721,
    RDCOMM        = 0x0722,
    STCOMM        = 0x0723,

    /* Mute and Unmute commands */
    MUTE 	       = 0x0028,
    UNMUTE        = 0x0029,

    /* Read command counter command */
    RSTCC         = 0x002E,

    /* Snap commands */
    SNAP          = 0x002D,
    UNSNAP        = 0x002F,
    SRST          = 0x0027,

    /* Read Serial ID command */
    RDSID         = 0x002C,
};