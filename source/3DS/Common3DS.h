/** fMSX: portable MSX emulator ******************************/
/**                                                         **/
/**                         Common.h                        **/
/**                                                         **/
/** This file contains standard screen refresh drivers      **/
/** common for X11, VGA, and other "chunky" bitmapped video **/
/** implementations. It also includes dummy sound drivers   **/
/** for fMSX.                                               **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1994-2020                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/

/** NINTENDO 3DS Version modified by h.tomioka 2023 **/

static int FirstLine = 18;     /* First scanline in the XBuf */

//void Sprites(register byte Y, register byte* ZBuf);
u32 Sprites(register byte Y, register byte* ZBuf);
u32  ColorSprites(byte Y, byte* ZBuf);
u32 ColorSpritesScr78(register byte Y, byte* ZBuf);
static pixel *RefreshBorder(byte Y,pixel C);
static void  ClearLine(pixel *P,pixel C);
static pixel YJKColor(int Y,int J,int K);
static pixel YAEColor(int Y, int J, int K);
static void ClearLine512(register pixel* P, register pixel C);
static pixel* RefreshBorder512(register byte Y, register pixel C);

#define ColAve( p1, p2 ) \
(((p1) & 0x821) | ((((p1) & 0xF7DE) >> 1) + (((p2) & 0xF7DE) >> 1)))
//((((p1) & 0xF7DE) >> 1) + (((p2) & 0xF7DE) >> 1))
//(p1 == p2 ? p1 : ((((p1) & 0xF7DE) >> 1) + (((p2) & 0xF7DE) >> 1)))
//((0x10000) | ((((p1) & 0xF7DE) >> 1) + (((p2) & 0xF7DE) >> 1)))
//	( ( 0x8000 ) | ( (((p1)&0x7BDE) >> 1) + (((p2)&0x7BDE) >> 1) ) )

//#define C3DTexPos(x,y) ((((y >> 3) * (1024 >> 3) + (x >> 3)) << 6) + ((x & 1) | ((y & 1) << 1) | ((x & 2) << 1) | \
//((y & 2) << 2) | ((x & 4) << 2) | ((y & 4) << 3)))

/** RefreshScreen() ******************************************/
/** Refresh screen. This function is called in the end of   **/
/** refresh cycle to show the entire screen.                **/
/*************************************************************/
//void RefreshScreen(void) { PutImage(); }
void RefreshScreen(void)
{
    PutImage();
}

/** ClearLine() **********************************************/
/** Clear 256 pixels from P with color C.                   **/
/*************************************************************/
static void ClearLine(register pixel *P,register pixel C)
{
  register int J;

  for(J=0;J<256;J++) P[J]=C;
}

/** YJKColor() ***********************************************/
/** Given a color in YJK format, return the corresponding   **/
/** palette entry.                                          **/
/*************************************************************/
INLINE pixel YJKColor(register int Y,register int J,register int K)
{
  register int R,G,B;
		
  R=Y+J;
  G=Y+K;
  /* Info from mdpc. */
  /* https://mdpc.dousetsu.com/other/msx/memo.htm */
  //B = ((5 * Y - 2 * J - K + 2) / 4);
  B = (((Y << 2) + Y) - (J << 1) - K + 2) >> 2;

  //B=((5*Y-2*J-K)/4);

  R=R<0? 0:R>31? 31:R;
  G=G<0? 0:G>31? 31:G;
  B=B<0? 0:B>31? 31:B;

  //return(
  //    ModeYJK ? (R << 11 | G << 6 | B) :
  //    BPal[(R & 0x1C) | ((G & 0x1C) << 3) | (B >> 3)]
  //    );

  return(R << 11 | G << 6 | B);
}


INLINE pixel YAEColor(register int Y, register int J, register int K)
{
    register int R, G, B;

    R = Y + J;
    G = Y + K;
    /* Info from mdpc. */
    /* https://mdpc.dousetsu.com/other/msx/memo.htm */
    //B = ((5 * Y - 2 * J - K + 2) / 4);
    B = (((Y << 2) + Y) - (J << 1) - K + 2) >> 2;

    //B=((5*Y-2*J-K)/4);

    R = R < 0 ? 0 : R>31 ? 31 : R;
    G = G < 0 ? 0 : G>31 ? 31 : G;
    B = B < 0 ? 0 : B>31 ? 31 : B;

    return(BPal[(R & 0x1C) | ((G & 0x1C) << 3) | (B >> 3)]);
}


/** RefreshBorder() ******************************************/
/** This function is called from RefreshLine#() to refresh  **/
/** the screen border. It returns a pointer to the start of **/
/** scanline Y in XBuf or 0 if scanline is beyond XBuf.     **/
/*************************************************************/
pixel* RefreshBorder(register byte Y, register pixel C)
{
    register pixel* P;
    register int H;

    /* First line number in the buffer */
    if (!Y) FirstLine = (ScanLines212 ? 8 : 18) + VAdjust;

    /* Return 0 if we've run out of the screen buffer due to overscan */
    if (Y + FirstLine >= HEIGHT) return(0);

    /* Set up the transparent color */
    XPal[0] = (!BGColor || SolidColor0) ? XPal0 : XPal[BGColor];

    /* Start of the buffer */
    P = (pixel*)XBuf;

    /* Paint top of the screen */
    if (!Y) for (H = WIDTH * FirstLine - 1; H >= 0; H--) P[H] = C;

    /* Start of the line */
    P += WIDTH * (FirstLine + Y);

    /* Paint left/right borders */
    for (H = (WIDTH - 256) / 2 + HAdjust; H > 0; H--) P[H - 1] = C;
    for (H = (WIDTH - 256) / 2 - HAdjust; H > 0; H--) P[WIDTH - H] = C;

    /* Paint bottom of the screen */
    H = ScanLines212 ? 212 : 192;
    if (Y == H - 1) for (H = WIDTH * (HEIGHT - H - FirstLine + 1) - 1; H >= WIDTH; H--) P[H] = C;

    /* Return pointer to the scanline in XBuf */
    return(P + (WIDTH - 256) / 2 + HAdjust);
}


/** Sprites() ************************************************/
/** This function is called from RefreshLine#() to refresh  **/
/** sprites in SCREENs 1-3.                                 **/
/*************************************************************/
//void Sprites(register byte Y, register byte* ZBuf)
u32 Sprites(register byte Y, register byte* ZBuf)
{
    static const byte SprHeights[4] = { 8,16,16,32 };
    register byte* P, C, FlagA, * CX;
    register byte OH, IH, * PT, * AT, X;
    register unsigned int M, HitX, HitY;
    register int L, S, K;
    u32 Retval;

    byte CXBuf[320];

    // /* No extra sprites yet */
    //VDPStatus[0] &= ~0x5F;

    memset((char*)ZBuf + 32, 0, 256);
    if (SpritesOFF) return 0;

    memset(CXBuf + 32, 0, 256);

    /* Assign initial values before counting */
    OH = SprHeights[VDP[1] & 0x03];
    IH = SprHeights[VDP[1] & 0x02];
    C = 0; M = 0; L = 0;
    FlagA = 0;
    Retval = 0;
    AT = SprTab - 4;
    Y += VScroll;
    /* Count displayed sprites */
    do
    {
        M <<= 1; AT += 4; L++;	/* Iterating through SprTab      */
        K = AT[0];			/* K = sprite Y coordinate       */
        if (K == 208) break;	/* Iteration terminates if Y=208 */
        if (K > 256 - IH) K -= 256;	/* Y coordinate may be negative  */

        /* Mark all valid sprites with 1s, break at MAXSPRITE1 sprites */
        if ((Y > K) && (Y <= K + OH))
        {
            /* Mark sprite as ready to draw */
            M |= 1; C++;
            /* If we exceed the maximum number of sprites per line... */
            if (C == MAXSPRITE1)
            {
                /* Set 5thSprite flag in the VDP status register */
                if ((VDPStatus[0] & 0xC0)==0)VDPStatus[0] = (VDPStatus[0] & 0xE0) | 0x40 | L;
                /* Stop drawing sprites, unless all-sprites option enabled */
                if (!OPTION(MSX_ALLSPRITE)) break;
            }
        }
    } while (L < 32);

    /* Mark last checked sprite (5th in line, Y=208, or sprite #31) */
    //VDPStatus[0] |= L < 32 ? L : 31;
    if ((VDPStatus[0] & 0xC0)==0)VDPStatus[0] = (VDPStatus[0] & 0xE0) | L < 32 ? L : 31;

    /* Draw all marked sprites */
    for (; M; M >>= 1, AT -= 4)
    {
        if (M & 1)
        {
            C = AT[3];                  /* C = sprite attributes */
            L = C & 0x80 ? AT[1] - 32 : AT[1]; /* Sprite may be shifted left by 32 */
            C &= 0x0F;                  /* C = sprite color */

            if ((L < 256) && (L > -OH) && C)
            {
                K = AT[0];                /* K = sprite Y coordinate */
                if (K > 256 - IH) K -= 256;     /* Y coordinate may be negative */

                P = ZBuf + 32 + L;
                CX = CXBuf + 32 + L;

                if (VDP[27] & 0x07)Retval |= 1 << (((32+L) >> 3) - 5);
                Retval |= 1 << (((32+L) >> 3) - 4);
                Retval |= 1 << (((32+L) >> 3) - 3);
                Retval |= 1 << (((32+L) >> 3) - 2);
                Retval |= 1 << (((32+L) >> 3) - 1);

                PT = SprGen + ((int)(IH > 8 ? AT[2] & 0xFC : AT[2]) << 3) + (OH > IH ? ((Y - K - 1) >> 1) : (Y - K - 1));
                if (OH>IH)
                {
                    S = ((int)PT[0] << 8) | (IH > 8 ? PT[16] : 0x00);               /* Get and clip the sprite data */
                    if (S & 0xFF00)   /* Draw left 8 pixels of the sprite */
                    {
                        if (S & 0x8000) { P[0] = P[1] = C; FlagA |= CX[0]; CX[0] = 1; FlagA |= CX[1]; CX[1] = 1; }
                        if (S & 0x4000) { P[2] = P[3] = C; FlagA |= CX[2]; CX[3] = 1; FlagA |= CX[2]; CX[3] = 1; }
                        if (S & 0x2000) { P[4] = P[5] = C; FlagA |= CX[4]; CX[5] = 1; FlagA |= CX[4]; CX[5] = 1; }
                        if (S & 0x1000) { P[6] = P[7] = C; FlagA |= CX[6]; CX[7] = 1; FlagA |= CX[6]; CX[7] = 1; }
                        if (S & 0x0800) { P[8] = P[9] = C; FlagA |= CX[8]; CX[9] = 1; FlagA |= CX[8]; CX[9] = 1; }
                        if (S & 0x0400) { P[10] = P[11] = C; FlagA |= CX[10]; CX[10] = 1; FlagA |= CX[11]; CX[11] = 1; }
                        if (S & 0x0200) { P[12] = P[13] = C; FlagA |= CX[12]; CX[12] = 1; FlagA |= CX[13]; CX[13] = 1; }
                        if (S & 0x0100) { P[14] = P[15] = C; FlagA |= CX[14]; CX[14] = 1; FlagA |= CX[15]; CX[15] = 1; }
                    }
                    if (S & 0x00FF)   /* Draw right 8 pixels of the sprite */
                    {
                        if (S & 0x0080) { P[16] = P[17] = C; FlagA |= CX[16]; CX[16] = 1; FlagA |= CX[17]; CX[17] = 1; }
                        if (S & 0x0040) { P[18] = P[19] = C; FlagA |= CX[18]; CX[18] = 1; FlagA |= CX[19]; CX[19] = 1; }
                        if (S & 0x0020) { P[20] = P[21] = C; FlagA |= CX[20]; CX[20] = 1; FlagA |= CX[21]; CX[21] = 1; }
                        if (S & 0x0010) { P[22] = P[23] = C; FlagA |= CX[22]; CX[22] = 1; FlagA |= CX[23]; CX[23] = 1; }
                        if (S & 0x0008) { P[24] = P[25] = C; FlagA |= CX[24]; CX[24] = 1; FlagA |= CX[25]; CX[25] = 1; }
                        if (S & 0x0004) { P[26] = P[27] = C; FlagA |= CX[26]; CX[26] = 1; FlagA |= CX[27]; CX[27] = 1; }
                        if (S & 0x0002) { P[28] = P[29] = C; FlagA |= CX[28]; CX[28] = 1; FlagA |= CX[29]; CX[29] = 1; }
                        if (S & 0x0001) { P[30] = P[31] = C; FlagA |= CX[30]; CX[30] = 1; FlagA |= CX[31]; CX[31] = 1; }
                    }
                }
                else
                {
                    /* Mask 1: clip left sprite boundary */
                    K = L >= 0 ? 0xFFFF : (0x10000 >> (OH > IH ? (-L >> 1) : -L)) - 1;
                    
                    /* Mask 2: clip right sprite boundary */
                    L += (int)OH - 257;
                    if (L >= 0)
                    {
                        L = (IH > 8 ? 0x0002 : 0x0200) << (OH > IH ? (L >> 1) : L);
                        K &= ~(L - 1);
                    }
                    
                    /* Get and clip the sprite data */
                    K &= ((int)PT[0] << 8) | (IH > 8 ? PT[16] : 0x00);

                    if (K & 0xFF00)   /* Draw left 8 pixels of the sprite */
                    {
                        if (K & 0x8000) { P[0] = C; FlagA |= CX[0]; CX[0] = 1; }
                        if (K & 0x4000) { P[1] = C; FlagA |= CX[1]; CX[1] = 1; }
                        if (K & 0x2000) { P[2] = C; FlagA |= CX[2]; CX[2] = 1; }
                        if (K & 0x1000) { P[3] = C; FlagA |= CX[3]; CX[3] = 1; }
                        if (K & 0x0800) { P[4] = C; FlagA |= CX[4]; CX[4] = 1; }
                        if (K & 0x0400) { P[5] = C; FlagA |= CX[5]; CX[5] = 1; }
                        if (K & 0x0200) { P[6] = C; FlagA |= CX[6]; CX[6] = 1; }
                        if (K & 0x0100) { P[7] = C; FlagA |= CX[7]; CX[7] = 1; }
                    }
                    if (K & 0x00FF)   /* Draw right 8 pixels of the sprite */
                    {
                        if (K & 0x0080) { P[8] = C; FlagA |= CX[8]; CX[8] = 1; }
                        if (K & 0x0040) { P[9] = C; FlagA |= CX[9]; CX[9] = 1; }
                        if (K & 0x0020) { P[10] = C; FlagA |= CX[10]; CX[10] = 1; }
                        if (K & 0x0010) { P[11] = C; FlagA |= CX[11]; CX[11] = 1; }
                        if (K & 0x0008) { P[12] = C; FlagA |= CX[12]; CX[12] = 1; }
                        if (K & 0x0004) { P[13] = C; FlagA |= CX[13]; CX[13] = 1; }
                        if (K & 0x0002) { P[14] = C; FlagA |= CX[14]; CX[14] = 1; }
                        if (K & 0x0001) { P[15] = C; FlagA |= CX[15]; CX[15] = 1; }
                    }
                }
            }
            if (FlagA && !IsSpriteColi)
            {
                IsSpriteColi = 1;
                for (X = 0; X < 32; X++)
                {
                    if (CX[X + 32] != 0)break;
                }
                HitX = L + X + 12;
                HitY = AT[0] + 8;
                VDPStatus[0] |= 0x20;
                VDPStatus[3] = HitX & 0xFF;
                VDPStatus[4] = HitX >> 8;
                VDPStatus[5] = HitY & 0xFF;
                VDPStatus[6] = HitY >> 8;
            }
        }
    }
    return Retval;
}


