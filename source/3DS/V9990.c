/** Completely rewritten by Alex Wulms:                     **/
/**  - VDP Command execution 'in parallel' with CPU         **/
/**  - Corrected behaviour of VDP commands                  **/
/**  - Made it easier to implement correct S7/8 mapping     **/
/**    by concentrating VRAM access in one single place     **/
/**  - Made use of the 'in parallel' VDP command exec       **/
/**    and correct timing. You must call the function       **/
/**    LoopVDP() from LoopZ80 in MSX.c. You must call it    **/
/**    exactly 256 times per screen refresh.                **/
/** Started on       : 11-11-1999                           **/
/** Beta release 1 on:  9-12-1999                           **/
/** Beta release 2 on: 20-01-2000                           **/
/**  - Corrected behaviour of VRM <-> Z80 transfer          **/
/**  - Improved performance of the code                     **/
/** Public release 1.0: 20-04-2000                          **/
/*************************************************************/

/** Modified for V9990 VDP h.tomioka 2024         **/

#include "V9990.h"
#include <string.h>

/*************************************************************/
/** Other usefull defines                                   **/
/*************************************************************/
#define VDP_VRMP_BP4(X, Y) (V9KVRAM + (((Y<<7) + (X>>1))&0x7FFFF))
#define VDP_VRMP_512(X, Y) (V9KVRAM + (((Y<<8) + (X>>1))&0x7FFFF))
#define VDP_VRMP_1024(X, Y) (V9KVRAM + (((Y<<9) + (X>>1))&0x7FFFF))
#define VDP_VRMP_BPP8(X, Y) (V9KVRAM + (((Y<<8) + X)&0x7FFFF))

#define VDP_VRMP(M, X, Y) VDPVRMP(M, X, Y)
#define VDP_POINT(M, X, Y) VDPpoint(M, X, Y)
#define VDP_PSET(M, X, Y, C, O) VDPpset(M, X, Y, C, O)

#define CMV9990_STOP  0x00
#define CMV9990_LMMC  0x01
#define CMV9990_LMMV  0x02
#define CMV9990_LMCM  0x03
#define CMV9990_LMMM  0x04
#define CMV9990_CMMC  0x05
#define CMV9990_CMMK  0x06
#define CMV9990_CMMM  0x07
#define CMV9990_BMXL  0x08
#define CMV9990_BMLX  0x09
#define CMV9990_BMLL  0x0A
#define CMV9990_LINE  0x0B
#define CMV9990_SRCH  0x0C
#define CMV9990_POINT 0x0D
#define CMV9990_PSET  0x0E
#define CMV9990_ADVN  0x0F

/*************************************************************/
/* Many VDP commands are executed in some kind of loop but   */
/* essentially, there are only a few basic loop structures   */
/* that are re-used. We define the loop structures that are  */
/* re-used here so that they have to be entered only once    */
/*************************************************************/
#define pre_loop \
    while ((cnt-=delta) > 0) {

/* Loop over DX, DY */
#define post__x_y_g9k(MX, MY) \
    if (!--ANX || ((ADX+=TX)&MX)) { \
      if (!(--NY&(MY-1)) || (DY+=TY)==-1) \
        break; \
      else { \
        ADX=DX; \
        ANX=NX; \
      } \
    } \
  }

#define post__x_ys_g9k(MX, MY) \
    if (!--ANX || ((ASX+=TX)&MX)) { \
      if (!(--NY&(MY-1)) || (SY+=TY)==-1) \
        break; \
      else { \
        ASX=SX; \
        ANX=NX; \
      } \
    } \
  }

/* Loop over SX, DX, SY, DY */
#define post_xxyy_g9k(MX, MY) \
    ASX = (ASX+TX)&(MX-1); ADX = (ADX+TX)&(MX-1); \
    if (!--ANX) { \
      if (!(--NY&(MY-1)) || (SY+=TY)==-1 || (DY+=TY)==-1) \
        break; \
      else { \
        ASX=SX; \
        ADX=DX; \
        ANX=NX; \
      } \
    } \
  }

/* MACRO for VDP Commmand to use const value for LO. It improve speed a bit. */
#define SWITCHCMD       switch (LO) { \
    case 0x00: DOCMD(0x00) case 0x01: DOCMD(0x01) case 0x02: DOCMD(0x02) case 0x03: DOCMD(0x03) \
    case 0x04: DOCMD(0x04) case 0x05: DOCMD(0x05) case 0x06: DOCMD(0x06) case 0x07: DOCMD(0x07) \
    case 0x08: DOCMD(0x08) case 0x09: DOCMD(0x09) case 0x0A: DOCMD(0x0A) case 0x0B: DOCMD(0x0B) \
    case 0x0C: DOCMD(0x0C) case 0x0D: DOCMD(0x0D) case 0x0E: DOCMD(0x0E) case 0x0F: DOCMD(0x0F) \
    case 0x10: DOCMD(0x10) case 0x11: DOCMD(0x11) case 0x12: DOCMD(0x12) case 0x13: DOCMD(0x13) \
    case 0x14: DOCMD(0x14) case 0x15: DOCMD(0x15) case 0x16: DOCMD(0x16) case 0x17: DOCMD(0x17) \
    case 0x18: DOCMD(0x18) case 0x19: DOCMD(0x19) case 0x1A: DOCMD(0x1A) case 0x1B: DOCMD(0x1B) \
    case 0x1C: DOCMD(0x1C) case 0x1D: DOCMD(0x1D) case 0x1E: DOCMD(0x1E) case 0x1F: DOCMD(0x1F) \
    default:break;}

/*************************************************************/
/** Structures and stuff                                    **/
/*************************************************************/
static struct {
    int SX, SY;
    int DX, DY;
    int TX, TY;
    int NX, NY;
    int MX;
    int ASX, ADX, ANX;
    int SA, DA, NA;
    int RectPos;
    byte LO;
    byte CM;
    byte RectData;
} MMCV9990;

/*************************************************************/
/** Function prototypes                                     **/
/*************************************************************/
static byte VDPpointBP4(register int SX, register int SY);
static byte VDPpoint512(register int SX, register int SY);
static byte VDPpoint1024(register int SX, register int SY);
static word VDPpointBD16(int SX, int SY);
static word VDPpointBD16_512(int SX, int SY);
static void V9Kpsetlowlevel(register byte* P, register byte CL,
    register byte M, register byte OP);

static void VDPpsetBP4(register int DX, register int DY,
    register byte CL, register byte OP);
static void VDPpsetBP4_512(register int DX, register int DY,
    register byte CL, register byte OP);
static void VDPpsetBP4_1024(register int DX, register int DY,
    register byte CL, register byte OP);
static void VDPpsetBD16(register int DX, register int DY,
    register word CW, register byte OP);
static void VDPpsetBD16_512(register int DX, register int DY,
    register word CW, register byte OP);
static void VDPpset16BP4(register int DX, register int DY,
    register int SX, register word CW, register byte OP);
static void VDPpset16BP4_512(register int DX, register int DY,
    register int SX, register word CW, register byte OP);
static void VDPpset16BP4_1024(register int DX, register int DY,
    register int SX, register word CW, register byte OP);

static void LmmcEngineV9990(void);
static void LmmvEngineV9990(void);
static void LmcmEngineV9990(void);
static void LmmmEngineV9990(void);
static void CmmcEngineV9990(void);
static void CmmmEngineV9990(void);
static void BmxlEngineV9990(void);
static void BmlxEngineV9990(void);
static void BmllEngineV9990(void);
static void LineEngineV9990(void);
static void SrchEngineV9990(void);
static void PointEngineV9990(void);
static void PsetEngineV9990(void);
static void AdvnEngineV9990(void);

/*************************************************************/
/** Variables visible only in this module                   **/
/*************************************************************/
static int  VdpOpsCntV9990 = 1;
static int V9990PrevData = -1;
static void (*VdpEngineV9990)(void) = 0;


/** VDPpointBP4 **********************************************/
/** Get a pixel on BP4 Mode                                 **/
/*************************************************************/
INLINE byte VDPpointBP4(int SX, int SY)
{
    return (*VDP_VRMP_BP4(SX, SY) >>
        (((~SX) & 1) << 2)
        ) & 15;
}

INLINE byte VDPpoint512(int SX, int SY)
{
    return (*VDP_VRMP_512(SX, SY) >>
        (((~SX) & 1) << 2)
        ) & 15;
}

INLINE byte VDPpoint1024(int SX, int SY)
{
    return (*VDP_VRMP_1024(SX, SY) >>
        (((~SX) & 1) << 2)
        ) & 15;
}

INLINE word VDPpointBD16(int SX, int SY)
{
    return(
        (*(V9KVRAM + (((SY << 9) + (SX << 1)) & 0x7FFFF)) << 8) |
        *(V9KVRAM + (((SY << 9) + (SX << 1) + 1) & 0x7FFFF))
        );
}

INLINE word VDPpointBD16_512(int SX, int SY)
{
    return(
        (*(V9KVRAM + (((SY << 10) + (SX << 1)) & 0x7FFFF))<<8) |
        *(V9KVRAM + (((SY << 10) + (SX << 1) + 1) & 0x7FFFF))
        );
}


