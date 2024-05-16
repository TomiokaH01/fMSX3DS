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
void RefreshLineB2(register byte Y)
{
    register pixel* P;
    u32* PL, sprDraw;
    register byte I, X, XE, * T, * R, R0, R1, R2, R3, R4, R5, R6, R7;
    int cnt, SR, SL;
    byte ZBuf[320];

    P = RefreshBorder(Y, XPal[BGColor]);
    if (!P) return;
    SR = VDP[27] & 0x07;
    SL = VDP[26];

    if (!VDP[8]&0x80) ClearLine(P, XPal[BGColor]);
    else
    {
        //sprDraw = ColorSprites(Y, ZBuf);
        //R = ZBuf + 32 + SR;
        //T = VRAM + ((((int)Y + (int)VDP[17]) <<8) & 0x7FFFF);
        T = VRAM + ((((int)Y + (int)VDP[17]) *256) & 0x7FFFF);
        //T += (int)VDP[19] | ((int)VDP[20] << 3);

        //T = VRAM + ((((int)Y)<<8) & 0x7FFFF);
        //T = VRAM + ((((int)Y) << 9) & 0x7FFFF);
        //T = VRAM + ((((int)Y + (int)VDP[17]) * 256) & 0x7FFFF);
        //T += (int)VDP[19] | ((int)VDP[20] << 3);

        //T = VRAM + ((((int)Y) * 512) & 0x7FFFF);

        //T = VRAM + ((((int)Y + (int)VDP[17]) * 512) & 0x7FFFF);
        //T = VRAM + ((((int)Y + (int)VDP[17]) * 256) & 0x7FFFF);

        //T = VRAM + ((((int)Y + (int)VDP[17]) *256) &0x7FFFF);
        //T = VRAM + (((int)Y << 7) & 0x7FFF);
        //T = VRAM + (((int)(Y + VDP[17]) << 7) & 0x7FFF);
        //T = VRAM + (((int)(Y + VDP[17]) << 9) & 0x7FFF);
        //T += VDP[22] << 8 | VDP[21];

       // T = VRAM + (((int)Y << 9) & 0x7FFF);
        //T = VRAM + (((int)(Y + VScroll) << 7) & 0x7FFF);
        //if (HScroll512)
        //{
        //    T += SL < 32 ? -0x8000 : -128;
        //    if (!(VDP[2] & 0x20))T += 0x8000;
        //    T += SL * 4;
        //}
        //else T += (SL % 32) * 4;

        //if (SR == 0)
        //{
        //    XE = 32;
        //}
        //else
        //{
        //    XE = 31;
        //    for (X = 0; X < SR; X++)
        //    {
        //        *P = XPal[BGColor]; P++;
        //    }
        //}

        XE = 32;

        PL = (u32*)P;
        if (!HScroll512)
        {
            for (X = 0; X < XE; X++, R += 8, P += 8, T += 4, PL += 4)
            //for (X = 0; X < XE; X++, R += 8, P += 8, T += 8, PL += 4)
            {
                //if (X == 32 - SL % 32)
                //{
                //    T = VRAM + (((int)Y << 7) & 0x7FFF);
                //    //T = VRAM + (((int)(Y + VScroll) << 7) & 0x7FFF);
                //    //if ((VDP[27] & 0x07) - ((VDP[26] & 0x03F) << 3) >= 0)T -= 128;
                //    //T += -128;
                //}

                //if (!(sprDraw & (1 << X)))
                //{
                    PL[0] = (XPal[T[0] >> 4]) | (XPal[T[0] & 0x0F]) << 16;
                    PL[1] = (XPal[T[1] >> 4]) | (XPal[T[1] & 0x0F]) << 16;
                    PL[2] = (XPal[T[2] >> 4]) | (XPal[T[2] & 0x0F]) << 16;
                    PL[3] = (XPal[T[3] >> 4]) | (XPal[T[3] & 0x0F]) << 16;
                //    continue;
                //}
                //R0 = R[0]; R1 = R[1]; R2 = R[2]; R3 = R[3];
                //R4 = R[4]; R5 = R[5]; R6 = R[6]; R7 = R[7];
                //PL[0] = (XPal[R[0] ? R[0] : T[0] >> 4]) | (XPal[R[1] ? R[1] : T[0] & 0x0F]) << 16;
                //PL[1] = (XPal[R[2] ? R[2] : T[1] >> 4]) | (XPal[R[3] ? R[3] : T[1] & 0x0F]) << 16;
                //PL[2] = (XPal[R[4] ? R[4] : T[2] >> 4]) | (XPal[R[5] ? R[5] : T[2] & 0x0F]) << 16;
                //PL[3] = (XPal[R[6] ? R[6] : T[3] >> 4]) | (XPal[R[7] ? R[7] : T[3] & 0x0F]) << 16;
            }
        }
        else
        {
            //for (X = 0; X < XE; X++, R += 8, P += 8, T += 8, PL += 4)
            for (X = 0; X < XE; X++, R += 8, P += 8, T += 4, PL += 4)
            {
                //if (X == 32 - SL % 32)
                //{
                //    T = VRAM + (((int)Y<< 7)& 0x7FFF);
                //    //T = VRAM + (((int)(Y + VScroll) << 7) & ChrTabM & 0x7FFF);
                //    //T += SL < 32 ? 0 : -0x8000;
                //}

                //if (!(sprDraw & (1 << X)))
                //{
                    PL[0] = (XPal[T[0] >> 4]) | (XPal[T[0] & 0x0F]) << 16;
                    PL[1] = (XPal[T[1] >> 4]) | (XPal[T[1] & 0x0F]) << 16;
                    PL[2] = (XPal[T[2] >> 4]) | (XPal[T[2] & 0x0F]) << 16;
                    PL[3] = (XPal[T[3] >> 4]) | (XPal[T[3] & 0x0F]) << 16;
                //    continue;
                //}
                //R0 = R[0]; R1 = R[1]; R2 = R[2]; R3 = R[3];
                //R4 = R[4]; R5 = R[5]; R6 = R[6]; R7 = R[7];
                //PL[0] = (XPal[R[0] ? R[0] : T[0] >> 4]) | (XPal[R[1] ? R[1] : T[0] & 0x0F]) << 16;
                //PL[1] = (XPal[R[2] ? R[2] : T[1] >> 4]) | (XPal[R[3] ? R[3] : T[1] & 0x0F]) << 16;
                //PL[2] = (XPal[R[4] ? R[4] : T[2] >> 4]) | (XPal[R[5] ? R[5] : T[2] & 0x0F]) << 16;
                //PL[3] = (XPal[R[6] ? R[6] : T[3] >> 4]) | (XPal[R[7] ? R[7] : T[3] & 0x0F]) << 16;
            }

        }
        //if (SR != 0)
        //{
        //    cnt = 8 - SR;
        //    if (cnt) { I = *R; *P = XPal[I ? I : T[0] >> 4];   cnt--; R++; P++; }
        //    if (cnt) { I = *R; *P = XPal[I ? I : T[0] & 0x0F]; cnt--; R++; P++; }
        //    if (cnt) { I = *R; *P = XPal[I ? I : T[1] >> 4];   cnt--; R++; P++; }
        //    if (cnt) { I = *R; *P = XPal[I ? I : T[1] & 0x0F]; cnt--; R++; P++; }
        //    if (cnt) { I = *R; *P = XPal[I ? I : T[2] >> 4];   cnt--; R++; P++; }
        //    if (cnt) { I = *R; *P = XPal[I ? I : T[2] & 0x0F]; cnt--; R++; P++; }
        //    if (cnt) { I = *R; *P = XPal[I ? I : T[3] >> 4];   cnt--; R++; P++; }
        //    if (cnt) { I = *R; *P = XPal[I ? I : T[3] & 0x0F]; cnt--; R++; P++; }
        //}
        //if (MaskEdges)
        //{
        //    P -= 256;
        //    P[0] = P[1] = P[2] = P[3] = P[4] = P[5] = P[6] = P[7] = P[8] = XPal[BGColor];
        //}
    }
}
#endif // VDP_V9990


#endif /* COMMONMUX_H */