/** ColorSprites() *******************************************/
/** This function is called from RefreshLine#() to refresh  **/
/** color sprites in SCREENs 4-8. The result is returned in **/
/** ZBuf, whose size must be 320 bytes (32+256+32).         **/
/*************************************************************/
//void ColorSprites(register byte Y, byte* ZBuf)
u32 ColorSprites(register byte Y, byte* ZBuf)
{
    static const byte SprHeights[4] = { 8,16,16,32 };
    register byte C, IH, OH, J, OrThem;
    register byte* P, * PT, * AT, * CX, FlagA, X, Color;
    register int L, K, N, H, HitX, HitY, FirstM;
    register unsigned int M;
    u32 Retval;

    byte CXBuf[320];

    // /* No extra sprites yet */
    //VDPStatus[0] &= ~0x5F;

    /* Clear ZBuffer and exit if sprites are off */
    memset(ZBuf + 32, 0, 256);
    if (SpritesOFF) return 0;
    //if (SpritesOFF) return;
    //if (VDPStatus[2] & 0x40)return;

    /* Assign initial values before counting */
    OrThem = 0x00;
    OH = SprHeights[VDP[1] & 0x03];
    IH = SprHeights[VDP[1] & 0x02];
    AT = SprTab - 4;
    C = MAXSPRITE2 + 1;
    M = 0;
    FlagA = 0;
    memset(CXBuf + 0, 0, 320);
    Retval = 0;
    FirstM = -1;

    byte DrawSprite = 0;

    /* Count displayed sprites */
    for (L = 0; L < 32; ++L)
    {
        M <<= 1; AT += 4;              /* Iterating through SprTab      */
        K = AT[0];                  /* Read Y from SprTab            */
        if (K == 216) break;         /* Iteration terminates if Y=216 */
        K = (byte)(K - VScroll);      /* Sprite's actual Y coordinate  */
        //if(K>256-IH) K-=256;       /* Y coordinate may be negative  */
        if (K >= 256 - IH) K -= 256;       /* Y coordinate may be negative  */

        /* Mark all valid sprites with 1s, break at MAXSPRITE2 sprites */
        if ((Y > K) && (Y <= K + OH))
        //if ((Y > K) && (Y <= K + OH + 1))
        {

            /* If we exceed the maximum number of sprites per line... */
            if (!--C)
            {
                /* Set 9thSprite flag in the VDP status register */
                //VDPStatus[0] |= 0x40;
                if ((VDPStatus[0] & 0xC0)==0)VDPStatus[0] = (VDPStatus[0] & 0xE0) | 0x40 | L;
                /* Stop drawing sprites, unless all-sprites option enabled */
                if (!OPTION(MSX_ALLSPRITE)) break;
            }

            J = Y - K - 1;
            //J = Y > K + OH ? OH - 1 : Y - K - 1;
            J = OH > IH ? (J >> 1) : J;
            Color = SprTab[-0x0200 + ((AT - SprTab) << 2) + J];
            if (!(Color & 0x40)) DrawSprite = 1;

            /* Mark sprite as ready to draw */
            M |= 1;
            if (FirstM < 0)FirstM = M;
        }
    }
    if (FirstM < 0)FirstM = M;

    L = L < 32 ? L : 31;
    /* Mark last checked sprite (9th in line, Y=216, or sprite #31) */
    //VDPStatus[0] |= L;
    if ((VDPStatus[0] & 0xC0)==0)VDPStatus[0] = (VDPStatus[0] & 0xE0) | L;

    /* Disappear the sprites if all sprites fas CC bit. */
    if (!DrawSprite)return Retval;

    //if (VDPStatus[0] & 0xc0 == 0){VDPStatus[0] = (VDPStatus[0] & 0xe0) | L < 32 ? L : 31;}

    /* Draw all marked sprites */
    for (; M; M >>= 1, AT -= 4, --L)
        if (M & 1)
        {
            K = (byte)(AT[0] - VScroll);  /* K = sprite Y coordinate */
            //if (K > 256 - IH) K -= 256;        /* Y coordinate may be negative */
            if (K >= 256 - IH) K -= 256;        /* Y coordinate may be negative */
            J = Y - K - 1;
            //J = Y > K + OH ? OH - 1 : Y - K - 1;

            J = OH > IH ? (J >> 1) : J;
            C = SprTab[-0x0200 + ((AT - SprTab) << 2) + J];
            H = C;
            OrThem |= C & 0x40;

            if ((C & 0x40) && M == FirstM)continue;     /* Disappear the sprite if that is first sprite and has CC bit.*/
                                                        /* Some game use this trick to hide sprite. (Shin Maou Golvellius(Golvellius 2) etc.)  */

            if ((C & 0x0F) || SolidColor0)
            {
                PT = SprGen + ((int)(IH > 8 ? AT[2] & 0xFC : AT[2]) << 3) + J;
                N = AT[1] + (C & 0x80 ? 0 : 32);
                if (N < 0)continue;
                //if (N + OH<=32)continue;
                CX = CXBuf + N;
                P = ZBuf + N;
                if (VDP[27] & 0x07)Retval |= 1 << ((N >> 3) - 5);
                Retval |= 1 << ((N >> 3) - 4);
                Retval |= 1 << ((N >> 3) - 3);
                Retval |= 1 << ((N >> 3) - 2);
                Retval |= 1 << ((N >> 3) - 1);
                if (ScrMode == 6)C = (C & 0x0F) << 1 | SolidColor0;
                //if (ScrMode == 6)C = (C & 0x0F) << 1;
                else C &= 0x0F;
                if (!C) C = 0x10; // "fake" color 0. Taken from Dingux-MSX PixaMod.

                J = PT[0];
                if ((H & 0x60) || (C == 0x10))
                {
                    //if (!(H & 0x40))DrawSprite = 1;
                    if (OH > IH)
                    {
                        if (J & 0x80) { P[0] |= C; P[1] |= C; }
                        if (J & 0x40) { P[2] |= C; P[3] |= C; }
                        if (J & 0x20) { P[4] |= C; P[5] |= C; }
                        if (J & 0x10) { P[6] |= C; P[7] |= C; }
                        if (J & 0x08) { P[8] |= C; P[9] |= C; }
                        if (J & 0x04) { P[10] |= C; P[11] |= C; }
                        if (J & 0x02) { P[12] |= C; P[13] |= C; }
                        if (J & 0x01) { P[14] |= C; P[15] |= C; }
                        if (IH > 8)
                        {
                            J = PT[16];
                            if (J & 0x80) { P[16] |= C; P[17] |= C; }
                            if (J & 0x40) { P[18] |= C; P[19] |= C; }
                            if (J & 0x20) { P[20] |= C; P[21] |= C; }
                            if (J & 0x10) { P[22] |= C; P[23] |= C; }
                            if (J & 0x08) { P[24] |= C; P[25] |= C; }
                            if (J & 0x04) { P[26] |= C; P[27] |= C; }
                            if (J & 0x02) { P[28] |= C; P[29] |= C; }
                            if (J & 0x01) { P[30] |= C; P[31] |= C; }
                        }
                    }
                    else
                    {
                        if (J & 0x80) P[0] |= C;
                        if (J & 0x40) P[1] |= C;
                        if (J & 0x20) P[2] |= C;
                        if (J & 0x10) P[3] |= C;
                        if (J & 0x08) P[4] |= C;
                        if (J & 0x04) P[5] |= C;
                        if (J & 0x02) P[6] |= C;
                        if (J & 0x01) P[7] |= C;
                        if (IH > 8)
                        {
                            J = PT[16];
                            if (J & 0x80) P[8] |= C;
                            if (J & 0x40) P[9] |= C;
                            if (J & 0x20) P[10] |= C;
                            if (J & 0x10) P[11] |= C;
                            if (J & 0x08) P[12] |= C;
                            if (J & 0x04) P[13] |= C;
                            if (J & 0x02) P[14] |= C;
                            if (J & 0x01) P[15] |= C;
                        }
                    }
                }
                else if (OrThem & 0x20)
                {
                    if (OH > IH)
                    {
                        if (J & 0x80) { P[0] |= C; FlagA |= CX[0]; CX[0] |= 1; P[1] |= C; FlagA |= CX[1]; CX[1] |= 1; }
                        if (J & 0x40) { P[2] |= C; FlagA |= CX[2]; CX[2] |= 1; P[3] |= C; FlagA |= CX[3]; CX[3] |= 1; }
                        if (J & 0x20) { P[4] |= C; FlagA |= CX[4]; CX[4] |= 1; P[5] |= C; FlagA |= CX[5]; CX[5] |= 1; }
                        if (J & 0x10) { P[6] |= C; FlagA |= CX[6]; CX[6] |= 1; P[7] |= C; FlagA |= CX[7]; CX[7] |= 1; }
                        if (J & 0x08) { P[8] |= C; FlagA |= CX[8]; CX[8] |= 1; P[9] |= C; FlagA |= CX[9]; CX[9] |= 1; }
                        if (J & 0x04) { P[10] |= C; FlagA |= CX[10]; CX[10] |= 1; P[11] |= C; FlagA |= CX[11]; CX[11] |= 1; }
                        if (J & 0x02) { P[12] |= C; FlagA |= CX[12]; CX[12] |= 1; P[13] |= C; FlagA |= CX[13]; CX[13] |= 1; }
                        if (J & 0x01) { P[14] |= C; FlagA |= CX[14]; CX[14] |= 1; P[15] |= C; FlagA |= CX[15]; CX[15] |= 1; }
                        if (IH > 8)
                        {
                            J = PT[16];
                            if (J & 0x80) { P[16] |= C; FlagA |= CX[16]; CX[16] |= 1; P[17] |= C; FlagA |= CX[17]; CX[17] |= 1; }
                            if (J & 0x40) { P[18] |= C; FlagA |= CX[18]; CX[18] |= 1; P[19] |= C; FlagA |= CX[19]; CX[19] |= 1; }
                            if (J & 0x20) { P[20] |= C; FlagA |= CX[20]; CX[20] |= 1; P[21] |= C; FlagA |= CX[21]; CX[21] |= 1; }
                            if (J & 0x10) { P[22] |= C; FlagA |= CX[22]; CX[22] |= 1; P[23] |= C; FlagA |= CX[23]; CX[23] |= 1; }
                            if (J & 0x08) { P[24] |= C; FlagA |= CX[24]; CX[24] |= 1; P[25] |= C; FlagA |= CX[25]; CX[25] |= 1; }
                            if (J & 0x04) { P[26] |= C; FlagA |= CX[26]; CX[26] |= 1; P[27] |= C; FlagA |= CX[27]; CX[27] |= 1; }
                            if (J & 0x02) { P[28] |= C; FlagA |= CX[28]; CX[28] |= 1; P[29] |= C; FlagA |= CX[29]; CX[29] |= 1; }
                            if (J & 0x01) { P[30] |= C; FlagA |= CX[30]; CX[30] |= 1; P[31] |= C; FlagA |= CX[31]; CX[31] |= 1; }
                        }
                    }
                    else
                    {
                        if (J & 0x80) { P[0] |= C; FlagA |= CX[0]; CX[0] |= 1; }
                        if (J & 0x40) { P[1] |= C; FlagA |= CX[1]; CX[1] |= 1; }
                        if (J & 0x20) { P[2] |= C; FlagA |= CX[2]; CX[2] |= 1; }
                        if (J & 0x10) { P[3] |= C; FlagA |= CX[3]; CX[3] |= 1; }
                        if (J & 0x08) { P[4] |= C; FlagA |= CX[4]; CX[4] |= 1; }
                        if (J & 0x04) { P[5] |= C; FlagA |= CX[5]; CX[5] |= 1; }
                        if (J & 0x02) { P[6] |= C; FlagA |= CX[6]; CX[6] |= 1; }
                        if (J & 0x01) { P[7] |= C; FlagA |= CX[7]; CX[7] |= 1; }
                        if (IH > 8)
                        {
                            J = PT[16];
                            if (J & 0x80) { P[8] |= C; FlagA |= CX[8]; CX[8] |= 1; }
                            if (J & 0x40) { P[9] |= C; FlagA |= CX[9]; CX[9] |= 1; }
                            if (J & 0x20) { P[10] |= C; FlagA |= CX[10]; CX[10] |= 1; }
                            if (J & 0x10) { P[11] |= C; FlagA |= CX[11]; CX[11] |= 1; }
                            if (J & 0x08) { P[12] |= C; FlagA |= CX[12]; CX[12] |= 1; }
                            if (J & 0x04) { P[13] |= C; FlagA |= CX[13]; CX[13] |= 1; }
                            if (J & 0x02) { P[14] |= C; FlagA |= CX[14]; CX[14] |= 1; }
                            if (J & 0x01) { P[15] |= C; FlagA |= CX[15]; CX[15] |= 1; }
                        }
                    }
                }
                else
                {
                    if (OH > IH)
                    {
                        if (J & 0x80) { P[0] = C; FlagA |= CX[0]; CX[0] = (N > 32 && N <= 288) ? 1 : 0;  P[1] = C; FlagA |= CX[1]; CX[1] = (N > 33 && N <= 289) ? 1 : 0; }
                        if (J & 0x40) { P[2] = C; FlagA |= CX[2]; CX[2] = (N > 34 && N <= 290) ? 1 : 0;  P[3] = C; FlagA |= CX[3]; CX[3] = (N > 35 && N <= 291) ? 1 : 0; }
                        if (J & 0x20) { P[4] = C; FlagA |= CX[4]; CX[4] = (N > 36 && N <= 292) ? 1 : 0;  P[5] = C; FlagA |= CX[5]; CX[5] = (N > 37 && N <= 293) ? 1 : 0; }
                        if (J & 0x10) { P[6] = C; FlagA |= CX[6]; CX[6] = (N > 38 && N <= 294) ? 1 : 0;  P[7] = C; FlagA |= CX[7]; CX[7] = (N > 39 && N <= 295) ? 1 : 0; }
                        if (J & 0x08) { P[8] = C; FlagA |= CX[8]; CX[8] = (N > 40 && N <= 296) ? 1 : 0;  P[9] = C; FlagA |= CX[9]; CX[9] = (N > 41 && N <= 297) ? 1 : 0; }
                        if (J & 0x04) { P[10] = C; FlagA |= CX[10]; CX[10] = (N > 42 && N <= 298) ? 1 : 0;  P[11] = C; FlagA |= CX[11]; CX[11] = (N > 43 && N <= 299) ? 1 : 0; }
                        if (J & 0x02) { P[12] = C; FlagA |= CX[12]; CX[12] = (N > 44 && N <= 300) ? 1 : 0;  P[13] = C; FlagA |= CX[13]; CX[13] = (N > 45 && N <= 301) ? 1 : 0; }
                        if (J & 0x01) { P[14] = C; FlagA |= CX[14]; CX[14] = (N > 46 && N <= 302) ? 1 : 0;  P[15] = C; FlagA |= CX[15]; CX[15] = (N > 47 && N <= 303) ? 1 : 0; }
                        if (IH > 8)
                        {
                            J = PT[16];
                            if (J & 0x80) { P[16] = C; FlagA |= CX[16]; CX[16] = (N > 48 && N <= 304) ? 1 : 0;  P[17] = C; FlagA |= CX[17]; CX[17] = (N > 49 && N <= 305) ? 1 : 0; }
                            if (J & 0x40) { P[18] = C; FlagA |= CX[18]; CX[18] = (N > 50 && N <= 306) ? 1 : 0;  P[19] = C; FlagA |= CX[19]; CX[19] = (N > 51 && N <= 307) ? 1 : 0; }
                            if (J & 0x20) { P[20] = C; FlagA |= CX[20]; CX[20] = (N > 52 && N <= 308) ? 1 : 0;  P[21] = C; FlagA |= CX[21]; CX[21] = (N > 53 && N <= 309) ? 1 : 0; }
                            if (J & 0x10) { P[22] = C; FlagA |= CX[22]; CX[22] = (N > 54 && N <= 310) ? 1 : 0;  P[23] = C; FlagA |= CX[23]; CX[23] = (N > 55 && N <= 311) ? 1 : 0; }
                            if (J & 0x08) { P[24] = C; FlagA |= CX[24]; CX[24] = (N > 56 && N <= 312) ? 1 : 0;  P[25] = C; FlagA |= CX[25]; CX[25] = (N > 57 && N <= 313) ? 1 : 0; }
                            if (J & 0x04) { P[26] = C; FlagA |= CX[26]; CX[26] = (N > 58 && N <= 314) ? 1 : 0;  P[27] = C; FlagA |= CX[27]; CX[27] = (N > 59 && N <= 315) ? 1 : 0; }
                            if (J & 0x02) { P[28] = C; FlagA |= CX[28]; CX[28] = (N > 60 && N <= 316) ? 1 : 0;  P[29] = C; FlagA |= CX[29]; CX[29] = (N > 61 && N <= 317) ? 1 : 0; }
                            if (J & 0x01) { P[30] = C; FlagA |= CX[30]; CX[30] = (N > 62 && N <= 318) ? 1 : 0;  P[31] = C; FlagA |= CX[31]; CX[31] = (N > 63 && N <= 319) ? 1 : 0; }
                        }
                    }
                    else
                    {
                        if (J & 0x80) { P[0] = C; FlagA |= CX[0]; CX[0] = (N > 32 && N <= 288) ? 1 : 0; }
                        if (J & 0x40) { P[1] = C; FlagA |= CX[1]; CX[1] = (N > 33 && N <= 289) ? 1 : 0; }
                        if (J & 0x20) { P[2] = C; FlagA |= CX[2]; CX[2] = (N > 34 && N <= 290) ? 1 : 0; }
                        if (J & 0x10) { P[3] = C; FlagA |= CX[3]; CX[3] = (N > 35 && N <= 291) ? 1 : 0; }
                        if (J & 0x08) { P[4] = C; FlagA |= CX[4]; CX[4] = (N > 36 && N <= 292) ? 1 : 0; }
                        if (J & 0x04) { P[5] = C; FlagA |= CX[5]; CX[5] = (N > 37 && N <= 293) ? 1 : 0; }
                        if (J & 0x02) { P[6] = C; FlagA |= CX[6]; CX[6] = (N > 38 && N <= 294) ? 1 : 0; }
                        if (J & 0x01) { P[7] = C; FlagA |= CX[7]; CX[7] = (N > 39 && N <= 295) ? 1 : 0; }
                        if (IH > 8)
                        {
                            J = PT[16];
                            if (J & 0x80) { P[8] = C; FlagA |= CX[8]; CX[8] = (N > 40 && N <= 296) ? 1 : 0; }
                            if (J & 0x40) { P[9] = C; FlagA |= CX[9]; CX[9] = (N > 41 && N <= 297) ? 1 : 0; }
                            if (J & 0x20) { P[10] = C; FlagA |= CX[10]; CX[10] = (N > 42 && N <= 298) ? 1 : 0; }
                            if (J & 0x10) { P[11] = C; FlagA |= CX[11]; CX[11] = (N > 43 && N <= 299) ? 1 : 0; }
                            if (J & 0x08) { P[12] = C; FlagA |= CX[12]; CX[12] = (N > 44 && N <= 300) ? 1 : 0; }
                            if (J & 0x04) { P[13] = C; FlagA |= CX[13]; CX[13] = (N > 45 && N <= 301) ? 1 : 0; }
                            if (J & 0x02) { P[14] = C; FlagA |= CX[14]; CX[14] = (N > 46 && N <= 302) ? 1 : 0; }
                            if (J & 0x01) { P[15] = C; FlagA |= CX[15]; CX[15] = (N > 47 && N <= 303) ? 1 : 0; }
                        }
                    }
                }
                if (FlagA && !IsSpriteColi)
                {
                    IsSpriteColi = 1;
                    for (X = 0; X < 32; X++)
                    {
                        if (CX[X + 32] != 0)break;
                    }
                    HitX = N + X + 12;
                    HitY = AT[0] + 8;
                    VDPStatus[0] |= 0x20;
                    VDPStatus[3] = HitX & 0xFF;
                    VDPStatus[4] = HitX >> 8;
                    VDPStatus[5] = HitY & 0xFF;
                    VDPStatus[6] = HitY >> 8;
                }
            }

            /* Update overlapping flag */
            OrThem >>= 1;
        }
        return Retval;
}


