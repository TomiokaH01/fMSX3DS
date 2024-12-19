/** fMSX: portable MSX emulator ******************************/
/**                                                         **/
/**                       CommonMux.h                       **/
/**                                                         **/
/** This file instantiates MSX screen drivers for every     **/
/** possible screen depth. It includes common driver code   **/
/** from Common.h and Wide.h.                               **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1994-2020                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/

/** NINTENDO 3DS Version modified by h.tomioka 2023-2024 **/

#ifndef COMMONMUX_H
#define COMMONMUX_H

#include "Common3DS.h"

#undef BPP8
#undef BPP16
#undef BPP24
#undef BPP32

/** Screen Mode Handlers [number of screens + 1] *************/
extern void (*RefreshLine[MAXSCREEN + 2])(byte Y);

#define BPP8
#define pixel            unsigned char
#define FirstLine        FirstLine_8
#define Sprites          Sprites_8
#define ColorSprites     ColorSprites_8
#define ColorSpritesScr78 ColorSpritesScr78_8
#define RefreshBorder    RefreshBorder_8
#define RefreshBorder512 RefreshBorder512_8
#define ClearLine        ClearLine_8
#define ClearLine512     ClearLine512_8
#define YJKColor         YJKColor_8
#define YAEColor         YAEColor_8
#define RefreshScreen    RefreshScreen_8
#define RefreshLineF     RefreshLineF_8
#define RefreshLine0     RefreshLine0_8
#define RefreshLine1     RefreshLine1_8
#define RefreshLine2     RefreshLine2_8
#define RefreshLine3     RefreshLine3_8
#define RefreshLine4     RefreshLine4_8
#define RefreshLine5     RefreshLine5_8
#define RefreshLine6     RefreshLine6_8
#define RefreshLine7     RefreshLine7_8
#define RefreshLine8     RefreshLine8_8
#define RefreshLine10    RefreshLine10_8
#define RefreshLine12    RefreshLine12_8
#define RefreshLineTx80  RefreshLineTx80_8
#include "Common3DS.h"
#undef pixel
#undef FirstLine
#undef Sprites
#undef ColorSprites
#undef ColorSpritesScr78
#undef RefreshBorder  
#undef RefreshBorder512
#undef ClearLine      
#undef ClearLine512
#undef YJKColor       
#undef RefreshScreen  
#undef RefreshLineF   
#undef RefreshLine0   
#undef RefreshLine1   
#undef RefreshLine2   
#undef RefreshLine3   
#undef RefreshLine4   
#undef RefreshLine5   
#undef RefreshLine6   
#undef RefreshLine7   
#undef RefreshLine8   
#undef RefreshLine10  
#undef RefreshLine12  
#undef RefreshLineTx80
#undef BPP8

#define BPP16
#define pixel            unsigned short
#define FirstLine        FirstLine_16
#define Sprites          Sprites_16
#define ColorSprites     ColorSprites_16
#define ColorSpritesScr78 ColorSpritesScr78_16
#define RefreshBorder    RefreshBorder_16
#define RefreshBorder512 RefreshBorder512_16
#define ClearLine        ClearLine_16
#define ClearLine512     ClearLine512_16
#define YJKColor         YJKColor_16
#define YAEColor         YAEColor_16
#define RefreshScreen    RefreshScreen_16
#define RefreshLineF     RefreshLineF_16
#define RefreshLine0     RefreshLine0_16
#define RefreshLine1     RefreshLine1_16
#define RefreshLine2     RefreshLine2_16
#define RefreshLine3     RefreshLine3_16
#define RefreshLine4     RefreshLine4_16
#define RefreshLine5     RefreshLine5_16
#define RefreshLine6     RefreshLine6_16
#define RefreshLine7     RefreshLine7_16
#define RefreshLine8     RefreshLine8_16
#define RefreshLine10    RefreshLine10_16
#define RefreshLine12    RefreshLine12_16
#define RefreshLineTx80  RefreshLineTx80_16
#include "Common3DS.h"
#undef pixel
#undef FirstLine
#undef Sprites
#undef ColorSprites
#undef ColorSpritesScr78
#undef RefreshBorder  
#undef RefreshBorder512
#undef ClearLine      
#undef ClearLine512
#undef YJKColor       
#undef RefreshScreen  
#undef RefreshLineF   
#undef RefreshLine0   
#undef RefreshLine1   
#undef RefreshLine2   
#undef RefreshLine3   
#undef RefreshLine4   
#undef RefreshLine5   
#undef RefreshLine6   
#undef RefreshLine7   
#undef RefreshLine8   
#undef RefreshLine10  
#undef RefreshLine12  
#undef RefreshLineTx80
#undef BPP16

#define BPP32
#define pixel            unsigned int
#define FirstLine        FirstLine_32
#define Sprites          Sprites_32
#define ColorSprites     ColorSprites_32
#define ColorSpritesScr78 ColorSpritesScr78_32
#define RefreshBorder    RefreshBorder_32
#define RefreshBorder512 RefreshBorder512_32
#define ClearLine        ClearLine_32
#define ClearLine512     ClearLine512_32
#define YJKColor         YJKColor_32
#define YAEColor         YAEColor_32
#define RefreshScreen    RefreshScreen_32
#define RefreshLineF     RefreshLineF_32
#define RefreshLine0     RefreshLine0_32
#define RefreshLine1     RefreshLine1_32
#define RefreshLine2     RefreshLine2_32
#define RefreshLine3     RefreshLine3_32
#define RefreshLine4     RefreshLine4_32
#define RefreshLine5     RefreshLine5_32
#define RefreshLine6     RefreshLine6_32
#define RefreshLine7     RefreshLine7_32
#define RefreshLine8     RefreshLine8_32
#define RefreshLine10    RefreshLine10_32
#define RefreshLine12    RefreshLine12_32
#define RefreshLineTx80  RefreshLineTx80_32
#include "Common3DS.h"
#undef pixel
#undef FirstLine
#undef Sprites
#undef ColorSprites
#undef ColorSpritesScr78
#undef RefreshBorder  
#undef RefreshBorder512
#undef ClearLine      
#undef ClearLine512
#undef YJKColor       
#undef RefreshScreen  
#undef RefreshLineF   
#undef RefreshLine0   
#undef RefreshLine1   
#undef RefreshLine2   
#undef RefreshLine3   
#undef RefreshLine4   
#undef RefreshLine5   
#undef RefreshLine6   
#undef RefreshLine7   
#undef RefreshLine8   
#undef RefreshLine10  
#undef RefreshLine12  
#undef RefreshLineTx80
#undef BPP32

/** SetScreenDepth() *****************************************/
/** Fill fMSX screen driver array with pointers matching    **/
/** the given image depth.                                  **/
/*************************************************************/
int SetScreenDepth(int Depth)
{
    if (Depth <= 8)
    {
        Depth = 8;
        RefreshLine[0] = RefreshLine0_8;
        RefreshLine[1] = RefreshLine1_8;
        RefreshLine[2] = RefreshLine2_8;
        RefreshLine[3] = RefreshLine3_8;
        RefreshLine[4] = RefreshLine4_8;
        RefreshLine[5] = RefreshLine5_8;
        RefreshLine[6] = RefreshLine6_8;
        RefreshLine[7] = RefreshLine7_8;
        RefreshLine[8] = RefreshLine8_8;
        RefreshLine[9] = 0;
        RefreshLine[10] = RefreshLine10_8;
        RefreshLine[11] = RefreshLine10_8;
        RefreshLine[12] = RefreshLine12_8;
        RefreshLine[13] = RefreshLineTx80_8;
    }
    else if (Depth <= 16)
    {
        Depth = 16;
        RefreshLine[0] = RefreshLine0_16;
        RefreshLine[1] = RefreshLine1_16;
        RefreshLine[2] = RefreshLine2_16;
        RefreshLine[3] = RefreshLine3_16;
        RefreshLine[4] = RefreshLine4_16;
        RefreshLine[5] = RefreshLine5_16;
        RefreshLine[6] = RefreshLine6_16;
        RefreshLine[7] = RefreshLine7_16;
        RefreshLine[8] = RefreshLine8_16;
        RefreshLine[9] = 0;
        RefreshLine[10] = RefreshLine10_16;
        RefreshLine[11] = RefreshLine10_16;
        RefreshLine[12] = RefreshLine12_16;
        RefreshLine[13] = RefreshLineTx80_16;
    }
    else if (Depth <= 32)
    {
        Depth = 32;
        RefreshLine[0] = RefreshLine0_32;
        RefreshLine[1] = RefreshLine1_32;
        RefreshLine[2] = RefreshLine2_32;
        RefreshLine[3] = RefreshLine3_32;
        RefreshLine[4] = RefreshLine4_32;
        RefreshLine[5] = RefreshLine5_32;
        RefreshLine[6] = RefreshLine6_32;
        RefreshLine[7] = RefreshLine7_32;
        RefreshLine[8] = RefreshLine8_32;
        RefreshLine[9] = 0;
        RefreshLine[10] = RefreshLine10_32;
        RefreshLine[11] = RefreshLine10_32;
        RefreshLine[12] = RefreshLine12_32;
        RefreshLine[13] = RefreshLineTx80_32;
    }
    else
    {
        /* Wrong screen depth */
        Depth = 0;
    }

    /* Done */
    return(Depth);
}


