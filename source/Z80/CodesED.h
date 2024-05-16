/** Z80: portable Z80 emulator *******************************/
/**                                                         **/
/**                         CodesED.h                       **/
/**                                                         **/
/** This file contains implementation for the ED table of   **/
/** Z80 commands. It is included from Z80.c.                **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1994-2020                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/

/** NINTENDO 3DS Version modified by h.tomioka 2023         **/

/** This is a special patch for emulating BIOS calls: ********/
case DB_FE:     PatchZ80(R);break;
#ifdef TURBO_R
case DB_E0: PatchR800(R, 0xE0);break;
case DB_E2: PatchR800(R, 0xE2);break;
case DB_E4: PatchR800(R, 0xE4);break;
case DB_E5: PatchR800(R, 0xE5);break;
case DB_E6: PatchR800(R, 0xE6);break;
case DB_E7: PatchR800(R, 0xE7);break;
case DB_E8: PatchR800(R, 0xE8);break;
case DB_EA: PatchR800(R, 0xEA);break;
#endif // TURBO_R

/*************************************************************/

case ADC_HL_BC: M_ADCW(BC);break;
case ADC_HL_DE: M_ADCW(DE);break;
case ADC_HL_HL: M_ADCW(HL);break;
case ADC_HL_SP: M_ADCW(SP);break;

case SBC_HL_BC: M_SBCW(BC);break;
case SBC_HL_DE: M_SBCW(DE);break;
case SBC_HL_HL: M_SBCW(HL);break;
case SBC_HL_SP: M_SBCW(SP);break;

case LD_xWORDe_HL:
  J.B.l=OpZ80(R->PC.W++);
  J.B.h=OpZ80(R->PC.W++);
  WrZ80(J.W++,R->HL.B.l);
  WrZ80(J.W,R->HL.B.h);
  break;
case LD_xWORDe_DE:
  J.B.l=OpZ80(R->PC.W++);
  J.B.h=OpZ80(R->PC.W++);
  WrZ80(J.W++,R->DE.B.l);
  WrZ80(J.W,R->DE.B.h);
  break;
case LD_xWORDe_BC:
  J.B.l=OpZ80(R->PC.W++);
  J.B.h=OpZ80(R->PC.W++);
  WrZ80(J.W++,R->BC.B.l);
  WrZ80(J.W,R->BC.B.h);
  break;
case LD_xWORDe_SP:
  J.B.l=OpZ80(R->PC.W++);
  J.B.h=OpZ80(R->PC.W++);
  WrZ80(J.W++,R->SP.B.l);
  WrZ80(J.W,R->SP.B.h);
  break;

case LD_HL_xWORDe:
  J.B.l=OpZ80(R->PC.W++);
  J.B.h=OpZ80(R->PC.W++);
  R->HL.B.l=RdZ80(J.W++);
  R->HL.B.h=RdZ80(J.W);
  break;
case LD_DE_xWORDe:
  J.B.l=OpZ80(R->PC.W++);
  J.B.h=OpZ80(R->PC.W++);
  R->DE.B.l=RdZ80(J.W++);
  R->DE.B.h=RdZ80(J.W);
  break;
case LD_BC_xWORDe:
  J.B.l=OpZ80(R->PC.W++);
  J.B.h=OpZ80(R->PC.W++);
  R->BC.B.l=RdZ80(J.W++);
  R->BC.B.h=RdZ80(J.W);
  break;
case LD_SP_xWORDe:
  J.B.l=OpZ80(R->PC.W++);
  J.B.h=OpZ80(R->PC.W++);
  R->SP.B.l=RdZ80(J.W++);
  R->SP.B.h=RdZ80(J.W);
  break;

case RRD:
  I=RdZ80(R->HL.W);
  J.B.l=(I>>4)|(R->AF.B.h<<4);
  WrZ80(R->HL.W,J.B.l);
  R->AF.B.h=(I&0x0F)|(R->AF.B.h&0xF0);
  R->AF.B.l=PZSTable[R->AF.B.h]|(R->AF.B.l&C_FLAG);
  break;
case RLD:
  I=RdZ80(R->HL.W);
  J.B.l=(I<<4)|(R->AF.B.h&0x0F);
  WrZ80(R->HL.W,J.B.l);
  R->AF.B.h=(I>>4)|(R->AF.B.h&0xF0);
  R->AF.B.l=PZSTable[R->AF.B.h]|(R->AF.B.l&C_FLAG);
  break;