/** ColorSprites() for Screen 7, 8, 10, 12 **/
//void ColorSpritesScr78(register byte Y, byte* ZBuf)
u32 ColorSpritesScr78(register byte Y, byte* ZBuf)
{
    static const byte SprHeights[4] = { 8,16,16,32 };
    register byte C, IH, OH, J, OrThem;
    register byte* P, * AT, * CX, FlagA, X, Color, TransP;
    register int L, K, N, H, HitX, HitY;
    register unsigned int M;
    int SprGenInt, SprTabInt, ATInt, PageInt, PageInt2, SprCnt;
    u32 Retval;

    byte CXBuf[320];
    byte CBuf[32];

    // /* No extra sprites yet */
    //VDPStatus[0] &= ~0x5F;

    /* Clear ZBuffer and exit if sprites are off */
    memset(ZBuf + 32, 0, 256);
    if (SpritesOFF) return 0;
    //if (SpritesOFF) return;
    //if (VDPStatus[2] & 0x40)return;

    /* Assign initial values before counting */
    OrThem = 0x00;
    OH = SprHeights[VDP[1] & 0x03];
    IH = SprHeights[VDP[1] & 0x02];
    AT = SprTab - 4;
    C = MAXSPRITE2 + 1;
    M = 0;
    FlagA = 0;
    Retval = 0;

    memset(CXBuf + 0, 0, 320);
    memset(CBuf + 0, 0, 32);
    TransP = 0xF0;

    SprGenInt = (int)(SprGen - VRAM);
    SprTabInt = (int)(SprTab - VRAM);
    //SprTabInt = ((int)(VDP[5] & 0xFC) << 7) + ((int)VDP[11] << 15);
    ATInt = SprTabInt - 4;

    /* Count displayed sprites */
    for (L = 0; L < 32; ++L)
    {
        M <<= 1; AT += 4; ATInt += 4;              /* Iterating through SprTab      */
        K = VRAM[((ATInt & 0x01) << 16) | ((ATInt) >> 1)];              /* Read Y from SprTab            */
        if (K == 216) break;         /* Iteration terminates if Y=216 */
        K = (byte)(K - VScroll);      /* Sprite's actual Y coordinate  */
        //if (K > 256 - IH) K -= 256;       /* Y coordinate may be negative  */
        if (K >= 256 - IH) K -= 256;       /* Y coordinate may be negative  */

        /* Mark all valid sprites with 1s, break at MAXSPRITE2 sprites */
        if ((Y > K) && (Y <= K + OH))
        {
            /* If we exceed the maximum number of sprites per line... */
            if (!--C)
            {
                /* Set 9thSprite flag in the VDP status register */
                //VDPStatus[0] |= 0x40;
                if ((VDPStatus[0] & 0xC0)==0)VDPStatus[0] = (VDPStatus[0] & 0xE0) | 0x40 | L;
                /* Stop drawing sprites, unless all-sprites option enabled */
                if (!OPTION(MSX_ALLSPRITE)) break;
            }

            J = Y - K - 1;
            J = OH > IH ? (J >> 1) : J;
            PageInt = SprTabInt - 0x200 + ((int)(AT - SprTab) << 2) + J;
            Color = VRAM[((PageInt & 0x01) << 16) | (PageInt >> 1)];
            if (!(Color & 0x40))
            {
                TransP = ((Color & 0x0F) || SolidColor0 || ScrMode == 6) ? 0xFF : 0xF0;
            }
            CBuf[L] = Color & TransP;

            /* Mark sprite as ready to draw */
            M |= 1;
        }
    }

    L = L < 32 ? L : 31;
    /* Mark last checked sprite (9th in line, Y=216, or sprite #31) */
    //VDPStatus[0] |= L;
    if ((VDPStatus[0] & 0xC0)==0)VDPStatus[0] = (VDPStatus[0] & 0xE0) | L;

    /* Draw all marked sprites */
    for (; M; M >>= 1, AT -= 4, ATInt -= 4,--L)
        if (M & 1)
        {
            //K = (byte)(AT[0] - VScroll);  /* K = sprite Y coordinate */
            K = (byte)(VRAM[((ATInt & 0x01) << 16) | (ATInt >> 1)] - VScroll);  /* K = sprite Y coordinate */
            //if (K > 256 - IH) K -= 256;        /* Y coordinate may be negative */
            if (K >= 256 - IH) K -= 256;        /* Y coordinate may be negative */

            J = Y - K - 1;
            J = OH > IH ? (J >> 1) : J;
            //PageInt = SprTabInt - 0x200 + ((int)(AT - SprTab) << 2) + J;
            //C = VRAM[((PageInt & 0x01) << 16) | (PageInt >> 1)];
            C = CBuf[L];
            H = C;
            OrThem |= C & 0x40;

            if (C & 0x0F || SolidColor0)
            {
                PageInt2 = SprGenInt + ((int)(IH > 8 ? VRAM[(((ATInt + 2) & 0x01) << 16) | ((ATInt + 2) >> 1)] & 0xFC
                    : VRAM[(((ATInt + 2) & 0x01) << 16) | ((ATInt + 2) >> 1)]) << 3) + J;
                N = VRAM[(((ATInt + 1) & 0x01) << 16) | ((ATInt + 1) >> 1)] + (C & 0x80 ? 0 : 32);
                //if(OH == 8)N -= 8;
                if (N < 0)continue;
                P = ZBuf + N;
                CX = CXBuf + N;
                if (VDP[27] & 0x07)Retval |= 1 << ((N >> 3) - 5);
                Retval |= 1 << ((N >> 3) - 4);
                Retval |= 1 << ((N >> 3) - 3);
                Retval |= 1 << ((N >> 3) - 2);
                Retval |= 1 << ((N >> 3) - 1);
                //Retval |= 1 << (((N-32) >> 3));
                //Retval |= 1 << (((N-32) >> 3) +1);
                //if (N + OH <=32)continue;
                H = C;
                C = ((C & 0x0F) << 1) | (SolidColor0 ? 1 : 0);
                if (!C) C = 0x10; // "fake" color 0. Taken from Dingux-MSX PixaMod.

                J = VRAM[(((PageInt2) & 0x01) << 16) | (PageInt2 >> 1)];

                if ((H & 0x60) || C==0x10)
                {
                    if (OH > IH)
                    {
                        if (J & 0x80) { P[0] |= C; P[1] |= C; }
                        if (J & 0x40) { P[2] |= C; P[3] |= C; }
                        if (J & 0x20) { P[4] |= C; P[5] |= C; }
                        if (J & 0x10) { P[6] |= C; P[7] |= C; }
                        if (J & 0x08) { P[8] |= C; P[9] |= C; }
                        if (J & 0x04) { P[10] |= C; P[11] |= C; }
                        if (J & 0x02) { P[12] |= C; P[13] |= C; }
                        if (J & 0x01) { P[14] |= C; P[15] |= C; }
                        if (IH > 8)
                        {
                            //J = PT[16];
                            J = VRAM[(((PageInt2 + 16) & 0x01) << 16) | ((PageInt2 + 16) >> 1)];
                            if (J & 0x80) { P[16] |= C; P[17] |= C; }
                            if (J & 0x40) { P[18] |= C; P[19] |= C; }
                            if (J & 0x20) { P[20] |= C; P[21] |= C; }
                            if (J & 0x10) { P[22] |= C; P[23] |= C; }
                            if (J & 0x08) { P[24] |= C; P[25] |= C; }
                            if (J & 0x04) { P[26] |= C; P[27] |= C; }
                            if (J & 0x02) { P[28] |= C; P[29] |= C; }
                            if (J & 0x01) { P[30] |= C; P[31] |= C; }
                        }
                    }
                    else
                    {
                        if (J & 0x80) P[0] |= C;
                        if (J & 0x40) P[1] |= C;
                        if (J & 0x20) P[2] |= C;
                        if (J & 0x10) P[3] |= C;
                        if (J & 0x08) P[4] |= C;
                        if (J & 0x04) P[5] |= C;
                        if (J & 0x02) P[6] |= C;
                        if (J & 0x01) P[7] |= C;
                        if (IH > 8)
                        {
                            //J = PT[16];
                            J = VRAM[(((PageInt2 + 16) & 0x01) << 16) | ((PageInt2 + 16) >> 1)];
                            if (J & 0x80) P[8] |= C;
                            if (J & 0x40) P[9] |= C;
                            if (J & 0x20) P[10] |= C;
                            if (J & 0x10) P[11] |= C;
                            if (J & 0x08) P[12] |= C;
                            if (J & 0x04) P[13] |= C;
                            if (J & 0x02) P[14] |= C;
                            if (J & 0x01) P[15] |= C;
                        }
                    }
                }
                else if (OrThem & 0x20)
                {
                    if (OH > IH)
                    {
                        if (J & 0x80) { P[0] |= C; FlagA |= CX[0]; CX[0] |= 1; P[1] |= C; FlagA |= CX[1]; CX[1] |= 1; }
                        if (J & 0x40) { P[2] |= C; FlagA |= CX[2]; CX[2] |= 1; P[3] |= C; FlagA |= CX[3]; CX[3] |= 1; }
                        if (J & 0x20) { P[4] |= C; FlagA |= CX[4]; CX[4] |= 1; P[5] |= C; FlagA |= CX[5]; CX[5] |= 1; }
                        if (J & 0x10) { P[6] |= C; FlagA |= CX[6]; CX[6] |= 1; P[7] |= C; FlagA |= CX[7]; CX[7] |= 1; }
                        if (J & 0x08) { P[8] |= C; FlagA |= CX[8]; CX[8] |= 1; P[9] |= C; FlagA |= CX[9]; CX[9] |= 1; }
                        if (J & 0x04) { P[10] |= C; FlagA |= CX[10]; CX[10] |= 1; P[11] |= C; FlagA |= CX[11]; CX[11] |= 1; }
                        if (J & 0x02) { P[12] |= C; FlagA |= CX[12]; CX[12] |= 1; P[13] |= C; FlagA |= CX[13]; CX[13] |= 1; }
                        if (J & 0x01) { P[14] |= C; FlagA |= CX[14]; CX[14] |= 1; P[15] |= C; FlagA |= CX[15]; CX[15] |= 1; }
                        if (IH > 8)
                        {
                            //J = PT[16];
                            J = VRAM[(((PageInt2 + 16) & 0x01) << 16) | ((PageInt2 + 16) >> 1)];
                            if (J & 0x80) { P[16] |= C; FlagA |= CX[16]; CX[16] |= 1; P[17] |= C; FlagA |= CX[17]; CX[17] |= 1; }
                            if (J & 0x40) { P[18] |= C; FlagA |= CX[18]; CX[18] |= 1; P[19] |= C; FlagA |= CX[19]; CX[19] |= 1; }
                            if (J & 0x20) { P[20] |= C; FlagA |= CX[20]; CX[20] |= 1; P[21] |= C; FlagA |= CX[21]; CX[21] |= 1; }
                            if (J & 0x10) { P[22] |= C; FlagA |= CX[22]; CX[22] |= 1; P[23] |= C; FlagA |= CX[23]; CX[23] |= 1; }
                            if (J & 0x08) { P[24] |= C; FlagA |= CX[24]; CX[24] |= 1; P[25] |= C; FlagA |= CX[25]; CX[25] |= 1; }
                            if (J & 0x04) { P[26] |= C; FlagA |= CX[26]; CX[26] |= 1; P[27] |= C; FlagA |= CX[27]; CX[27] |= 1; }
                            if (J & 0x02) { P[28] |= C; FlagA |= CX[28]; CX[28] |= 1; P[29] |= C; FlagA |= CX[29]; CX[29] |= 1; }
                            if (J & 0x01) { P[30] |= C; FlagA |= CX[30]; CX[30] |= 1; P[31] |= C; FlagA |= CX[31]; CX[31] |= 1; }
                        }
                    }
                    else
                    {
                        if (J & 0x80) { P[0] |= C; FlagA |= CX[0]; CX[0] |= 1; }
                        if (J & 0x40) { P[1] |= C; FlagA |= CX[1]; CX[1] |= 1; }
                        if (J & 0x20) { P[2] |= C; FlagA |= CX[2]; CX[2] |= 1; }
                        if (J & 0x10) { P[3] |= C; FlagA |= CX[3]; CX[3] |= 1; }
                        if (J & 0x08) { P[4] |= C; FlagA |= CX[4]; CX[4] |= 1; }
                        if (J & 0x04) { P[5] |= C; FlagA |= CX[5]; CX[5] |= 1; }
                        if (J & 0x02) { P[6] |= C; FlagA |= CX[6]; CX[6] |= 1; }
                        if (J & 0x01) { P[7] |= C; FlagA |= CX[7]; CX[7] |= 1; }
                        if (IH > 8)
                        {
                            //J = PT[16];
                            J = VRAM[(((PageInt2 + 16) & 0x01) << 16) | ((PageInt2 + 16) >> 1)];
                            if (J & 0x80) { P[8] |= C; FlagA |= CX[8]; CX[8] |= 1; }
                            if (J & 0x40) { P[9] |= C; FlagA |= CX[9]; CX[9] |= 1; }
                            if (J & 0x20) { P[10] |= C; FlagA |= CX[10]; CX[10] |= 1; }
                            if (J & 0x10) { P[11] |= C; FlagA |= CX[11]; CX[11] |= 1; }
                            if (J & 0x08) { P[12] |= C; FlagA |= CX[12]; CX[12] |= 1; }
                            if (J & 0x04) { P[13] |= C; FlagA |= CX[13]; CX[13] |= 1; }
                            if (J & 0x02) { P[14] |= C; FlagA |= CX[14]; CX[14] |= 1; }
                            if (J & 0x01) { P[15] |= C; FlagA |= CX[15]; CX[15] |= 1; }
                        }
                    }
                }
                else
                {
                    if (OH > IH)
                    {
                        if (J & 0x80) { P[0] = C; FlagA |= CX[0]; CX[0] = (N > 32 && N <= 288) ? 1 : 0;  P[1] = C; FlagA |= CX[1]; CX[1] = (N > 33 && N <= 289) ? 1 : 0; }
                        if (J & 0x40) { P[2] = C; FlagA |= CX[2]; CX[2] = (N > 34 && N <= 290) ? 1 : 0;  P[3] = C; FlagA |= CX[3]; CX[3] = (N > 35 && N <= 291) ? 1 : 0; }
                        if (J & 0x20) { P[4] = C; FlagA |= CX[4]; CX[4] = (N > 36 && N <= 292) ? 1 : 0;  P[5] = C; FlagA |= CX[5]; CX[5] = (N > 37 && N <= 293) ? 1 : 0; }
                        if (J & 0x10) { P[6] = C; FlagA |= CX[6]; CX[6] = (N > 38 && N <= 294) ? 1 : 0;  P[7] = C; FlagA |= CX[7]; CX[7] = (N > 39 && N <= 295) ? 1 : 0; }
                        if (J & 0x08) { P[8] = C; FlagA |= CX[8]; CX[8] = (N > 40 && N <= 296) ? 1 : 0;  P[9] = C; FlagA |= CX[9]; CX[9] = (N > 41 && N <= 297) ? 1 : 0; }
                        if (J & 0x04) { P[10] = C; FlagA |= CX[10]; CX[10] = (N > 42 && N <= 298) ? 1 : 0;  P[11] = C; FlagA |= CX[11]; CX[11] = (N > 43 && N <= 299) ? 1 : 0; }
                        if (J & 0x02) { P[12] = C; FlagA |= CX[12]; CX[12] = (N > 44 && N <= 300) ? 1 : 0;  P[13] = C; FlagA |= CX[13]; CX[13] = (N > 45 && N <= 301) ? 1 : 0; }
                        if (J & 0x01) { P[14] = C; FlagA |= CX[14]; CX[14] = (N > 46 && N <= 302) ? 1 : 0;  P[15] = C; FlagA |= CX[15]; CX[15] = (N > 47 && N <= 303) ? 1 : 0; }
                        if (IH > 8)
                        {
                            //J = PT[16];
                            J = VRAM[(((PageInt2 + 16) & 0x01) << 16) | ((PageInt2 + 16) >> 1)];
                            if (J & 0x80) { P[16] = C; FlagA |= CX[16]; CX[16] = (N > 48 && N <= 304) ? 1 : 0;  P[17] = C; FlagA |= CX[17]; CX[17] = (N > 49 && N <= 305) ? 1 : 0; }
                            if (J & 0x40) { P[18] = C; FlagA |= CX[18]; CX[18] = (N > 50 && N <= 306) ? 1 : 0;  P[19] = C; FlagA |= CX[19]; CX[19] = (N > 51 && N <= 307) ? 1 : 0; }
                            if (J & 0x20) { P[20] = C; FlagA |= CX[20]; CX[20] = (N > 52 && N <= 308) ? 1 : 0;  P[21] = C; FlagA |= CX[21]; CX[21] = (N > 53 && N <= 309) ? 1 : 0; }
                            if (J & 0x10) { P[22] = C; FlagA |= CX[22]; CX[22] = (N > 54 && N <= 310) ? 1 : 0;  P[23] = C; FlagA |= CX[23]; CX[23] = (N > 55 && N <= 311) ? 1 : 0; }
                            if (J & 0x08) { P[24] = C; FlagA |= CX[24]; CX[24] = (N > 56 && N <= 312) ? 1 : 0;  P[25] = C; FlagA |= CX[25]; CX[25] = (N > 57 && N <= 313) ? 1 : 0; }
                            if (J & 0x04) { P[26] = C; FlagA |= CX[26]; CX[26] = (N > 58 && N <= 314) ? 1 : 0;  P[27] = C; FlagA |= CX[27]; CX[27] = (N > 59 && N <= 315) ? 1 : 0; }
                            if (J & 0x02) { P[28] = C; FlagA |= CX[28]; CX[28] = (N > 60 && N <= 316) ? 1 : 0;  P[29] = C; FlagA |= CX[29]; CX[29] = (N > 61 && N <= 317) ? 1 : 0; }
                            if (J & 0x01) { P[30] = C; FlagA |= CX[30]; CX[30] = (N > 62 && N <= 318) ? 1 : 0;  P[31] = C; FlagA |= CX[31]; CX[31] = (N > 63 && N <= 319) ? 1 : 0; }
                        }
                    }
                    else
                    {
                        if (J & 0x80) { P[0] = C; FlagA |= CX[0]; CX[0] = (N > 32 && N <= 288) ? 1 : 0; }
                        if (J & 0x40) { P[1] = C; FlagA |= CX[1]; CX[1] = (N > 33 && N <= 289) ? 1 : 0; }
                        if (J & 0x20) { P[2] = C; FlagA |= CX[2]; CX[2] = (N > 34 && N <= 290) ? 1 : 0; }
                        if (J & 0x10) { P[3] = C; FlagA |= CX[3]; CX[3] = (N > 35 && N <= 291) ? 1 : 0; }
                        if (J & 0x08) { P[4] = C; FlagA |= CX[4]; CX[4] = (N > 36 && N <= 292) ? 1 : 0; }
                        if (J & 0x04) { P[5] = C; FlagA |= CX[5]; CX[5] = (N > 37 && N <= 293) ? 1 : 0; }
                        if (J & 0x02) { P[6] = C; FlagA |= CX[6]; CX[6] = (N > 38 && N <= 294) ? 1 : 0; }
                        if (J & 0x01) { P[7] = C; FlagA |= CX[7]; CX[7] = (N > 39 && N <= 295) ? 1 : 0; }
                        if (IH > 8)
                        {
                            //J = PT[16];
                            J = VRAM[(((PageInt2 + 16) & 0x01) << 16) | ((PageInt2 + 16) >> 1)];
                            if (J & 0x80) { P[8] = C; FlagA |= CX[8]; CX[8] = (N > 40 && N <= 296) ? 1 : 0; }
                            if (J & 0x40) { P[9] = C; FlagA |= CX[9]; CX[9] = (N > 41 && N <= 297) ? 1 : 0; }
                            if (J & 0x20) { P[10] = C; FlagA |= CX[10]; CX[10] = (N > 42 && N <= 298) ? 1 : 0; }
                            if (J & 0x10) { P[11] = C; FlagA |= CX[11]; CX[11] = (N > 43 && N <= 299) ? 1 : 0; }
                            if (J & 0x08) { P[12] = C; FlagA |= CX[12]; CX[12] = (N > 44 && N <= 300) ? 1 : 0; }
                            if (J & 0x04) { P[13] = C; FlagA |= CX[13]; CX[13] = (N > 45 && N <= 301) ? 1 : 0; }
                            if (J & 0x02) { P[14] = C; FlagA |= CX[14]; CX[14] = (N > 46 && N <= 302) ? 1 : 0; }
                            if (J & 0x01) { P[15] = C; FlagA |= CX[15]; CX[15] = (N > 47 && N <= 303) ? 1 : 0; }
                        }
                    }
                }
                if (FlagA && !IsSpriteColi)
                {
                    IsSpriteColi = 1;
                    for (X = 0; X < 32; X++)
                    {
                        if (CX[X + 32] != 0)break;
                    }
                    HitX = N + X + 12;
                    HitY = AT[0] + 8;
                    VDPStatus[0] |= 0x20;
                    VDPStatus[3] = HitX & 0xFF;
                    VDPStatus[4] = HitX >> 8;
                    VDPStatus[5] = HitY & 0xFF;
                    VDPStatus[6] = HitY >> 8;
                }
            }

            /* Update overlapping flag */
            OrThem >>= 1;
        }
        return Retval;
}


