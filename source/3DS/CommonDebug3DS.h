
/* CommonDebug3DS.h */
/* Show VRAM in the debugger. */

void DrawVRAM0(register int Y, register int DY)
{
	register unsigned short* P, FC, BC;
	register byte* T, * G, X;
	BC = XPal[BGColor];
	G = ChrGen + (Y & 0x07);
	P = (unsigned short*)XBuf + (WIDTH * DY);
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
}


void DrawVRAM1(register int Y, register int DY)
{
	register unsigned short * P, FC, BC;
	register byte K, X, * T, * G;
	P = (unsigned short*)XBuf + (WIDTH * DY);
	G = ChrGen + (Y & 0x07);
	T = ChrTab + ((int)(Y & 0xF8) << 2);
	for (X = 0; X < 32; X++, T++)
	{
		K = ColTab[*T >> 3];
		FC = XPal[K >> 4];
		BC = XPal[K & 0x0F];
		K = G[(int)*T << 3];

		*P++ = (K & 0x80) ? FC : BC;
		*P++ = (K & 0x40) ? FC : BC;
		*P++ = (K & 0x20) ? FC : BC;
		*P++ = (K & 0x10) ? FC : BC;
		*P++ = (K & 0x08) ? FC : BC;
		*P++ = (K & 0x04) ? FC : BC;
		*P++ = (K & 0x02) ? FC : BC;
		*P++ = (K & 0x01) ? FC : BC;
	}
}


void DrawVRAM2(register int Y, register int DY)
{
	register unsigned short* P, FC, BC;
	u32* PL;
	register byte X, K, * T;
	register int I, J;
	P = (unsigned short*)XBuf + (WIDTH * DY);
	//T = VRAM + (((int)(VDP[2] & 0x7F) << 10)+ ((int)(Y & 0xF8) << 2));
	T = ChrTab + ((int)(Y & 0xF8) << 2);
	I = ((int)(Y & 0xC0) << 5) + (Y & 0x07);
	PL = (u32*)P;
	for (X = 0; X < 32; X++, P += 8, PL += 4, T++)
	{
		J = (int)*T << 3;
		K = ColTab[(I + J) & ColTabM];
		FC = XPal[K >> 4];
		BC = XPal[K & 0x0F];
		K = ChrGen[(I + J) & ChrGenM];
		PL[0] = ((K & 0x80) ? FC : BC) | (((K & 0x40) ? FC : BC) << 16);
		PL[1] = ((K & 0x20) ? FC : BC) | (((K & 0x10) ? FC : BC) << 16);
		PL[2] = ((K & 0x08) ? FC : BC) | (((K & 0x04) ? FC : BC) << 16);
		PL[3] = ((K & 0x02) ? FC : BC) | (((K & 0x01) ? FC : BC) << 16);
	}

}


void DrawVRAM3(register int Y, register int DY)
{
	register unsigned short* P;
	register byte X, XE, C, K, * T, * G, * R;
	P = (unsigned short*)XBuf + (WIDTH * DY);
	T = ChrTab + ((int)(Y & 0xF8) << 2);
	G = ChrGen + ((Y & 0x1C) >> 2);
	for (X = 0; X < 32; X++, T++, P += 8, R += 8)
	{
		K = G[(int)*T << 3];
		P[0] = XPal[K >> 4];
		P[1] = XPal[K >> 4];
		P[2] = XPal[K >> 4];
		P[3] = XPal[K >> 4];
		P[4] = XPal[K & 0x0F];
		P[5] = XPal[K & 0x0F];
		P[6] = XPal[K & 0x0F];
		P[7] = XPal[K & 0x0F];
	}
}


void DrawVRAM5(register int Y, register int DY)
{
	register unsigned short* P;
	u32* PL;
	register byte X, * T;
	P = (unsigned short*)XBuf + (WIDTH * DY);
	T = VRAM + ((int)Y << 7);
	PL = (u32*)P;
	for (X = 0; X < 32; X++, P += 8, T += 4, PL += 4)
	{
		PL[0] = (XPal[T[0] >> 4]) | (XPal[T[0] & 0x0F]) << 16;
		PL[1] = (XPal[T[1] >> 4]) | (XPal[T[1] & 0x0F]) << 16;
		PL[2] = (XPal[T[2] >> 4]) | (XPal[T[2] & 0x0F]) << 16;
		PL[3] = (XPal[T[3] >> 4]) | (XPal[T[3] & 0x0F]) << 16;
	}
}