/** VDPpsetlowlevel() ****************************************/
/** Low level function to set a pixel on a screen           **/
/** Make it inline to make it fast                          **/
/*************************************************************/
INLINE void V9Kpsetlowlevel(byte* P, byte CL, byte M, byte OP)
{
    if ((OP & 0x10))
    {
        if (!CL)return;
    }
    switch (OP&0x0F)
    {
    case 0:     /* NULL */
        *P = *P & M;
        break;
    case 1:     /* NOR*/
        //*P = (*P & M) | ~((*P | CL) | M);
        *P = (*P & M) | ~(*P | CL);
        break;
    case 2:     /* EXD */
       // *P = *P & ~(CL | M);
        *P &= ~CL;
        break;
    case 3:     /* NOTS */
        //*P = (*P & M) | ~(CL | M);
        *P = (*P & M) | ~CL;
        break;
    case 4: /* EXS */
        //*P = (*P & M) | ~((~CL | *P) | M);
        *P = (*P & M) | (CL & ~(*P));
        break;
    case 5:     /* NOTD */
        //*P = (*P & M) | ~(*P | M);
        *P = (*P & M) | ~*P;
        break;
    case 6:     /* XOR */
        //*P = *P ^ (~(~CL | M));
        *P = *P ^ CL;
        break;
    case 7:     /* NAND */
        //*P = (*P & M) | ~((*P & CL) | M);
        *P = (*P & M) | ~(*P & CL);
        break;
    case 8:     /* AND */
        //*P = *P & (CL | M);
        *P &= CL;
        break;
    case 9:     /* EQV */
        //*P = (*P & M) | ~((*P ^ CL) | M);
        *P = (*P & M) | (~(*P ^ CL));
        break;
    case 10:    /* D */
        //*P = *P;
        break;
    case 11:    /* NEXS */
        //*P = (*P & M) | ~((CL & *P) | M);
        *P = (*P & M) | (~CL | *P);
        break;
    case 12:    /* S */
        //*P = (*P & M) | ~(~CL | M);
        *P = (*P & M) | CL;
        break;
    case 13:    /* NEXD */
        //*P = *P | ~((*P & ~CL) | M);
        *P = *P | (~(*P) | CL);
        break;
    case 14:    /* OR */
        //*P = *P | ~(~CL | M);
        *P |= CL;
        break;
    case 15:    /* ID */
        //*P = ~(~*P & M);
        break;
    default:
        break;
    }
}

/** VDPpsetBP4() *********************************************/
/** Set a pixel on BP4 Mode                                 **/
/*************************************************************/
INLINE void VDPpsetBP4(int DX, int DY, byte CL, byte OP)
{
    register byte SH = ((~DX) & 1) << 2;
    
    V9Kpsetlowlevel(VDP_VRMP_BP4(DX, DY),
        CL << SH, ~(15 << SH), OP);
}

INLINE void VDPpsetBP4_512(int DX, int DY, byte CL, byte OP)
{
    register byte SH = ((~DX) & 1) << 2;

    V9Kpsetlowlevel(VDP_VRMP_512(DX, DY),
        CL << SH, ~(15 << SH), OP);
}

INLINE void VDPpsetBP4_1024(int DX, int DY, byte CL, byte OP)
{
    register byte SH = ((~DX) & 1) << 2;

    V9Kpsetlowlevel(VDP_VRMP_1024(DX, DY),
        CL << SH, ~(15 << SH), OP);
}

INLINE void VDPpsetBD16(int DX, int DY, word CW, byte OP)
{
    V9Kpsetlowlevel((V9KVRAM + (((DY << 9) + (DX << 1)) & 0x7FFFF)),
        CW >> 8, ~255, OP);
    V9Kpsetlowlevel((V9KVRAM + (((DY << 9) + (DX << 1) + 1) & 0x7FFFF)),
        CW & 0xFF, ~255, OP);
}

INLINE void VDPpsetBD16_512(int DX, int DY, word CW, byte OP)
{
    V9Kpsetlowlevel((V9KVRAM + (((DY << 10) + (DX << 1)) & 0x7FFFF)),
        CW >> 8, ~255, OP);
    V9Kpsetlowlevel((V9KVRAM + (((DY << 10) + (DX << 1) + 1) & 0x7FFFF)),
        CW & 0xFF, ~255, OP);
}

INLINE void VDPpset16BP4(int DX, int DY, int SX, word CW, byte OP)
{
    register byte SH = ((~SX) & 3) << 2;
    register byte DH = ((~DX) & 1) << 2;

    V9Kpsetlowlevel(VDP_VRMP_BP4(DX, DY),
        ((CW >> SH) & 15) << DH, ~(15 << DH), OP);
}

INLINE void VDPpset16BP4_512(int DX, int DY, int SX, word CW, byte OP)
{
    register byte SH = ((~SX) & 3) << 2;
    register byte DH = ((~DX) & 1) << 2;

    V9Kpsetlowlevel(VDP_VRMP_512(DX, DY),
        ((CW >> SH) & 15) << DH, ~(15 << DH), OP);
}

INLINE void VDPpset16BP4_1024(int DX, int DY, int SX, word CW, byte OP)
{
    register byte SH = ((~SX) & 3) << 2;
    register byte DH = ((~DX) & 1) << 2;

    V9Kpsetlowlevel(VDP_VRMP_1024(DX, DY),
        ((CW >> SH) & 15) << DH, ~(15 << DH), OP);
}

INLINE void VDPpset16BP6(int DX, int DY, int SX, word CW, byte OP)
{
    register byte SH = ((~SX) & 1) << 3;
    //register byte DH = ((~DX) & 1) << 3;

    V9Kpsetlowlevel(VDP_VRMP_BPP8(DX, DY),
        (CW >> SH) & 255, ~255, OP);
}

INLINE void VDPPSetLXBP4(int DA, int DX, int SX, int SY, byte OP)
{
    register byte DH = ((~DX) & 1) << 2;
    register byte SH = ((~SX) & 1) << 2;
    register byte SC = (((*VDP_VRMP_BP4(SX, SY)) >> SH) & 15) << DH;
    V9Kpsetlowlevel(V9KVRAM + DA, SC, ~(15 << DH), OP);
}

INLINE void VDPPsetLXBP4_512(int DA, int DX, int SX, int SY, byte OP)
{
    register byte DH = ((~DX) & 1) << 2;
    register byte SH = ((~SX) & 1) << 2;
    register byte SC = (((*VDP_VRMP_512(SX, SY)) >> SH) & 15) << DH;
    V9Kpsetlowlevel(V9KVRAM + DA, SC, ~(15 << DH), OP);
}

INLINE void VDPPsetLXBP4_1024(int DA, int DX, int SX, int SY, byte OP)
{
    register byte DH = ((~DX) & 1) << 2;
    register byte SH = ((~SX) & 1) << 2;
    register byte SC = (((*VDP_VRMP_1024(SX, SY)) >> SH) & 15) << DH;
    V9Kpsetlowlevel(V9KVRAM + DA, SC, ~(15 << DH), OP);
}

INLINE void VDPPSetLXBP6(int DA, int SX, int SY, byte OP)
{
    register byte SC = (*VDP_VRMP_BP4(SX, SY)) & 255;
    V9Kpsetlowlevel(V9KVRAM + DA, SC, ~255, OP);
}

INLINE void VDPPSetLXBP6_512(int DA, int SX, int SY, byte OP)
{
    register byte SC = (*VDP_VRMP_512(SX, SY)) & 255;
    V9Kpsetlowlevel(V9KVRAM + DA, SC, ~255, OP);
}

INLINE void VDPPSetLXBD16(int DA, int SX, int SY, byte OP)
{
    V9Kpsetlowlevel(V9KVRAM + DA, V9KVRAM[((SY << 9) + (SX << 1)) & 0x7FFFF],
        ~255, OP);
    V9Kpsetlowlevel(V9KVRAM + (DA+1), V9KVRAM[((SY << 9) + (SX << 1) + 1) & 0x7FFFF],
        ~255, OP);
}

INLINE void VDPPSetLXBD16_512(int DA, int SX, int SY, byte OP)
{
    V9Kpsetlowlevel(V9KVRAM + DA, V9KVRAM[((SY << 10) + (SX << 1)) & 0x7FFFF],
        ~255, OP);
    V9Kpsetlowlevel(V9KVRAM + (DA + 1), V9KVRAM[((SY << 10) + (SX << 1) + 1) & 0x7FFFF],
        ~255, OP);
}

/** GetVdpTimingValue() **************************************/
/** Get timing value for a certain VDP command              **/
/*************************************************************/
static int GetVdpTimingValue(register int* timing_values)
{
    return(timing_values[((V9990VDP[8]>>6)&0x01) | ((V9990VDP[8]>>4)&0x02) | ((V9990VDP[7]>>1)&0x04)]);
}