/** RefreshLineF() *******************************************/
/** Dummy refresh function called for non-existing screens. **/
/*************************************************************/
void RefreshLineF(register byte Y)
{
  register pixel *P;

  if(Verbose>1)
    printf
    (
      "ScrMODE %d: ChrTab=%X ChrGen=%X ColTab=%X SprTab=%X SprGen=%X\n",
      ScrMode,
      (int)(ChrTab-VRAM),
      (int)(ChrGen-VRAM),
      (int)(ColTab-VRAM),
      (int)(SprTab-VRAM),
      (int)(SprGen-VRAM)
    );

  P=RefreshBorder(Y,XPal[BGColor]);
  if(P) ClearLine(P,XPal[BGColor]);
}

/** RefreshLine0() *******************************************/
/** Refresh line Y (0..191/211) of SCREEN0.                 **/
/*************************************************************/
void RefreshLine0(register byte Y)
{
    register pixel* P, FC, BC;
    register byte* T, * G, X;
    int i,Scr;

    BC = XPal[BGColor];
    P = RefreshBorder(Y, BC);
    if (!P) return;
    Scr = HScroll;

    if (!ScreenON) ClearLine(P, BC);
    else
    {
        for (i = 0; i < 8 + Scr; i++) {
            *P++ = BC;
        }

        G = ChrGen + ((Y + VScroll) & 0x07);
        T = ChrTab + 40 * (Y >> 3);
        FC = XPal[FGColor];

        for (X = 0; X < 40; X++, T++, P += 6)
        {
            Y = G[(int)*T << 3];
            P[0] = Y & 0x80 ? FC : BC;
            P[1] = Y & 0x40 ? FC : BC;
            P[2] = Y & 0x20 ? FC : BC;
            P[3] = Y & 0x10 ? FC : BC;
            P[4] = Y & 0x08 ? FC : BC;
            P[5] = Y & 0x04 ? FC : BC;
        }

        for (i = 0; i < 8 - Scr; i++) {
            *P++ = BC;
        }
    }
}

/** RefreshLine1() *******************************************/
/** Refresh line Y (0..191/211) of SCREEN1, including       **/
/** sprites in this line.                                   **/
/*************************************************************/
void RefreshLine1(register byte Y)
{
    register pixel* P, FC, BC;
    register byte K, X, XE, C, * T, * G, * R;
    int cnt, SR, SL;
    byte ZBuf[320];

    P = RefreshBorder(Y, XPal[BGColor]);
    if (!P) return;
    SR = VDP[27] & 0x07;
    SL = VDP[26];

    if (!ScreenON) ClearLine(P, XPal[BGColor]);
    else
    {
        Sprites(Y, ZBuf);
        R = ZBuf + 32 + SR;
        Y += VScroll;
        G = ChrGen + (Y & 0x07);
        T = ChrTab + ((int)(Y & 0xF8) << 2);
        T += (SL % 32);

        if (SR == 0)
        {
            XE = 32;
        }
        else {
			XE = 31;
			K = ColTab[*T >> 3];
			FC = XPal[K >> 4];
			BC = XPal[K & 0x0F];
			K = G[(int)*T << 3];

			for (X = 0; X < SR; X++)
			{
				*P = XPal[BGColor]; P++;
			}
		}
        for (X = 0; X < XE; X++, T++)
        {
            if (X == 32 - SL % 32)
            {
                T = ChrTab + ((int)(Y & 0xF8) << 2);
                if ((VDP[27] & 0x07) - ((VDP[26] & 0x03F) << 3) >= 0)T -= 128;
            }

            K = ColTab[*T >> 3];
            FC = XPal[K >> 4];
            BC = XPal[K & 0x0F];
            K = G[(int)*T << 3];

            C = *R++; *P++ = (C ? XPal[C] : (K & 0x80) ? FC : BC);
            C = *R++; *P++ = (C ? XPal[C] : (K & 0x40) ? FC : BC);
            C = *R++; *P++ = (C ? XPal[C] : (K & 0x20) ? FC : BC);
            C = *R++; *P++ = (C ? XPal[C] : (K & 0x10) ? FC : BC);
            C = *R++; *P++ = (C ? XPal[C] : (K & 0x08) ? FC : BC);
            C = *R++; *P++ = (C ? XPal[C] : (K & 0x04) ? FC : BC);
            C = *R++; *P++ = (C ? XPal[C] : (K & 0x02) ? FC : BC);
            C = *R++; *P++ = (C ? XPal[C] : (K & 0x01) ? FC : BC);
        }
		if (SR != 0)
		{
			K = ColTab[*T >> 3];
			FC = XPal[K >> 4];
			BC = XPal[K & 0x0F];
			K = G[(int)*T << 3];

			cnt = 8 - SR;
			if (cnt) { C = *R; *P = C ? XPal[C] : (K & 0x80) ? FC : BC; cnt--; R++; P++; }
			if (cnt) { C = *R; *P = C ? XPal[C] : (K & 0x40) ? FC : BC; cnt--; R++; P++; }
			if (cnt) { C = *R; *P = C ? XPal[C] : (K & 0x20) ? FC : BC; cnt--; R++; P++; }
			if (cnt) { C = *R; *P = C ? XPal[C] : (K & 0x10) ? FC : BC; cnt--; R++; P++; }
			if (cnt) { C = *R; *P = C ? XPal[C] : (K & 0x08) ? FC : BC; cnt--; R++; P++; }
			if (cnt) { C = *R; *P = C ? XPal[C] : (K & 0x04) ? FC : BC; cnt--; R++; P++; }
			if (cnt) { C = *R; *P = C ? XPal[C] : (K & 0x02) ? FC : BC; cnt--; R++; P++; }
			if (cnt) { C = *R; *P = C ? XPal[C] : (K & 0x01) ? FC : BC; cnt--; R++; P++; }
		}
        if (MaskEdges)
        {
            P -= 256;
            P[0] = P[1] = P[2] = P[3] = P[4] = P[5] = P[6] = P[7] = P[8] = XPal[BGColor];
        }
    }
}

/** RefreshLine2() *******************************************/
/** Refresh line Y (0..191/211) of SCREEN2, including       **/
/** sprites in this line.                                   **/
/*************************************************************/
void RefreshLine2(register byte Y)
{
    register pixel* P, FC, BC;
    u32* PL, sprDraw;
    register byte K, X, XE, C, * T, * R, R0, R1, R2, R3, R4, R5, R6, R7;
    register int I, J, cnt;
    int SR, SL;
    byte ZBuf[320];

    P = RefreshBorder(Y, XPal[BGColor]);
    if (!P) return;
    SR = VDP[27] & 0x07;
    SL = VDP[26];

    if (!ScreenON) ClearLine(P, XPal[BGColor]);
    else
    {
        sprDraw = Sprites(Y, ZBuf);
        R = ZBuf + 32 + SR;
        Y += VScroll;
        T = ChrTab + ((int)(Y & 0xF8) << 2);
        T += (SL % 32);
        I = ((int)(Y & 0xC0) << 5) + (Y & 0x07);

        if (SR == 0)
        {
            XE = 32;
        }
        else {
            XE = 31;

            J = (int)*T << 3;
            K = ColTab[(I + J) & ColTabM];
            FC = XPal[K >> 4];
            BC = XPal[K & 0x0F];
            K = ChrGen[(I + J) & ChrGenM];
            for (X = 0; X < SR; X++)
            {
                *P = XPal[BGColor]; P++;
            }
        }
        PL = (u32*)P;
        for (X = 0; X < XE; X++, R += 8, P += 8, PL += 4, T++)
        {
            if (X == 32 - SL % 32)
            {
                T = ChrTab + ((int)(Y & 0xF8) << 2);
                if ((VDP[27] & 0x07) - ((VDP[26] & 0x03F) << 3) >= 0)T -= 128;
            }
            J = (int)*T << 3;
            K = ColTab[(I + J) & ColTabM];
            FC = XPal[K >> 4];
            BC = XPal[K & 0x0F];
            K = ChrGen[(I + J) & ChrGenM];

            if (!(sprDraw & (1 << X)))
            {
                PL[0] = ((K & 0x80) ? FC : BC) | (((K & 0x40) ? FC : BC) << 16);
                PL[1] = ((K & 0x20) ? FC : BC) | (((K & 0x10) ? FC : BC) << 16);
                PL[2] = ((K & 0x08) ? FC : BC) | (((K & 0x04) ? FC : BC) << 16);
                PL[3] = ((K & 0x02) ? FC : BC) | (((K & 0x01) ? FC : BC) << 16);
                continue;
            }
            R0 = R[0]; R1 = R[1]; R2 = R[2]; R3 = R[3];
            R4 = R[4]; R5 = R[5]; R6 = R[6]; R7 = R[7];

            PL[0] = (R0 ? XPal[R0] : (K & 0x80) ? FC : BC) | ((R1 ? XPal[R1] : (K & 0x40) ? FC : BC) << 16);
            PL[1] = (R2 ? XPal[R2] : (K & 0x20) ? FC : BC) | ((R3 ? XPal[R3] : (K & 0x10) ? FC : BC) << 16);
            PL[2] = (R4 ? XPal[R4] : (K & 0x08) ? FC : BC) | ((R5 ? XPal[R5] : (K & 0x04) ? FC : BC) << 16);
            PL[3] = (R6 ? XPal[R6] : (K & 0x02) ? FC : BC) | ((R7 ? XPal[R7] : (K & 0x01) ? FC : BC) << 16);
        }
        if (SR != 0)
        {
            if (X == 32 - SL % 32)
            {
                T = ChrTab + ((int)(Y & 0xF8) << 2);
                if ((VDP[27] & 0x07) - ((VDP[26] & 0x03F) << 3) >= 0)T -= 128;
            }
            J = (int)*T << 3;
            K = ColTab[(I + J) & ColTabM];
            FC = XPal[K >> 4];
            BC = XPal[K & 0x0F];
            K = ChrGen[(I + J) & ChrGenM];

            cnt = 8 - SR;
            if (cnt) { C = *R; *P = C ? XPal[C] : (K & 0x80) ? FC : BC; cnt--; R++; P++; }
            if (cnt) { C = *R; *P = C ? XPal[C] : (K & 0x40) ? FC : BC; cnt--; R++; P++; }
            if (cnt) { C = *R; *P = C ? XPal[C] : (K & 0x20) ? FC : BC; cnt--; R++; P++; }
            if (cnt) { C = *R; *P = C ? XPal[C] : (K & 0x10) ? FC : BC; cnt--; R++; P++; }
            if (cnt) { C = *R; *P = C ? XPal[C] : (K & 0x08) ? FC : BC; cnt--; R++; P++; }
            if (cnt) { C = *R; *P = C ? XPal[C] : (K & 0x04) ? FC : BC; cnt--; R++; P++; }
            if (cnt) { C = *R; *P = C ? XPal[C] : (K & 0x02) ? FC : BC; cnt--; R++; P++; }
            if (cnt) { C = *R; *P = C ? XPal[C] : (K & 0x01) ? FC : BC; cnt--; R++; P++; }

        }
        if (MaskEdges)
        {
            P -= 256;
            P[0] = P[1] = P[2] = P[3] = P[4] = P[5] = P[6] = P[7] = P[8] = XPal[BGColor];
        }
    }
}

/** RefreshLine3() *******************************************/
/** Refresh line Y (0..191/211) of SCREEN3, including       **/
/** sprites in this line.                                   **/
/*************************************************************/
void RefreshLine3(register byte Y)
{
    register pixel * P;
    register byte X, XE, C, K, * T, * G, * R;
    int cnt, HSC, Scr;
    byte ZBuf[320];

    P = RefreshBorder(Y, XPal[BGColor]);
    if (!P) return;
    HSC = HScroll;
    Scr = HSC;

    if (!ScreenON) ClearLine(P, XPal[BGColor]);
    else
    {
        Sprites(Y, ZBuf);
        R = ZBuf + 32 + Scr;
        Y += VScroll;
        T = ChrTab + ((int)(Y & 0xF8) << 2);
        G = ChrGen + ((Y & 0x1C) >> 2);

        if (Scr == 0)
        {
            XE = 32;
        }
        else {
            XE = 31;
            K = G[(int)*T << 3];
            if (Scr < 0)
            {
                cnt = -Scr;
                if (cnt) { cnt--; }
                else { C = *R; *P = XPal[C ? C : (K >> 4)]; P++; } R++;
                if (cnt) { cnt--; }
                else { C = *R; *P = XPal[C ? C : (K >> 4)]; P++; } R++;
                if (cnt) { cnt--; }
                else { C = *R; *P = XPal[C ? C : (K >> 4)]; P++; } R++;
                if (cnt) { cnt--; }
                else { C = *R; *P = XPal[C ? C : (K >> 4)]; P++; } R++;
                if (cnt) { cnt--; }
                else { C = *R; *P = XPal[C ? C : (K & 0x0F)]; P++; } R++;
                if (cnt) { cnt--; }
                else { C = *R; *P = XPal[C ? C : (K & 0x0F)]; P++; } R++;
                if (cnt) { cnt--; }
                else { C = *R; *P = XPal[C ? C : (K & 0x0F)]; P++; } R++;
                if (cnt) { cnt--; }
                else { C = *R; *P = XPal[C ? C : (K & 0x0F)]; P++; } R++;
                T++;
            }
            else
            {
                for (X = 0; X < Scr; X++)
                {
                    *P = XPal[BGColor]; P++;
                }
            }
        }
        for (X = 0; X < XE; X++, T++, P += 8, R += 8)
        {
            K = G[(int)*T << 3];
            C = R[0]; P[0] = XPal[C ? C : (K >> 4)];
            C = R[1]; P[1] = XPal[C ? C : (K >> 4)];
            C = R[2]; P[2] = XPal[C ? C : (K >> 4)];
            C = R[3]; P[3] = XPal[C ? C : (K >> 4)];
            C = R[4]; P[4] = XPal[C ? C : (K & 0x0F)];
            C = R[5]; P[5] = XPal[C ? C : (K & 0x0F)];
            C = R[6]; P[6] = XPal[C ? C : (K & 0x0F)];
            C = R[7]; P[7] = XPal[C ? C : (K & 0x0F)];
        }
        if (Scr != 0)
        {
            if (Scr < 0)
            {
                for (X = 0; X < -Scr; X++)
                {
                    *P = XPal[BGColor]; P++;
                }
            }
            else
            {
                cnt = 8 - Scr;
                K = G[(int)*T << 3];
                if (cnt) { C = *R; *P = XPal[C ? C : (K >> 4)]; cnt--; P++; R++; }
                if (cnt) { C = *R; *P = XPal[C ? C : (K >> 4)]; cnt--; P++; R++; }
                if (cnt) { C = *R; *P = XPal[C ? C : (K >> 4)]; cnt--; P++; R++; }
                if (cnt) { C = *R; *P = XPal[C ? C : (K >> 4)]; cnt--; P++; R++; }
                if (cnt) { C = *R; *P = XPal[C ? C : (K & 0x0F)]; cnt--; P++; R++; }
                if (cnt) { C = *R; *P = XPal[C ? C : (K & 0x0F)]; cnt--; P++; R++; }
                if (cnt) { C = *R; *P = XPal[C ? C : (K & 0x0F)]; cnt--; P++; R++; }
                if (cnt) { C = *R; *P = XPal[C ? C : (K & 0x0F)]; cnt--; P++; R++; }
            }
        }
    }
}