case LD_A_I:
  R->AF.B.h=R->I;
  R->AF.B.l=(R->AF.B.l&C_FLAG)|(R->IFF&IFF_2? P_FLAG:0)|ZSTable[R->AF.B.h];
  break;

case LD_A_R:
  R->AF.B.h=R->R;
  R->AF.B.l=(R->AF.B.l&C_FLAG)|(R->IFF&IFF_2? P_FLAG:0)|ZSTable[R->AF.B.h];
  break;

case LD_I_A:   R->I=R->AF.B.h;break;
case LD_R_A:   R->R=R->AF.B.h;break;

case IM_0:     R->IFF&=~(IFF_IM1|IFF_IM2);break;
case IM_1:     R->IFF=(R->IFF&~IFF_IM2)|IFF_IM1;break;
case IM_2:     R->IFF=(R->IFF&~IFF_IM1)|IFF_IM2;break;

case RETI:
case RETN:     if(R->IFF&IFF_2) R->IFF|=IFF_1; else R->IFF&=~IFF_1;
               M_RET;break;

case NEG:      I=R->AF.B.h;R->AF.B.h=0;M_SUB(I);break;

case IN_B_xC:  M_IN(R->BC.B.h);break;
case IN_C_xC:  M_IN(R->BC.B.l);break;
case IN_D_xC:  M_IN(R->DE.B.h);break;
case IN_E_xC:  M_IN(R->DE.B.l);break;
case IN_H_xC:  M_IN(R->HL.B.h);break;
case IN_L_xC:  M_IN(R->HL.B.l);break;
case IN_A_xC:  M_IN(R->AF.B.h);break;
case IN_F_xC:  M_IN(J.B.l);break;

case OUT_xC_B: OutZ80(R->BC.W,R->BC.B.h);break;
case OUT_xC_C: OutZ80(R->BC.W,R->BC.B.l);break;
case OUT_xC_D: OutZ80(R->BC.W,R->DE.B.h);break;
case OUT_xC_E: OutZ80(R->BC.W,R->DE.B.l);break;
case OUT_xC_H: OutZ80(R->BC.W,R->HL.B.h);break;
case OUT_xC_L: OutZ80(R->BC.W,R->HL.B.l);break;
case OUT_xC_A: OutZ80(R->BC.W,R->AF.B.h);break;
case OUT_xC_F: OutZ80(R->BC.W,0);break;

case INI:
#ifdef TURBO_R
   --R->BC.B.h;
    I = InZ80(R->BC.W);
    WrZ80(R->HL.W++, I);
  if (R->User&0x01)
  {
      R->AF.B.l = (R->AF.B.l & ~Z_FLAG) | (R->BC.B.h ? 0 : Z_FLAG) | N_FLAG;
  }
  else
  {
      K = ((R->BC.B.l + 1) & 0xFF) + I;
      R->AF.B.l = ZSTable[R->BC.B.h] | ((I>>6)&N_FLAG) | (K > 255 ? (C_FLAG | H_FLAG) : 0) | (PZSTable[(K&0x07) ^ (R->BC.B.h)] & P_FLAG);
  }
#else
  WrZ80(R->HL.W++, InZ80(R->BC.W));
  --R->BC.B.h;
  R->AF.B.l=N_FLAG|(R->BC.B.h? 0:Z_FLAG);
#endif // TURBO_R
  break;

case INIR:
#ifdef TURBO_R
    --R->BC.B.h;
    I = InZ80(R->BC.W);
    WrZ80(R->HL.W++, I);
    if (R->User & 0x01)
    {
        if (R->BC.B.h) { R->AF.B.l = (R->AF.B.l & ~Z_FLAG) | N_FLAG; R->ICount -= 12; R->PC.W -= 2; }
        else { R->AF.B.l = (R->AF.B.l & ~Z_FLAG) | Z_FLAG | N_FLAG; R->ICount -= 11; }
    }
    else
    {
        K = ((R->BC.B.l + 1) & 0xFF) + I;
        R->AF.B.l = ZSTable[R->BC.B.h] | ((I >> 6) & N_FLAG) | (K > 255 ?(C_FLAG | H_FLAG) : 0) | (PZSTable[(K&0x07) ^ (R->BC.B.h)] & P_FLAG);
        if (R->BC.B.h) { R->ICount -= 21; R->PC.W -= 2; }
        else { R->ICount -= 16; }
    }
#else
  WrZ80(R->HL.W++, InZ80(R->BC.W));
  if(--R->BC.B.h) { R->AF.B.l=N_FLAG;R->ICount-=21;R->PC.W-=2; }
  else            { R->AF.B.l=Z_FLAG|N_FLAG;R->ICount-=16; }