void DrawVRAM6(register int Y, register int DY)
{
	if (!IsWide)
	{
		register unsigned short* P;
		register u32* PL;
		register byte X, * T, TA0, TA1, TA2, TA3;
		P = (unsigned short*)XBuf + (WIDTH * DY);
		T = VRAM + ((int)Y << 7);
		PL = (u32*)P;
		for (X = 0; X < 32; X++, P += 8, PL += 4, T += 4)
		{
			P[0] = ColAve(XPal[T[0] >> 6], XPal[(T[0] >> 4) & 0x03]);
			P[1] = ColAve(XPal[(T[0] >> 2) & 0x03], XPal[T[0] & 0x03]);
			P[2] = ColAve(XPal[T[1] >> 6], XPal[(T[1] >> 4) & 0x03]);
			P[3] = ColAve(XPal[(T[1] >> 2) & 0x03], XPal[T[1] & 0x03]);
			P[4] = ColAve(XPal[T[2] >> 6], XPal[(T[2] >> 4) & 0x03]);
			P[5] = ColAve(XPal[(T[2] >> 2) & 0x03], XPal[T[2] & 0x03]);
			P[6] = ColAve(XPal[T[3] >> 6], XPal[(T[3] >> 4) & 0x03]);
			P[7] = ColAve(XPal[(T[3] >> 2) & 0x03], XPal[T[3] & 0x03]);
		}
	}
	else
	{
		register unsigned short* P;
		register u32* PL;
		register byte X, * T, TA0, TA1, TA2, TA3;
		P = (unsigned short*)WBuf + (2 * WIDTH * DY);
		T = VRAM + ((int)Y << 7);
		PL = (u32*)P;
		for (X = 0; X < 32; X++, P += 16, PL += 8, T += 4)
		{
			TA0 = T[0];
			TA1 = T[1];
			TA2 = T[2];
			TA3 = T[3];
			PL[0] = (XPal[TA0 >> 6]) | (XPal[(TA0 >> 4) & 0x03]) << 16;
			PL[1] = (XPal[(TA0 >> 2) & 0x03]) | (XPal[TA0 & 0x03]) << 16;
			PL[2] = (XPal[TA1 >> 6]) | (XPal[(TA1 >> 4) & 0x03]) << 16;
			PL[3] = (XPal[(TA1 >> 2) & 0x03]) | (XPal[TA1 & 0x03]) << 16;
			PL[4] = (XPal[TA2 >> 6]) | (XPal[(TA2 >> 4) & 0x03]) << 16;
			PL[5] = (XPal[(TA2 >> 2) & 0x03]) | (XPal[TA2 & 0x03]) << 16;
			PL[6] = (XPal[TA3 >> 6]) | (XPal[(TA3 >> 4) & 0x03]) << 16;
			PL[7] = (XPal[(TA3 >> 2) & 0x03]) | (XPal[TA3 & 0x03]) << 16;
		}
	}
}


void DrawVRAM7(register int Y, register int DY)
{
	if (!IsWide)
	{
		register unsigned short* P;
		register u32* PL;
		register byte * T, * T2, X;
		P = (unsigned short*)XBuf + (WIDTH * DY);
		T = VRAM + (((int)Y << 8) >> 1);
		T2 = T + 0x10000;
		PL = (u32*)P;
		for (X = 0; X < 32; X++, P += 8, T += 4, T2 += 4)
		{
			P[0] = ColAve(XPal[T[0] >> 4], XPal[T[0] & 0xF]);
			P[1] = ColAve(XPal[T2[0] >> 4], XPal[T2[0] & 0xF]);
			P[2] = ColAve(XPal[T[1] >> 4], XPal[T[1] & 0xF]);
			P[3] = ColAve(XPal[T2[1] >> 4], XPal[T2[1] & 0xF]);
			P[4] = ColAve(XPal[T[2] >> 4], XPal[T[2] & 0xF]);
			P[5] = ColAve(XPal[T2[2] >> 4], XPal[T2[2] & 0xF]);
			P[6] = ColAve(XPal[T[3] >> 4], XPal[T[3] & 0xF]);
			P[7] = ColAve(XPal[T2[3] >> 4], XPal[T2[3] & 0xF]);
		}
	}
	else
	{
		register unsigned short* P;
		register u32* PL;
		register byte* T, * T2;
		register byte X, TA0, TA1, TA2, TA3, TA4, TA5, TA6, TA7, TB0, TB1, TB2, TB3, TB4, TB5, TB6, TB7;
		P = (unsigned short*)WBuf + (2 * WIDTH * DY);
		T = VRAM + (((int)Y << 8) >> 1);
		T2 = T + 0x10000;
		PL = (u32*)P;
		for (X = 0; X < 32; X++, P += 16, T += 4, T2 += 4, PL += 8)
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
		}
	}
}