/** VDPDraw() ************************************************/
/** Perform a given V9938 operation Op.                     **/
/*************************************************************/
byte VDPDrawV9990(byte Op)
{
    MMCV9990.CM = Op >> 4;

    //if (Verbose & 0x20)printf("V9990 Draw[%02Xh]\n", Op >> 4);

    /* Fetch unconditional arguments */
    /* Strikingly similar to V9938 */
    MMCV9990.SX = (V9990VDP[32] + ((unsigned int)(V9990VDP[33] & 0x07) << 8)) & (V9KImgWidth-1);
    MMCV9990.SY = (V9990VDP[34] + ((unsigned int)(V9990VDP[35] & 0x0F) << 8)) & (V9KImgHeight-1);
    MMCV9990.DX = (V9990VDP[36] + ((unsigned int)(V9990VDP[37] & 0x07) << 8)) & (V9KImgWidth-1);
    MMCV9990.DY = (V9990VDP[38] + ((unsigned int)(V9990VDP[39] & 0x0F) << 8)) & (V9KImgHeight-1);
    MMCV9990.NX = (V9990VDP[40] + ((unsigned int)(V9990VDP[41] & 0x07) << 8));
    MMCV9990.NX = !MMCV9990.NX ? 2048 : MMCV9990.NX;
    MMCV9990.NY = (V9990VDP[42] + ((unsigned int)(V9990VDP[43] & 0x0F) << 8));
    MMCV9990.NY = !MMCV9990.NY ? 4096 : MMCV9990.NY;
    MMCV9990.TX = V9990VDP[44] & 0x04 ? -1 : 1;
    MMCV9990.TY = V9990VDP[44] & 0x08 ? -1 : 1;
    MMCV9990.ADX = MMCV9990.DX;
    MMCV9990.ANX = MMCV9990.NX;
    MMCV9990.ASX = MMCV9990.SX;
    MMCV9990.MX = 256 << ((V9990VDP[6] & 0x0C) >> 2);
    MMCV9990.LO = V9990VDP[45] & 0x1F;
    MMCV9990.RectPos = 0;

    switch (Op >> 4)
    {
    case CMV9990_STOP:
        if (Verbose & 0x80)printf("V9990 Draw[%02Xh]\n", Op >> 4);
        V9990Port[5] &= 0xFE;
        VdpEngineV9990 = 0;
        return 1;
    case CMV9990_LMMC:
        V9990Port[5] |= 0x80;
        VdpEngineV9990 = LmmcEngineV9990;
        break;
    case CMV9990_LMMV:
        VdpEngineV9990 = LmmvEngineV9990;
        break;
    case CMV9990_LMCM:
        V9990Port[5] &= 0x7F;
        VdpEngineV9990 = LmcmEngineV9990;
        break;
    case CMV9990_LMMM:
        VdpEngineV9990 = LmmmEngineV9990;
        break;
    case CMV9990_CMMC:
        V9990Port[5] |= 0x80;
        MMCV9990.RectData = V9990Port[2];
        VdpEngineV9990 = CmmcEngineV9990;
        break;
    case CMV9990_CMMM:
        MMCV9990.SA = (V9990VDP[32] + ((unsigned int)V9990VDP[34] << 8) + ((unsigned int)(V9990VDP[35] & 0x07) << 16));
        VdpEngineV9990 = CmmmEngineV9990;
        break;
    case CMV9990_BMXL:
        MMCV9990.SA = (V9990VDP[32] + ((unsigned int)V9990VDP[34] << 8) + ((unsigned int)(V9990VDP[35] & 0x07) << 16));
        VdpEngineV9990 = BmxlEngineV9990;
        break;
    case CMV9990_BMLX:
        MMCV9990.DA = (V9990VDP[36] + ((unsigned int)V9990VDP[38] << 8) + ((unsigned int)(V9990VDP[39] & 0x07) << 16));
        VdpEngineV9990 = BmlxEngineV9990;
        break;
    case CMV9990_BMLL:
        VdpEngineV9990 = BmllEngineV9990;
        MMCV9990.SA = (V9990VDP[32] + ((unsigned int)V9990VDP[34] << 8) + ((unsigned int)(V9990VDP[35] & 0x07) << 16));
        MMCV9990.DA = (V9990VDP[36] + ((unsigned int)V9990VDP[38] << 8) + ((unsigned int)(V9990VDP[39] & 0x07) << 16));
        MMCV9990.NA = (V9990VDP[40] + ((unsigned int)V9990VDP[42] << 8) + ((unsigned int)(V9990VDP[43] & 0x07) << 16));
        MMCV9990.NA = !MMCV9990.NA ? 524288 : MMCV9990.NA;
        break;
    case CMV9990_LINE:
        MMCV9990.ADX = 0;
        MMCV9990.ASX = MMCV9990.NX | (((unsigned int)V9990VDP[41] & 0x08) << 8);
        //MMCV9990.ASX = (MMCV9990.NX - 1) >> 1;
        VdpEngineV9990 = LineEngineV9990;
        break;
    case CMV9990_SRCH:
        VdpEngineV9990 = SrchEngineV9990;
        break;
    case CMV9990_POINT:
        VdpEngineV9990 = PointEngineV9990;
        break;
    case CMV9990_PSET:
        VdpEngineV9990 = PsetEngineV9990;
        break;
    case CMV9990_ADVN:
        VdpEngineV9990 = AdvnEngineV9990;
        break;
    default:
        if (Verbose & 0x80)printf("V9990 Draw[%02Xh]\n", Op >> 4);
        break;
    }

    /* Command execution started */
    V9990Port[5] |= 0x01;

    /* Start execution if we still have time slices */
    if (VdpEngineV9990 && (VdpOpsCntV9990 > 0)) VdpEngineV9990();

    return 0;
}

void VDPReadV9990(void)
{
    if (VdpEngineV9990 && (VdpOpsCntV9990 > 0))
    {
        V9990Port[5] &= 0x7F;
        VdpEngineV9990();
    }
}


void VDPWriteV9990(register byte V)
{
    if (VdpEngineV9990 && (VdpOpsCntV9990 > 0))
    {
        if ((VdpEngineV9990 == LmmcEngineV9990) && (V9KPixelRes==3))
        {
            if (V9990PrevData < 0)
            {
                V9990Port[2] = V;
                V9990PrevData = V9990Port[2];
                return;
            }
        }
        V9990Port[2] = V;
        V9990Port[5] &= 0x7F;
        VdpEngineV9990();
    }
}


void LmmcEngineV9990(void)
{
    register int DY = MMCV9990.DY;
    if ((V9990Port[5] & 0x80) != 0x80)
    //if ((V9990Port[5] & 0x01))
    {
        if (V9KScrMode == 0)
        {
            if (V9990VDP[37] & 0x02)DY |= 2048;
        }
        for (int i = 0; i < 2; i++)
        {
            switch (V9KImgWidth)
            {
            case 256:
                switch (V9KPixelRes)
                {
                case 1:
                    VDPpset16BP4(MMCV9990.ADX, DY, MMCV9990.ASX, V9990Port[2] | (V9990Port[2] << 8), MMCV9990.LO);
                    break;
                case 2:
                    VDPpset16BP6(MMCV9990.ADX, DY, MMCV9990.ASX, V9990Port[2] | (V9990Port[2] << 8), MMCV9990.LO);
                    i++;
                    break;
                case 3:
                    VDPpsetBD16(MMCV9990.ADX, DY, V9990Port[2] | (V9990PrevData << 8), MMCV9990.LO);
                    V9990PrevData = -1;
                    i++;
                    break;
                default:
                    VDPpset16BP4(MMCV9990.ADX, DY, MMCV9990.ASX, V9990Port[2] | (V9990Port[2] << 8), MMCV9990.LO);
                    break;
                }
                break;
            case 512:
                switch (V9KPixelRes)
                {
                case 1:
                    VDPpset16BP4_512(MMCV9990.ADX, DY, MMCV9990.ASX, V9990Port[2] | (V9990Port[2] << 8), MMCV9990.LO);
                    break;
                case 3:
                    VDPpsetBD16_512(MMCV9990.ADX, DY, V9990Port[2] | (V9990PrevData << 8), MMCV9990.LO);
                    V9990PrevData = -1;
                    i++;
                    break;
                default:
                    VDPpset16BP4_512(MMCV9990.ADX, DY, MMCV9990.ASX, V9990Port[2] | (V9990Port[2] << 8), MMCV9990.LO);
                    break;
                }
                break;
            case 1024:
                VDPpset16BP4_1024(MMCV9990.ADX, DY, MMCV9990.ASX, V9990Port[2] | (V9990Port[2] << 8), MMCV9990.LO);
                break;
            default:
                break;
            }

            V9990Port[5] |= 0x80;
            //if (!--MMCV9990.ANX || ((MMCV9990.ADX += MMCV9990.TX) & MMCV9990.MX))
            MMCV9990.ADX += MMCV9990.TX;
            MMCV9990.ADX = MMCV9990.ADX & (MMCV9990.MX - 1);
            MMCV9990.ASX++;
            if (!(--MMCV9990.ANX))
            {
                //if (!(--MMCV9990.NY & 1023) || (MMCV9990.DY += MMCV9990.TY) == -1)
                DY += MMCV9990.TY;
                if (!(--MMCV9990.NY) || (MMCV9990.DY += MMCV9990.TY) == -1)
                {
                    //if (Verbose & 0x80)printf("V9990 LMMC Finished\n");
                    //V9990Port[5] &= 0xFE;
                    V9990Port[5] &= 0x7E;
                    VdpEngineV9990 = 0;
                    //if (!MMCV9990.NY)MMCV9990.DY += MMCV9990.TY;
                    V9990VDP[38] = MMCV9990.DY & 0xFF;
                    V9990VDP[39] = (MMCV9990.DY >> 8) & 0x0F;
                }
                else
                {
                    //if (Verbose & 0x80)printf("V9990 LMMC Command NY[%02Xh]\n",MMCV9990.NY);
                    MMCV9990.ADX = MMCV9990.DX;
                    MMCV9990.ANX = MMCV9990.NX;
                }
            }
            if (!(V9990Port[5] & 0x01))break;
        }
        //VdpOpsCntV9990 -= 60;
    }
}