#endif // TURBO_R
  break;

case IND:
#ifdef TURBO_R
    --R->BC.B.h;
    I = InZ80(R->BC.W);
    WrZ80(R->HL.W--, I);
  if (R->User & 0x01)
  {
      R->AF.B.l = (R->AF.B.l & ~Z_FLAG) | (R->BC.B.h ? 0 : Z_FLAG) | N_FLAG;
  }
  else
  {
      K = ((R->BC.B.l - 1) & 0xFF) + I;
      R->AF.B.l = ZSTable[R->BC.B.h] | ((I >> 6) & N_FLAG) | (K > 255 ? (C_FLAG | H_FLAG) : 0) | (PZSTable[(K & 0x07) ^ (R->BC.B.h)] & P_FLAG);
  }
#else
    WrZ80(R->HL.W--, InZ80(R->BC.W));
    --R->BC.B.h;
    R->AF.B.l=N_FLAG|(R->BC.B.h? 0:Z_FLAG);
#endif // TURBO_R
  break;

case INDR:
#ifdef TURBO_R
    --R->BC.B.h;
    I = InZ80(R->BC.W);
    WrZ80(R->HL.W--, I);
    if (R->User & 0x01)
    {
        if (R->BC.B.h) { R->AF.B.l = (R->AF.B.l & ~Z_FLAG) | N_FLAG; R->ICount -= 12; R->PC.W -= 2; }
        else { R->AF.B.l = (R->AF.B.l & ~Z_FLAG) | Z_FLAG | N_FLAG; R->ICount -= 11; }
    }
    else
    {
        K = ((R->BC.B.l - 1) & 0xFF) + I;
        R->AF.B.l = ZSTable[R->BC.B.h] | ((I >> 6) & N_FLAG) | (K> 255 ? (C_FLAG | H_FLAG) : 0) | (PZSTable[(K & 0x07) ^ (R->BC.B.h)] & P_FLAG);
        if (R->BC.B.h) { R->ICount -= 21; R->PC.W -= 2; }
        else { R->ICount -= 16; }
    }
#else
  WrZ80(R->HL.W--, InZ80(R->BC.W));
  if(!--R->BC.B.h) { R->AF.B.l=N_FLAG;R->ICount-=21;R->PC.W-=2; }
  else             { R->AF.B.l=Z_FLAG|N_FLAG;R->ICount-=16; }
#endif // TURBO_R
  break;

case OUTI:
#ifdef TURBO_R
  I = RdZ80(R->HL.W++);
  OutZ80(R->BC.W, I);
  --R->BC.B.h;
  if (R->User & 0x01)
  {
      R->AF.B.l = (R->AF.B.l & ~Z_FLAG) | (R->BC.B.h ? 0 : Z_FLAG) | N_FLAG;
  }
  else
  {
      K = R->HL.B.l + I;
      R->AF.B.l = ZSTable[R->BC.B.h] | ((I >> 6) & N_FLAG) | (K > 255 ? (C_FLAG | H_FLAG) : 0) | (PZSTable[(K & 0x07) ^ (R->BC.B.h)] & P_FLAG);
  }
#else
  --R->BC.B.h;
  I = RdZ80(R->HL.W++);
  OutZ80(R->BC.W, I);
#ifdef _3DS
  // Taken from FreeMSX/FreeM)
  // ( https://web.archive.org/web/20030708121854/http://nemoto.tri6.net/download/msxemu_tech.html )
  R->AF.B.l = (R->AF.B.l & S_FLAG) | N_FLAG | (R->BC.B.h ? 0 : Z_FLAG) | (R->HL.B.l + I > 255 ? (C_FLAG | H_FLAG) : 0);
#else
  R->AF.B.l=N_FLAG|(R->BC.B.h? 0:Z_FLAG)|(R->HL.B.l+I>255? (C_FLAG|H_FLAG):0);
#endif
#endif // TURBO_R
  break;

case OTIR:
#ifdef TURBO_R
  I = RdZ80(R->HL.W++);
  OutZ80(R->BC.W, I);
  --R->BC.B.h;
  /* Flags are same as OUTI */
  if (R->User & 0x01)
  {
      if (R->BC.B.h){ R->AF.B.l = (R->AF.B.l & ~Z_FLAG) | N_FLAG; R->ICount -= 12; R->PC.W -= 2;}
      else { R->AF.B.l = (R->AF.B.l & ~Z_FLAG) | Z_FLAG | N_FLAG; R->ICount -= 11;}
  }
  else
  {
      K = R->HL.B.l + I;
      R->AF.B.l = ZSTable[R->BC.B.h] | ((I >> 6) & N_FLAG) | (K > 255 ? (C_FLAG | H_FLAG) : 0) | (PZSTable[(K & 0x07) ^ (R->BC.B.h)] & P_FLAG);
      if (R->BC.B.h) { R->ICount -= 21; R->PC.W -= 2; }
      else { R->ICount -= 16; }
  }