/** RefreshLine4() *******************************************/
/** Refresh line Y (0..191/211) of SCREEN4, including       **/
/** sprites in this line.                                   **/
/*************************************************************/
void RefreshLine4(register byte Y)
{
    register pixel* P, FC, BC;
    u32* PL, sprDraw;
    register byte K, X, XE, C, * T, * R, R0, R1, R2, R3, R4, R5, R6, R7;
    register int I, J, cnt;
    int SR, SL;
    byte ZBuf[320];

    P = RefreshBorder(Y, XPal[BGColor]);
    if (!P) return;
    SR = VDP[27] & 0x07;
    SL = VDP[26];

    if (!ScreenON) ClearLine(P, XPal[BGColor]);
    else
    {
        sprDraw = ColorSprites(Y, ZBuf);
        R = ZBuf + 32 + SR;
        Y += VScroll;
        T = ChrTab + ((int)(Y & 0xF8) << 2);
        T += (SL % 32);
        I = ((int)(Y & 0xC0) << 5) + (Y & 0x07);

        if (SR == 0)
        {
            XE = 32;
        }
        else {
            XE = 31;

            J = (int)*T << 3;
            K = ColTab[(I + J) & ColTabM];
            FC = XPal[K >> 4];
            BC = XPal[K & 0x0F];
            K = ChrGen[(I + J) & ChrGenM];
            for (X = 0; X < SR; X++)
            {
                *P = XPal[BGColor]; P++;
            }
        }
        PL = (u32*)P;
        for (X = 0; X < XE; X++, R += 8, P += 8, PL += 4, T++)
        {
            if (X == 32 - SL % 32)
            {
                T = ChrTab + ((int)(Y & 0xF8) << 2);
                if ((VDP[27] & 0x07) - ((VDP[26] & 0x03F) << 3) >= 0)T -= 128;
            }
            J = (int)*T << 3;
            K = ColTab[(I + J) & ColTabM];
            FC = XPal[K >> 4];
            BC = XPal[K & 0x0F];
            K = ChrGen[(I + J) & ChrGenM];

            if (!(sprDraw & (1 << X)))
            {
                PL[0] = ((K & 0x80) ? FC : BC) | (((K & 0x40) ? FC : BC)<<16);
                PL[1] = ((K & 0x20) ? FC : BC) | (((K & 0x10) ? FC : BC)<<16);
                PL[2] = ((K & 0x08) ? FC : BC) | (((K & 0x04) ? FC : BC)<<16);
                PL[3] = ((K & 0x02) ? FC : BC) | (((K & 0x01) ? FC : BC)<<16);
                continue;
            }
            R0 = R[0]; R1 = R[1]; R2 = R[2]; R3 = R[3];
            R4 = R[4]; R5 = R[5]; R6 = R[6]; R7 = R[7];

            PL[0] = (R0 ? XPal[R0] : (K & 0x80) ? FC : BC) | ((R1 ? XPal[R1] : (K & 0x40) ? FC : BC) << 16);
            PL[1] = (R2 ? XPal[R2] : (K & 0x20) ? FC : BC) | ((R3 ? XPal[R3] : (K & 0x10) ? FC : BC) << 16);
            PL[2] = (R4 ? XPal[R4] : (K & 0x08) ? FC : BC) | ((R5 ? XPal[R5] : (K & 0x04) ? FC : BC) << 16);
            PL[3] = (R6 ? XPal[R6] : (K & 0x02) ? FC : BC) | ((R7 ? XPal[R7] : (K & 0x01) ? FC : BC) << 16);
        }
		if (SR != 0)
		{
            if (X == 32 - SL % 32)
            {
                T = ChrTab + ((int)(Y & 0xF8) << 2);
                if ((VDP[27] & 0x07) - ((VDP[26] & 0x03F) << 3) >= 0)T -= 128;
            }
			J = (int)*T << 3;
			K = ColTab[(I + J) & ColTabM];
			FC = XPal[K >> 4];
			BC = XPal[K & 0x0F];
			K = ChrGen[(I + J) & ChrGenM];

			cnt = 8 - SR;
			if (cnt) { C = *R; *P = C ? XPal[C] : (K & 0x80) ? FC : BC; cnt--; R++; P++; }
			if (cnt) { C = *R; *P = C ? XPal[C] : (K & 0x40) ? FC : BC; cnt--; R++; P++; }
			if (cnt) { C = *R; *P = C ? XPal[C] : (K & 0x20) ? FC : BC; cnt--; R++; P++; }
			if (cnt) { C = *R; *P = C ? XPal[C] : (K & 0x10) ? FC : BC; cnt--; R++; P++; }
			if (cnt) { C = *R; *P = C ? XPal[C] : (K & 0x08) ? FC : BC; cnt--; R++; P++; }
			if (cnt) { C = *R; *P = C ? XPal[C] : (K & 0x04) ? FC : BC; cnt--; R++; P++; }
			if (cnt) { C = *R; *P = C ? XPal[C] : (K & 0x02) ? FC : BC; cnt--; R++; P++; }
			if (cnt) { C = *R; *P = C ? XPal[C] : (K & 0x01) ? FC : BC; cnt--; R++; P++; }

		}
        if (MaskEdges)
        {
            P -= 256;
            P[0] = P[1] = P[2] = P[3] = P[4] = P[5] = P[6] = P[7] = P[8] = XPal[BGColor];
        }
    }
}


/** RefreshLine5() *******************************************/
/** Refresh line Y (0..191/211) of SCREEN5, including       **/
/** sprites in this line.                                   **/
/*************************************************************/
void RefreshLine5(register byte Y)
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

    if (!ScreenON) ClearLine(P, XPal[BGColor]);
    else
    {
        sprDraw = ColorSprites(Y, ZBuf);
        R = ZBuf + 32 + SR;
        T = ChrTab + (((int)(Y + VScroll) << 7) & ChrTabM & 0x7FFF);
        if (HScroll512)
        {
            T += SL < 32 ? -0x8000 : -128;
            if (!(VDP[2] & 0x20))T += 0x8000;
            T += SL * 4;
        }
        else T += (SL%32) * 4;

		if (SR == 0)
		{
			XE = 32;
		}
		else
		{
			XE = 31;
			for (X = 0; X < SR; X++)
			{
				*P = XPal[BGColor]; P++;
			}
		}

        PL = (u32*)P;
        if (!HScroll512)
        {
            for (X = 0; X < XE; X++, R += 8, P += 8, T += 4, PL += 4)
            {
                if (X == 32 - SL % 32)
                {
                    T = ChrTab + (((int)(Y + VScroll) << 7) & ChrTabM & 0x7FFF);
                    if ((VDP[27] & 0x07) - ((VDP[26] & 0x03F) << 3) >= 0)T -= 128;
                    //T += -128;
                }

                if (!(sprDraw & (1 << X)))
                {
                    PL[0] = (XPal[T[0] >> 4]) | (XPal[T[0] & 0x0F])<<16;
                    PL[1] = (XPal[T[1] >> 4]) | (XPal[T[1] & 0x0F])<<16;
                    PL[2] = (XPal[T[2] >> 4]) | (XPal[T[2] & 0x0F])<<16;
                    PL[3] = (XPal[T[3] >> 4]) | (XPal[T[3] & 0x0F])<<16;
                    continue;
                }
                R0 = R[0]; R1 = R[1]; R2 = R[2]; R3 = R[3];
                R4 = R[4]; R5 = R[5]; R6 = R[6]; R7 = R[7];
                PL[0] = (XPal[R[0] ? R[0] : T[0] >> 4]) | (XPal[R[1] ? R[1] : T[0] & 0x0F]) << 16;
                PL[1] = (XPal[R[2] ? R[2] : T[1] >> 4]) | (XPal[R[3] ? R[3] : T[1] & 0x0F]) << 16;
                PL[2] = (XPal[R[4] ? R[4] : T[2] >> 4]) | (XPal[R[5] ? R[5] : T[2] & 0x0F]) << 16;
                PL[3] = (XPal[R[6] ? R[6] : T[3] >> 4]) | (XPal[R[7] ? R[7] : T[3] & 0x0F]) << 16;
            }
        }
        else
        {
            for (X = 0; X < XE; X++, R += 8, P += 8, T += 4, PL += 4)
            {
                if (X == 32 - SL % 32)
                {
                    T = ChrTab + (((int)(Y + VScroll) << 7) & ChrTabM & 0x7FFF);
                    T += SL < 32 ? 0 : -0x8000;
                }

                if (!(sprDraw & (1 << X)))
                {
                    PL[0] = (XPal[T[0] >> 4]) | (XPal[T[0] & 0x0F]) << 16;
                    PL[1] = (XPal[T[1] >> 4]) | (XPal[T[1] & 0x0F]) << 16;
                    PL[2] = (XPal[T[2] >> 4]) | (XPal[T[2] & 0x0F]) << 16;
                    PL[3] = (XPal[T[3] >> 4]) | (XPal[T[3] & 0x0F]) << 16;
                    continue;
                }
                R0 = R[0]; R1 = R[1]; R2 = R[2]; R3 = R[3];
                R4 = R[4]; R5 = R[5]; R6 = R[6]; R7 = R[7];
                PL[0] = (XPal[R[0] ? R[0] : T[0] >> 4]) | (XPal[R[1] ? R[1] : T[0] & 0x0F]) << 16;
                PL[1] = (XPal[R[2] ? R[2] : T[1] >> 4]) | (XPal[R[3] ? R[3] : T[1] & 0x0F]) << 16;
                PL[2] = (XPal[R[4] ? R[4] : T[2] >> 4]) | (XPal[R[5] ? R[5] : T[2] & 0x0F]) << 16;
                PL[3] = (XPal[R[6] ? R[6] : T[3] >> 4]) | (XPal[R[7] ? R[7] : T[3] & 0x0F]) << 16;
            }

        }
		if (SR != 0)
		{
			cnt = 8 - SR;
			if (cnt) { I = *R; *P = XPal[I ? I : T[0] >> 4];   cnt--; R++; P++; }
			if (cnt) { I = *R; *P = XPal[I ? I : T[0] & 0x0F]; cnt--; R++; P++; }
			if (cnt) { I = *R; *P = XPal[I ? I : T[1] >> 4];   cnt--; R++; P++; }
			if (cnt) { I = *R; *P = XPal[I ? I : T[1] & 0x0F]; cnt--; R++; P++; }
			if (cnt) { I = *R; *P = XPal[I ? I : T[2] >> 4];   cnt--; R++; P++; }
			if (cnt) { I = *R; *P = XPal[I ? I : T[2] & 0x0F]; cnt--; R++; P++; }
			if (cnt) { I = *R; *P = XPal[I ? I : T[3] >> 4];   cnt--; R++; P++; }
			if (cnt) { I = *R; *P = XPal[I ? I : T[3] & 0x0F]; cnt--; R++; P++; }
		}
        if (MaskEdges)
        {
            P -= 256;
            P[0] = P[1] = P[2] = P[3] = P[4] = P[5] = P[6] = P[7] = P[8] = XPal[BGColor];
        }
    }
}


/** RefreshLine8() *******************************************/
/** Refresh line Y (0..191/211) of SCREEN8, including       **/
/** sprites in this line.                                   **/
/*************************************************************/
void RefreshLine8(register byte Y)
{
    /*static byte SprToScr[16] =
    {
      0x00,0x02,0x10,0x12,0x80,0x82,0x90,0x92,
      0x49,0x4B,0x59,0x5B,0xC9,0xCB,0xD9,0xDB
    };*/
    //Value is vary from devices. Use Debug_CalcBPalVlue() in the 3DSLib.cpp for get value for 3DS.
    static byte SprToScr[16] =
    {
      0,2,12,14,96,98,108,110,
      158,3,28,31,224,227,252,255
    };
    register pixel* P;
    register byte C, X, XE, * T,* T2, * R, R0, R1, R2, R3, R4, R5, R6, R7;
    int cnt, SL, SR;
    u32 sprDraw, *PL;
    SR = VDP[27] & 0x07;
    SL = VDP[26];
    byte ZBuf[320];

    P = RefreshBorder(Y, BPal[VDP[7]]);
    if (!P) return;

    if (!ScreenON) ClearLine(P, BPal[VDP[7]]);
    else
    {
        sprDraw = ColorSpritesScr78(Y, ZBuf);
        R = ZBuf + 32 + SR;
        T = ChrTab + ((((int)(Y + VScroll) << 8) & ChrTabM & 0xFFFF) >> 1);
        if (UseInterlace && InterlaceON && FlipEvenOdd && (VDPStatus[2] & 0x02))T -= 0x8000;

        if (HScroll512)
        {
            T += SL < 32 ? -0x8000 : -128;
            if (!(VDP[2] & 0x20))T += 0x8000;
            T += SL * 4;
        }
        else T += (SL%32) * 4;
        T2 = T + 0x10000;

		if (SR == 0)
		{
			XE = 32;
		}
		else
		{
			XE = 31;
			for (X = 0; X < SR; X++)
			{
				*P = BPal[VDP[7]]; P++;
			}
		}
        PL = (u32*)P;
        if (!HScroll512)
        {
            for (X = 0; X < XE; X++, T += 4, T2 += 4, R += 8, P += 8, PL += 4)
            {
                if (!(sprDraw & (1 << X)))
                {
                    PL[0] = (BPal[T[0]]) | (BPal[T2[0]] << 16);
                    PL[1] = (BPal[T[1]]) | (BPal[T2[1]] << 16);
                    PL[2] = (BPal[T[2]]) | (BPal[T2[2]] << 16);
                    PL[3] = (BPal[T[3]]) | (BPal[T2[3]] << 16);
                    continue;
                }
                R0 = R[0]; R1 = R[1]; R2 = R[2]; R3 = R[3];
                R4 = R[4]; R5 = R[5]; R6 = R[6]; R7 = R[7];
                PL[0] = (BPal[R0 ? SprToScr[R0 >> 1] : T[0]]) | (BPal[R1 ? SprToScr[R1 >> 1] : T2[0]] << 16);
                PL[1] = (BPal[R2 ? SprToScr[R2 >> 1] : T[1]]) | (BPal[R3 ? SprToScr[R3 >> 1] : T2[1]] << 16);
                PL[2] = (BPal[R4 ? SprToScr[R4 >> 1] : T[2]]) | (BPal[R5 ? SprToScr[R5 >> 1] : T2[2]] << 16);
                PL[3] = (BPal[R6 ? SprToScr[R6 >> 1] : T[3]]) | (BPal[R7 ? SprToScr[R7 >> 1] : T2[3]] << 16);
            }
        }
        else
        {
            for (X = 0; X < XE; X++, T += 4, T2 += 4, R += 8, P += 8, PL += 4)
            {
                if (X == 32 - SL % 32)
                {
                    T = ChrTab + ((((int)(Y + VScroll) << 8) & ChrTabM & 0xFFFF) >> 1);
                    T += SL < 32 ? 0 : -0x8000;
                    T2 = T + 0x10000;
                }
                if (!(sprDraw & (1 << X)))
                {
                    PL[0] = (BPal[T[0]]) | (BPal[T2[0]] << 16);
                    PL[1] = (BPal[T[1]]) | (BPal[T2[1]] << 16);
                    PL[2] = (BPal[T[2]]) | (BPal[T2[2]] << 16);
                    PL[3] = (BPal[T[3]]) | (BPal[T2[3]] << 16);
                    continue;
                }
                R0 = R[0]; R1 = R[1]; R2 = R[2]; R3 = R[3];
                R4 = R[4]; R5 = R[5]; R6 = R[6]; R7 = R[7];
                PL[0] = (BPal[R0 ? SprToScr[R0 >> 1] : T[0]]) | (BPal[R1 ? SprToScr[R1 >> 1] : T2[0]] << 16);
                PL[1] = (BPal[R2 ? SprToScr[R2 >> 1] : T[1]]) | (BPal[R3 ? SprToScr[R3 >> 1] : T2[1]] << 16);
                PL[2] = (BPal[R4 ? SprToScr[R4 >> 1] : T[2]]) | (BPal[R5 ? SprToScr[R5 >> 1] : T2[2]] << 16);
                PL[3] = (BPal[R6 ? SprToScr[R6 >> 1] : T[3]]) | (BPal[R7 ? SprToScr[R7 >> 1] : T2[3]] << 16);
            }
        }
		if (SR != 0)
		{
			cnt = 8 - SR;
			if (cnt) { C = *R; *P = BPal[C ? SprToScr[C >> 1] : T[0]]; cnt--; P++; R++; }
			if (cnt) { C = *R; *P = BPal[C ? SprToScr[C >> 1] : T2[0]]; cnt--; P++; R++; }
			if (cnt) { C = *R; *P = BPal[C ? SprToScr[C >> 1] : T[1]]; cnt--; P++; R++; }
			if (cnt) { C = *R; *P = BPal[C ? SprToScr[C >> 1] : T2[1]]; cnt--; P++; R++; }
			if (cnt) { C = *R; *P = BPal[C ? SprToScr[C >> 1] : T[2]]; cnt--; P++; R++; }
			if (cnt) { C = *R; *P = BPal[C ? SprToScr[C >> 1] : T2[2]]; cnt--; P++; R++; }
			if (cnt) { C = *R; *P = BPal[C ? SprToScr[C >> 1] : T[3]]; cnt--; P++; R++; }
			if (cnt) { C = *R; *P = BPal[C ? SprToScr[C >> 1] : T2[3]]; cnt--; P++; R++; }
		}

        if (MaskEdges)
        {
            P -= 256;
            P[0] = P[1] = P[2] = P[3] = P[4] = P[5] = P[6] = P[7] = P[8] = XPal[BGColor];
        }
    }
}