#ifdef VDP_V9990
static int V9KFirstLine = 0;

/** RefreshBorder() ******************************************/
/** This function is called from RefreshLine#() to refresh  **/
/** the screen border. It returns a pointer to the start of **/
/** scanline Y in XBuf or 0 if scanline is beyond XBuf.     **/
/*************************************************************/
unsigned short* RefreshBorderV9K(register byte Y, register unsigned short C)
{
    register unsigned short* P;
    register int H, AdjX, AdjY;

    /* First line number in the buffer */
    //if (!Y) FirstLine = (ScanLines212 ? 8 : 18) + VAdjust;
    if (!Y)V9KFirstLine = 8;

    /* Return 0 if we've run out of the screen buffer due to overscan */
    //if (Y + FirstLine >= HEIGHT) return(0);
    if (Y + V9KFirstLine >= 256) return(0);

    /* Set up the transparent color */
    //V9kXPal[0] = (!BGColor || SolidColor0) ? V9kXPal0 : V9kXPal[BGColor];

    /* Start of the buffer */
    //P = (unsigned short*)XBuf;
    P = (unsigned short*)V9KXBuf;
    AdjX = -7 + ((V9990VDP[16] & 0x0F) ^ 0x07);
    AdjY = -7 + (((V9990VDP[16] >> 4) & 0x0F) ^ 0x07);

    /* Paint top of the screen */
    //if (!Y) for (H = WIDTH * V9KFirstLine - 1; H >= 0; H--) P[H] = C;
    if(!Y) for (H = WIDTH * (V9KFirstLine + AdjY) - 1; H >= 0; H--) P[H] = C;

    /* Start of the line */
    P += WIDTH * (V9KFirstLine + (Y+AdjY)) + AdjX;

    /* Paint left/right borders */
    /* In P1 mode, paint border after sprite drawing.  */
    if (V9KScrMode)
    {
        for (H = (WIDTH - 256) / 2; H > 0; H--) P[H - 1] = C;
        for (H = (WIDTH - 256) / 2; H > 0; H--) P[WIDTH - H] = C;
    }

    /* Paint bottom of the screen */
    H = 212;
    if (Y == H - 1) for (H = WIDTH * (HEIGHT - H - V9KFirstLine + 1 - AdjY) - 1; H >= WIDTH; H--) P[H] = C;

    /* Return pointer to the scanline in XBuf */
   return(P + ((WIDTH - 256) / 2));
}


unsigned short* RefreshBorderV9K512(register byte Y, register unsigned short C)
{
    register unsigned short* P;
    register int H, AdjX, AdjY;

    /* First line number in the buffer */
    if (!Y) V9KFirstLine = 8;

    /* Return 0 if we've run out of the screen buffer due to overscan */
    if (Y + V9KFirstLine >= 256) return(0);

    /* Set up the transparent color */
    //V9kXPal[0] = (!BGColor || SolidColor0) ? V9kXPal0 : V9kXPal[BGColor];

    /* Start of the buffer */
    P = (unsigned short*)WBuf;

    AdjX = -7 + ((V9990VDP[16] & 0x0F) ^ 0x07);
    AdjY = -7 + (((V9990VDP[16] >> 4) & 0x0F) ^ 0x07);

    /* Paint top of the screen */
    if (!Y) for (H = 2 * WIDTH * (V9KFirstLine + AdjY) - 1; H >= 0; H--) P[H] = C;

    /* Start of the line */
    P += 2 * (WIDTH * (V9KFirstLine + AdjY + Y) +AdjX);

    /* Paint left/right borders */
    for (H = (WIDTH - 256); H > 0; H--) P[H - 1] = C;
    for (H = (WIDTH - 256); H > 0; H--) P[2 * WIDTH - H] = C;

    /* Paint bottom of the screen */
    H = 212;
    if (Y == H - 1) for (H = 2 * WIDTH * (HEIGHT - H - V9KFirstLine + 1 - AdjY) - 1; H >= 2 * WIDTH; H--) P[H - 2 * WIDTH] = C;

    /* Return pointer to the scanline in XBuf */
    return(P + (WIDTH - 256));
}


void SpritesDrawP2(register byte Y, unsigned short* P)
{
    register byte C, I, J, PaletteOff, SprCnt;
    register byte* PT, * AT, X, Color;
    register int L, K, M, N;
    register unsigned short* Buf;
    byte SprDraw[17] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };

    /* SPD(Sprite Disable) */
    //if ((V9990VDP[8] & 0x040))return;
    if ((V9KVDP8Val & 0x040))return;

    AT = V9KVRAM + 0x7BE00;
    SprCnt = 0;

    for (L = 0; L < 125; ++L, AT += 4)
    {
        K = AT[0];
        /* Mark all valid sprites with 1s, break at 16 sprites */
        if (((Y - K - 1) & 0xFF) >= 16)continue;
        //if ((Y > K) && (Y <= K + 16))
        {
            SprDraw[SprCnt] = L;
            SprCnt++;
            /* If we exceed the maximum number of sprites per line... */
            if (SprCnt >= 16)
            {
                break;
                //if (!OPTION(MSX_ALLSPRITE)) break;
            }
        }
    }
    AT = V9KVRAM + 0x7BE00;
    for (M = SprCnt - 1; M >= 0; --M)
    {
        L = SprDraw[M];
        K = AT[L << 2];     /* Y position */
        I = AT[(L << 2) + 3]; /* bit4:disbale(1)  bit5:priority */
        if ((I & 0x10))continue;
        J = (Y - K - 1) & 0xFF;
        N = AT[(L << 2) + 2] | (((int)I & 0x03) << 8);
        if ((N >= 512) && (N < 1024 - 16))continue;
        PaletteOff = (I & 0xC0) >> 2;
        PT = V9KVRAM + ((((uint)V9990VDP[25] & 0x0F) << 15) + (((uint)AT[(L << 2) + 1] >> 5) << 12)
            + (((uint)AT[(L << 2) + 1] & 0x1F) << 3) + ((uint)J << 8));
        if (N >= 512)N -= 1024;
        Buf = P + N;
        if (!(I & 0x20))
        {
            for (X = 0; X < 8; X++, PT++, Buf += 2)
            {
                C = PT[0] >> 4;
                if (C)Buf[0] = V9kXPal[PaletteOff | C];
                C = PT[0] & 0x0F;
                if (C)Buf[1] = V9kXPal[PaletteOff | C];
            }
        }
        else
        {
            for (X = 0; X < 8; X++, PT++, Buf += 2)
            {
                C = PT[0] >> 4;
                if ((C) && (Buf[0]== V9kXPal[V9990VDP[15] & 0x3F]))Buf[0] = V9kXPal[PaletteOff | C];
                C = PT[0] & 0x0F;
                if ((C) && (Buf[1]== V9kXPal[V9990VDP[15] & 0x3F]))Buf[1] = V9kXPal[PaletteOff | C];
            }
        }
    }
    return;
}