void DrawVRAM8(register int Y, register int DY)
{
	register unsigned short* P;
	u32* PL;
	register byte X, *T, *T2;
	P = (unsigned short*)XBuf + (WIDTH * DY);
	T = VRAM + (((int)Y << 8)>>1);
	T2 = T + 0x10000;
	PL = (u32*)P;
	for (X = 0; X < 32; X++, T += 4, T2 += 4, P += 8, PL += 4)
	{
		PL[0] = (BPal[T[0]]) | (BPal[T2[0]] << 16);
		PL[1] = (BPal[T[1]]) | (BPal[T2[1]] << 16);
		PL[2] = (BPal[T[2]]) | (BPal[T2[2]] << 16);
		PL[3] = (BPal[T[3]]) | (BPal[T2[3]] << 16);
	}
}


void DrawVRAM10(register int Y, register int DY)
{
	register unsigned short* P;
	u32* PL;
	register byte X, * T, Y1;
	register int J, K;
	P = (unsigned short*)XBuf + (WIDTH * DY);
	PL = (u32*)P;
	T = VRAM + (((int)Y << 8) >> 1);
	for (X = 0; X < 64; X++, T += 2, P += 4, PL += 2)
	{
		K = (T[0] & 0x07) | ((T[0x10000] & 0x07) << 3);
		if (K & 0x20) K -= 64;
		J = (T[1] & 0x07) | ((T[0x10001] & 0x07) << 3);
		if (J & 0x20) J -= 64;
		Y = T[0] >> 3; Y1 = T[0x10000] >> 3; PL[0] = (Y & 1 ? XPal[Y >> 1] : YAEColor(Y, J, K)) | (Y1 & 1 ? XPal[Y1 >> 1] : YAEColor(Y1, J, K)) << 16;
		Y = T[1] >> 3; Y1 = T[0x10001] >> 3; PL[1] = (Y & 1 ? XPal[Y >> 1] : YAEColor(Y, J, K)) | (Y1 & 1 ? XPal[Y1 >> 1] : YAEColor(Y1, J, K)) << 16;
	}
}


void DrawVRAM12(register int Y, register int DY)
{
	register unsigned short* P;
	u32* PL;
	register byte C, X, * T, * T2, TA0, TA1, TB0, TB1;
	int J, J2, K, K2, cnt;
	P = (unsigned short*)XBuf + (WIDTH * DY);
	PL = (u32*)P;
	T = VRAM + (((int)Y << 8) >> 1);
	T2 = T + 0x010000;
	for (X = 0; X < 64; X++, T += 2, T2 += 2, P += 4, PL += 2)
	{
		TA0 = T[0]; TA1 = T[1];
		TB0 = T2[0]; TB1 = T2[1];
		K = (TA0 & 0x07) | ((TB0 & 0x07) << 3);
		if (K & 0x20) K -= 64;
		J = (TA1 & 0x07) | ((TB1 & 0x07) << 3);
		if (J & 0x20) J -= 64;
		PL[0] = (YJKColor(TA0 >> 3, J, K)) | (YJKColor(TB0 >> 3, J, K)) << 16;
		PL[1] = (YJKColor(TA1 >> 3, J, K)) | (YJKColor(TB1 >> 3, J, K)) << 16;
	}
}