/** RefreshLine10() ******************************************/
/** Refresh line Y (0..191/211) of SCREEN10/11, including   **/
/** sprites in this line.                                   **/
/*************************************************************/
void RefreshLine10(register byte Y)
{
	register pixel* P;
	register byte C, X, XE, * T, * R, Y1;
	register int J, K, cnt;
	u32* PL, sprDraw;
	int SL, SR;
	byte ZBuf[320];

	P = RefreshBorder(Y, BPal[VDP[7]]);
	if (!P) return;
	SR = VDP[27] & 0x07;
	SL = VDP[26];

	if (!ScreenON) ClearLine(P, BPal[VDP[7]]);
	else
	{
		sprDraw = ColorSpritesScr78(Y, ZBuf);
        R = ZBuf + 32 + SR;
		T = ChrTab + ((((int)(Y + VScroll) << 8) & ChrTabM & 0xFFFF) >> 1);
		if (UseInterlace && InterlaceON && FlipEvenOdd && (VDPStatus[2] & 0x02))T -= 0x8000;
		if (HScroll512)
		{
			T += SL < 32 ? -0x8000 : -128;
			if (!(VDP[2] & 0x20))T += 0x8000;
            T += SL * 4;
		}
		else T += (SL%32) * 4;
        if (MaskEdges)
        {
            T -= 2;
            P -= 4;
            R -= 4;
        }

		if (SR == 0)
		{
			XE = 64;
		}
		else
		{
			XE = 63;
			for (X = 0; X < SR; X++)
			{
				*P = BPal[VDP[7]]; P++;
			}
		}
		PL = (u32*)P;
		if (!HScroll512)
		{
			if (ModeYJK)
			{
				for (X = 0; X < XE; X++, T += 2, R += 4, P += 4, PL += 2)
				{
					//if (X == 64 - SL % 64)
					//{
					//	T = ChrTab + ((((int)(Y + VScroll) << 8) & ChrTabM & 0xFFFF) >> 1);
					//	if ((VDP[27] & 0x07) - ((VDP[26] & 0x03F) << 3) >= 0)T -= 128;
					//}
					K = (T[0] & 0x07) | ((T[0x10000] & 0x07) << 3);
					if (K & 0x20) K -= 64;
					J = (T[1] & 0x07) | ((T[0x10001] & 0x07) << 3);
					if (J & 0x20) J -= 64;

					if (!(sprDraw & (1 << (X >> 1))))
					{

						Y = T[0] >> 3; Y1 = T[0x10000] >> 3; PL[0] = (Y & 1 ? XPal[Y >> 1] : YJKColor(Y, J, K)) | (Y1 & 1 ? XPal[Y1 >> 1] : YJKColor(Y1, J, K)) << 16;
						Y = T[1] >> 3; Y1 = T[0x10001] >> 3; PL[1] = (Y & 1 ? XPal[Y >> 1] : YJKColor(Y, J, K)) | (Y1 & 1 ? XPal[Y1 >> 1] : YJKColor(Y1, J, K)) << 16;
						//T += 2; R += 4; P += 4; PL += 2;
						continue;
					}

					Y = T[0] >> 3; Y1 = T[0x10000] >> 3; PL[0] = (R[0] ? XPal[R[0] >> 1] : Y & 1 ? XPal[Y >> 1] : YJKColor(Y, J, K))
						| (R[1] ? XPal[R[1] >> 1] : Y1 & 1 ? XPal[Y1 >> 1] : YJKColor(Y1, J, K))<<16;
					Y = T[1] >> 3; Y1 = T[0x10001] >> 3; PL[1] = (R[2] ? XPal[R[2] >> 1] : Y & 1 ? XPal[Y >> 1] : YJKColor(Y, J, K))
						| (R[3] ? XPal[R[3] >> 1] : Y & 1 ? XPal[Y >> 1] : YJKColor(Y, J, K))<<16;
					//T += 2; R += 4; P += 4; PL += 2;
				}
			}
			else
			{
				for (X = 0; X < XE; X++, T += 2, R += 4, P += 4, PL += 2)
				{
					//if (X == 64 - SL % 64)
     //               if (X == 64 - (SL << 1) % 64)
					//{
					//	T = ChrTab + ((((int)(Y + VScroll) << 8) & ChrTabM & 0xFFFF) >> 1);
					//	//if ((VDP[27] & 0x07) - ((VDP[26] & 0x03F) << 3) >= 0)T -= 128;
					//}
					K = (T[0] & 0x07) | ((T[0x10000] & 0x07) << 3);
					if (K & 0x20) K -= 64;
					J = (T[1] & 0x07) | ((T[0x10001] & 0x07) << 3);
					if (J & 0x20) J -= 64;

					if (!(sprDraw & (1 << (X >> 1))))
					{

						Y = T[0] >> 3; Y1 = T[0x10000] >> 3; PL[0] = (Y & 1 ? XPal[Y >> 1] : YAEColor(Y, J, K)) | (Y1 & 1 ? XPal[Y1 >> 1] : YAEColor(Y1, J, K)) << 16;
						Y = T[1] >> 3; Y1 = T[0x10001] >> 3; PL[1] = (Y & 1 ? XPal[Y >> 1] : YAEColor(Y, J, K)) | (Y1 & 1 ? XPal[Y1 >> 1] : YAEColor(Y1, J, K)) << 16;
						continue;
					}

					Y = T[0] >> 3; Y1 = T[0x10000] >> 3; PL[0] = (R[0] ? XPal[R[0] >> 1] : Y & 1 ? XPal[Y >> 1] : YAEColor(Y, J, K))
						| (R[1] ? XPal[R[1] >> 1] : Y1 & 1 ? XPal[Y1 >> 1] : YAEColor(Y1, J, K))<<16;
					Y = T[1] >> 3; Y1 = T[0x10001] >> 3; PL[1] = (R[2] ? XPal[R[2] >> 1] : Y & 1 ? XPal[Y >> 1] : YAEColor(Y, J, K))
						| (R[3] ? XPal[R[3] >> 1] : Y & 1 ? XPal[Y >> 1] : YAEColor(Y, J, K))<<16;
				}
			}
		}
		else
		{
			if (ModeYJK)
			{
				for (X = 0; X < XE; X++, T += 2, R += 4, P += 4, PL += 2)
				{
					//if (X == 64 - SL % 64)
                    if (X == 64 - (SL<<1) % 64)
					{
						T = ChrTab + ((((int)(Y + VScroll) << 8) & ChrTabM & 0xFFFF) >> 1);
						T += SL < 32 ? 0 : -0x8000;
					}
					K = (T[0] & 0x07) | ((T[0x10000] & 0x07) << 3);
					if (K & 0x20) K -= 64;
					J = (T[1] & 0x07) | ((T[0x10001] & 0x07) << 3);
					if (J & 0x20) J -= 64;

					if (!(sprDraw & (1 << (X >> 1))))
					{

						Y = T[0] >> 3; Y1 = T[0x10000] >> 3; PL[0] = (Y & 1 ? XPal[Y >> 1] : YJKColor(Y, J, K)) | (Y1 & 1 ? XPal[Y1 >> 1] : YJKColor(Y1, J, K)) << 16;
						Y = T[1] >> 3; Y1 = T[0x10001] >> 3; PL[1] = (Y & 1 ? XPal[Y >> 1] : YJKColor(Y, J, K)) | (Y1 & 1 ? XPal[Y1 >> 1] : YJKColor(Y1, J, K)) << 16;
						continue;
					}

					Y = T[0] >> 3; Y1 = T[0x10000] >> 3; PL[0] = (R[0] ? XPal[R[0] >> 1] : Y & 1 ? XPal[Y >> 1] : YJKColor(Y, J, K))
						| (R[1] ? XPal[R[1] >> 1] : Y1 & 1 ? XPal[Y1 >> 1] : YJKColor(Y1, J, K))<<16;
					Y = T[1] >> 3; Y1 = T[0x10001] >> 3; PL[1] = (R[2] ? XPal[R[2] >> 1] : Y & 1 ? XPal[Y >> 1] : YJKColor(Y, J, K))
						| (R[3] ? XPal[R[3] >> 1] : Y1 & 1 ? XPal[Y1 >> 1] : YJKColor(Y1, J, K))<<16;
				}
			}
			else
			{
				for (X = 0; X < XE; X++, T += 2, R += 4, P += 4, PL += 2)
				{
					//if (X == 64 - SL % 64)
                    if (X == 64 - (SL << 1) % 64)
					{
						T = ChrTab + ((((int)(Y + VScroll) << 8) & ChrTabM & 0xFFFF) >> 1);
						T += SL < 32 ? 0 : -0x8000;
					}
					K = (T[0] & 0x07) | ((T[0x10000] & 0x07) << 3);
					if (K & 0x20) K -= 64;
					J = (T[1] & 0x07) | ((T[0x10001] & 0x07) << 3);
					if (J & 0x20) J -= 64;

					if (!(sprDraw & (1 << (X >> 1))))
					{

						Y = T[0] >> 3; Y1 = T[0x10000] >> 3; PL[0] = (Y & 1 ? XPal[Y >> 1] : YAEColor(Y, J, K)) | (Y1 & 1 ? XPal[Y1 >> 1] : YAEColor(Y1, J, K)) << 16;
						Y = T[1] >> 3; Y1 = T[0x10001] >> 3; PL[1] = (Y & 1 ? XPal[Y >> 1] : YAEColor(Y, J, K)) | (Y1 & 1 ? XPal[Y1 >> 1] : YAEColor(Y1, J, K)) << 16;
						continue;
					}

					Y = T[0] >> 3; Y1 = T[0x10000] >> 3; PL[0] = (R[0] ? XPal[R[0] >> 1] : Y & 1 ? XPal[Y >> 1] : YAEColor(Y, J, K))
						| (R[1] ? XPal[R[1] >> 1] : Y1 & 1 ? XPal[Y1 >> 1] : YAEColor(Y1, J, K))<<16;
					Y = T[1] >> 3; Y1 = T[0x10001] >> 3; PL[1] = (R[2] ? XPal[R[2] >> 1] : Y & 1 ? XPal[Y >> 1] : YAEColor(Y, J, K))
						| (R[3] ? XPal[R[3] >> 1] : Y1 & 1 ? XPal[Y1 >> 1] : YAEColor(Y1, J, K))<<16;
				}
			}
		}
		if (SR != 0)
		{
			cnt = 8 - SR;
			if (ModeYJK)
			{
				for (X = 0; X < 2; X++)
				{
					K = (T[0] & 0x07) | ((T[0x10000] & 0x07) << 3);
					if (K & 0x20) K -= 64;
					J = (T[1] & 0x07) | ((T[0x10001] & 0x07) << 3);
					if (J & 0x20) J -= 64;
					if (cnt) { C = R[0]; Y = T[0] >> 3; P[0] = C ? XPal[C >> 1] : Y & 1 ? XPal[Y >> 1] : YJKColor(Y, J, K); cnt--; }
					else break;
					if (cnt) { C = R[1]; Y = T[0x10000] >> 3; P[1] = C ? XPal[C >> 1] : Y & 1 ? XPal[Y >> 1] : YJKColor(Y, J, K); cnt--; }
					else break;
					if (cnt) { C = R[2]; Y = T[1] >> 3; P[2] = C ? XPal[C >> 1] : Y & 1 ? XPal[Y >> 1] : YJKColor(Y, J, K); cnt--; }
					else break;
					if (cnt) { C = R[3]; Y = T[0x10001] >> 3; P[3] = C ? XPal[C >> 1] : Y & 1 ? XPal[Y >> 1] : YJKColor(Y, J, K); cnt--; }
					else break;
					T += 2; R += 4; P += 4;
				}
			}
			else
			{
				for (X = 0; X < 2; X++)
				{
					K = (T[0] & 0x07) | ((T[0x10000] & 0x07) << 3);
					if (K & 0x20) K -= 64;
					J = (T[1] & 0x07) | ((T[0x10001] & 0x07) << 3);
					if (J & 0x20) J -= 64;
					if (cnt) { C = R[0]; Y = T[0] >> 3; P[0] = C ? XPal[C >> 1] : Y & 1 ? XPal[Y >> 1] : YAEColor(Y, J, K); cnt--; }
					else break;
					if (cnt) { C = R[1]; Y = T[0x10000] >> 3; P[1] = C ? XPal[C >> 1] : Y & 1 ? XPal[Y >> 1] : YAEColor(Y, J, K); cnt--; }
					else break;
					if (cnt) { C = R[2]; Y = T[1] >> 3; P[2] = C ? XPal[C >> 1] : Y & 1 ? XPal[Y >> 1] : YAEColor(Y, J, K); cnt--; }
					else break;
					if (cnt) { C = R[3]; Y = T[0x10001] >> 3; P[3] = C ? XPal[C >> 1] : Y & 1 ? XPal[Y >> 1] : YAEColor(Y, J, K); cnt--; }
					else break;
					T += 2; R += 4; P += 4;
				}
			}
		}
		if (MaskEdges)
		{
			P -= 256;
			P[0] = P[1] = P[2] = P[3] = P[4] = P[5] = P[6] = P[7] = P[8] = BPal[VDP[7]];
		}
	}
}


/** RefreshLine12() ******************************************/
/** Refresh line Y (0..191/211) of SCREEN12, including      **/
/** sprites in this line.                                   **/
/*************************************************************/
void RefreshLine12(register byte Y)
{
    pixel* P;
    register byte C, X, XE, * T, * T2, * R, TA0, TA1, TB0, TB1, R0, R1, R2, R3;
    int J, J2, K, K2, cnt;
    int SL, SR;
    register u32 sprDraw, *PL;
    byte ZBuf[320];

    P = RefreshBorder(Y, BPal[VDP[7]]);
    if (!P) return;
    SR = VDP[27] & 0x07;
    SL = VDP[26];

    if (!ScreenON) ClearLine(P, BPal[VDP[7]]);
    else
    {
        sprDraw = ColorSpritesScr78(Y, ZBuf);
        R = ZBuf + 32 + SR;
        T = ChrTab + ((((int)(Y + VScroll) << 8) & ChrTabM & 0xFFFF) >> 1);
        if (UseInterlace && InterlaceON && FlipEvenOdd && (VDPStatus[2] & 0x02))T -= 0x8000;
        if (HScroll512)
        {
            T += SL < 32 ? -0x8000 : -128;
            if (!(VDP[2] & 0x20))T += 0x8000;
            T += SL * 4;
        }
        else T += (SL%32) * 4;
        T2 = T + 0x010000;
        if (MaskEdges)
        {
            T -= 2;
            T2 -= 2;
            P -= 4;
            R -= 4;
        }

		if (SR == 0)
		{
			XE = 64;
		}
		else
		{
			XE = 63;
			for (X = 0; X < SR; X++)
			{
				*P = BPal[VDP[7]]; P++;
			}
		}
        PL = (u32*)P;
        if (!HScroll512)
        {
            for (X = 0; X < XE; X++, T += 2, T2 += 2, R += 4, P += 4, PL += 2)
            {
                TA0 = T[0]; TA1 = T[1];
                TB0 = T2[0]; TB1 = T2[1];
                K = (TA0 & 0x07) | ((TB0 & 0x07) << 3);
                if (K & 0x20) K -= 64;
                J = (TA1 & 0x07) | ((TB1 & 0x07) << 3);
                if (J & 0x20) J -= 64;

                if (!(sprDraw & (1 << (X >> 1))))
                {
                    PL[0] = (YJKColor(TA0 >> 3, J, K)) | (YJKColor(TB0 >> 3, J, K)) << 16;
                    PL[1] = (YJKColor(TA1 >> 3, J, K)) | (YJKColor(TB1 >> 3, J, K)) << 16;
                    continue;
                }
                R0 = R[0]; R1 = R[1];
                R2 = R[2]; R3 = R[3];
                PL[0] = (R0 ? XPal[R0 >> 1] : YJKColor(TA0 >> 3, J, K)) | (R1 ? XPal[R1 >> 1] : YJKColor(TB0 >> 3, J, K)) << 16;
                PL[1] = (R2 ? XPal[R2 >> 1] : YJKColor(TA1 >> 3, J, K)) | (R3 ? XPal[R3 >> 1] : YJKColor(TB1 >> 3, J, K)) << 16;
            }
        }
        else
        {
            for (X = 0; X < XE; X++, T += 2, T2 += 2, R += 4, P += 4, PL += 2)
            {
                if (X == 64 - (SL << 1) % 64)
                {
                    T = ChrTab + ((((int)(Y + VScroll) << 8) & ChrTabM & 0xFFFF) >> 1);
                    T += SL < 32 ? 0 : -0x8000;
                    T2 = T + 0x10000;
                }

                TA0 = T[0]; TA1 = T[1];
                TB0 = T2[0]; TB1 = T2[1];
                K = (TA0 & 0x07) | ((TB0 & 0x07) << 3);
                if (K & 0x20) K -= 64;
                J = (TA1 & 0x07) | ((TB1 & 0x07) << 3);
                if (J & 0x20) J -= 64;

                if (!(sprDraw & (1 << (X >> 1))))
                {
                    PL[0] = (YJKColor(TA0 >> 3, J, K)) | (YJKColor(TB0 >> 3, J, K)) << 16;
                    PL[1] = (YJKColor(TA1 >> 3, J, K)) | (YJKColor(TB1 >> 3, J, K)) << 16;
                    continue;
                }
                R0 = R[0]; R1 = R[1];
                R2 = R[2]; R3 = R[3];
                PL[0] = (R0 ? XPal[R0 >> 1] : YJKColor(TA0 >> 3, J, K)) | (R1 ? XPal[R1 >> 1] : YJKColor(TB0 >> 3, J, K)) << 16;
                PL[1] = (R2 ? XPal[R2 >> 1] : YJKColor(TA1 >> 3, J, K)) | (R3 ? XPal[R3 >> 1] : YJKColor(TB1 >> 3, J, K)) << 16;
            }
        }
		if (SR != 0)
		{
			cnt = 8 - SR;
			for (X = 0; X < 2; X++)
			{
				K = (T[0] & 0x07) | ((T2[0] & 0x07) << 3);
				if (K & 0x20) K -= 64;
				J = (T[1] & 0x07) | ((T2[1] & 0x07) << 3);
				if (J & 0x20) J -= 64;
				if (cnt) { C = R[0]; P[0] = C ? XPal[C >> 1] : YJKColor(T[0] >> 3, J, K); cnt--; }
				else break;
				if (cnt) { C = R[1]; P[1] = C ? XPal[C >> 1] : YJKColor(T2[0] >> 3, J, K); cnt--; }
				else break;
				if (cnt) { C = R[2]; P[2] = C ? XPal[C >> 1] : YJKColor(T[1] >> 3, J, K); cnt--; }
				else break;
				if (cnt) { C = R[3]; P[3] = C ? XPal[C >> 1] : YJKColor(T2[1] >> 3, J, K); cnt--; }
				else break;
				T += 2, R += 4, P += 4;
			}
		}
        if (MaskEdges)
        {
            P -= 256;
            P[0] = P[1] = P[2] = P[3] = P[4] = P[5] = P[6] = P[7] = P[8] = BPal[VDP[7]];
        }
    }
}