/* Cursor for V9990 (Screen ModeB0-B7) */
void CursorDraw(register byte Y, unsigned short* P)
{
    register byte C, I, PaletteOff;
    register byte* PT, * AT, X;
    register int J, L, K, M, N;
    register unsigned short* Buf;
    register unsigned int ColorVal;

    /* SPD(Sprite Disable) */
    //if ((V9990VDP[8] & 0x040))return;
    if ((V9KVDP8Val & 0x040))return;

    AT = V9KVRAM + (0x7FE00 + 8);
    PaletteOff = (V9990VDP[28] & 0x0F) << 2;
    for (M = 1; M >=0; M--, AT -=8)
    {
        K = AT[0] | ((AT[2]&0x01)<<8);     /* Y position */
        I = AT[6]; /* bit4:disbale(1)  bit5:EOR  bit6,7:Cursor Color Value */
        if ((I & 0x10))continue;
        if (!(I & 0xE0))continue;   /* bit5-7 == 0 */
         /* V9990VDP[7] & 0x07) == 0x06-> Interlace  with double height */
        J = ((V9990VDP[7] & 0x07) == 0x06) ? ((int)Y<<1) - K - 1 : (int)Y - K - 1;
        J = (V9990Port[5] & 0x02) ? (J + 1) & 511 : J & 511;
        //J = ((int)Y - K - 1) & 511;
        if (J >= 32)continue;
        N = AT[4] | (((int)I & 0x03) << 8);
        if ((N >= V9KImgWidth) && (N < 1024 - 32))continue;
        C = ((I & 0xC0) >> 6) | ((V9990VDP[28]&0x0F)<<2);
        if (C)
        {
            ColorVal = V9kXPal[C | PaletteOff];
            PT = V9KVRAM + (0x7FF00 | (J << 2) | (M << 7));
            if (V9KImgWidth >= 1024)N >>= 1;
            if (V9KImgWidth >= 512 && (!IsWide))
            {
                N >>= 1;
                Buf = P + (N >= V9KImgWidth ? N - 1024 : N);
                if (I & 0xC0)
                {
                    ColorVal = V9kXPal[C | PaletteOff];
                    if (I & 0x20)ColorVal = (ColorVal & 0xff000000) | (~ColorVal & 0x00ffffff);      // invert but maintain alpha
                    for (X = 0; X < 4; X++, PT++, Buf += 4)
                    {
                        L = PT[0];
                        if ((L & 0x80) || (L & 0x40))Buf[0] = ColorVal;
                        if ((L & 0x20) || (L & 0x10))Buf[1] = ColorVal;
                        if ((L & 0x08) || (L & 0x04))Buf[2] = ColorVal;
                        if ((L & 0x02) || (L & 0x01))Buf[3] = ColorVal;
                    }
                }
                /* EOR Color. Code taken from WebMSX. */
                /* https://webmsx.org/ */
                else
                {
                    if ((L & 0x80) || (L & 0x40)) { ColorVal = Buf[0]; Buf[0] = (ColorVal & 0xFF000000) | (~ColorVal & 0x00FFFFFF); }
                    if ((L & 0x20) || (L & 0x10)) { ColorVal = Buf[1]; Buf[1] = (ColorVal & 0xFF000000) | (~ColorVal & 0x00FFFFFF); }
                    if ((L & 0x08) || (L & 0x04)) { ColorVal = Buf[2]; Buf[2] = (ColorVal & 0xFF000000) | (~ColorVal & 0x00FFFFFF); }
                    if ((L & 0x02) || (L & 0x01)) { ColorVal = Buf[3]; Buf[3] = (ColorVal & 0xFF000000) | (~ColorVal & 0x00FFFFFF); }
                }
            }
            else
            {
                Buf = P + (N >= V9KImgWidth ? N - 1024 : N);
                if (I & 0xC0)
                {
                    ColorVal = V9kXPal[C | PaletteOff];
                    if (I & 0x20)ColorVal = (ColorVal & 0xff000000) | (~ColorVal & 0x00ffffff);      // invert but maintain alpha
                    for (X = 0; X < 4; X++, PT++, Buf += 8)
                    {
                        L = PT[0];
                        if (L & 0x80)Buf[0] = ColorVal;
                        if (L & 0x40)Buf[1] = ColorVal;
                        if (L & 0x20)Buf[2] = ColorVal;
                        if (L & 0x10)Buf[3] = ColorVal;
                        if (L & 0x08)Buf[4] = ColorVal;
                        if (L & 0x04)Buf[5] = ColorVal;
                        if (L & 0x02)Buf[6] = ColorVal;
                        if (L & 0x01)Buf[7] = ColorVal;
                    }
                }
                /* EOR Color. Code taken from WebMSX. */
                /* https://webmsx.org/ */
                else
                {
                    for (X = 0; X < 4; X++, PT++, Buf += 8)
                    {
                        L = PT[0];
                        if (L & 0x80) { ColorVal = Buf[0]; Buf[0] = (ColorVal & 0xFF000000) | (~ColorVal & 0x00FFFFFF); }
                        if (L & 0x40) { ColorVal = Buf[1]; Buf[1] = (ColorVal & 0xFF000000) | (~ColorVal & 0x00FFFFFF); }
                        if (L & 0x20) { ColorVal = Buf[2]; Buf[2] = (ColorVal & 0xFF000000) | (~ColorVal & 0x00FFFFFF); }
                        if (L & 0x10) { ColorVal = Buf[3]; Buf[3] = (ColorVal & 0xFF000000) | (~ColorVal & 0x00FFFFFF); }
                        if (L & 0x08) { ColorVal = Buf[4]; Buf[4] = (ColorVal & 0xFF000000) | (~ColorVal & 0x00FFFFFF); }
                        if (L & 0x04) { ColorVal = Buf[5]; Buf[5] = (ColorVal & 0xFF000000) | (~ColorVal & 0x00FFFFFF); }
                        if (L & 0x02) { ColorVal = Buf[6]; Buf[6] = (ColorVal & 0xFF000000) | (~ColorVal & 0x00FFFFFF); }
                        if (L & 0x01) { ColorVal = Buf[7]; Buf[7] = (ColorVal & 0xFF000000) | (~ColorVal & 0x00FFFFFF); }
                    }
                }
            }
        }
    }
    return;
}


void RefreshLineB1(register byte Y)
{
    register unsigned short* P;
    register u32* PL;
    register byte X, XE, * T;
    //register int PaletteOff;
    register uint fixY;
    unsigned int* PV9kXPal;

    P = RefreshBorderV9K(Y, V9kXPal[V9990VDP[15]&0x3F]);
    if (!P) return;

    //if (!(V9990VDP[8] & 0x80)) ClearLine((pixel*)P, V9kXPal[V9990VDP[15] & 0x3F]);
    if (!(V9KVDP8Val & 0x80)) ClearLine((pixel*)P, V9kXPal[V9990VDP[15] & 0x3F]);
    else
    {
        //fixY = (uint)Y + (uint)V9990VDP[17] + ((uint)(V9990VDP[18] & 0x1F) << 8);
        //fixY = (uint)Y + (uint)V9990VDP[17] + V9KYScrollH;
        fixY = (uint)Y + V9KYScrollOff2;
        if (V9990VDP[18] & 0x80)fixY &= 511;
        else if (V9990VDP[18] & 0x40)fixY &= 255;
        else fixY &= V9KImgHeight - 1;
        if ((V9990VDP[7] & 0x06) == 0x06)
        {
            fixY <<= 1;
            if (V9990Port[5] & 0x02)fixY++;
        }
        T = V9KVRAM + ((fixY *(V9KImgWidth>>1)) & 0x7FFFF);
        T += V9990VDP[19] | V9990VDP[20] << 3;

        XE = 16;
        PL = (u32*)P;

        PV9kXPal = V9kXPal + ((V9990VDP[13] & 0x0C) << 2);
        for (X = 0; X < XE; X++, T += 8, PL += 8)
            {
                PL[0] = (PV9kXPal[T[0] >> 4]) | (PV9kXPal[T[0] & 0x0F]) << 16;
                PL[1] = (PV9kXPal[T[1] >> 4]) | (PV9kXPal[T[1] & 0x0F]) << 16;
                PL[2] = (PV9kXPal[T[2] >> 4]) | (PV9kXPal[T[2] & 0x0F]) << 16;
                PL[3] = (PV9kXPal[T[3] >> 4]) | (PV9kXPal[T[3] & 0x0F]) << 16;
                PL[4] = (PV9kXPal[T[4] >> 4]) | (PV9kXPal[T[4] & 0x0F]) << 16;
                PL[5] = (PV9kXPal[T[5] >> 4]) | (PV9kXPal[T[5] & 0x0F]) << 16;
                PL[6] = (PV9kXPal[T[6] >> 4]) | (PV9kXPal[T[6] & 0x0F]) << 16;
                PL[7] = (PV9kXPal[T[7] >> 4]) | (PV9kXPal[T[7] & 0x0F]) << 16;
            }
        CursorDraw(Y, P);
    }
}