#else
  --R->BC.B.h;
  I = RdZ80(R->HL.W++);
  OutZ80(R->BC.W, I);
  if (R->BC.B.h)
  {
      R->AF.B.l = N_FLAG | (R->HL.B.l + I > 255 ? (C_FLAG | H_FLAG) : 0);
      R->ICount -= 21;
      R->PC.W -= 2;
  }
  else
  {
      R->AF.B.l = Z_FLAG | N_FLAG | (R->HL.B.l + I > 255 ? (C_FLAG | H_FLAG) : 0);
      R->ICount -= 16;
  }
#endif // TURBO_R
  break;

case OUTD:
#ifdef TURBO_R
  I = RdZ80(R->HL.W--);
  OutZ80(R->BC.W, I);
  --R->BC.B.h;
  /* Flags are same as OUTI */
  if (R->User & 0x01)
  {
      R->AF.B.l = (R->AF.B.l & ~Z_FLAG) | (R->BC.B.h ? 0 : Z_FLAG) | N_FLAG;
  }
  else
  {
      K = R->HL.B.l + I;
      R->AF.B.l = ZSTable[R->BC.B.h] | ((I >> 6) & N_FLAG) | (K > 255 ? (C_FLAG | H_FLAG) : 0) | (PZSTable[(K & 0x07) ^ (R->BC.B.h)] & P_FLAG);
  }
#else
  --R->BC.B.h;
  I = RdZ80(R->HL.W--);
  OutZ80(R->BC.W, I);
  R->AF.B.l=N_FLAG|(R->BC.B.h? 0:Z_FLAG)|(R->HL.B.l+I>255? (C_FLAG|H_FLAG):0);
#endif // TURBO_R
  break;

case OTDR:
#ifdef TURBO_R
  I = RdZ80(R->HL.W--);
  OutZ80(R->BC.W, I);
  --R->BC.B.h;
  /* Flags are same as OUTI */
  if (R->User & 0x01)
  {
      if (R->BC.B.h){ R->AF.B.l = (R->AF.B.l & ~Z_FLAG) | N_FLAG; R->ICount -= 12; R->PC.W -= 2;}
      else { R->AF.B.l = (R->AF.B.l & ~Z_FLAG) | Z_FLAG | N_FLAG; R->ICount -= 11;}
  }
  else
  {
      K = R->HL.B.l + I;
      R->AF.B.l = ZSTable[R->BC.B.h] | ((I >> 6) & N_FLAG) | (K > 255 ? (C_FLAG | H_FLAG) : 0) | (PZSTable[(K & 0x07) ^ (R->BC.B.h)] & P_FLAG);
      if (R->BC.B.h) { R->ICount -= 21; R->PC.W -= 2; }
      else { R->ICount -= 16; }
  }
#else
  --R->BC.B.h;
  I = RdZ80(R->HL.W--);
  OutZ80(R->BC.W, I);
  if (R->BC.B.h)
  {
      R->AF.B.l = N_FLAG | (R->HL.B.l + I > 255 ? (C_FLAG | H_FLAG) : 0);
      R->ICount -= 21;
      R->PC.W -= 2;
  }
  else
  {
      R->AF.B.l = Z_FLAG | N_FLAG | (R->HL.B.l + I > 255 ? (C_FLAG | H_FLAG) : 0);
      R->ICount -= 16;
  }
#endif // TURBO_R
  break;

case LDI:
  WrZ80(R->DE.W++,RdZ80(R->HL.W++));
  --R->BC.W;
  R->AF.B.l=(R->AF.B.l&~(N_FLAG|H_FLAG|P_FLAG))|(R->BC.W? P_FLAG:0);
  break;

case LDIR:
  WrZ80(R->DE.W++,RdZ80(R->HL.W++));
  if(--R->BC.W)
  {
    R->AF.B.l=(R->AF.B.l&~(H_FLAG|P_FLAG))|N_FLAG;
#ifdef TURBO_R
    R->ICount -= (R->User & 0x01) ? 5 : 21;
#else
    R->ICount-=21;
#endif // TURBO_R
    R->PC.W-=2;
  }
  else
  {
    R->AF.B.l&=~(N_FLAG|H_FLAG|P_FLAG);
#ifdef TURBO_R
    R->ICount -= (R->User & 0x01) ? 4 : 16;
#else
    R->ICount-=16;
#endif // TURBO_R
  }
  break;