void LmmvEngineV9990(void)
{
    register int SX = MMCV9990.SX;
    register int SY = MMCV9990.SY;

    register int DX = MMCV9990.DX;
    register int DY = MMCV9990.DY;
    register int TX = MMCV9990.TX;
    register int TY = MMCV9990.TY;
    register int NX = MMCV9990.NX;
    register int NY = MMCV9990.NY;
    register int ASX = MMCV9990.ASX;
    register int ADX = MMCV9990.ADX;
    register int ANX = MMCV9990.ANX;
    register byte LO = MMCV9990.LO;
    register int cnt, FC;
    register int delta;
    //delta = 60;
    delta = 30;
    cnt = VdpOpsCntV9990;

    if (V9KScrMode == 0)
    {
        if (V9990VDP[37] & 0x02)DY |= 2048;
        if (V9990VDP[33] & 0x02)SY |= 2048;
    }
        
    FC = !V9KScrMode ?(V9990VDP[37] & 0x02 ? ((V9990VDP[49] << 8) | V9990VDP[49]): ((V9990VDP[48] << 8) | V9990VDP[48])) : (V9990VDP[48] << 8) | V9990VDP[49];
    //FC = (V9990VDP[48] << 8) | V9990VDP[49];

    //#define DOCMD(N)     pre_loop VDPpsetBP4(ADX, DY, FC, N); post__x_y_g9k(256, 4096) break;
    //SWITCHCMD
    //#undef DOCMD

        switch (V9KImgWidth)
        {
        case 256:
            switch (V9KImgHeight)
            {
            case 2048:  /* P1 Mode */
                #define DOCMD(N)     pre_loop VDPpset16BP4(ADX, DY, ADX ,FC, N); post__x_y_g9k(256,2048) break;
                SWITCHCMD
                #undef DOCMD
                break;
            case 4096:
                #define DOCMD(N)     pre_loop VDPpset16BP4(ADX, DY, ADX, FC, N); post__x_y_g9k(256,4096) break;
                SWITCHCMD
                #undef DOCMD
                break;
            default:
                #define DOCMD(N)     pre_loop VDPpset16BP4(ADX, DY, ADX ,FC, N); post__x_y_g9k(256,2048) break;
                SWITCHCMD
                #undef DOCMD
                break;
            }
            break;
        case 512:
            switch (V9KImgHeight)
            {
            case 512:       /* BD16(BytePerPixel 16) */
                #define DOCMD(N)     pre_loop VDPpsetBD16_512(ADX, DY, FC, N); post__x_y_g9k(512,512) break;
                SWITCHCMD
                #undef DOCMD
                break;
            case 2048:
                #define DOCMD(N)     pre_loop VDPpset16BP4_512(ADX, DY, ADX, FC, N); post__x_y_g9k(512,2048) break;
                SWITCHCMD
                #undef DOCMD
                break;
            default:
                #define DOCMD(N)     pre_loop VDPpset16BP4_512(ADX, DY, ADX, FC, N); post__x_y_g9k(512,2048) break;
                SWITCHCMD
                #undef DOCMD
                break;
            }
            break;
        case 1024:
            #define DOCMD(N)     pre_loop VDPpset16BP4_1024(ADX, DY, ADX, FC, N); post__x_y_g9k(1024,1024) break;
            SWITCHCMD
            #undef DOCMD
            break;
        default:
            #define DOCMD(N)     pre_loop VDPpset16BP4_512(ADX, DY, ADX,FC, N); post__x_y_g9k(512,2048) break;
            SWITCHCMD
            #undef DOCMD
            break;
        }

if (V9KScrMode == 0)
{
    if (V9990VDP[37] & 0x02)DY &= ~2048;
    if (V9990VDP[33] & 0x02)SY &= ~2048;
}

    if ((VdpOpsCntV9990 = cnt) > 0)
    {
        /* Command execution done */
        V9990Port[5] &= 0xFE;
        //V9990Port[5] &= 0x7E;
        VdpEngineV9990 = 0;
        if (!NY)
        {
            SY += TY;
            DY += TY;
        }
        else if (SY == -1)DY += TY;

        //V9990VDP[42] = NY & 0xFF;
        //V9990VDP[43] = (NY >> 8) & 0x0F;
        //V9990VDP[34] = SY & 0xFF;
        //V9990VDP[35] = (SY >> 8) & 0x0F;
        V9990VDP[38] = DY & 0xFF;
        V9990VDP[39] = (DY >> 8) & 0x0F;
    }
    else
    {
        MMCV9990.SY = SY;
        MMCV9990.DY = DY;
        MMCV9990.NY = NY;
        MMCV9990.ANX = ANX;
        MMCV9990.ASX = ASX;
        MMCV9990.ADX = ADX;
        //V9990Port[5] |= 0x80;
    }
}


void LmcmEngineV9990(void)
{
    register int DY = MMCV9990.DY;
    register int SY = MMCV9990.SY;
    register int SYOff = 0;
    register unsigned char LMCMData = 0;
    if ((V9990Port[5] & 0x80) != 0x80)
    {
        if (V9KScrMode == 0)
        {
            if (V9990VDP[33] & 0x02)SYOff= 2048;
        }
        for (int i = 0; i < 2; i++)
        {
            switch (V9KImgWidth)
            {
            case 256:
                switch (V9KPixelRes)
                {
                case 1:
                    //LMCMData |= VDPpointBP4(MMCV9990.ASX, MMCV9990.SY);
                    //V9990Port[2] = (V9990Port[2] & (0x0F << ((i&0x01) << 2))) | VDPpointBP4(MMCV9990.ASX, MMCV9990.SY);
                    if(i==0)V9990Port[2] = (V9990Port[2] & 0x0F) | (VDPpointBP4(MMCV9990.ASX, MMCV9990.SY|SYOff)<<4);
                    else V9990Port[2] = (V9990Port[2] & 0xF0) | VDPpointBP4(MMCV9990.ASX, MMCV9990.SY|SYOff);
                    break;
                case 2:
                    //LMCMData |= *VDP_VRMP_BPP8(MMCV9990.ASX, MMCV9990.SY);
                    V9990Port[2] |= *VDP_VRMP_BPP8(MMCV9990.ASX, MMCV9990.SY);
                    i++;
                    break;
                case 3:
                    V9990Port[2] |= ((*V9KVRAM + (((MMCV9990.SY << 9) + (MMCV9990.ASX << 1))&0x7FFFF))<<8) |
                        (*V9KVRAM + (((MMCV9990.SY << 9) + (MMCV9990.ASX << 1) + 1) & 0x7FFFF));
                    i++;
                    break;
                default:
                    V9990Port[2] |= VDPpointBP4(MMCV9990.ASX, MMCV9990.SY);
                    break;
                }
                break;
            case 512:
                switch (V9KPixelRes)
                {
                case 1:
                    //LMCMData <<= 2;
                    //LMCMData |= VDPpoint512(MMCV9990.ASX, MMCV9990.SY);
                    //V9990Port[2] = V9990Port[2] & (0x0F << ((i & 0x01) << 2)) | VDPpoint512(MMCV9990.ASX, MMCV9990.SY);
                    if (i == 0)V9990Port[2] = (V9990Port[2] & 0x0F) | (VDPpoint512(MMCV9990.ASX, MMCV9990.SY | SYOff) << 4);
                    else V9990Port[2] = (V9990Port[2] & 0xF0) | VDPpoint512(MMCV9990.ASX, MMCV9990.SY | SYOff);
                    break;
                case 3:
                    V9990Port[2] |= ((*V9KVRAM + (((MMCV9990.SY << 10) + (MMCV9990.ASX << 1)) & 0x7FFFF)) << 8) |
                        (*V9KVRAM + (((MMCV9990.SY << 10) + (MMCV9990.ASX << 1) + 1) & 0x7FFFF));
                    i++;
                    break;
                default:
                    V9990Port[2] |= VDPpoint512(MMCV9990.ASX, MMCV9990.SY);
                    break;
                }
                break;
            case 1024:
                V9990Port[2] |= VDPpoint1024(MMCV9990.ASX, MMCV9990.SY);
                break;
            default:
                break;
            }

            V9990Port[5] |= 0x80;
            MMCV9990.ASX += MMCV9990.TX;
            MMCV9990.ASX = MMCV9990.ASX & (MMCV9990.MX - 1);
            if (!(--MMCV9990.ANX))
            {
                if (!(--MMCV9990.NY) || (MMCV9990.SY += MMCV9990.TY) == -1)
                {
                    //V9990Port[5] &= 0xFE;
                    V9990Port[5] &= 0x7E;
                    VdpEngineV9990 = 0;
                    //if (!MMCV9990.NY)MMCV9990.SY += MMCV9990.TY;
                    V9990VDP[34] = MMCV9990.SY & 0xFF;
                    V9990VDP[35] = (MMCV9990.SY >> 8) & 0x0F;
                }
                else
                {
                    MMCV9990.SY = MMCV9990.SY & (V9KImgHeight-1);
                    MMCV9990.ANX = MMCV9990.NX;
                    MMCV9990.ASX = MMCV9990.SX;
                }
            }
            if (!(V9990Port[5] & 0x01))break;
        }
        //V9990Port[2] = LMCMData;
        //VdpOpsCntV9990 -= 60;
    }
}