void RefreshLineB3(register byte Y)
{
    if (!IsWide)
    {
        register unsigned short* P;
        u32* PL;
        register byte X, XE, * T;
        int PaletteOff;
        uint fixY;
        unsigned int* PV9kXPal;

        P = RefreshBorderV9K(Y, V9kXPal[V9990VDP[15] & 0x3F]);
        if (!P) return;

        //if (!(V9990VDP[8] & 0x80)) ClearLine((pixel*)P, V9kXPal[V9990VDP[15] & 0x3F]);
        if (!(V9KVDP8Val & 0x80)) ClearLine((pixel*)P, V9kXPal[V9990VDP[15] & 0x3F]);
        else
        {
            //fixY = (uint)Y + (uint)V9990VDP[17] + ((uint)V9990VDP[18] << 8);
            //fixY = (uint)Y + (uint)V9990VDP[17] + ((uint)(V9990VDP[18] & 0x1F) << 8);
            //fixY = (uint)Y + (uint)V9990VDP[17] + V9KYScrollH;
            fixY = (uint)Y + V9KYScrollOff2;
            if (V9990VDP[18] & 0x80)fixY &= 511;
            else if (V9990VDP[18] & 0x40)fixY &= 255;
            else fixY &= V9KImgHeight - 1;
            if ((V9990VDP[7] & 0x06) == 0x06)
            {
                fixY <<= 1;
                if (V9990Port[5] & 0x02)fixY++;
            }
            T = V9KVRAM + ((fixY * (V9KImgWidth >> 1)) & 0x7FFFF);
            T += V9990VDP[19] | V9990VDP[20] << 3;

            XE = 32;
            PV9kXPal = V9kXPal + ((V9990VDP[13] & 0x0C) << 2);
            for (X = 0; X < XE; X++, P += 8, T += 8)
            {
                P[0] = ColAve((PV9kXPal[T[0] >> 4]), (PV9kXPal[T[0] & 0x0F]));
                P[1] = ColAve((PV9kXPal[T[1] >> 4]), (PV9kXPal[T[1] & 0x0F]));
                P[2] = ColAve((PV9kXPal[T[2] >> 4]), (PV9kXPal[T[2] & 0x0F]));
                P[3] = ColAve((PV9kXPal[T[3] >> 4]), (PV9kXPal[T[3] & 0x0F]));
                P[4] = ColAve((PV9kXPal[T[4] >> 4]), (PV9kXPal[T[4] & 0x0F]));
                P[5] = ColAve((PV9kXPal[T[5] >> 4]), (PV9kXPal[T[5] & 0x0F]));
                P[6] = ColAve((PV9kXPal[T[6] >> 4]), (PV9kXPal[T[6] & 0x0F]));
                P[7] = ColAve((PV9kXPal[T[7] >> 4]), (PV9kXPal[T[7] & 0x0F]));
            }
            CursorDraw(Y, P-256);
        }
    }
    else
    {
        register unsigned short* P;
        u32* PL;
        register byte X, XE, * T;
        //int PaletteOff;
        uint fixY;
        unsigned int* PV9kXPal;

        P = RefreshBorderV9K512(Y, V9kXPal[V9990VDP[15] & 0x3F]);
        if (!P) return;

        //if (!(V9990VDP[8] & 0x80)) ClearLine512((pixel*)P, V9kXPal[V9990VDP[15] & 0x3F]);
        if (!(V9KVDP8Val & 0x80)) ClearLine512((pixel*)P, V9kXPal[V9990VDP[15] & 0x3F]);
        else
        {
            //fixY = (uint)Y + (uint)V9990VDP[17] + ((uint)V9990VDP[18] << 8);
            //fixY = (uint)Y + (uint)V9990VDP[17] + V9KYScrollH;
            fixY = (uint)Y + V9KYScrollOff2;
            if (V9990VDP[18] & 0x80)fixY &= 511;
            else if (V9990VDP[18] & 0x40)fixY &= 255;
            if ((V9990VDP[7] & 0x06) == 0x06)fixY <<= 1;
            if (V9990Port[5] & 0x02)fixY++;
            T = V9KVRAM + ((fixY * (V9KImgWidth >> 1)) & 0x7FFFF);
            T += V9990VDP[19] | V9990VDP[20] << 3;

            XE = 32;
            PL = (u32*)P;
            PV9kXPal = V9kXPal + ((V9990VDP[13] & 0x0C) << 2);
            for (X = 0; X < XE; X++, P += 16, T += 8, PL += 8)
            {
                PL[0] = (PV9kXPal[T[0] >> 4]) | (PV9kXPal[T[0] & 0x0F]) << 16;
                PL[1] = (PV9kXPal[T[1] >> 4]) | (PV9kXPal[T[1] & 0x0F]) << 16;
                PL[2] = (PV9kXPal[T[2] >> 4]) | (PV9kXPal[T[2] & 0x0F]) << 16;
                PL[3] = (PV9kXPal[T[3] >> 4]) | (PV9kXPal[T[3] & 0x0F]) << 16;
                PL[4] = (PV9kXPal[T[4] >> 4]) | (PV9kXPal[T[4] & 0x0F]) << 16;
                PL[5] = (PV9kXPal[T[5] >> 4]) | (PV9kXPal[T[5] & 0x0F]) << 16;
                PL[6] = (PV9kXPal[T[6] >> 4]) | (PV9kXPal[T[6] & 0x0F]) << 16;
                PL[7] = (PV9kXPal[T[7] >> 4]) | (PV9kXPal[T[7] & 0x0F]) << 16;
            }
            CursorDraw(Y, P);
        }
    }
}


void RefreshLineB6(register byte Y)
{
    if (!IsWide)
    {
        register unsigned short* P;
        u32* PL;
        register byte X, XE, * T;
        //int PaletteOff;
        uint fixY;
        unsigned int* PV9kXPal;

        P = RefreshBorderV9K(Y, V9kXPal[V9990VDP[15] & 0x3F]);
        if (!P) return;

        //if (!(V9990VDP[8] & 0x80)) ClearLine((pixel*)P, V9kXPal[V9990VDP[15] & 0x3F]);
        if (!(V9KVDP8Val & 0x80)) ClearLine((pixel*)P, V9kXPal[V9990VDP[15] & 0x3F]);
        else
        {
            //fixY = (uint)Y + (uint)V9990VDP[17] + ((uint)V9990VDP[18] << 8);
            //fixY = (uint)Y + (uint)V9990VDP[17] + ((uint)(V9990VDP[18] & 0x1F) << 8);
            //fixY = (uint)Y + (uint)V9990VDP[17] + V9KYScrollH;
            fixY = (uint)Y + V9KYScrollOff2;
            if (V9990VDP[18] & 0x80)fixY &= 511;
            else if (V9990VDP[18] & 0x40)fixY &= 255;
            else fixY &= V9KImgHeight - 1;
            //T = V9KVRAM + ((fixY << 9) & 0x7FFFF);
            if ((V9990VDP[7] & 0x06) == 0x06)
            {
                fixY <<= 1;
                if (V9990Port[5] & 0x02)fixY++;
            }
            T = V9KVRAM + ((fixY * (V9KImgWidth >> 1)) & 0x7FFFF);
            T += V9990VDP[19] | V9990VDP[20] << 3;

            XE = 32;
            PV9kXPal = V9kXPal + ((V9990VDP[13] & 0x0C) << 2);

            for (X = 0; X < XE; X++, T += 16, P += 8)
            {
                P[0] = ColAve((ColAve(PV9kXPal[T[0] >> 4], PV9kXPal[T[0] & 0x0F])) , (ColAve(PV9kXPal[T[1] >> 4], PV9kXPal[T[1] & 0x0F])));
                P[1] = ColAve((ColAve(PV9kXPal[T[2] >> 4], PV9kXPal[T[2] & 0x0F])), (ColAve(PV9kXPal[T[3] >> 4], PV9kXPal[T[3] & 0x0F])));
                P[2] = ColAve((ColAve(PV9kXPal[T[4] >> 4], PV9kXPal[T[4] & 0x0F])), (ColAve(PV9kXPal[T[5] >> 4], PV9kXPal[T[5] & 0x0F])));
                P[3] = ColAve((ColAve(PV9kXPal[T[6] >> 4], PV9kXPal[T[6] & 0x0F])), (ColAve(PV9kXPal[T[7] >> 4], PV9kXPal[T[7] & 0x0F])));
                P[4] = ColAve((ColAve(PV9kXPal[T[8] >> 4], PV9kXPal[T[8] & 0x0F])), (ColAve(PV9kXPal[T[9] >> 4], PV9kXPal[T[9] & 0x0F])));
                P[5] = ColAve((ColAve(PV9kXPal[T[10] >> 4], PV9kXPal[T[10] & 0x0F])), (ColAve(PV9kXPal[T[11] >> 4], PV9kXPal[T[11] & 0x0F])));
                P[6] = ColAve((ColAve(PV9kXPal[T[12] >> 4], PV9kXPal[T[12] & 0x0F])), (ColAve(PV9kXPal[T[13] >> 4], PV9kXPal[T[13] & 0x0F])));
                P[7] = ColAve((ColAve(PV9kXPal[T[14] >> 4], PV9kXPal[T[14] & 0x0F])), (ColAve(PV9kXPal[T[15] >> 4], PV9kXPal[T[15] & 0x0F])));
            }
            CursorDraw(Y, P-256);
        }
    }
    else
    {
        register unsigned short* P;
        u32* PL;
        register byte X, XE, * T;
        //int PaletteOff;
        uint fixY;
        unsigned int* PV9kXPal;

        P = RefreshBorderV9K512(Y, V9kXPal[V9990VDP[15] & 0x3F]);
        if (!P) return;

        //if (!(V9990VDP[8] & 0x80)) ClearLine512((pixel*)P, V9kXPal[V9990VDP[15] & 0x3F]);
        if (!(V9KVDP8Val & 0x80)) ClearLine512((pixel*)P, V9kXPal[V9990VDP[15] & 0x3F]);
        else
        {
            //fixY = (uint)Y + (uint)V9990VDP[17] + ((uint)V9990VDP[18] << 8);
            //fixY = (uint)Y + (uint)V9990VDP[17] + V9KYScrollH;
            fixY = (uint)Y + V9KYScrollOff2;
            if (V9990VDP[18] & 0x80)fixY &= 511;
            else if (V9990VDP[18] & 0x40)fixY &= 255;
            //T = V9KVRAM + ((fixY << 9) & 0x7FFFF);
            if ((V9990VDP[7] & 0x06) == 0x06)fixY <<= 1;
            if (V9990Port[5] & 0x02)fixY++;
            T = V9KVRAM + ((fixY * (V9KImgWidth >> 1)) & 0x7FFFF);
            T += V9990VDP[19] | V9990VDP[20] << 3;

            //XE = 32;
            //PaletteOff = (V9990VDP[13] & 0x0C) << 2;

            XE = 32;
            PV9kXPal = V9kXPal + ((V9990VDP[13] & 0x0C) << 2);

            PL = (u32*)P;
            //for (X = 0; X < XE; X++, P += 16, T += 8, PL += 8)
            for (X = 0; X < XE; X++, T += 16, PL += 8)
            {
                PL[0] = ColAve(PV9kXPal[T[0] >> 4], PV9kXPal[T[0] & 0x0F]) | (ColAve(PV9kXPal[T[1] >> 4], PV9kXPal[T[1] & 0x0F]) << 16);
                PL[1] = ColAve(PV9kXPal[T[2] >> 4], PV9kXPal[T[2] & 0x0F]) | (ColAve(PV9kXPal[T[3] >> 4], PV9kXPal[T[3] & 0x0F]) << 16);
                PL[2] = ColAve(PV9kXPal[T[4] >> 4], PV9kXPal[T[4] & 0x0F]) | (ColAve(PV9kXPal[T[5] >> 4], PV9kXPal[T[5] & 0x0F]) << 16);
                PL[3] = ColAve(PV9kXPal[T[6] >> 4], PV9kXPal[T[6] & 0x0F]) | (ColAve(PV9kXPal[T[7] >> 4], PV9kXPal[T[7] & 0x0F]) << 16);
                PL[4] = ColAve(PV9kXPal[T[8] >> 4], PV9kXPal[T[8] & 0x0F]) | (ColAve(PV9kXPal[T[9] >> 4], PV9kXPal[T[9] & 0x0F]) << 16);
                PL[5] = ColAve(PV9kXPal[T[10] >> 4], PV9kXPal[T[10] & 0x0F]) | (ColAve(PV9kXPal[T[11] >> 4], PV9kXPal[T[11] & 0x0F]) << 16);
                PL[6] = ColAve(PV9kXPal[T[12] >> 4], PV9kXPal[T[12] & 0x0F]) | (ColAve(PV9kXPal[T[13] >> 4], PV9kXPal[T[13] & 0x0F]) << 16);
                PL[7] = ColAve(PV9kXPal[T[14] >> 4], PV9kXPal[T[14] & 0x0F]) | (ColAve(PV9kXPal[T[15] >> 4], PV9kXPal[T[15] & 0x0F]) << 16);
            }
            CursorDraw(Y, P);
        }
    }
}