/** RefreshLine6() *******************************************/
/** Refresh line Y (0..191/211) of SCREEN6, including       **/
/** sprites in this line.                                   **/
/*************************************************************/
void RefreshLine6(register byte Y)
{
    if (!IsWide)
    {
        register pixel* P;
        u32 sprDraw;
        register byte X, XE, * T, * R, C;
        int cnt, SL, SR;
        byte ZBuf[320];

        P = RefreshBorder(Y, XPal[BGColor & 0x03]);
        if (!P) return;

        SR = VDP[27] & 0x07;
        SL = VDP[26];

        if (!ScreenON) ClearLine(P, XPal[BGColor & 0x03]);
        else
        {
            sprDraw = ColorSprites(Y, ZBuf);
            R = ZBuf + 32 + SR;
            T = ChrTab + (((int)(Y + VScroll) << 7) & ChrTabM & 0x7FFF);
            if (UseInterlace && InterlaceON && FlipEvenOdd && (VDPStatus[2] & 0x02))T -= 0x8000;
            if (HScroll512)
            {
                T += SL < 32 ? -0x8000 : -128;
                if (!(VDP[2] & 0x20))T += 0x8000;
                T += SL * 4;
            }
            else  T += (SL%32) * 4;

			if (SR == 0)
			{
				XE = 32;
			}
			else
			{
				XE = 31;
				for (X = 0; X < SR; X++)
				{
					*P = XPal[BGColor & 0x03]; P++;
				}
			}
            if (!HScroll512)
            {
                for (X = 0; X < XE; X++, R += 8, P += 8, T += 4)
                {
                    if (X == 32 - SL % 32)
                    {
                        T = ChrTab + (((int)(Y + VScroll) << 7) & ChrTabM & 0x7FFF);
                        if ((VDP[27] & 0x07) - ((VDP[26] & 0x03F) << 3) >= 0)T -= 128;
                    }
                    if (!(sprDraw & (1 << X)))
                    {
                        P[0] = ColAve(XPal[T[0] >> 6], XPal[(T[0] >> 4) & 0x03]);
                        P[1] = ColAve(XPal[(T[0] >> 2) & 0x03], XPal[T[0] & 0x03]);
                        P[2] = ColAve(XPal[T[1] >> 6], XPal[(T[1] >> 4) & 0x03]);
                        P[3] = ColAve(XPal[(T[1] >> 2) & 0x03], XPal[T[1] & 0x03]);
                        P[4] = ColAve(XPal[T[2] >> 6], XPal[(T[2] >> 4) & 0x03]);
                        P[5] = ColAve(XPal[(T[2] >> 2) & 0x03], XPal[T[2] & 0x03]);
                        P[6] = ColAve(XPal[T[3] >> 6], XPal[(T[3] >> 4) & 0x03]);
                        P[7] = ColAve(XPal[(T[3] >> 2) & 0x03], XPal[T[3] & 0x03]);
                        continue;
                    }
                    C = R[0]; P[0] = C ? XPal[(C >> 1) & 0x03] : ColAve(XPal[T[0] >> 6], XPal[(T[0] >> 4) & 0x03]);
                    C = R[1]; P[1] = C ? XPal[(C >> 1) & 0x03] : ColAve(XPal[(T[0] >> 2) & 0x03], XPal[T[0] & 0x03]);
                    C = R[2]; P[2] = C ? XPal[(C >> 1) & 0x03] : ColAve(XPal[T[1] >> 6], XPal[(T[1] >> 4) & 0x03]);
                    C = R[3]; P[3] = C ? XPal[(C >> 1) & 0x03] : ColAve(XPal[(T[1] >> 2) & 0x03], XPal[T[1] & 0x03]);
                    C = R[4]; P[4] = C ? XPal[(C >> 1) & 0x03] : ColAve(XPal[T[2] >> 6], XPal[(T[2] >> 4) & 0x03]);
                    C = R[5]; P[5] = C ? XPal[(C >> 1) & 0x03] : ColAve(XPal[(T[2] >> 2) & 0x03], XPal[T[2] & 0x03]);
                    C = R[6]; P[6] = C ? XPal[(C >> 1) & 0x03] : ColAve(XPal[T[3] >> 6], XPal[(T[3] >> 4) & 0x03]);
                    C = R[7]; P[7] = C ? XPal[(C >> 1) & 0x03] : ColAve(XPal[(T[3] >> 2) & 0x03], XPal[T[3] & 0x03]);
                }
            }
            else
            {
                for (X = 0; X < XE; X++, R += 8, P += 8, T += 4)
                {
                    if (X == 32 - SL % 32)
                    {
                        T = ChrTab + (((int)(Y + VScroll) << 7) & ChrTabM & 0x7FFF);
                        T += SL < 32 ? 0 : -0x8000;
                    }
                    if (!(sprDraw & (1 << X)))
                    {
                        P[0] = ColAve(XPal[T[0] >> 6], XPal[(T[0] >> 4) & 0x03]);
                        P[1] = ColAve(XPal[(T[0] >> 2) & 0x03], XPal[T[0] & 0x03]);
                        P[2] = ColAve(XPal[T[1] >> 6], XPal[(T[1] >> 4) & 0x03]);
                        P[3] = ColAve(XPal[(T[1] >> 2) & 0x03], XPal[T[1] & 0x03]);
                        P[4] = ColAve(XPal[T[2] >> 6], XPal[(T[2] >> 4) & 0x03]);
                        P[5] = ColAve(XPal[(T[2] >> 2) & 0x03], XPal[T[2] & 0x03]);
                        P[6] = ColAve(XPal[T[3] >> 6], XPal[(T[3] >> 4) & 0x03]);
                        P[7] = ColAve(XPal[(T[3] >> 2) & 0x03], XPal[T[3] & 0x03]);
                        continue;
                    }
                    C = R[0]; P[0] = C ? XPal[(C >> 1) & 0x03] : ColAve(XPal[T[0] >> 6], XPal[(T[0] >> 4) & 0x03]);
                    C = R[1]; P[1] = C ? XPal[(C >> 1) & 0x03] : ColAve(XPal[(T[0] >> 2) & 0x03], XPal[T[0] & 0x03]);
                    C = R[2]; P[2] = C ? XPal[(C >> 1) & 0x03] : ColAve(XPal[T[1] >> 6], XPal[(T[1] >> 4) & 0x03]);
                    C = R[3]; P[3] = C ? XPal[(C >> 1) & 0x03] : ColAve(XPal[(T[1] >> 2) & 0x03], XPal[T[1] & 0x03]);
                    C = R[4]; P[4] = C ? XPal[(C >> 1) & 0x03] : ColAve(XPal[T[2] >> 6], XPal[(T[2] >> 4) & 0x03]);
                    C = R[5]; P[5] = C ? XPal[(C >> 1) & 0x03] : ColAve(XPal[(T[2] >> 2) & 0x03], XPal[T[2] & 0x03]);
                    C = R[6]; P[6] = C ? XPal[(C >> 1) & 0x03] : ColAve(XPal[T[3] >> 6], XPal[(T[3] >> 4) & 0x03]);
                    C = R[7]; P[7] = C ? XPal[(C >> 1) & 0x03] : ColAve(XPal[(T[3] >> 2) & 0x03], XPal[T[3] & 0x03]);
                }
            }
			if (SR != 0)
			{
				cnt = 8 - SR;
				if (cnt) { C = *R; *P = C ? XPal[(C >> 1) & 0x03] : ColAve(XPal[T[0] >> 6], XPal[(T[0] >> 4) & 0x03]); cnt--; P++; R++; }
				if (cnt) { C = *R; *P = C ? XPal[(C >> 1) & 0x03] : ColAve(XPal[(T[0] >> 2) & 0x03], XPal[T[0] & 0x03]); cnt--; P++; R++; }
				if (cnt) { C = *R; *P = C ? XPal[(C >> 1) & 0x03] : ColAve(XPal[T[1] >> 6], XPal[(T[1] >> 4) & 0x03]); cnt--; P++; R++; }
				if (cnt) { C = *R; *P = C ? XPal[(C >> 1) & 0x03] : ColAve(XPal[(T[1] >> 2) & 0x03], XPal[T[1] & 0x03]); cnt--; P++; R++; }
				if (cnt) { C = *R; *P = C ? XPal[(C >> 1) & 0x03] : ColAve(XPal[T[2] >> 6], XPal[(T[2] >> 4) & 0x03]); cnt--; P++; R++; }
				if (cnt) { C = *R; *P = C ? XPal[(C >> 1) & 0x03] : ColAve(XPal[(T[2] >> 2) & 0x03], XPal[T[2] & 0x03]); cnt--; P++; R++; }
				if (cnt) { C = *R; *P = C ? XPal[(C >> 1) & 0x03] : ColAve(XPal[T[3] >> 6], XPal[(T[3] >> 4) & 0x03]); cnt--; P++; R++; }
				if (cnt) { C = *R; *P = C ? XPal[(C >> 1) & 0x03] : ColAve(XPal[(T[3] >> 2) & 0x03], XPal[T[3] & 0x03]); cnt--; P++; R++; }
			}
        }
    }
    else
    {
        register pixel* P;
        u32* PL, sprDraw;
        register byte X, XE, * T, * R, C, TA0, TA1, TA2, TA3;
        int cnt,SL,SR;
        byte ZBuf[320];
        register byte R0, R1, R2, R3, R4, R5, R6, R7;

        P = RefreshBorder512(Y, XPal[BGColor & 0x03]);
        if (!P) return;
        SR = VDP[27] & 0x07;
        SL = VDP[26];

        if (!ScreenON) ClearLine512(P, XPal[BGColor & 0x03]);
        else
        {
            sprDraw = ColorSprites(Y, ZBuf);
            R = ZBuf + 32 + SR;
            T = ChrTab + (((int)(Y + VScroll) << 7) & ChrTabM & 0x7FFF);
            if (UseInterlace && InterlaceON && FlipEvenOdd && (VDPStatus[2] & 0x02))T -= 0x8000;
            if (HScroll512)
            {
                T += SL < 32 ? -0x8000 : -128;
                if (!(VDP[2] & 0x20))T += 0x8000;
                T += SL * 4;
            }
            else T += (SL%32) * 4;

			if (SR == 0)
			{
				XE = 32;
			}
			else
			{
				XE = 31;
				for (X = 0; X < (SR << 1); X++)
				{
					*P = XPal[BGColor & 0x03]; P++;
				}
			}
            PL = (u32*)P;
            if (!HScroll512)
            {
                for (X = 0; X < XE; X++, R += 8, P += 16, PL += 8, T += 4)
                {
                    if (X == 32 - SL % 32)
                    {
                        T = ChrTab + (((int)(Y + VScroll) << 7) & ChrTabM & 0x7FFF);
                        if ((VDP[27] & 0x07) - ((VDP[26] & 0x03F) << 3) >= 0)T -= 128;
                    }

                    TA0 = T[0];
                    TA1 = T[1];
                    TA2 = T[2];
                    TA3 = T[3];

                    if (!(sprDraw & (1 << X)))
                    {
                        PL[0] = (XPal[TA0 >> 6]) | (XPal[(TA0 >> 4) & 0x03]) << 16;
                        PL[1] = (XPal[(TA0 >> 2) & 0x03]) | (XPal[TA0 & 0x03]) << 16;
                        PL[2] = (XPal[TA1 >> 6]) | (XPal[(TA1 >> 4) & 0x03]) << 16;
                        PL[3] = (XPal[(TA1 >> 2) & 0x03]) | (XPal[TA1 & 0x03]) << 16;
                        PL[4] = (XPal[TA2 >> 6]) | (XPal[(TA2 >> 4) & 0x03]) << 16;
                        PL[5] = (XPal[(TA2 >> 2) & 0x03]) | (XPal[TA2 & 0x03]) << 16;
                        PL[6] = (XPal[TA3 >> 6]) | (XPal[(TA3 >> 4) & 0x03]) << 16;
                        PL[7] = (XPal[(TA3 >> 2) & 0x03]) | (XPal[TA3 & 0x03]) << 16;
                        continue;
                    }
                    C = R[0]; PL[0] = (C ? XPal[(C >> 1) & 0x03] : XPal[TA0 >> 6]) | (C ? XPal[(C >> 1) & 0x03] : XPal[(TA0 >> 4) & 0x03]) << 16;
                    C = R[1]; PL[1] = (C ? XPal[(C >> 1) & 0x03] : XPal[(TA0 >> 2) & 0x03]) | (C ? XPal[(C >> 1) & 0x03] : XPal[TA0 & 0x03]) << 16;
                    C = R[2]; PL[2] = (C ? XPal[(C >> 1) & 0x03] : XPal[TA1 >> 6]) | (C ? XPal[(C >> 1) & 0x03] : XPal[(TA1 >> 4) & 0x03]) << 16;
                    C = R[3]; PL[3] = (C ? XPal[(C >> 1) & 0x03] : XPal[(TA1 >> 2) & 0x03]) | (C ? XPal[(C >> 1) & 0x03] : XPal[TA1 & 0x03]) << 16;
                    C = R[4]; PL[4] = (C ? XPal[(C >> 1) & 0x03] : XPal[TA2 >> 6]) | (C ? XPal[(C >> 1) & 0x03] : XPal[(TA2 >> 4) & 0x03]) << 16;
                    C = R[5]; PL[5] = (C ? XPal[(C >> 1) & 0x03] : XPal[(TA2 >> 2) & 0x03]) | (C ? XPal[(C >> 1) & 0x03] : XPal[TA2 & 0x03]) << 16;
                    C = R[6]; PL[6] = (C ? XPal[(C >> 1) & 0x03] : XPal[TA3 >> 6]) | (C ? XPal[(C >> 1) & 0x03] : XPal[(TA3 >> 4) & 0x03]) << 16;
                    C = R[7]; PL[7] = (C ? XPal[(C >> 1) & 0x03] : XPal[(TA3 >> 2) & 0x03]) | (C ? XPal[(C >> 1) & 0x03] : XPal[TA3 & 0x03]) << 16;
                }
            }
            else
            {
                for (X = 0; X < XE; X++, R += 8, P += 16, PL += 8, T += 4)
                {
                    if (X == 32 - SL % 32)
                    {
                        T = ChrTab + (((int)(Y + VScroll) << 7) & ChrTabM & 0x7FFF);
                        T += SL < 32 ? 0 : -0x8000;
                    }

                    TA0 = T[0];
                    TA1 = T[1];
                    TA2 = T[2];
                    TA3 = T[3];
                    
                    if (!(sprDraw & (1 << X)))
                    {
                        PL[0] = (XPal[TA0 >> 6]) | (XPal[(TA0 >> 4) & 0x03]) << 16;
                        PL[1] = (XPal[(TA0 >> 2) & 0x03]) | (XPal[TA0 & 0x03]) << 16;
                        PL[2] = (XPal[TA1 >> 6]) | (XPal[(TA1 >> 4) & 0x03]) << 16;
                        PL[3] = (XPal[(TA1 >> 2) & 0x03]) | (XPal[TA1 & 0x03]) << 16;
                        PL[4] = (XPal[TA2 >> 6]) | (XPal[(TA2 >> 4) & 0x03]) << 16;
                        PL[5] = (XPal[(TA2 >> 2) & 0x03]) | (XPal[TA2 & 0x03]) << 16;
                        PL[6] = (XPal[TA3 >> 6]) | (XPal[(TA3 >> 4) & 0x03]) << 16;
                        PL[7] = (XPal[(TA3 >> 2) & 0x03]) | (XPal[TA3 & 0x03]) << 16;
                        continue;
                    }
                    PL[0] = (R[0] ? XPal[(R[0] >> 1) & 0x03] : XPal[TA0 >> 6]) | (R[0] ? XPal[(R[0] >> 1) & 0x03] : XPal[(TA0 >> 4) & 0x03]) << 16;
                    PL[1] = (R[1] ? XPal[(R[1] >> 1) & 0x03] : XPal[(TA0 >> 2) & 0x03]) | (R[1] ? XPal[(R[1] >> 1) & 0x03] : XPal[TA0 & 0x03]) << 16;
                    PL[2] = (R[2] ? XPal[(R[2] >> 1) & 0x03] : XPal[TA1 >> 6]) | (R[2] ? XPal[(R[2] >> 1) & 0x03] : XPal[(TA1 >> 4) & 0x03]) << 16;
                    PL[3] = (R[3] ? XPal[(R[3] >> 1) & 0x03] : XPal[(TA1 >> 2) & 0x03]) | (R[3] ? XPal[(R[3] >> 1) & 0x03] : XPal[TA1 & 0x03]) << 16;
                    PL[4] = (R[4] ? XPal[(R[4] >> 1) & 0x03] : XPal[TA2 >> 6]) | (R[4] ? XPal[(R[4] >> 1) & 0x03] : XPal[(TA2 >> 4) & 0x03]) << 16;
                    PL[5] = (R[5] ? XPal[(R[5] >> 1) & 0x03] : XPal[(TA2 >> 2) & 0x03]) | (R[5] ? XPal[(R[5] >> 1) & 0x03] : XPal[TA2 & 0x03]) << 16;
                    PL[6] = (R[6] ? XPal[(R[6] >> 1) & 0x03] : XPal[TA3 >> 6]) | (R[6] ? XPal[(R[6] >> 1) & 0x03] : XPal[(TA3 >> 4) & 0x03]) << 16;
                    PL[7] = (R[7] ? XPal[(R[7] >> 1) & 0x03] : XPal[(TA3 >> 2) & 0x03]) | (R[7] ? XPal[(R[7] >> 1) & 0x03] : XPal[TA3 & 0x03]) << 16;
                }
            }
			if (SR != 0)
			{
				cnt = 16 - SR;
				if (cnt) { C = *R; *P = C ? XPal[(C >> 1) & 0x03] : XPal[T[0] >> 6]; cnt--; P++; }
				if (cnt) { C = *R; *P = C ? XPal[(C >> 1) & 0x03] : XPal[(T[0] >> 4) & 0x03]; cnt--; P++; R++; }
				if (cnt) { C = *R; *P = C ? XPal[(C >> 1) & 0x03] : XPal[(T[0] >> 2) & 0x03]; cnt--; P++; }
				if (cnt) { C = *R; *P = C ? XPal[(C >> 1) & 0x03] : XPal[T[0] & 0x03]; cnt--; P++; R++; }
				if (cnt) { C = *R; *P = C ? XPal[(C >> 1) & 0x03] : XPal[T[1] >> 6]; cnt--; P++; }
				if (cnt) { C = *R; *P = C ? XPal[(C >> 1) & 0x03] : XPal[(T[1] >> 4) & 0x03]; cnt--; P++; R++; }
				if (cnt) { C = *R; *P = C ? XPal[(C >> 1) & 0x03] : XPal[(T[1] >> 2) & 0x03]; cnt--; P++; }
				if (cnt) { C = *R; *P = C ? XPal[(C >> 1) & 0x03] : XPal[T[1] & 0x03]; cnt--; P++; R++; }
				if (cnt) { C = *R; *P = C ? XPal[(C >> 1) & 0x03] : XPal[T[2] >> 6]; cnt--; P++; }
				if (cnt) { C = *R; *P = C ? XPal[(C >> 1) & 0x03] : XPal[(T[2] >> 4) & 0x03]; cnt--; P++; R++; }
				if (cnt) { C = *R; *P = C ? XPal[(C >> 1) & 0x03] : XPal[(T[2] >> 2) & 0x03]; cnt--; P++; }
				if (cnt) { C = *R; *P = C ? XPal[(C >> 1) & 0x03] : XPal[T[2] & 0x03]; cnt--; P++; R++; }
				if (cnt) { C = *R; *P = C ? XPal[(C >> 1) & 0x03] : XPal[T[3] >> 6]; cnt--; P++; }
				if (cnt) { C = *R; *P = C ? XPal[(C >> 1) & 0x03] : XPal[(T[3] >> 4) & 0x03]; cnt--; P++; R++; }
				if (cnt) { C = *R; *P = C ? XPal[(C >> 1) & 0x03] : XPal[(T[3] >> 2) & 0x03]; cnt--; P++; }
				if (cnt) { C = *R; *P = C ? XPal[(C >> 1) & 0x03] : XPal[T[3] & 0x03]; cnt--; P++; R++; }
			}
        }
    }
}