/** LmmmEngineV9990() *********************************************/
/** Vram -> Vram                                            **/
/*************************************************************/
void LmmmEngineV9990(void)
{
    register int SX = MMCV9990.SX;
    register int SY = MMCV9990.SY;
    register int DX = MMCV9990.DX;
    register int DY = MMCV9990.DY;
    register int TX = MMCV9990.TX;
    register int TY = MMCV9990.TY;
    register int NX = MMCV9990.NX;
    register int NY = MMCV9990.NY;
    register int ASX = MMCV9990.ASX;
    register int ADX = MMCV9990.ADX;
    register int ANX = MMCV9990.ANX;
    register byte LO = MMCV9990.LO;
    register int cnt, i, j;
    register int delta;

    if (V9KScrMode == 0)
    {
        if (V9990VDP[37] & 0x02)DY |= 2048;
        if (V9990VDP[33] & 0x02)SY |= 2048;
    }


    delta = 60;
    cnt = VdpOpsCntV9990;

        switch (V9KImgWidth)
        {
        case 256:
            switch (V9KImgHeight)
            {
            case 1024:  /* BD16(16BytePerPixel) */
                #define DOCMD(N)     pre_loop VDPpsetBD16(ADX, DY, VDPpointBD16(ASX, SY), N); post_xxyy_g9k(256,1024) break;
                SWITCHCMD
                #undef DOCMD
                break;
            case 2048:
                if (!V9KScrMode)    /* P1 Mode */
                {
                    #define DOCMD(N)     pre_loop VDPpsetBP4(ADX, DY, VDPpointBP4(ASX, SY), N); post_xxyy_g9k(256,2048) break;
                    SWITCHCMD
                    #undef DOCMD
                }
                break;
            case 4096:  /* BP4 */
                #define DOCMD(N)     pre_loop VDPpsetBP4(ADX, DY, VDPpointBP4(ASX, SY), N); post_xxyy_g9k(256, 4096) break;
                SWITCHCMD
                #undef DOCMD
                break;
            default:
                #define DOCMD(N)     pre_loop VDPpsetBP4(ADX, DY, VDPpointBP4(ASX, SY), N); post_xxyy_g9k(256,2048) break;
                SWITCHCMD
                #undef DOCMD
                break;
            }
            break;
        case 512:
            switch (V9KImgHeight)
            {
                case 512:   /* BD16(16BytePerPixel) */
                    #define DOCMD(N)     pre_loop VDPpsetBD16_512(ADX, DY, VDPpointBD16_512(ASX, SY), N); post_xxyy_g9k(512,512) break;
                    SWITCHCMD
                    #undef DOCMD
                    break;
                case 2048:  /* P2Mode or BP4 */
                    #define DOCMD(N)     pre_loop VDPpsetBP4_512(ADX, DY, VDPpoint512(ASX, SY), N); post_xxyy_g9k(512,2048) break;
                    SWITCHCMD
                    #undef DOCMD
                    break;
                default:
                    break;
            }
            break;
        case 1024:
            #define DOCMD(N)     pre_loop VDPpsetBP4_1024(ADX, DY, VDPpoint1024(ASX, SY), N); post_xxyy_g9k(1024,1024) break;
            SWITCHCMD
            #undef DOCMD
            break;
        default:
            #define DOCMD(N)     pre_loop VDPpsetBP4_512(ADX, DY, VDPpoint512(ASX, SY), N); post_xxyy_g9k(512,2048) break;
            SWITCHCMD
            #undef DOCMD
            break;
        }
//    SWITCHCMD
//#undef DOCMD

    if (V9KScrMode == 0)
    {
        if (V9990VDP[37] & 0x02)DY &= ~2048;
        if (V9990VDP[33] & 0x02)SY &= ~2048;
    }

        if ((VdpOpsCntV9990 = cnt) > 0)
        {
            /* Command execution done */
            V9990Port[5] &= 0xFE;
            //V9990Port[5] &= 0x7E;
            VdpEngineV9990 = 0;
            if (!NY)
            {
                SY += TY;
                DY += TY;
            }
            else if (SY == -1)DY += TY;

            //V9990VDP[42] = NY & 0xFF;
            //V9990VDP[43] = (NY >> 8) & 0x0F;
            V9990VDP[34] = SY & 0xFF;
            V9990VDP[35] = (SY >> 8) & 0x0F;
            V9990VDP[38] = DY & 0xFF;
            V9990VDP[39] = (DY >> 8) & 0x0F;
        }
        else
        {
            MMCV9990.SY = SY;
            MMCV9990.DY = DY;
            MMCV9990.NY = NY;
            MMCV9990.ANX = ANX;
            MMCV9990.ASX = ASX;
            MMCV9990.ADX = ADX;
            //V9990Port[5] |= 0x80;
        }
}


void CmmcEngineV9990(void)
{
    register int DX = MMCV9990.DX;
    register int DY = MMCV9990.DY;
    register int TX = MMCV9990.TX;
    register int TY = MMCV9990.TY;
    register int NX = MMCV9990.NX;
    register int NY = MMCV9990.NY;
    register int ADX = MMCV9990.ADX;
    register int ANX = MMCV9990.ANX;
    register int RectPos = MMCV9990.RectPos;
    register byte LO = MMCV9990.LO;
    register byte RectData = MMCV9990.RectData;
    register int i, FC, BC, SC;

    if ((V9990Port[5] & 0x80) != 0x80)
    {
        if (V9KScrMode == 0)
        {
            if (V9990VDP[37] & 0x02)DY |= 2048;
        }
        FC = (V9990VDP[48] << 8) | V9990VDP[49];
        BC = (V9990VDP[50] << 8) | V9990VDP[51];
        for (i = 0; i < 7; i++)
        {
            SC = (RectData >>i) & 0x01 ? FC : BC;
            switch (V9KImgWidth)
            {
            case 256:
                switch (V9KImgHeight)
                {
                case 2048:  /* P1 Mode */
                    VDPpset16BP4(ADX, DY, ADX ,SC, LO);
                    break;
                case 4096:
                    VDPpset16BP4(ADX, DY, ADX, SC, LO);
                    break;
                default:
                    VDPpset16BP4(ADX, DY, ADX ,SC, LO);
                    break;
                }
                break;
            case 512:
                switch (V9KImgHeight)
                {
                case 512:       /* BD16(BytePerPixel 16) */
                    VDPpsetBD16_512(ADX, DY, SC, LO);
                    break;
                case 2048:
                    VDPpset16BP4_512(ADX, DY, ADX, SC, LO);
                    break;
                default:
                    VDPpset16BP4_512(ADX, DY, ADX, SC, LO);
                    break;
                }
                break;
            case 1024:
                VDPpset16BP4_1024(ADX, DY, ADX, SC, LO);
                break;
            default:
                VDPpset16BP4_512(ADX, DY, ADX,SC, LO);
                break;
            }

            //V9990Port[5] |= 0x80;
            ADX = (ADX + TX) & (V9KImgWidth - 1);
            if (!--ANX)
            {
                if (!(--NY & (V9KImgHeight - 1)) || (DY += TY) == -1)
                {
                    V9990Port[5] &= 0x7E;
                    VdpEngineV9990 = 0;
                    if (!NY)DY += TY;
                    V9990VDP[38] = DY & 0xFF;
                    V9990VDP[39] = (DY >> 8) & 0x0F;
                    break;
                }
                else
                {
                    ADX = DX;
                    ANX = NX;
                }
            }
            if (!(V9990Port[5] & 0x01))break;
        }
        MMCV9990.DX = DX;
        MMCV9990.DY = DY;
        MMCV9990.NX = NX;
        MMCV9990.NY = NY;
        MMCV9990.ADX = ADX;
        MMCV9990.ANX = ANX;
    }
}


void CmmmEngineV9990(void)
{
    register int DX = MMCV9990.DX;
    register int DY = MMCV9990.DY;
    register int TX = MMCV9990.TX;
    register int TY = MMCV9990.TY;
    register int NX = MMCV9990.NX;
    register int NY = MMCV9990.NY;
    register int ADX = MMCV9990.ADX;
    register int ANX = MMCV9990.ANX;
    register int SA = MMCV9990.SA;
    register int RectPos = MMCV9990.RectPos;
    register byte LO = MMCV9990.LO;
    register byte RectData = MMCV9990.RectData;
    register int cnt, FC, BC, SC;
    register int delta;

#define exec_cmmm    \
    if(!RectPos) { RectData = V9KVRAM[SA]; \
    SA = (SA +1)&0x7FFFF;}   \
    SC = (RectData>>(7-RectPos))&0x01 ? FC : BC; \
    RectPos = (RectPos+1)&0x07;

    delta = 60;
    cnt = VdpOpsCntV9990;

    if (V9KScrMode == 0)
    {
        if (V9990VDP[37] & 0x02)DY |= 2048;
    }
    FC = (V9990VDP[48] << 8) | V9990VDP[49];
    BC = (V9990VDP[50] << 8) | V9990VDP[51];

    switch (V9KImgWidth)
    {
    case 256:
        switch (V9KImgHeight)
        {
        case 2048:  /* P1 Mode */
            #define DOCMD(N)     pre_loop exec_cmmm VDPpset16BP4(ADX, DY, ADX ,SC, N); post__x_y_g9k(256,2048) break;
            SWITCHCMD
            #undef DOCMD
            break;
        case 4096:
            #define DOCMD(N)     pre_loop exec_cmmm VDPpset16BP4(ADX, DY, ADX, SC, N); post__x_y_g9k(256,4096) break;
            SWITCHCMD
            #undef DOCMD
            break;
        default:
            #define DOCMD(N)     pre_loop exec_cmmm VDPpset16BP4(ADX, DY, ADX ,SC, N); post__x_y_g9k(256,2048) break;
            SWITCHCMD
            #undef DOCMD
            break;
        }
        break;
    case 512:
        switch (V9KImgHeight)
        {
        case 512:       /* BD16(BytePerPixel 16) */
            #define DOCMD(N)     pre_loop exec_cmmm VDPpsetBD16_512(ADX, DY, SC, N); post__x_y_g9k(512,512) break;
            SWITCHCMD
            #undef DOCMD
            break;
        case 2048:
            #define DOCMD(N)     pre_loop exec_cmmm VDPpset16BP4_512(ADX, DY, ADX, SC, N); post__x_y_g9k(512,2048) break;
            SWITCHCMD
            #undef DOCMD
            break;
        default:
            #define DOCMD(N)     pre_loop exec_cmmm VDPpset16BP4_512(ADX, DY, ADX, SC, N); post__x_y_g9k(512,2048) break;
            SWITCHCMD
            #undef DOCMD
            break;
        }
        break;
    case 1024:
        #define DOCMD(N)     pre_loop exec_cmmm VDPpset16BP4_1024(ADX, DY, ADX, SC, N); post__x_y_g9k(1024,1024) break;
        SWITCHCMD
        #undef DOCMD
        break;
    default:
        #define DOCMD(N)     pre_loop exec_cmmm VDPpset16BP4_512(ADX, DY, ADX,SC, N); post__x_y_g9k(512,2048) break;
        SWITCHCMD
        #undef DOCMD
        break;
    }

    if ((VdpOpsCntV9990 = cnt) > 0)
    {
        /* Command execution done */
        V9990Port[5] &= 0xFE;
        //V9990Port[5] &= 0x7E;
        VdpEngineV9990 = 0;
        if (!NY)
        {
            DY += TY;
        }

        V9990VDP[38] = DY & 0xFF;
        V9990VDP[39] = (DY >> 8) & 0x0F;
        V9990VDP[32] = SA & 0xFF;
        V9990VDP[34] = (SA >> 8) & 0xFF;
        V9990VDP[35] = (SA >> 16) & 0x07;
    }
    else
    {
        MMCV9990.DY = DY;
        MMCV9990.NY = NY;
        MMCV9990.ANX = ANX;
        MMCV9990.ADX = ADX;
        MMCV9990.SA = SA;
        MMCV9990.RectPos = RectPos;
    }
}