void RefreshLineBP6(register byte Y)
{
    register unsigned short* P;
    register u32* PL;
    register byte X, XE, * T;
    register uint fixY;

    P = RefreshBorderV9K(Y, V9kXPal[V9990VDP[15] & 0x3F]);
    if (!P) return;
    //if (!(V9990VDP[8] & 0x80)) ClearLine((pixel*)P, V9kXPal[V9990VDP[15] & 0x3F]);
    if (!(V9KVDP8Val & 0x80)) ClearLine((pixel*)P, V9kXPal[V9990VDP[15] & 0x3F]);
    else
    {
        //fixY = (uint)Y + (uint)V9990VDP[17] + ((uint)V9990VDP[18] << 8);
        //fixY = (uint)Y + (uint)V9990VDP[17] + ((uint)(V9990VDP[18] & 0x1F) << 8);
        //fixY = (uint)Y + (uint)V9990VDP[17] + V9KYScrollH;
        fixY = (uint)Y + V9KYScrollOff2;
        if (V9990VDP[18] & 0x80)fixY &= 511;
        else if (V9990VDP[18] & 0x40)fixY &= 255;
        else fixY &= V9KImgHeight - 1;
        if ((V9990VDP[7] & 0x06) == 0x06)
        {
            fixY <<= 1;
            if (V9990Port[5] & 0x02)fixY++;
        }
        T = V9KVRAM + ((fixY * V9KImgWidth) & 0x7FFFF);
        T += V9990VDP[19] | V9990VDP[20] << 3;

        XE = 16;

        PL = (u32*)P;
        for (X = 0; X < XE; X++, T += 16, PL += 8)
        {
            PL[0] = V9kXPal[T[0] & 0x3F] | (V9kXPal[T[1] & 0x3F] << 16);
            PL[1] = V9kXPal[T[2] & 0x3F] | (V9kXPal[T[3] & 0x3F] << 16);
            PL[2] = V9kXPal[T[4] & 0x3F] | (V9kXPal[T[5] & 0x3F] << 16);
            PL[3] = V9kXPal[T[6] & 0x3F] | (V9kXPal[T[7] & 0x3F] << 16);
            PL[4] = V9kXPal[T[8] & 0x3F] | (V9kXPal[T[9] & 0x3F] << 16);
            PL[5] = V9kXPal[T[10] & 0x3F] | (V9kXPal[T[11] & 0x3F] << 16);
            PL[6] = V9kXPal[T[12] & 0x3F] | (V9kXPal[T[13] & 0x3F] << 16);
            PL[7] = V9kXPal[T[14] & 0x3F] | (V9kXPal[T[15] & 0x3F] << 16);
        }
        CursorDraw(Y, P);
    }
}


void RefreshLineBP6Wide(register byte Y)
{
    if (!IsWide)
    {
        register unsigned short* P;
        register u32* PL;
        register byte X, XE, * T;
        register uint fixY;

        P = RefreshBorderV9K(Y, V9kXPal[V9990VDP[15] & 0x3F]);
        if (!P) return;
        //if (!(V9990VDP[8] & 0x80)) ClearLine((pixel*)P, V9kXPal[V9990VDP[15] & 0x3F]);
        if (!(V9KVDP8Val & 0x80)) ClearLine((pixel*)P, V9kXPal[V9990VDP[15] & 0x3F]);
        else
        {
            //fixY = (uint)Y + (uint)V9990VDP[17] + ((uint)V9990VDP[18] << 8);
            //fixY = (uint)Y + (uint)V9990VDP[17] + ((uint)(V9990VDP[18] & 0x1F) << 8);
            //fixY = (uint)Y + (uint)V9990VDP[17] + V9KYScrollH;
            fixY = (uint)Y + V9KYScrollOff2;
            if (V9990VDP[18] & 0x80)fixY &= 511;
            else if (V9990VDP[18] & 0x40)fixY &= 255;
            else fixY &= V9KImgHeight - 1;
            if ((V9990VDP[7] & 0x06) == 0x06)
            {
                fixY <<= 1;
                if (V9990Port[5] & 0x02)fixY++;
            }
            T = V9KVRAM + ((fixY * V9KImgWidth) & 0x7FFFF);
            T += V9990VDP[19] | V9990VDP[20] << 3;

            XE = 32;

            PL = (u32*)P;
            for (X = 0; X < XE; X++, T += 16, PL += 4)
            {
                PL[0] = ColAve(V9kXPal[T[0] & 0x3F],V9kXPal[T[1] & 0x3F]) |
                    ColAve(V9kXPal[T[2] & 0x3F],V9kXPal[T[3] & 0x3F])<<16;
                PL[1] = ColAve(V9kXPal[T[4] & 0x3F],V9kXPal[T[5] & 0x3F]) |
                     ColAve(V9kXPal[T[6] & 0x3F],V9kXPal[T[7] & 0x3F])<<16;
                PL[2] = ColAve(V9kXPal[T[8] & 0x3F],V9kXPal[T[9] & 0x3F]) |
                     ColAve(V9kXPal[T[10] & 0x3F],V9kXPal[T[11] & 0x3F])<<16;
                PL[3] = ColAve(V9kXPal[T[12] & 0x3F],V9kXPal[T[13] & 0x3F]) |
                     ColAve(V9kXPal[T[14] & 0x3F],V9kXPal[T[15] & 0x3F])<<16;
            }
            CursorDraw(Y, P);
        }
    }
    else
    {
        register unsigned short* P;
        register u32* PL;
        register byte X, XE, * T;
        register uint fixY;

        P = RefreshBorderV9K512(Y, V9kXPal[V9990VDP[15] & 0x3F]);
        if (!P) return;
        //if (!(V9990VDP[8] & 0x80)) ClearLine((pixel*)P, V9kXPal[V9990VDP[15] & 0x3F]);
        if (!(V9KVDP8Val & 0x80)) ClearLine((pixel*)P, V9kXPal[V9990VDP[15] & 0x3F]);
        else
        {
            //fixY = (uint)Y + (uint)V9990VDP[17] + ((uint)V9990VDP[18] << 8);
            //fixY = (uint)Y + (uint)V9990VDP[17] + ((uint)(V9990VDP[18] & 0x1F) << 8);
            //fixY = (uint)Y + (uint)V9990VDP[17] + V9KYScrollH;
            fixY = (uint)Y + V9KYScrollOff2;
            if (V9990VDP[18] & 0x80)fixY &= 511;
            else if (V9990VDP[18] & 0x40)fixY &= 255;
            else fixY &= V9KImgHeight - 1;
            if ((V9990VDP[7] & 0x06) == 0x06)
            {
                fixY <<= 1;
                if (V9990Port[5] & 0x02)fixY++;
            }
            T = V9KVRAM + ((fixY * V9KImgWidth) & 0x7FFFF);
            T += V9990VDP[19] | V9990VDP[20] << 3;

            XE = 32;

            PL = (u32*)P;
            for (X = 0; X < XE; X++, T += 16, PL += 8)
            {
                PL[0] = V9kXPal[T[0] & 0x3F] | (V9kXPal[T[1] & 0x3F] << 16);
                PL[1] = V9kXPal[T[2] & 0x3F] | (V9kXPal[T[3] & 0x3F] << 16);
                PL[2] = V9kXPal[T[4] & 0x3F] | (V9kXPal[T[5] & 0x3F] << 16);
                PL[3] = V9kXPal[T[6] & 0x3F] | (V9kXPal[T[7] & 0x3F] << 16);
                PL[4] = V9kXPal[T[8] & 0x3F] | (V9kXPal[T[9] & 0x3F] << 16);
                PL[5] = V9kXPal[T[10] & 0x3F] | (V9kXPal[T[11] & 0x3F] << 16);
                PL[6] = V9kXPal[T[12] & 0x3F] | (V9kXPal[T[13] & 0x3F] << 16);
                PL[7] = V9kXPal[T[14] & 0x3F] | (V9kXPal[T[15] & 0x3F] << 16);
            }
            CursorDraw(Y, P);
        }
    }
}