/** RefreshLine7() *******************************************/
/** Refresh line Y (0..191/211) of SCREEN7, including       **/
/** sprites in this line.                                   **/
/*************************************************************/
void RefreshLine7(register byte Y)
{
    if (!IsWide)
    {
        //register pixel* P;
        pixel* P;
        u32 sprDraw;
        register byte C, * T, * T2, * R, X, XE;
        int cnt, SL, SR;
        byte ZBuf[320];
        P = RefreshBorder(Y, XPal[BGColor]);
        if (!P) return;
        SR = VDP[27] & 0x07;
        SL = VDP[26];

        if (!ScreenON) ClearLine(P, XPal[BGColor]);
        else
        {
            sprDraw = ColorSpritesScr78(Y, ZBuf);
            R = ZBuf + 32 + SR;
            T = ChrTab + ((((int)(Y + VScroll) << 8) & ChrTabM & 0xFFFF) >> 1);
            if (UseInterlace && InterlaceON && FlipEvenOdd && (VDPStatus[2] & 0x02))T -= 0x8000;
            T2 = T + 0x10000;

			if (SR == 0)
			{
				XE = 32;
			}
			else
			{
				XE = 31;
				for (X = 0; X < SR; X++)
				{
					*P = XPal[BGColor]; P++;
				}
			}
            for (X = 0; X < XE; X++, R += 8, P += 8, T += 4, T2 += 4)
            {
                if (!(sprDraw & (1 << X)))
                {
                    P[0] = ColAve(XPal[T[0] >> 4], XPal[T[0] & 0xF]);
                    P[1] = ColAve(XPal[T2[0] >> 4], XPal[T2[0] & 0xF]);
                    P[2] = ColAve(XPal[T[1] >> 4], XPal[T[1] & 0xF]);
                    P[3] = ColAve(XPal[T2[1] >> 4], XPal[T2[1] & 0xF]);
                    P[4] = ColAve(XPal[T[2] >> 4], XPal[T[2] & 0xF]);
                    P[5] = ColAve(XPal[T2[2] >> 4], XPal[T2[2] & 0xF]);
                    P[6] = ColAve(XPal[T[3] >> 4], XPal[T[3] & 0xF]);
                    P[7] = ColAve(XPal[T2[3] >> 4], XPal[T2[3] & 0xF]);
                    continue;
                }
                C = R[0]; P[0] = C ? XPal[C >> 1] : ColAve(XPal[T[0] >> 4], XPal[T[0] & 0xF]);
                C = R[1]; P[1] = C ? XPal[C >> 1] : ColAve(XPal[T2[0] >> 4], XPal[T2[0] & 0xF]);
                C = R[2]; P[2] = C ? XPal[C >> 1] : ColAve(XPal[T[1] >> 4], XPal[T[1] & 0xF]);
                C = R[3]; P[3] = C ? XPal[C >> 1] : ColAve(XPal[T2[1] >> 4], XPal[T2[1] & 0xF]);
                C = R[4]; P[4] = C ? XPal[C >> 1] : ColAve(XPal[T[2] >> 4], XPal[T[2] & 0xF]);
                C = R[5]; P[5] = C ? XPal[C >> 1] : ColAve(XPal[T2[2] >> 4], XPal[T2[2] & 0xF]);
                C = R[6]; P[6] = C ? XPal[C >> 1] : ColAve(XPal[T[3] >> 4], XPal[T[3] & 0xF]);
                C = R[7]; P[7] = C ? XPal[C >> 1] : ColAve(XPal[T2[3] >> 4], XPal[T2[3] & 0xF]);
            }
			if (SR != 0)
			{
				cnt = 8 - SR;
				if (cnt) { C = *R; *P = C ? XPal[C >> 1] : ColAve(XPal[T[0] >> 4], XPal[T[0] & 0xF]); cnt--; P++; R++; }
				if (cnt) { C = *R; *P = C ? XPal[C >> 1] : ColAve(XPal[T2[0] >> 4], XPal[T2[0] & 0xF]); cnt--; P++; R++; }
				if (cnt) { C = *R; *P = C ? XPal[C >> 1] : ColAve(XPal[T[1] >> 4], XPal[T[1] & 0xF]); cnt--; P++; R++; }
				if (cnt) { C = *R; *P = C ? XPal[C >> 1] : ColAve(XPal[T2[1] >> 4], XPal[T2[1] & 0xF]); cnt--; P++; R++; }
				if (cnt) { C = *R; *P = C ? XPal[C >> 1] : ColAve(XPal[T[2] >> 4], XPal[T[2] & 0xF]); cnt--; P++; R++; }
				if (cnt) { C = *R; *P = C ? XPal[C >> 1] : ColAve(XPal[T2[2] >> 4], XPal[T2[2] & 0xF]); cnt--; P++; R++; }
				if (cnt) { C = *R; *P = C ? XPal[C >> 1] : ColAve(XPal[T[3] >> 4], XPal[T[3] & 0xF]); cnt--; P++; R++; }
				if (cnt) { C = *R; *P = C ? XPal[C >> 1] : ColAve(XPal[T2[3] >> 4], XPal[T2[3] & 0xF]); cnt--; P++; R++; }
			}
        }
    }
    else
    {
        //register pixel* P;
        pixel* P;
        register u32* PL, sprDraw;
        register byte* T, * T2, * R;
        register byte C, X, XE, TA0, TA1, TA2, TA3, TA4, TA5, TA6, TA7, TB0, TB1, TB2, TB3, TB4, TB5, TB6, TB7;
        int cnt, SL ,SR;
        byte ZBuf[320];
        P = RefreshBorder512(Y, XPal[BGColor]);
        if (!P) return;
        SR = VDP[27] & 0x07;
        SL = VDP[26];

        if (!ScreenON) ClearLine512(P, XPal[BGColor]);
        else
        {
            sprDraw = ColorSpritesScr78(Y, ZBuf);
            R = ZBuf + 32;
            T = ChrTab + ((((int)(Y + VScroll) << 8) & ChrTabM & 0xFFFF) >> 1);
            if (UseInterlace && InterlaceON && FlipEvenOdd && (VDPStatus[2] & 0x02))T -= 0x8000;
            T2 = T + 0x10000;

			if (SR == 0)
			{
				XE = 32;
			}
			else
			{
				XE = 31;
				for (X = 0; X < SR; X++)
				{
					*P = XPal[BGColor]; P++;
				}
			}

            PL = (u32*)P;
            for (X = 0; X < XE; X++, R += 8, P += 16, T += 4, T2 += 4, PL += 8)
            {
                if (!(sprDraw & (1 << X)))
                {
                    TA0 = T[0]; TA1 = T[1]; TA2 = T[2]; TA3 = T[3];
                    TB0 = T2[0]; TB1 = T2[1]; TB2 = T2[2]; TB3 = T2[3];

                    PL[0] = XPal[TA0 >> 4] | XPal[TA0 & 0x0F] << 16;
                    PL[1] = XPal[TB0 >> 4] | XPal[TB0 & 0x0F] << 16;
                    PL[2] = XPal[TA1 >> 4] | XPal[TA1 & 0x0F] << 16;
                    PL[3] = XPal[TB1 >> 4] | XPal[TB1 & 0x0F] << 16;
                    PL[4] = XPal[TA2 >> 4] | XPal[TA2 & 0x0F] << 16;
                    PL[5] = XPal[TB2 >> 4] | XPal[TB2 & 0x0F] << 16;
                    PL[6] = XPal[TA3 >> 4] | XPal[TA3 & 0x0F] << 16;
                    PL[7] = XPal[TB3 >> 4] | XPal[TB3 & 0x0F] << 16;
                    continue;
                }
                TA0 = T[0]; TA1 = T[1]; TA2 = T[2]; TA3 = T[3];
                TB0 = T2[0]; TB1 = T2[1]; TB2 = T2[2]; TB3 = T2[3];
                C = R[0]; PL[0] = XPal[C ? C >> 1 : TA0 >> 4] | XPal[C ? C >> 1 : TA0 & 0x0F] << 16;
                C = R[1]; PL[1] = XPal[C ? C >> 1 : TB0 >> 4] | XPal[C ? C >> 1 : TB0 & 0x0F] << 16;
                C = R[2]; PL[2] = XPal[C ? C >> 1 : TA1 >> 4] | XPal[C ? C >> 1 : TA1 & 0x0F] << 16;
                C = R[3]; PL[3] = XPal[C ? C >> 1 : TB1 >> 4] | XPal[C ? C >> 1 : TB1 & 0x0F] << 16;
                C = R[4]; PL[4] = XPal[C ? C >> 1 : TA2 >> 4] | XPal[C ? C >> 1 : TA2 & 0x0F] << 16;
                C = R[5]; PL[5] = XPal[C ? C >> 1 : TB2 >> 4] | XPal[C ? C >> 1 : TB2 & 0x0F] << 16;
                C = R[6]; PL[6] = XPal[C ? C >> 1 : TA3 >> 4] | XPal[C ? C >> 1 : TA3 & 0x0F] << 16;
                C = R[7]; PL[7] = XPal[C ? C >> 1 : TB3 >> 4] | XPal[C ? C >> 1 : TB3 & 0x0F] << 16;
            }
			if (SR != 0)
			{
				cnt = 16 - SR;
				if (cnt) { C = *R; *P = C ? XPal[C >> 1] : XPal[T[0] >> 4]; cnt--; P++; }
				if (cnt) { C = *R; *P = C ? XPal[C >> 1] : XPal[T[0] & 0xF]; cnt--; P++; R++; }
				if (cnt) { C = *R; *P = C ? XPal[C >> 1] : XPal[T2[0] >> 4]; cnt--; P++; }
				if (cnt) { C = *R; *P = C ? XPal[C >> 1] : XPal[T2[0] & 0xF]; cnt--; P++; R++; }
				if (cnt) { C = *R; *P = C ? XPal[C >> 1] : XPal[T[1] >> 4]; cnt--; P++; }
				if (cnt) { C = *R; *P = C ? XPal[C >> 1] : XPal[T[1] & 0xF]; cnt--; P++; R++; }
				if (cnt) { C = *R; *P = C ? XPal[C >> 1] : XPal[T2[1] >> 4]; cnt--; P++; }
				if (cnt) { C = *R; *P = C ? XPal[C >> 1] : XPal[T2[1] & 0xF]; cnt--; P++; R++; }
				if (cnt) { C = *R; *P = C ? XPal[C >> 1] : XPal[T[2] >> 4]; cnt--; P++; }
				if (cnt) { C = *R; *P = C ? XPal[C >> 1] : XPal[T[2] & 0xF]; cnt--; P++; R++; }
				if (cnt) { C = *R; *P = C ? XPal[C >> 1] : XPal[T2[2] >> 4]; cnt--; P++; }
				if (cnt) { C = *R; *P = C ? XPal[C >> 1] : XPal[T2[2] & 0xF]; cnt--; P++; R++; }
				if (cnt) { C = *R; *P = C ? XPal[C >> 1] : XPal[T[3] >> 4]; cnt--; P++; }
				if (cnt) { C = *R; *P = C ? XPal[C >> 1] : XPal[T[3] & 0xF]; cnt--; P++; R++; }
				if (cnt) { C = *R; *P = C ? XPal[C >> 1] : XPal[T2[3] >> 4]; cnt--; P++; }
				if (cnt) { C = *R; *P = C ? XPal[C >> 1] : XPal[T2[3] & 0xF]; cnt--; P++; R++; }
			}
        }
    }
}


/** RefreshLineTx80() ****************************************/
/** Refresh line Y (0..191/211) of TEXT80.                  **/
/*************************************************************/
void RefreshLineTx80(register byte Y)
{
    if (!IsWide)
    {
        register pixel* P, FC, BC;
        register byte X, M, * T, * C, * G;
        register int i;
        int SR, SL;

        SR = VDP[27] & 0x07;
        SL = VDP[26];

        BC = XPal[BGColor];
        P = RefreshBorder(Y, BC);
        if (!P) return;

        if (!ScreenON) ClearLine(P, BC);
        else
        {
            for (i = 0; i < 8 + SR; i++)
            {
                *P++ = BC;
            }
            G = ChrGen + ((Y + VScroll) & 0x07);
            T = ChrTab + ((80 * (Y >> 3)) & ChrTabM);
            C = ColTab + ((10 * (Y >> 3)) & ColTabM);
            P += 9;

            for (X = 0, M = 0x00; X < 80; X++, T++, P += 3)
            {
                if (!(X & 0x07)) M = *C++;
                if (M & 0x80) { FC = XPal[XFGColor]; BC = XPal[XBGColor]; }
                else { FC = XPal[FGColor]; BC = XPal[BGColor]; }
                M <<= 1;
                Y = *(G + ((int)*T << 3));
                P[0] = Y & 0xC0 ? ColAve(Y & 0x80 ? FC : BC, Y & 0x40 ? FC : BC) : BC;
                P[1] = Y & 0x30 ? ColAve(Y & 0x20 ? FC : BC, Y & 0x10 ? FC : BC) : BC;
                P[2] = Y & 0x0C ? ColAve(Y & 0x08 ? FC : BC, Y & 0x04 ? FC : BC) : BC;
            }
            for (i = 0; i < 8 - SR; i++) {
                *P++ = XPal[BGColor];
            }
        }
    }
    else
    {
        register pixel* P, FC, BC;
        register byte X, M, * T, * C, * G;

        BC = XPal[BGColor];
        P = RefreshBorder512(Y, BC);
        if (!P) return;

        if (!ScreenON) ClearLine512(P, BC);
        else
        {
            //G = (FontBuf && (Mode & MSX_FIXEDFONT) ? FontBuf : ChrGen) + (Y & 0x07);
            G = (FontBuf && (Mode & MSX_FIXEDFONT) ? FontBuf : ChrGen) + ((Y+VScroll) & 0x07);
            T = ChrTab + ((80 * (Y >> 3)) & ChrTabM);
            C = ColTab + ((10 * (Y >> 3)) & ColTabM);

            P[0] = P[1] = P[2] = P[3] = P[4] = P[5] = P[6] = P[7] = P[8] =
                P[9] = P[10] = P[11] = P[12] = P[13] = P[14] = P[15] =
                P[16] = P[17] = XPal[BGColor];
            P += 18;

            for (X = M = 0; X < 80; X++, T++, P += 6)
            {
                if (!(X & 0x07)) M = *C++;
                if (M & 0x80) { FC = XPal[XFGColor]; BC = XPal[XBGColor]; }
                else { FC = XPal[FGColor]; BC = XPal[BGColor]; }
                M <<= 1;
                Y = *(G + ((int)*T << 3));
                P[0] = Y & 0x80 ? FC : BC;
                P[1] = Y & 0x40 ? FC : BC;
                P[2] = Y & 0x20 ? FC : BC;
                P[3] = Y & 0x10 ? FC : BC;
                P[4] = Y & 0x08 ? FC : BC;
                P[5] = Y & 0x04 ? FC : BC;
            }

            P[0] = P[1] = P[2] = P[3] = P[4] = P[5] = P[6] = P[7] = P[8] =
                P[9] = P[10] = P[11] = P[12] = P[13] = XPal[BGColor];
        }
    }
}


/** ClearLine512() *******************************************/
/** Clear 512 pixels from P with color C.                   **/
/*************************************************************/
static void ClearLine512(register pixel* P, register pixel C)
{
    register int J;

    for (J = 0; J < 512; J++) P[J] = C;
}


/** RefreshBorder512() ***************************************/
/** This function is called from RefreshLine#() to refresh  **/
/** the screen border. It returns a pointer to the start of **/
/** scanline Y in XBuf or 0 if scanline is beyond XBuf.     **/
/*************************************************************/
pixel* RefreshBorder512(register byte Y, register pixel C)
{
    register pixel* P;
    register int H;

    /* First line number in the buffer */
    if (!Y) FirstLine = (ScanLines212 ? 8 : 18) + VAdjust;

    /* Return 0 if we've run out of the screen buffer due to overscan */
    if (Y + FirstLine >= HEIGHT) return(0);

    /* Set up the transparent color */
    XPal[0] = (!BGColor || SolidColor0) ? XPal0 : XPal[BGColor];

    /* Start of the buffer */
    P = (pixel*)WBuf;

    /* Paint top of the screen */
    if (!Y) for (H = 2 * WIDTH * FirstLine - 1; H >= 0; H--) P[H] = C;

    /* Start of the line */
    P += 2 * WIDTH * (FirstLine + Y);

    /* Paint left/right borders */
    //for (H = (WIDTH - 256) + 2 * HAdjust; H > 0; H--) P[H - 1] = C;
    //for (H = (WIDTH - 256) - 2 * HAdjust; H > 0; H--) P[2 * WIDTH - H] = C;
    for (H = (WIDTH - 256) + 2 * HAdjust; H > 0; H--) P[H - 1 - 2*WIDTH] = C;
    for (H = (WIDTH - 256) - 2 * HAdjust; H > 0; H--) P[- H] = C;

    /* Paint bottom of the screen */
    H = ScanLines212 ? 212 : 192;
    //if (Y == H - 1) for (H = 2 * WIDTH * (HEIGHT - H - FirstLine + 1) - 2; H >= 2 * WIDTH; H--) P[H] = C;
    //if (Y == H - 1) for (H = 2 * WIDTH * (HEIGHT - H - FirstLine + 1) - 1; H >= 2 * WIDTH; H--) P[H] = C;
    if (Y == H - 1) for (H = 2 * WIDTH * (HEIGHT - H - FirstLine + 1) - 1; H >= 2 * WIDTH; H--) P[H - 2*WIDTH] = C;

    /* Return pointer to the scanline in XBuf */
    return(P + WIDTH - 256 + 2 * HAdjust);
}