void BmxlEngineV9990(void)
{
    register int DX = MMCV9990.DX;
    register int DY = MMCV9990.DY;
    register int TX = MMCV9990.TX;
    register int TY = MMCV9990.TY;
    register int NX = MMCV9990.NX;
    register int NY = MMCV9990.NY;
    register int ADX = MMCV9990.ADX;
    register int ANX = MMCV9990.ANX;
    register int SA = MMCV9990.SA;
    register int RectPos = MMCV9990.RectPos;
    register byte LO = MMCV9990.LO;
    register int cnt, SC;
    register int delta;

#define exec_bmxl    \
    if(!RectPos) { SC = (V9KVRAM[SA]<<8) | V9KVRAM[SA]; \
    SA = (SA +1)&0x7FFFF;}   \
    RectPos = (RectPos+(2<<V9KPixelRes))&0x07;

#define exec_bmxl_bd16    \
    if(!RectPos) { SC = (V9KVRAM[SA+1]<<8) | V9KVRAM[SA]; \
    SA = (SA +2)&0x7FFFF;}   \
    RectPos = (RectPos+(2<<V9KPixelRes))&0x07;

    delta = 60;
    cnt = VdpOpsCntV9990;

    switch (V9KImgWidth)
    {
    case 256:
        switch (V9KImgHeight)
        {
        case 2048:  /* P1 Mode */
            #define DOCMD(N)     pre_loop exec_bmxl VDPpset16BP4(ADX, DY, ADX ,SC, N); post__x_y_g9k(256,2048) break;
            SWITCHCMD
            #undef DOCMD
            break;
        case 4096:
            #define DOCMD(N)     pre_loop exec_bmxl VDPpset16BP4(ADX, DY, ADX, SC, N); post__x_y_g9k(256,4096) break;
            SWITCHCMD
            #undef DOCMD
            break;
        default:
            #define DOCMD(N)     pre_loop exec_bmxl VDPpset16BP4(ADX, DY, ADX ,SC, N); post__x_y_g9k(256,2048) break;
            SWITCHCMD
            #undef DOCMD
            break;
        }
        break;
    case 512:
        switch (V9KImgHeight)
        {
        case 512:       /* BD16(BytePerPixel 16) */
            #define DOCMD(N)     pre_loop exec_bmxl_bd16 VDPpsetBD16_512(ADX, DY, SC, N); post__x_y_g9k(512,512) break;
            SWITCHCMD
            #undef DOCMD
            break;
        case 2048:
            #define DOCMD(N)     pre_loop exec_bmxl VDPpset16BP4_512(ADX, DY, ADX, SC, N); post__x_y_g9k(512,2048) break;
            SWITCHCMD
            #undef DOCMD
            break;
        default:
            #define DOCMD(N)     pre_loop exec_bmxl VDPpset16BP4_512(ADX, DY, ADX, SC, N); post__x_y_g9k(512,2048) break;
            SWITCHCMD
            #undef DOCMD
            break;
        }
        break;
    case 1024:
        #define DOCMD(N)     pre_loop exec_bmxl VDPpset16BP4_1024(ADX, DY, ADX, SC, N); post__x_y_g9k(1024,1024) break;
        SWITCHCMD
        #undef DOCMD
        break;
    default:
        #define DOCMD(N)     pre_loop exec_bmxl VDPpset16BP4_512(ADX, DY, ADX,SC, N); post__x_y_g9k(512,2048) break;
        SWITCHCMD
        #undef DOCMD
        break;
    }

    if ((VdpOpsCntV9990 = cnt) > 0)
    {
        /* Command execution done */
        V9990Port[5] &= 0xFE;
        //V9990Port[5] &= 0x7E;
        VdpEngineV9990 = 0;
        if (!NY)
        {
            DY += TY;
        }

        V9990VDP[38] = DY & 0xFF;
        V9990VDP[39] = (DY >> 8) & 0x0F;
        V9990VDP[32] = SA & 0xFF;
        V9990VDP[34] = (SA >> 8) & 0xFF;
        V9990VDP[35] = (SA >> 16) & 0x07;
    }
    else
    {
        MMCV9990.DY = DY;
        MMCV9990.NY = NY;
        MMCV9990.ANX = ANX;
        MMCV9990.ADX = ADX;
        MMCV9990.SA = SA;
        MMCV9990.RectPos = RectPos;
    }
}


void BmlxEngineV9990(void)
{
    register int DX = MMCV9990.DX;
    register int SX = MMCV9990.SX;
    register int SY = MMCV9990.SY;
    register int DA = MMCV9990.DA;
    register int TX = MMCV9990.TX;
    register int TY = MMCV9990.TY;
    register int NX = MMCV9990.NX;
    register int NY = MMCV9990.NY;
    register int ASX = MMCV9990.ASX;
    register int ANX = MMCV9990.ANX;
    register int RectPos = MMCV9990.RectPos;
    register byte LO = MMCV9990.LO;
    register int cnt;
    register int delta;

#define exec_bmlx    \
    RectPos = RectPos+(2<<V9KPixelRes); \
    DA = DA + RectPos>>3;

    delta = 60;
    cnt = VdpOpsCntV9990;

    switch (V9KImgWidth)
    {
    case 256:
        switch (V9KImgHeight)
        {
        case 2048:  /* P1 Mode */
            #define DOCMD(N)     pre_loop exec_bmlx  VDPPSetLXBP4(DA, DX, ASX, SY, LO); post__x_ys_g9k(256,2048) break;
            SWITCHCMD
            #undef DOCMD
                break;
        case 4096:  /* BP4 */
            #define DOCMD(N)     pre_loop exec_bmlx  VDPPSetLXBP4(DA, DX, ASX, SY, LO); post__x_ys_g9k(256,4096) break;
            SWITCHCMD
            #undef DOCMD
                break;
        default:
            #define DOCMD(N)     pre_loop exec_bmlx  VDPPSetLXBP4(DA, DX, ASX, SY, LO); post__x_ys_g9k(256,4096) break;
            SWITCHCMD
            #undef DOCMD
                break;
        }
        break;
    case 512:
        switch (V9KImgHeight)
        {
        case 512:       /* BD16(BytePerPixel 16) */
            #define DOCMD(N)     pre_loop exec_bmlx VDPPSetLXBD16_512(DA, ASX, SY, LO); post__x_ys_g9k(512,512) break;
            SWITCHCMD
            #undef DOCMD
                break;
        case 2048:  /* P2 Mode or BP4*/
            #define DOCMD(N)     pre_loop exec_bmlx  VDPPsetLXBP4_512(DA, DX, ASX, SY, LO);  post__x_ys_g9k(512,2048) break;
            SWITCHCMD
            #undef DOCMD
                break;
        default:
            #define DOCMD(N)     pre_loop exec_bmlx  VDPPsetLXBP4_512(DA, DX, ASX, SY, LO);  post__x_ys_g9k(512,2048) break;
            SWITCHCMD
            #undef DOCMD
                break;
        }
        break;
    case 1024:
        #define DOCMD(N)     pre_loop exec_bmlx  VDPPsetLXBP4_1024(DA, DX, ASX, SY, LO);  post__x_ys_g9k(1024,1024) break;
        SWITCHCMD
        #undef DOCMD
            break;
    default:
        #define DOCMD(N)     pre_loop exec_bmlx  VDPPsetLXBP4_512(DA, DX, ASX, SY, LO);  post__x_ys_g9k(512,2048) break;
        SWITCHCMD
        #undef DOCMD
            break;
    }

    if ((VdpOpsCntV9990 = cnt) > 0)
    {
        /* Command execution done */
        V9990Port[5] &= 0xFE;
        //V9990Port[5] &= 0x7E;
        VdpEngineV9990 = 0;

        V9990VDP[36] = DA & 0xFF;
        V9990VDP[38] = (DA >> 8) & 0xFF;
        V9990VDP[39] = (DA >> 16) & 0x07;
        V9990VDP[34] = SY & 0xFF;
        V9990VDP[35] = (SY >> 8) & 0x0F;
    }
    else
    {
        MMCV9990.NY = NY;
        MMCV9990.ANX = ANX;
        MMCV9990.ASX = ASX;
        MMCV9990.SY = SY;
        MMCV9990.DA = DA;
        MMCV9990.RectPos = RectPos;
    }
}