void RefreshLineBYUV(register byte Y)
{
    register unsigned short* P;
    register u32* PL;
    register byte X, XE, * T;
    register int U, V, PaletteOff;
    register uint fixY;

    P = RefreshBorderV9K(Y, V9kXPal[V9990VDP[15] & 0x3F]);
    if (!P) return;

    //if (!(V9990VDP[8] & 0x80)) ClearLine((pixel*)P, V9kXPal[V9990VDP[15] & 0x3F]);
    if (!V9KVDP8Val) ClearLine((pixel*)P, V9kXPal[V9990VDP[15] & 0x3F]);
    else
    {
        fixY = (uint)Y + V9KYScrollOff2;
        if (V9990VDP[18] & 0x80)fixY &= 511;
        else if (V9990VDP[18] & 0x40)fixY &= 255;
        else fixY &= V9KImgHeight - 1;
        if ((V9990VDP[7] & 0x06) == 0x06)
        {
            fixY <<= 1;
            if (V9990Port[5] & 0x02)fixY++;
        }
        T = V9KVRAM + ((fixY * V9KImgWidth) & 0x7FFFF);
        T += V9990VDP[19] | V9990VDP[20] << 3;

        XE = 32;
        PaletteOff = (V9990VDP[13] & 0x0C) << 2;

        PL = (u32*)P;
        for (X = 0; X < XE; X++, T += 8, PL += 4)
        {
            V = (T[0] & 0x07) | ((T[1] & 0x07) << 3);
            U = (T[2] & 0x07) | ((T[3] & 0x07) << 3);
            if (V & 0x20)V -= 64;
            if (U & 0x20)U -= 64;
            PL[0] = YUVColor(T[0] >> 3, U, V) | YUVColor(T[1] >> 3, U, V) << 16;
            PL[1] = YUVColor(T[2] >> 3, U, V) | YUVColor(T[3] >> 3, U, V) << 16;
            V = (T[4] & 0x07) | ((T[5] & 0x07) << 3);
            U = (T[6] & 0x07) | ((T[7] & 0x07) << 3);
            if (V & 0x20)V -= 64;
            if (U & 0x20)U -= 64;
            PL[2] = YUVColor(T[4] >> 3, U, V) | YUVColor(T[5] >> 3, U, V) << 16;
            PL[3] = YUVColor(T[6] >> 3, U, V) | YUVColor(T[7] >> 3, U, V) << 16;
        }
        CursorDraw(Y, P);
    }
}


void RefreshLineBD16Wide(register byte Y)
{
    if (!IsWide)
    {
        register unsigned short* P;
        u32* PL;
        register byte X, XE, * T, T0, T1, T2, T3;
        uint fixY;

        P = RefreshBorderV9K(Y, V9kXPal[V9990VDP[15] & 0x3F]);
        if (!P) return;

        //if (!(V9990VDP[8] & 0x80)) ClearLine((pixel*)P, V9kXPal[V9990VDP[15] & 0x3F]);
        if (!V9KVDP8Val) ClearLine((pixel*)P, V9kXPal[V9990VDP[15] & 0x3F]);
        else
        {
            //fixY = (uint)Y + (uint)V9990VDP[17] + ((uint)(V9990VDP[18] & 0x1F) << 8);
            //fixY = (uint)Y + (uint)V9990VDP[17] + V9KYScrollH;
            fixY = (uint)Y + V9KYScrollOff2;
            if (V9990VDP[18] & 0x80)fixY &= 511;
            else if (V9990VDP[18] & 0x40)fixY &= 255;
            else fixY &= V9KImgHeight - 1;
            if ((V9990VDP[7] & 0x06) == 0x06)
            {
                fixY <<= 1;
                if (V9990Port[5] & 0x02)fixY++;
            }
            T = V9KVRAM + ((fixY * (V9KImgWidth << 1)) & 0x7FFFF);
            T += (V9990VDP[19] | V9990VDP[20] << 3)<<1;
            //T += V9990VDP[19] | V9990VDP[20] << 3;

            XE = 128;
            PL = (u32*)P;
            for (X = 0; X < XE; X++, T += 8, PL++)
            {
                PL[0] = ColAve((unsigned int)(((T[0] >> 5) | ((T[1] & 0x03) << 3)) & 0x1F) << 11 | (unsigned int)((T[1] >> 2) & 0x1F) << 6 | (unsigned int)(T[0] & 0x1F)
                    , ((unsigned int)(((T[2] >> 5) | ((T[3] & 0x03) << 3)) & 0x1F) << 11 | (unsigned int)((T[3] >> 2) & 0x1F) << 6 | (unsigned int)(T[2] & 0x1F)))
                    | (ColAve((unsigned int)(((T[4] >> 5) | ((T[5] & 0x03) << 3)) & 0x1F) << 11 | (unsigned int)((T[5] >> 2) & 0x1F) << 6 | (unsigned int)(T[4] & 0x1F)
                        , ((unsigned int)(((T[6] >> 5) | ((T[7] & 0x03) << 3)) & 0x1F) << 11 | (unsigned int)((T[7] >> 2) & 0x1F) << 6 | (unsigned int)(T[6] & 0x1F)))) << 16;
            }
            CursorDraw(Y, P);
        }
    }
    else
    {
        register unsigned short* P;
        u32* PL;
        register byte X, XE, * T;
        uint fixY;

        P = RefreshBorderV9K512(Y, V9kXPal[V9990VDP[15] & 0x3F]);
        if (!P) return;

        //if (!(V9990VDP[8] & 0x80)) ClearLine512((pixel*)P, V9kXPal[V9990VDP[15] & 0x3F]);
        if (!(V9KVDP8Val & 0x80)) ClearLine512((pixel*)P, V9kXPal[V9990VDP[15] & 0x3F]);
        else
        {
            fixY = (uint)Y + V9KYScrollOff2;
            if (V9990VDP[18] & 0x80)fixY &= 511;
            else if (V9990VDP[18] & 0x40)fixY &= 255;
            if ((V9990VDP[7] & 0x06) == 0x06)
            {
                fixY <<= 1;
                if (V9990Port[5] & 0x02)fixY++;
            }
            T = V9KVRAM + ((fixY * (V9KImgWidth << 1)) & 0x7FFFF);
            T += (V9990VDP[19] | V9990VDP[20] << 3)<<1;
            //T += V9990VDP[19] | V9990VDP[20] << 3;

            XE = 128;

            PL = (u32*)P;
            for (X = 0; X < XE; X++, T += 8 ,PL +=2)
            {
                PL[0] = (unsigned int)(((T[0] >> 5) | ((T[1] & 0x03) << 3)) & 0x1F) << 11 | (unsigned int)((T[1] >> 2) & 0x1F) << 6 | (unsigned int)(T[0] & 0x1F)
                    | ((unsigned int)(((T[2] >> 5) | ((T[3] & 0x03) << 3)) & 0x1F) << 11 | (unsigned int)((T[3] >> 2) & 0x1F) << 6 | (unsigned int)(T[2] & 0x1F)) << 16;
                PL[1] = (unsigned int)(((T[4] >> 5) | ((T[5] & 0x03) << 3)) & 0x1F) << 11 | (unsigned int)((T[5] >> 2) & 0x1F) << 6 | (unsigned int)(T[4] & 0x1F)
                    | ((unsigned int)(((T[6] >> 5) | ((T[7] & 0x03) << 3)) & 0x1F) << 11 | (unsigned int)((T[7] >> 2) & 0x1F) << 6 | (unsigned int)(T[6] & 0x1F)) << 16;
            }
            CursorDraw(Y, P);
        }
    }
}