case LDD:
  WrZ80(R->DE.W--,RdZ80(R->HL.W--));
  --R->BC.W;
  R->AF.B.l=(R->AF.B.l&~(N_FLAG|H_FLAG|P_FLAG))|(R->BC.W? P_FLAG:0);
  break;

case LDDR:
  WrZ80(R->DE.W--,RdZ80(R->HL.W--));
  R->AF.B.l&=~(N_FLAG|H_FLAG|P_FLAG);
  if(--R->BC.W)
  {
    R->AF.B.l=(R->AF.B.l&~(H_FLAG|P_FLAG))|N_FLAG;
#ifdef TURBO_R
    R->ICount -= (R->User & 0x01) ? 5 : 21;
#else
    R->ICount-=21;
#endif // TURBO_R
    R->PC.W-=2;
  }
  else
  {
    R->AF.B.l&=~(N_FLAG|H_FLAG|P_FLAG);
#ifdef TURBO_R
    R->ICount -= (R->User & 0x01) ? 4 : 16;
#else
    R->ICount-=16;
#endif // TURBO_R
  }
  break;

case CPI:
  I=RdZ80(R->HL.W++);
  J.B.l=R->AF.B.h-I;
  --R->BC.W;
  R->AF.B.l =
    N_FLAG|(R->AF.B.l&C_FLAG)|ZSTable[J.B.l]|
    ((R->AF.B.h^I^J.B.l)&H_FLAG)|(R->BC.W? P_FLAG:0);
  break;

case CPIR:
  I=RdZ80(R->HL.W++);
  J.B.l=R->AF.B.h-I;
#ifdef TURBO_R
  if (--R->BC.W && J.B.l) { R->ICount -= (R->User & 0x01) ? 6 : 21; R->PC.W -= 2; }
  else R->ICount -= (R->User & 0x01) ? 5 : 16;
#else
  if(--R->BC.W&&J.B.l) { R->ICount-=21;R->PC.W-=2; } else R->ICount-=16;
#endif // TURBO_R
  R->AF.B.l =
    N_FLAG|(R->AF.B.l&C_FLAG)|ZSTable[J.B.l]|
    ((R->AF.B.h^I^J.B.l)&H_FLAG)|(R->BC.W? P_FLAG:0);
  break;  

case CPD:
  I=RdZ80(R->HL.W--);
  J.B.l=R->AF.B.h-I;
  --R->BC.W;
  R->AF.B.l =
    N_FLAG|(R->AF.B.l&C_FLAG)|ZSTable[J.B.l]|
    ((R->AF.B.h^I^J.B.l)&H_FLAG)|(R->BC.W? P_FLAG:0);
  break;

case CPDR:
  I=RdZ80(R->HL.W--);
  J.B.l=R->AF.B.h-I;
#ifdef TURBO_R
  if (--R->BC.W && J.B.l) { R->ICount -= (R->User & 0x01) ? 6 : 21; R->PC.W -= 2; }
  else R->ICount -= (R->User & 0x01) ? 5 : 16;
#else
  if(--R->BC.W&&J.B.l) { R->ICount-=21;R->PC.W-=2; } else R->ICount-=16;
#endif // TURBO_R
  R->AF.B.l =
    N_FLAG|(R->AF.B.l&C_FLAG)|ZSTable[J.B.l]|
    ((R->AF.B.h^I^J.B.l)&H_FLAG)|(R->BC.W? P_FLAG:0);
  break;

#ifdef TURBO_R
case DB_C1:
    M_MULU(R->AF.B.h, R->BC.B.h);
    break;
case DB_C3:
    M_MULUW(R->HL, R->BC);
    break;
case DB_C9:
    M_MULU(R->AF.B.h, R->BC.B.l);
    break;
case DB_D1:
    M_MULU(R->AF.B.h, R->DE.B.h);
    break;
case DB_D3:
case DB_E1:
case DB_E3:
case DB_E9:
case DB_F9:
    printf("TurboR command!");
    break;

case DB_D9:
    M_MULU(R->AF.B.h, R->DE.B.l);
    break;
case DB_F3:
    M_MULUW(R->HL, R->SP);
    break;
#endif // TURBO_R