void BmllEngineV9990(void)
{
    register int SA = MMCV9990.SA;
    register int DA = MMCV9990.DA;
    register int NA = MMCV9990.NA;
    register int TX = MMCV9990.TX;
    register int TY = MMCV9990.TY;
    register byte LO = MMCV9990.LO;
    register byte WMASK;
    register int cnt;
    register int delta;

    delta = 60;
    cnt = VdpOpsCntV9990;

    if (!V9KScrMode)
    {
        WMASK = V9990VDP[37] & 0x02 ? V9990VDP[47] : V9990VDP[46];
        for (; NA > 0; --NA)
        {
            cnt -= delta;
            if (cnt < 0)break;

            #define DOCMD(N)            V9Kpsetlowlevel(V9KVRAM + (DA>>1 | (DA&0x01)<<18), V9KVRAM[SA>>1 | (SA&0x01)<<18] << 4, (WMASK&0x0F), N); \
            V9Kpsetlowlevel(V9KVRAM + (DA>>1 | (DA&0x01)<<18), V9KVRAM[SA>>1 | (SA&0x01)<<18], (WMASK&0xF0), N); break;
            SWITCHCMD
            #undef DOCMD

            SA = (SA + TX) & 0x7FFFF;
            DA = (DA + TX) & 0x7FFFF;
        }
    }
    else
    {
        for (; NA > 0; --NA)
        {
            cnt -= delta;
            if (cnt < 0)break;

            WMASK = DA & 0x40000 ? V9990VDP[47] : V9990VDP[46];

            //#define DOCMD(N)            V9Kpsetlowlevel(V9KVRAM + DA, V9KVRAM[SA] << 4, 0x0F, N); \
            //V9Kpsetlowlevel(V9KVRAM + DA, V9KVRAM[SA], 0xF0, N); break;
            //SWITCHCMD
            //#undef DOCMD

            #define DOCMD(N)            V9Kpsetlowlevel(V9KVRAM + DA, V9KVRAM[SA] << 4, (WMASK&0x0F), N); \
            V9Kpsetlowlevel(V9KVRAM + DA, V9KVRAM[SA], (WMASK&0xF0) & 0xF0, N); break;
            SWITCHCMD
            #undef DOCMD

            SA = (SA + TX) & 0x7FFFF;
            DA = (DA + TX) & 0x7FFFF;
        }
    }

    //while ((cnt -= delta) > 0)
    //{
    //
    //}

    if ((VdpOpsCntV9990 = cnt) > 0)
    {
        /* Command execution done */
        V9990Port[5] &= 0xFE;
        //V9990Port[5] &= 0x7E;
        VdpEngineV9990 = 0;
        V9990VDP[32] = SA & 0xFF;
        V9990VDP[34] = (SA >> 8) & 0xFF;
        V9990VDP[35] = (SA >> 16) & 0x07;
        V9990VDP[36] = DA & 0xFF;
        V9990VDP[38] = (DA >> 8) & 0xFF;
        V9990VDP[39] = (DA >> 16) & 0x07;

        //SA = SA + (V9990VDP[44] & 0x04 ? -1 : 1);
    }
    else
    {
        MMCV9990.SA = SA;
        MMCV9990.DA = DA;
        MMCV9990.NA = NA;
        //V9990Port[5] |= 0x80;
    }
}


void LineEngineV9990(void)
{
    register int DX = MMCV9990.DX;
    register int DY = MMCV9990.DY;
    register int TX = MMCV9990.TX;
    register int TY = MMCV9990.TY;
    register int MJ = MMCV9990.NX | (((unsigned int)V9990VDP[41] & 0x08) << 8);
    register int MI = MMCV9990.NY;
    register int ASX = MMCV9990.ASX;
    //register int ASX = (MMCV9990.NX-1)>>1;
    register int ADX = MMCV9990.ADX;
    register byte LO = MMCV9990.LO;
    register int cnt, FC;
    register int delta;

#define post_linexmaj_v9k(MX, MY) \
    DX+=TX; \
    if (MI>0) { \
    ADX += MI;  \
    if((ADX<<1)>=MJ){DY +=TY; ADX -= MJ;} \
    }   \
    if (!ASX-- || (DX&MX) || (DY&MY)) \
        break; \
    }
#define post_lineymaj_v9k(MX, MY) \
    DY += TY; \
    if (MI>0) { \
    ADX += MI; \
    if((ADX<<1)>=MJ){DX +=TX; ADX -= MJ;}   \
    }   \
    if (!ASX-- || (DX&MX) || (DY&MY)) \
        break; \
    }

    delta = 60;
    cnt = VdpOpsCntV9990;

    //FC = V9990VDP[48] << 8 | V9990VDP[49];
    FC = !V9KScrMode ? (V9990VDP[37] & 0x02 ? ((V9990VDP[49] << 8) | V9990VDP[49]) : ((V9990VDP[48] << 8) | V9990VDP[48])) : (V9990VDP[48] << 8) | V9990VDP[49];
    if (!(V9990VDP[44] & 0x01))
    {
        switch (V9KImgWidth)
        {
        case 256:
            switch (V9KImgHeight)
            {
            case 1024:  /* BD16 */
                #define DOCMD(N)    pre_loop VDPpsetBD16(DX, DY, FC, N); post_linexmaj_v9k(256,1024) break;
                SWITCHCMD
                #undef DOCMD
                break;
            case 2048:  /* P1 Mode */
                #define DOCMD(N)    pre_loop VDPpsetBP4(DX, DY, FC, N); post_linexmaj_v9k(256,2048) break;
                SWITCHCMD
                #undef DOCMD
                break;
            case 4096:  /* BP4 */
                #define DOCMD(N)    pre_loop VDPpsetBP4(DX, DY, FC, N); post_linexmaj_v9k(256,4096) break;
                SWITCHCMD
                #undef DOCMD
                break;
            default :
                #define DOCMD(N)    pre_loop VDPpsetBP4(DX, DY, FC, N); post_linexmaj_v9k(256,4096) break;
                SWITCHCMD
                #undef DOCMD
                break;
            }
            break;
        case 512:
            switch (V9KImgHeight)
            {
            case 512:   /* BD16(16BytePerPixel) */
                #define DOCMD(N)    pre_loop VDPpsetBD16_512(DX, DY, FC, N); post_linexmaj_v9k(512,512) break;
                SWITCHCMD
                #undef DOCMD
                break;
            case 2048:  /* P2 Mode or BP4 */
                #define DOCMD(N)    pre_loop VDPpsetBP4_512(DX, DY, FC, N); post_linexmaj_v9k(512,2048) break;
                SWITCHCMD
                #undef DOCMD
                break;
            }
            break;
        case 1024:
            #define DOCMD(N)    pre_loop VDPpsetBP4_1024(DX, DY, FC, N); post_linexmaj_v9k(1024,1024) break;
            SWITCHCMD
            #undef DOCMD
            break;
        default:
            break;
        }
    }
    else
    {
        switch (V9KImgWidth)
        {
        case 256:
            switch (V9KImgHeight)
            {
            case 2048:  /* P1 Mode */
                #define DOCMD(N)    pre_loop VDPpsetBP4(DX, DY, FC, N); post_lineymaj_v9k(256,2048) break;
                SWITCHCMD
                #undef DOCMD
                break;
            case 4096:  /* BP4 */
                #define DOCMD(N)    pre_loop VDPpsetBP4(DX, DY, FC, N); post_lineymaj_v9k(256,4096) break;
                SWITCHCMD
                #undef DOCMD
                break;
            default:
                #define DOCMD(N)    pre_loop VDPpsetBP4(DX, DY, FC, N); post_lineymaj_v9k(256,4096) break;
                SWITCHCMD
                #undef DOCMD
                break;
            }
            break;
        case 512:
            switch (V9KImgHeight)
            {
            case 512:   /* BD16(16BytePerPixel) */
                #define DOCMD(N)    pre_loop VDPpsetBD16_512(DX, DY, FC, N); post_lineymaj_v9k(512, 512) break;
                SWITCHCMD
                #undef DOCMD
                break;
            case 2048:  /* P2 Mode or BP4 */
                #define DOCMD(N)    pre_loop VDPpsetBP4_512(DX, DY, FC, N); post_lineymaj_v9k(512, 2048) break;
                SWITCHCMD
                #undef DOCMD
                break;
            }
            break;
        default:
            break;
        }
    }

    if ((VdpOpsCntV9990 = cnt) > 0)
    {
        /* Command execution done */
        V9990Port[5] &= 0xFE;
        VdpEngineV9990 = 0;
        V9990VDP[36] = DX & 0xFF;
        V9990VDP[37] = (DX >> 8) & 0x07;
        V9990VDP[38] = DY & 0xFF;
        V9990VDP[39] = (DY >> 8) & 0x0F;
    }
    else
    {
        MMCV9990.DX = DX;
        MMCV9990.DY = DY;
        MMCV9990.ASX = ASX;
        MMCV9990.ADX = ADX;
    }
}


void SrchEngineV9990(void)
{
    register int SX = MMCV9990.SX;
    register int SY = MMCV9990.SY;
    register int TX = MMCV9990.TX;
    register int ANX = MMCV9990.ANX;
    register int cnt, FC;
    register int delta;
    register byte NEQ;

#define pre_srch_v9k \
    pre_loop \
      if (

#define post_srch_v9k(MX) \
           ==((FC>>(((~SX)&0x01)<<2))&0x0F)^NEQ) { \
      V9990Port[5] |=0x10; /* Border detected */ \
      break; \
    } \
    SX = SX+TX; \
    if ((SX & MX) || (SX==-1)) { \
      V9990Port[5] &= 0xEF; /* Border not detected */ \
      break; \
    } \
  }

    delta = 60;
    cnt = VdpOpsCntV9990;

    NEQ = (V9990VDP[44] & 0x02) != 2;

    //NEQ = V9990VDP[44] & 0x02 ? 0 : 1;

    FC = !V9KScrMode ? (V9990VDP[37] & 0x02 ? ((V9990VDP[49] << 8) | V9990VDP[49]) : ((V9990VDP[48] << 8) | V9990VDP[48])) : (V9990VDP[48] << 8) | V9990VDP[49];
    //FC = V9990VDP[48] << 8 | V9990VDP[49];

    switch (V9KImgWidth)
    {
    case 256:
        switch (V9KImgHeight)
        {
        case 2048:  /* P1 Mode */
        case 4096:  /* BP4 */
            pre_srch_v9k VDPpointBP4(SX, SY) post_srch_v9k(256)
            break;
        default:
            pre_srch_v9k VDPpointBP4(SX, SY) post_srch_v9k(256)
            break;
        }
        break;
    case 512:
        switch (V9KImgHeight)
        {
        case 512:   /* BD16(16BytePerPixel) */
            pre_srch_v9k VDPpointBD16(SX, SY) post_srch_v9k(512)
            break;
        case 2048:  /* BP4 */
            pre_srch_v9k VDPpoint512(SX, SY) post_srch_v9k(512)
            break;
        default:
            pre_srch_v9k VDPpoint512(SX, SY) post_srch_v9k(512)
            break;
        }
        break;
    case 1024:
        pre_srch_v9k VDPpoint1024(SX, SY) post_srch_v9k(1024)
        break;
    default:
        break;
    }

        //if (((VdpOpsCntV9990 = cnt) > 0) || ((V9990Port[5] & 0x10)==0x10))
    if ((VdpOpsCntV9990 = cnt) > 0)
        {
            if (!(V9990Port[5] & 0x10))
            {
                if (TX == -1)SX = 0x7FF;
            }
            /* Command execution done */
            V9990Port[5] &= 0xFE;
            V9990VDP[32] = SX & 0xFF;
            V9990VDP[33] = (SX >> 8) & 0x07;
            V9990VDP[53] = SX & 0xFF;
            V9990VDP[54] = (SX >> 8) & 0x07;
            VdpEngineV9990 = 0;
        }
        else
        {
            MMCV9990.SX = SX;
            MMCV9990.SY = SY;
        }
}