void RefreshLineBD16(register byte Y)
{
    register unsigned short* P;
    u32* PL;
    register byte X, XE, * T, T0, T1, T2, T3;
    uint fixY;

    P = RefreshBorderV9K(Y, V9kXPal[V9990VDP[15] & 0x3F]);
    if (!P) return;

    //if (!(V9990VDP[8] & 0x80)) ClearLine((pixel*)P, V9kXPal[V9990VDP[15] & 0x3F]);
    if (!(V9KVDP8Val & 0x80)) ClearLine((pixel*)P, V9kXPal[V9990VDP[15] & 0x3F]);
    else
    {
        fixY = (uint)Y + V9KYScrollOff2;
        if (V9990VDP[18] & 0x80)fixY &= 511;
        else if (V9990VDP[18] & 0x40)fixY &= 255;
        if ((V9990VDP[7] & 0x06) == 0x06)
        {
            fixY <<= 1;
            if (V9990Port[5] & 0x02)fixY++;
        }
        T = V9KVRAM + ((fixY * (V9KImgWidth << 1)) & 0x7FFFF);
        T += (V9990VDP[19] | V9990VDP[20] << 3)<<1;

        XE = 64;

        PL = (u32*)P;
        for (X = 0; X < XE; X++, T += 8, PL += 2)
        {
            PL[0] = (unsigned int)(((T[0] >> 5) | ((T[1] & 0x03) << 3)) & 0x1F) << 11 | (unsigned int)((T[1] >> 2) & 0x1F) << 6 | (unsigned int)(T[0] & 0x1F)
                | ((unsigned int)(((T[2] >> 5) | ((T[3] & 0x03) << 3)) & 0x1F) << 11 | (unsigned int)((T[3] >> 2) & 0x1F) << 6 | (unsigned int)(T[2] & 0x1F)) << 16;
            PL[1] = (unsigned int)(((T[4] >> 5) | ((T[5] & 0x03) << 3)) & 0x1F) << 11 | (unsigned int)((T[5] >> 2) & 0x1F) << 6 | (unsigned int)(T[4] & 0x1F)
                | ((unsigned int)(((T[6] >> 5) | ((T[7] & 0x03) << 3)) & 0x1F) << 11 | (unsigned int)((T[7] >> 2) & 0x1F) << 6 | (unsigned int)(T[6] & 0x1F)) << 16;
        }
        CursorDraw(Y, P);
    }
}


/* P1 Mode With Layer A priority(>Layer B). no RefreshLineP1BA() for now. because i find no homebrew game/app that use Layer B priority. */
void RefreshLineP1AB(register byte Y)
{
	register unsigned short* P, * P2, BGColor, * Buf;
	u32* PL, * PLB;
	register byte X, XE, * T, * T2, * NameT, * NameTB, NamePosA, NamePosB, C, C1, TA0, TA1, TA2, TA3, SprCnt, * AT, * PT, I, J;
	int NameTIntA, NameTIntB, NameGen, NameGen2, cnt, SR, SRB, K, L, M, N;
	uint fixYA, fixYB;
	unsigned int* PV9kXPal;
	byte SprDraw[17] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };

	BGColor = V9kXPal[V9990VDP[15] & 0x3F];
	P = RefreshBorderV9K(Y, BGColor);
	if (!P) return;

	if (!(V9KVDP8Val & 0x80)) ClearLine((pixel*)P, BGColor);
	else
	{
        P -= 8;     /* Fixes Right hardware scroll value. */

		/* Layer B */
		NamePosB = (((V9990VDP[23] & 0x07) | ((V9990VDP[24] & 0x3F) << 3)) & 511) >> 3 << 1;
		//fixYB = (uint)Y + (uint)V9990VDP[21] + V9KYScrollHB;
        fixYB = (uint)Y + V9KYScrollOffB2;
		if (V9990VDP[18] & 0x80)fixYB &= 511;
		else if (V9990VDP[18] & 0x40)fixYB &= 255;
		else fixYB &= 511;
		//fixYB &= 511;
		NameTB = V9KVRAM + (0x7E000 + (((uint)(fixYB >> 3) << 6) << 1));

		SRB = 8 - (V9990VDP[23] & 0x07);
		P2 = P + 0;
        XE = 33;
        for (X = 0; X < SRB; X++, P2++)
		{
			*P2 = BGColor;
		}
		PV9kXPal = V9kXPal + ((V9990VDP[13] & 0x0C) << 2);
		PLB = (u32*)P2;
		for (X = 0; X < XE; X++, PLB += 4)
		{
			NameGen2 = NameTB[NamePosB] + (int)(NameTB[NamePosB + 1] << 8);
			NamePosB = (NamePosB + 2) & 0x7F;
			T2 = V9KVRAM + (((int)(fixYB & 0x07) << 7) | 0x40000 + (NameGen2 >> 5 << 10) | ((NameGen2 & 0x1F) << 2));

			TA0 = T2[0]; TA1 = T2[1]; TA2 = T2[2]; TA3 = T2[3];
			C = TA0 >> 4; C1 = TA0 & 0x0F; PLB[0] = (C ? PV9kXPal[C] : BGColor) | ((C1 ? PV9kXPal[C1] : BGColor) << 16);
			C = TA1 >> 4; C1 = TA1 & 0x0F; PLB[1] = (C ? PV9kXPal[C] : BGColor) | ((C1 ? PV9kXPal[C1] : BGColor) << 16);
			C = TA2 >> 4; C1 = TA2 & 0x0F; PLB[2] = (C ? PV9kXPal[C] : BGColor) | ((C1 ? PV9kXPal[C1] : BGColor) << 16);
			C = TA3 >> 4; C1 = TA3 & 0x0F; PLB[3] = (C ? PV9kXPal[C] : BGColor) | ((C1 ? PV9kXPal[C1] : BGColor) << 16);
		}
		P2 = (unsigned short*)PLB;
		cnt = 8 - SRB;
		if (SRB != 0)
		{
			NameGen2 = NameTB[NamePosB] + (int)(NameTB[NamePosB + 1] << 8);
			NamePosB = (NamePosB + 2) & 0x7F;
			T2 = V9KVRAM + (((int)(fixYB & 0x07) << 7) | 0x40000 + (NameGen2 >> 5 << 10) | ((NameGen2 & 0x1F) << 2));
			if (cnt) { TA0 = T2[0]; C = TA0 >> 4; *P2 = C ? PV9kXPal[C] : BGColor; cnt--; P2++; }
			if (cnt) { C = TA0 & 0x0F; *P2 = PV9kXPal[C]; cnt--; P2++; }
			if (cnt) { TA0 = T2[1]; C = TA0 >> 4; *P2 = C ? PV9kXPal[C] : BGColor; cnt--; P2++; }
			if (cnt) { C = TA0 & 0x0F; *P2 = PV9kXPal[C]; cnt--; P2++; }
			if (cnt) { TA0 = T2[2]; C = TA0 >> 4; *P2 = C ? PV9kXPal[C] : BGColor; cnt--; P2++; }
			if (cnt) { C = TA0 & 0x0F; *P2 = PV9kXPal[C]; cnt--; P2++; }
			if (cnt) { TA0 = T2[3]; C = TA0 >> 4; *P2 = C ? PV9kXPal[C] : BGColor; cnt--; P2++; }
			if (cnt) { C = TA0 & 0x0F; *P2 = PV9kXPal[C]; cnt--; P2++; }
		}
        
		/* Sprite(Lower Priority) */
		if (!(V9KVDP8Val & 0x040))
		{
			AT = V9KVRAM + 0x3FE00;
			SprCnt = 0;
			for (L = 0; L < 125; ++L, AT += 4)
			{
				K = AT[0];
				/* Mark all valid sprites with 1s, break at 16 sprites */
				if ((Y > K) && (Y <= K + 16))
				{
					SprDraw[SprCnt] = L;
					SprCnt++;
					/* If we exceed the maximum number of sprites per line... */
					if (SprCnt >= 16)
					{
						break;
						//if (!OPTION(MSX_ALLSPRITE)) break;
					}
				}
			}

			AT = V9KVRAM + 0x3FE00;
			for (M = SprCnt - 1; M >= 0; --M)
			{
				L = SprDraw[M];
				K = AT[L << 2];     /* Y position */
				I = AT[(L << 2) + 3]; /* bit4:disbale(1)  bit5:priority */
				if ((I & 0x10))continue;
                if (!(I & 0x20))continue;
				J = (Y - K - 1) & 0xFF;
				N = AT[(L << 2) + 2] | (((int)I & 0x03) << 8);
				if ((N >= 256) && (N < 1024 - 16))continue;
				PV9kXPal = V9kXPal + ((I & 0xC0) >> 2);
				PT = V9KVRAM + ((((uint)V9990VDP[25] & 0x0E) << 14) + (((uint)AT[(L << 2) + 1] >> 4) << 11)
					+ (((uint)AT[(L << 2) + 1] & 0x0F) << 3) + ((uint)J << 7));
				if (N >= 256)N -= 1024;
				//Buf = P + N;
                Buf = P + N + 8;
                for (X = 0; X < 8; X++, PT++, Buf += 2)
                {
                    C = PT[0] >> 4;
                    if (C)Buf[0] = PV9kXPal[C];
                    C = PT[0] & 0x0F;
                    if (C)Buf[1] = PV9kXPal[C];
			    }
			}
		}

		/* Layer A */
		NamePosA = (((V9990VDP[19] & 0x07) | (V9990VDP[20] << 3)) & 511) >> 3 << 1;
		//fixYA = (uint)Y + (uint)V9990VDP[17] + V9KYScrollH;
        fixYA = (uint)Y + V9KYScrollOff2;
		if (V9990VDP[18] & 0x80)fixYA &= 511;
		else if (V9990VDP[18] & 0x40)fixYA &= 255;
		else fixYA &= 511;

		NameT = V9KVRAM + (0x7C000 + (((uint)(fixYA >> 3) << 6) << 1));

		SR = 8 - (V9990VDP[19] & 0x07);
		P2 = P + 0;
        XE = 33;
        for (X = 0; X < SR; X++, P2++)
		{
			*P2 = BGColor;
		}

		PV9kXPal = V9kXPal + ((V9990VDP[13] & 0x03) << 4);
		for (X = 0; X < XE; X++, P2 += 8)
		{
			NameGen = NameT[NamePosA] + (NameT[NamePosA + 1] << 8);
			NamePosA = (NamePosA + 2) & 0x7F;
			T = V9KVRAM + (((int)(fixYA & 0x07) << 7) + (NameGen >> 5 << 10) | ((NameGen & 0x1F) << 2));

			TA0 = T[0]; TA1 = T[1]; TA2 = T[2]; TA3 = T[3];
			C = TA0 >> 4; if (C)P2[0] = PV9kXPal[C];
			C = TA0 & 0x0F; if (C)P2[1] = PV9kXPal[C];
			C = TA1 >> 4; if (C)P2[2] = PV9kXPal[C];
			C = TA1 & 0x0F; if (C)P2[3] = PV9kXPal[C];
			C = TA2 >> 4; if (C)P2[4] = PV9kXPal[C];
			C = TA2 & 0x0F; if (C)P2[5] = PV9kXPal[C];
			C = TA3 >> 4; if (C)P2[6] = PV9kXPal[C];
			C = TA3 & 0x0F; if (C)P2[7] = PV9kXPal[C];
		}
		if (SR != 0)
		{
			cnt = 8 - SR;
			NameGen = NameT[NamePosA] + (NameT[NamePosA + 1] << 8);
			NamePosA = (NamePosA + 2) & 0x7F;
			T = V9KVRAM + (((int)(fixYA & 0x07) << 7) + (NameGen >> 5 << 10) | ((NameGen & 0x1F) << 2));
			if (cnt) { TA0 = T[0]; C = TA0 >> 4; if (C)*P2 = PV9kXPal[C]; cnt--; P2++; }
			if (cnt) { C = TA0 & 0x0F; if (C)*P2 = PV9kXPal[C]; cnt--; P2++; }
			if (cnt) { TA0 = T[1]; C = TA0 >> 4; if (C)*P2 = PV9kXPal[C]; cnt--; P2++; }
			if (cnt) { C = TA0 & 0x0F; if (C)*P2 = PV9kXPal[C]; cnt--; P2++; }
			if (cnt) { TA0 = T[2]; C = TA0 >> 4; if (C)*P2 = PV9kXPal[C]; cnt--; P2++; }
			if (cnt) { C = TA0 & 0x0F; if (C)*P2 = PV9kXPal[C]; cnt--; P2++; }
			if (cnt) { TA0 = T[3]; C = TA0 >> 4; if (C)*P2 = PV9kXPal[C]; cnt--; P2++; }
			if (cnt) { C = TA0 & 0x0F; if (C)*P2 = PV9kXPal[C]; cnt--; P2++; }
		}

        /* Sprite(Higher Priority)*/
        if (!(V9KVDP8Val & 0x040))
        {
            AT = V9KVRAM + 0x3FE00;
            for (M = SprCnt - 1; M >= 0; --M)
            {
                L = SprDraw[M];
                K = AT[L << 2];     /* Y position */
                I = AT[(L << 2) + 3]; /* bit4:disbale(1)  bit5:priority */
                if ((I & 0x10))continue;
                if ((I & 0x20))continue;
                J = (Y - K - 1) & 0xFF;
                N = AT[(L << 2) + 2] | (((int)I & 0x03) << 8);
                if ((N >= 256) && (N < 1024 - 16))continue;
                PV9kXPal = V9kXPal + ((I & 0xC0) >> 2);
                PT = V9KVRAM + ((((uint)V9990VDP[25] & 0x0E) << 14) + (((uint)AT[(L << 2) + 1] >> 4) << 11)
                    + (((uint)AT[(L << 2) + 1] & 0x0F) << 3) + ((uint)J << 7));
                if (N >= 256)N -= 1024;
                //Buf = P + N;
                Buf = P + N + 8;
                for (X = 0; X < 8; X++, PT++, Buf += 2)
                {
                    C = PT[0] >> 4;
                    if (C)Buf[0] = PV9kXPal[C];
                    C = PT[0] & 0x0F;
                    if (C)Buf[1] = PV9kXPal[C];
                }
            }
        }
        
		P[-8] = P[-7] = P[-6] = P[-5] = P[-4] = P[-3] = P[-2] = P[-1] = BGColor;
		P[0] = P[1] = P[2] = P[3] = P[4] = P[5] = P[6] = P[7] = BGColor;
		P += 264;
		P[0] = P[1] = P[2] = P[3] = P[4] = P[5] = P[6] = P[7] = BGColor;
	}
}


void RefreshLineP2(register byte Y)
{
    register unsigned short* P;
    u32* PL;
    register byte X, XE, * T, *NameT, NamePos;
    int NameTInt, NameGen, PaletteOff;
    uint fixY;
    unsigned int* PV9kXPal, *PV9kXPalB;

    P = RefreshBorderV9K512(Y, V9kXPal[V9990VDP[15] & 0x3F]);
    if (!P) return;

    if (!(V9KVDP8Val & 0x80)) ClearLine512((pixel*)P, V9kXPal[V9990VDP[15] & 0x3F]);
    else
    {
        NamePos = ((V9990VDP[19]&0x07) | (V9990VDP[20] << 3)) >> 3 << 1;
        fixY = (uint)Y + V9KYScrollOff2;
        if (V9990VDP[18] & 0x80)fixY &= 511;
        else if (V9990VDP[18] & 0x40)fixY &= 255;
        else fixY &= 511;
        NameTInt = (0x7C000 + (((uint)(fixY >> 3) << 7) << 1));
        PaletteOff = (V9990VDP[13] & 0x0C) << 2;

        XE = 64;
        PV9kXPal = V9kXPal + ((V9990VDP[13] & 0x03) << 4);
        PV9kXPalB = V9kXPal + ((V9990VDP[13] & 0x0C) << 2);

		PL = (u32*)P;
		for (X = 0; X < XE; X++, PL += 4)
		{
			NameGen = V9KVRAM[NameTInt + NamePos] + ((int)(V9KVRAM[NameTInt + NamePos + 1]) << 8);
			NamePos = (NamePos + 2) & 0xFF;
			T = V9KVRAM + (((int)(fixY & 0x07) << 8) + (NameGen >> 6 << 11) | ((NameGen & 0x3F) << 2));

			PL[0] = (PV9kXPal[T[0] >> 4]) | (PV9kXPal[T[0] & 0x0F]) << 16;
			PL[1] = (PV9kXPalB[T[1] >> 4]) | (PV9kXPalB[T[1] & 0x0F]) << 16;
			PL[2] = (PV9kXPal[T[2] >> 4]) | (PV9kXPal[T[2] & 0x0F]) << 16;
			PL[3] = (PV9kXPalB[T[3] >> 4]) | (PV9kXPalB[T[3] & 0x0F]) << 16;
		}

		SpritesDrawP2(Y, P);
    }
}

/* YUV Color. Very similar to YJK. Green and Blue values are swapped.*/
unsigned short YUVColor(register int Y, register int U, register int V)
{
    register int R, G, B;

    R = Y + U;
    /* Info from mdpc. */
    /* https://mdpc.dousetsu.com/other/msx/memo.htm */
    G = (((Y << 2) + Y) - (U << 1) - V + 2) >> 2;
    B = Y + V;

    R = R < 0 ? 0 : R>31 ? 31 : R;
    G = G < 0 ? 0 : G>31 ? 31 : G;
    B = B < 0 ? 0 : B>31 ? 31 : B;

    return(R << 11 | G << 6 | B);
}

#endif // VDP_V9990


#endif /* COMMONMUX_H */