void PointEngineV9990(void)
{
    register int DY = MMCV9990.DY;
    register int SY = MMCV9990.SY;
    register int SYOff = 0;
    register unsigned char POINTData = 0;
    if ((V9990Port[5] & 0x80) != 0x80)
    {
        if (V9KScrMode == 0)
        {
            if (V9990VDP[33] & 0x02)SYOff = 2048;
        }
        for (int i = 0; i < 2; i++)
        {
            switch (V9KImgWidth)
            {
            case 256:
                switch (V9KPixelRes)
                {
                case 1:
                    if (i == 0)V9990Port[2] = (V9990Port[2] & 0x0F) | (VDPpointBP4(MMCV9990.ASX, MMCV9990.SY | SYOff) << 4);
                    else V9990Port[2] = (V9990Port[2] & 0xF0) | VDPpointBP4(MMCV9990.ASX, MMCV9990.SY | SYOff);
                    break;
                case 2:
                    //POINTData |= *VDP_VRMP_BPP8(MMCV9990.ASX, MMCV9990.SY);
                    V9990Port[2] |= *VDP_VRMP_BPP8(MMCV9990.ASX, MMCV9990.SY);
                    i++;
                    break;
                case 3:
                    // POINTData |= ((*V9KVRAM + (((MMCV9990.SY << 9) + (MMCV9990.ASX << 1)) & 0x7FFFF)) << 8) |
                    //     (*V9KVRAM + (((MMCV9990.SY << 9) + (MMCV9990.ASX << 1) + 1) & 0x7FFFF));
                    V9990Port[2] |= ((*V9KVRAM + (((MMCV9990.SY << 9) + (MMCV9990.ASX << 1)) & 0x7FFFF)) << 8) |
                        (*V9KVRAM + (((MMCV9990.SY << 9) + (MMCV9990.ASX << 1) + 1) & 0x7FFFF));
                    i++;
                    break;
                default:
                    POINTData |= VDPpointBP4(MMCV9990.ASX, MMCV9990.SY);
                    break;
                }
                break;
            case 512:
                switch (V9KPixelRes)
                {
                case 1:
                    //V9990Port[2] = V9990Port[2] & (0x0F << ((i & 0x01) << 2)) | VDPpoint512(MMCV9990.ASX, MMCV9990.SY);
                    if (i == 0)V9990Port[2] = (V9990Port[2] & 0x0F) | (VDPpoint512(MMCV9990.ASX, MMCV9990.SY | SYOff) << 4);
                    else V9990Port[2] = (V9990Port[2] & 0xF0) | VDPpoint512(MMCV9990.ASX, MMCV9990.SY | SYOff);
                    break;
                case 3:
                    //POINTData |= ((*V9KVRAM + (((MMCV9990.SY << 10) + (MMCV9990.ASX << 1)) & 0x7FFFF)) << 8) |
                    //    (*V9KVRAM + (((MMCV9990.SY << 10) + (MMCV9990.ASX << 1) + 1) & 0x7FFFF));
                    V9990Port[2] |= ((*V9KVRAM + (((MMCV9990.SY << 10) + (MMCV9990.ASX << 1)) & 0x7FFFF)) << 8) |
                        (*V9KVRAM + (((MMCV9990.SY << 10) + (MMCV9990.ASX << 1) + 1) & 0x7FFFF));
                    i++;
                    break;
                default:
                    //POINTData |= VDPpoint512(MMCV9990.ASX, MMCV9990.SY);
                    if (i == 0)V9990Port[2] = (V9990Port[2] & 0x0F) | (VDPpoint512(MMCV9990.ASX, MMCV9990.SY | SYOff) << 4);
                    else V9990Port[2] = (V9990Port[2] & 0xF0) | VDPpoint512(MMCV9990.ASX, MMCV9990.SY | SYOff);
                    break;
                }
                break;
            case 1024:
                //POINTData |= VDPpoint1024(MMCV9990.ASX, MMCV9990.SY);
                V9990Port[2] |= VDPpoint1024(MMCV9990.ASX, MMCV9990.SY);
                break;
            default:
                break;
            }

            V9990Port[5] |= 0x80;
        }
    }
    /* Command execution done */
    V9990Port[5] &= 0xFE;
    //V9990Port[2] = POINTData;
    VdpEngineV9990 = 0;
}


void PsetEngineV9990(void)
{
    register byte LO = MMCV9990.LO;
    register int FC = V9990VDP[48] << 8 | V9990VDP[49];
    VdpOpsCntV9990 -= 60;
    switch (V9KImgWidth)
    {
    case 256:
        switch (V9KImgHeight)
        {
        case 2048:  /* P1 Mode */
        case 4096:  /* BP4 */
            VDPpsetBP4(MMCV9990.DX, MMCV9990.DY, FC, LO);
            break;
        default:
            VDPpsetBP4(MMCV9990.DX, MMCV9990.DY, FC, LO);
            break;
        }
        break;
    case 512:
        switch (V9KImgHeight)
        {
        case 512:   /* BD16(16BytePerPixel) */
            VDPpsetBD16(MMCV9990.DX, MMCV9990.DY, FC, LO);
            break;
        case 2048:  /* BP4 */
            VDPpsetBP4_512(MMCV9990.DX, MMCV9990.DY, FC, LO);
            break;
        default:
            VDPpsetBP4_512(MMCV9990.DX, MMCV9990.DY, FC, LO);
            break;
        }
        break;
    case 1024:
        VDPpsetBP4_1024(MMCV9990.DX, MMCV9990.DY, FC, LO);
        break;
    default:
        break;
    }
    switch (V9990VDP[52] & 0x03)
    {
    case 1:
        MMCV9990.DX++;
        break;
    case 3:
        MMCV9990.DX--;
        break;
    default:
        break;
    }
    switch ((VDP[52] >> 2) & 0x03)
    {
    case 1:
        MMCV9990.DY++;
        break;
    case 3:
        MMCV9990.DY--;
        break;
    default:
        break;
    }
    /* Command execution done */
    V9990Port[5] &= 0xFE;
    VdpEngineV9990 = 0;
    V9990VDP[36] = MMCV9990.DX & 0xFF;
    V9990VDP[37] = (MMCV9990.DX >> 8) & 0x07;
    V9990VDP[38] = MMCV9990.DY & 0xFF;
    V9990VDP[39] = (MMCV9990.DY >> 8) & 0x0F;
}


void AdvnEngineV9990(void)
{
    register byte LO = MMCV9990.LO;
    register int FC = V9990VDP[48] << 8 | V9990VDP[49];
    VdpOpsCntV9990 -= 30;
    switch (V9990VDP[52] & 0x03)
    {
    case 1:
        MMCV9990.DX++;
        break;
    case 3:
        MMCV9990.DX--;
        break;
    default:
        break;
    }
    switch ((VDP[52] >> 2) & 0x03)
    {
    case 1:
        MMCV9990.DY++;
        break;
    case 3:
        MMCV9990.DY--;
        break;
    default:
        break;
    }
    /* Command execution done */
    V9990Port[5] &= 0xFE;
    VdpEngineV9990 = 0;
    V9990VDP[36] = MMCV9990.DX & 0xFF;
    V9990VDP[37] = (MMCV9990.DX >> 8) & 0x07;
    V9990VDP[38] = MMCV9990.DY & 0xFF;
    V9990VDP[39] = (MMCV9990.DY >> 8) & 0x0F;
}


void InitV9990CMD(void)
{
    VdpEngineV9990 = 0;
    V9990PrevData = -1;
}


/** LoopVDP() ************************************************/
/** Run X steps of active VDP command                       **/
/*************************************************************/
void LoopVDPV9990(void)
{
    if (VdpOpsCntV9990 <= 0)
    {
        VdpOpsCntV9990 += 13662;
        if (VdpEngineV9990 && (VdpOpsCntV9990 > 0))VdpEngineV9990();
    }
    else
    {
        VdpOpsCntV9990 = 13662;
        if (VdpEngineV9990) VdpEngineV9990();
    }
}


void FlushV9990(void)
{
    if (VdpEngineV9990 != LmmcEngineV9990)return;
    //while (VdpOpsCntV9990 <= 0 && !(V9990Port[5] & 0x80))
    while ((!(V9990Port[5] & 0x80)) && (VdpEngineV9990))
    {
        LoopVDPV9990();
    }
}


void ExecV9990(void)
{
    if (VdpEngineV9990 != LmmcEngineV9990)return;
    if (VdpEngineV9990 && (VdpOpsCntV9990 > 0))VdpEngineV9990();
}


void ResetV9990VDPRegister(void)
{
    VdpEngineV9990 = 0;
    V9990Port[5] &= 0x7E;
}