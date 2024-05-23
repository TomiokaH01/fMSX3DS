/** fMSX: portable MSX emulator ******************************/
/**                                                         **/
/**                        Patch.c                          **/
/**                                                         **/
/** This file implements patches for MSX disk and tape I/O  **/
/** routines. Note that the disk I/O patches are optional,  **/
/** as there is a proper WD1793 FDC emulation now.          **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1994-2020                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/

/** NINTENDO 3DS Version modified by h.tomioka 2023         **/

#include "MSX.h"
#include "Boot.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

#ifdef _3DS
#include "V9938.h"
#include "3DSLib.h"

void SSlot(byte Value); /* Used to switch secondary slots */
void PSlot(byte Value); /* Used to switch primary slots   */

//void SSlot_BDOS(byte V);

/** DiskPresent() ********************************************/
/** Return 1 if disk drive with a given ID is present.      **/
/*************************************************************/
byte DiskPresent(byte ID)
{
  return((ID<MAXDRIVES)&&FDD[ID].Data);
}

/** DiskRead() ***********************************************/
/** Read requested sector from the drive into a buffer.     **/
/*************************************************************/
byte DiskRead(byte ID,byte *Buf,int N)
{
  byte *P;

  if(ID<MAXDRIVES)
  {
      if (Verbose & 0x04)printf("Read Disk Side:%d Track:%d SideID:%d TrackID:%d SectorID:%d\n",
          (N / FDD[ID].Sectors) % FDD[ID].Sides, N / FDD[ID].Sectors / FDD[ID].Sides, (N / FDD[ID].Sectors) % FDD[ID].Sides, N / FDD[ID].Sectors / FDD[ID].Sides,
          N % FDD[ID].Sectors+1);

    /* Get data pointer to requested sector */
    P = LinearFDI(&FDD[ID],N);
    /* If seek operation succeeded, read sector */
    if(P) memcpy(Buf,P,FDD[ID].SecSize);
    DiskAccess[ID] = 3;

    /* Done */
    return(!!P);
  }

  return(0);
}

/** DiskWrite() **********************************************/
/** Write contents of the buffer into a given sector of the **/
/** disk.                                                   **/
/*************************************************************/
byte DiskWrite(byte ID,const byte *Buf,int N)
{
  byte *P;

  if(ID<MAXDRIVES)
  {
    /* Get data pointer to requested sector */
    P = LinearFDI(&FDD[ID],N);
    /* If seek operation succeeded, write sector */
    if(P) memcpy(P,Buf,FDD[ID].SecSize);
    /* Done */
    DiskWrited[ID] = 1;
    DiskAccess[ID] = 3;
    return(!!P);
  }

  return(0);
}


byte DiskReadPatch(byte ID, byte* Buf, int N, byte PerTrack, byte Heads)
{
	byte* P;
	if (ID < MAXDRIVES)
	{
		/* Get data pointer to requested sector */
		P = LinearFDIPatch(ID, N, PerTrack, Heads);
		/* If seek operation succeeded, read sector */
		if (P) memcpy(Buf, P, FDD[ID].SecSize);
		DiskAccess[ID] = 3;

		/* Done */
		return(!!P);
	}

	return(0);
}


byte DiskWritePatch(byte ID, byte* Buf, int N, byte PerTrack, byte Heads)
{
    byte* P;

    if (ID < MAXDRIVES)
    {
        /* Get data pointer to requested sector */
        P = LinearFDIPatch(ID, N, PerTrack, Heads);
        /* If seek operation succeeded, write sector */
        if (P) memcpy(P, Buf, FDD[ID].SecSize);
        /* Done */
        DiskWrited[ID] = 1;
        DiskAccess[ID] = 3;
        return(!!P);
    }

    return(0);
}


/* Modified version of LinearFDI(). */
/* It can work more disks to work with the patched BIOS. Fony's Demo Disk 2-1 etc. */
/* Idea from Common Source Code Project(yayaMSX).*/
/* http://takeda-toshiya.my.coocan.jp/common/index.html */
/* PerTrack: Info[R->BC.B.l - 0xF8].PerTrack   Heads:Info[R->BC.B.l - 0xF8].Heads */
byte* LinearFDIPatch(byte ID, int SectorN, byte PerTrack, byte Heads)
{
    if (!FDD[ID].Sectors || !FDD[ID].Sides || (SectorN < 0)) return(0);
    else
    {
        int Sector = SectorN % PerTrack;
        int Track = SectorN / PerTrack >> (Heads - 1);
        int Side = (SectorN / PerTrack) & (Heads - 1);

        if (Verbose & 0x04)printf("Seek Disk Side:%d Track:%d SideID:%d TrackID:%d SectorID:%d SectorNum:%d\n",
            Side, Track, Side, Track, Sector+1, SectorN);
        return(SeekFDI(&FDD[ID], Side, Track, Side, Track, Sector + 1));
    }
}

/** PatchZ80() ***********************************************/
/** Emulate BIOS calls. This function is called on an ED FE **/
/** instruction to emulate disk/tape access, etc.           **/
/*************************************************************/
void PatchZ80(Z80 *R)
{
  static const byte TapeHeader[8] = { 0x1F,0xA6,0xDE,0xBA,0xCC,0x13,0x7D,0x74 };

  static const struct
  { int Sectors;byte Heads,Names,PerTrack,PerFAT,PerCluster; }
  Info[8] =
  {
    {  720,1,112,9,2,2 },
    { 1440,2,112,9,3,2 },
    {  640,1,112,8,1,2 },
    { 1280,2,112,8,2,2 },
    {  360,1, 64,9,2,1 },
    {  720,2,112,9,2,2 },
    {  320,1, 64,8,1,1 },
    {  640,2,112,8,1,2 }
  };

  byte Buf[512],Count,PS,SS,N,*P;
  int J,I,Sector;
  word Addr;

  byte PSO, SSO;
  byte OSSLReg[4];

  if (VDPStatus[2] & 0x01)FlushVDP();

  switch(R->PC.W-2)
  {
  case 0xDE:
      R->AF.B.h = GetPaddle(R->AF.B.h);
      return;

case 0x4010:
/** PHYDIO: Read/write sectors to disk **************************
*** Input:                                                    ***
*** [F] CARRY=WRITE                  [A] Drive number (0=A:)  ***
*** [B] Number of sectors to write   [C] Media descriptor     ***
*** [DE] Logical sector number (starts at 0)                  ***
*** [HL] Transfer address                                     ***
*** Output:                                                   ***
*** [F] CARRY=ERROR                  [A] If error: errorcode  ***
*** [B] Number of sectors remaining (not read/written)        ***
*** Error codes in [A] can be:                                ***
*** 0 Write protected      8  Record not found                ***
*** 2 Not ready            10 Write fault                     ***
*** 4 Data (CRC) error     12 Other errors                    ***
*** 6 Seek error                                              ***
****************************************************************/
{
    if (Verbose & 0x04)
        printf
        (
            "%s DISK %c: %d sectors starting from %04Xh [buffer at %04Xh]\n",
            R->AF.B.l & C_FLAG ? "WRITE" : "READ", R->AF.B.h + 'A', R->BC.B.h,
            R->DE.W, R->HL.W
            );
    if (Verbose & 0x04) printf("MediaID:%d\n", R->BC.B.l - 0xF8);

    R->IFF |= 1;
    Addr = R->HL.W;
    Count = R->BC.B.h;

    if (!DiskPresent(R->AF.B.h))
    {
        R->AF.W = 0x0201; return;
    }  /* No disk      -> "Not ready"        */

    if ((int)(R->DE.W) + Count > Info[R->BC.B.l - 0xF8].Sectors)
    {
        if (Verbose & 0x04) printf("Wrong Sector\n");
        R->AF.W = 0x0801; return;
    }  /* Wrong sector -> "Record not found" */

/* If data does not fit into 64kB address space, trim it */
    //if (Addr + R->BC.B.h * 512 > 0x10000)
    if ((int)Addr + Count * 512 > 0x10000)
    {
        R->BC.B.h = (0x10000 - Addr) / 512;
        Count = R->BC.B.h;
        if (Verbose & 0x04) printf("Trimed\n");
    }
    
    /* Save slot states */
    PS = PSLReg;
    SS = SSLReg[3];

    SSO = 0x55 * ((SS >> 6) & 0x03);
    PSO = (PS & 0xF3) | ((RdZ80(0xF342)&0xFF) << 2);

    //PSlot(PS | 0x0C);
    PSlot(PSO);
    SSlot(SSO);

    if (R->AF.B.l & C_FLAG)
        for (Sector = R->DE.W; Count--; Sector++) /* WRITE */
        {
            for (J = 0; J < 512; J++) Buf[J] = RdZ80(Addr++);

            //if (DiskWrite(R->AF.B.h, Buf, Sector)) R->BC.B.h--;
            if(DiskWritePatch(R->AF.B.h, Buf, Sector, Info[R->BC.B.l - 0xF8].PerTrack, Info[R->BC.B.l - 0xF8].Heads))R->BC.B.h--;
            else
            {
                R->AF.W = 0x0A01;
                SSlot(SS);
                PSlot(PS);
                return;
            }
        }
    else
        for (Sector = R->DE.W; Count--; Sector++) /* READ */
        {
            //if (DiskRead(R->AF.B.h, Buf, Sector)) R->BC.B.h--;
            if (DiskReadPatch(R->AF.B.h, Buf, Sector, Info[R->BC.B.l - 0xF8].PerTrack, Info[R->BC.B.l - 0xF8].Heads))R->BC.B.h--;
            else
            {
                if (Verbose & 0x04) printf("Disk Read Error\n");
                R->AF.W = 0x0401;
                SSlot(SS);
                PSlot(PS);
                return;
            }

            for (J = 0; J < 512; J++) WrZ80(Addr++, Buf[J]);
        }

    /* Restore slot states */
    SSlot(SS);
    PSlot(PS);

    /* Return "Success" */
    R->AF.B.l &= ~C_FLAG;
    return;
}

case 0x4013:
/** DSKCHG: Check if the disk was changed ***********************
*** Input:                                                    ***
*** [A] Drive number (0=A:)       [B]  Media descriptor       ***
*** [C] Media descriptor          [HL] Base address of DPB    ***
*** Output:                                                   ***
*** [F] CARRY=ERROR       [A] If error: errorcode (see DSKIO) ***
*** [B] If success: 1=Unchanged, 0=Unknown, -1=Changed        ***
*** Note:                                                     ***
*** If the disk has been changed or may have been  changed    ***
*** (unknown) read the boot sector or the FAT sector for disk ***
*** media descriptor and transfer a new DPB as with GETDPB.   ***
****************************************************************/
{
  if(Verbose&0x04) printf("CHECK DISK %c\n",R->AF.B.h+'A');

  R->IFF|=1;

  /* If no disk, return "Not ready": */
  if(!DiskPresent(R->AF.B.h)) { R->AF.W=0x0201;return; }

//#ifndef _3DS
//  /* This requires some major work to be done: */
//  R->BC.B.h=0;R->AF.B.l&=~C_FLAG;
//#endif // !_3DS

  //if (!DiskRead(R->AF.B.h, Buf, 1)) { R->AF.W = 0x0A01; return; }
  if(!DiskReadPatch(R->AF.B.h, Buf, 1, Info[R->BC.B.l - 0xF8].PerTrack, Info[R->BC.B.l - 0xF8].Heads)) { R->AF.W = 0x0A01; return; }
  R->BC.B.h = Buf[0];

  /* We continue with GETDPB now... */
}

case 0x4016:
/** GETDPB: Disk format *****************************************
*** Input:                                                    ***
*** [A] Drive number   [B] 1st byte of FAT (media descriptor) ***
*** [C] Media descriptor  [HL] Base address of DPB            ***
*** Output:                                                   ***
*** [HL+1] .. [HL+18] = DPB for specified drive               ***
*** DPB consists of:                                          ***
*** Name   Offset Size Description                            ***
*** MEDIA    0     1   Media type (F8..FF)                    ***
*** SECSIZ   1     2   Sector size (must be 2^n)              ***
*** DIRMSK   3     1   (SECSIZE/32)-1                         ***
*** DIRSHFT  4     1   Number of one bits in DIRMSK           ***
*** CLUSMSK  5     1   (Sectors per cluster)-1                ***
*** CLUSSHFT 6     1   (Number of one bits in CLUSMSK)+1      ***
*** FIRFAT   7     2   Logical sector number of first FAT     ***
*** FATCNT   8     1   Number of FATs                         ***
*** MAXENT   A     1   Number of directory entries (max 254)  ***
*** FIRREC   B     2   Logical sector number of first data    ***
*** MAXCLUS  D     2   Number of clusters (not including      ***
***                    reserved, FAT and directory sectors)+1 ***
*** FATSIZ   F     1   Number of sectors used                 ***
*** FIRDIR   10    2   FAT logical sector number of start of  ***
***                    directory                              ***
****************************************************************/
{
    if (Verbose & 0x04) printf("GET DPB: %04Xh\n", R->BC.B.h);
    /* Idea from early version of openMSX and BlueMSX. Support boot sector cracked disks such as Jikuu no Hanayome etc.*/
    /* http://bluemsx.msxblue.com/jindex.htm */
    word FIRREC, MAXCLUS, FIRDIR;
    byte FATSIZ;

    /* If no disk, return "Not ready": */
    if (!DiskPresent(R->AF.B.h)) { R->AF.W = 0x0201; return; }

    //if (!DiskRead(R->AF.B.h, Buf, 1)) { R->AF.W = 0x0A01; return; }
    //R->BC.B.h = Buf[0];

    switch (R->BC.B.h)
    {
    case 0xF8:
        FATSIZ = 2;
        MAXCLUS = 355;
        break;
    case 0xF9:
        FATSIZ = 3;
        MAXCLUS = 714;
        break;
    case 0xFA:
        FATSIZ = 1;
        MAXCLUS = 316;
        break;
    case 0xFB:
        FATSIZ = 2;
        MAXCLUS = 635;
        break;
    case 0xFC:
        FATSIZ = 2;
        MAXCLUS = 316;
        break;
    default:
        R->AF.W = 0x0C01;
        if ((R->PC.W - 2) != 0x4016)
        {
            R->AF.W = 0x0A01;
            R->BC.B.h = 0;
        }
        return;
    }
    FIRDIR = 1 + FATSIZ * 2;
    FIRREC = FIRDIR + 7;

    Addr = R->HL.W;
    if (R->BC.B.h == 0xFC)
    {
        WrZ80(Addr + 1, R->BC.B.h);
        WrZ80(Addr + 2, 0);
        WrZ80(Addr + 3, 2);
        WrZ80(Addr + 4, 15);
        WrZ80(Addr + 5, 4);
        WrZ80(Addr + 6, 0);
        WrZ80(Addr + 7, 1);
        WrZ80(Addr + 8, 1);
        WrZ80(Addr + 9, 0);
        WrZ80(Addr + 10, 2);
        WrZ80(Addr + 11, 0x40);
        WrZ80(Addr + 12, 9);
        WrZ80(Addr + 13, 0);
        WrZ80(Addr + 14, 0x60);
        WrZ80(Addr + 15, 1);
        WrZ80(Addr + 16, 2);
        WrZ80(Addr + 17, 5);
        WrZ80(Addr + 18, 0);
        if ((R->PC.W - 2) != 0x4016)R->BC.B.h = 0;
        /* Return success      */
        R->AF.B.l &= ~C_FLAG;
        return;
    }
    WrZ80(Addr+1, R->BC.B.h);
    WrZ80(Addr+2, 0);
    WrZ80(Addr+3, 2);
    WrZ80(Addr+4, 15);
    WrZ80(Addr+5, 4);
    WrZ80(Addr+6, 1);
    //WrZ80(Addr + 6, CLUSMSK);
    WrZ80(Addr+7, 2);
    //WrZ80(Addr + 7, CLUSMSK+1);
    WrZ80(Addr+8, 1);
    WrZ80(Addr+9, 0);
    WrZ80(Addr+10, 2);
    WrZ80(Addr+11, 112);
    //WrZ80(Addr + 11, MAXENT);
    WrZ80(Addr+12, (FIRDIR + 7) & 0xFF);
    WrZ80(Addr+13, (FIRDIR + 7) >> 8);
    WrZ80(Addr+14, MAXCLUS & 0xFF);
    WrZ80(Addr+15, MAXCLUS >> 8);
    WrZ80(Addr+16, FATSIZ);
    WrZ80(Addr+17, FIRDIR & 0xFF);
    WrZ80(Addr+18, FIRDIR >> 8);
    if ((R->PC.W - 2) != 0x4016)R->BC.B.h = 0;
  /* Return success      */
  R->AF.B.l&=~C_FLAG;
  return;
}


case 0x401C:
    /** DSKFMT: Disk format *****************************************
    *** Input:                                                    ***
    *** [A]  Specified choice (1-9)      [D]  Drive number (0=A:) ***
    *** [HL] Begin address of work area  [BC] Length of work area ***
    *** Output:                                                   ***
    *** [F] CARRY=ERROR                                           ***
    *** Notes:                                                    ***
    *** 1) Also writes a MSX boot sector at sector 0, clears all  ***
    ***    FATs (media descriptor at first byte, 0FFh at second/  ***
    ***    third byte and rest zero) and clears the directory     ***
    ***    filling it with zeros.                                 ***
    *** 2) Error codes are:                                       ***
    ***    0 Write protected       10 Write fault                 ***
    ***    2 Not ready             12 Bad parameter               ***
    ***    4 Data (CRC) error      14 Insufficient memory         ***
    ***    6 Seek error            16 Other errors                ***
    ***    8 Record not found                                     ***
    ****************************************************************/
{
    R->IFF |= 1;

    /* If invalid choice, return "Bad parameter": */
    if (!R->AF.B.h || (R->AF.B.h > 2)) { R->AF.W = 0x0C01; return; }
    /* If no disk, return "Not ready": */
    if (!DiskPresent(R->DE.B.h)) { R->AF.W = 0x0201; return; }

    /* Fill bootblock with data: */
    P = BootBlock + 3;
    //N = 2 - R->AF.B.h;
    /* Value from Common Source Code Project(yayaMSX).*/
    /* http://takeda-toshiya.my.coocan.jp/common/index.html */
    /* N: 1=single-side 2=double-side */
    N = R->AF.B.h - 1;
    memcpy(P, "fMSXdisk", 8); P += 10;    /* Manufacturer's ID   */
    *P = Info[N].PerCluster; P += 4;      /* Sectors per cluster */
    *P++ = Info[N].Names; *P++ = 0x00;    /* Number of names     */
    *P++ = Info[N].Sectors & 0xFF;       /* Number of sectors   */
    *P++ = (Info[N].Sectors >> 8) & 0xFF;
    *P++ = N + 0xF8;                     /* Format ID [F8h-FFh] */
    *P++ = Info[N].PerFAT; *P++ = 0x00;   /* Sectors per FAT     */
    *P++ = Info[N].PerTrack; *P++ = 0x00; /* Sectors per track   */
    *P++ = Info[N].Heads; *P = 0x00;      /* Number of heads     */

    /* If can't write bootblock, return "Write protected": */
    if (!DiskWrite(R->DE.B.h, BootBlock, 0)) { R->AF.W = 0x0001; return; };

    /* Writing FATs: */
    for (Sector = 1, J = 0; J < 2; J++)
    {
        Buf[0] = N + 0xF8;
        Buf[1] = Buf[2] = 0xFF;
        memset(Buf + 3, 0x00, 509);

        if (!DiskWrite(R->DE.B.h, Buf, Sector++)) { R->AF.W = 0x0A01; return; }

        memset(Buf, 0x00, 512);

        for (I = Info[N].PerFAT; I > 1; I--)
            if (!DiskWrite(R->DE.B.h, Buf, Sector++)) { R->AF.W = 0x0A01; return; }
    }

    J = Info[N].Names / 16;                     /* Directory size */
    I = Info[N].Sectors - 2 * Info[N].PerFAT - J - 1; /* Data size */

    for (memset(Buf, 0x00, 512); J; J--)
        if (!DiskWrite(R->DE.B.h, Buf, Sector++)) { R->AF.W = 0x0A01; return; }
    for (memset(Buf, 0xFF, 512); I; I--)
        if (!DiskWrite(R->DE.B.h, Buf, Sector++)) { R->AF.W = 0x0A01; return; }

    /* Return success      */
    R->AF.B.l &= ~C_FLAG;
    return;
}


case 0x401F:
/** DRVOFF: Stop drives *****************************************
*** Input:  None                                              ***
*** Output: None                                              ***
****************************************************************/
  return;

case 0x00E1:
/** TAPION: Open for read and read header ***********************
****************************************************************/
{
  long Pos;

  if(Verbose&0x04) printf("TAPE: Looking for header...");

  R->AF.B.l|=C_FLAG;
  if(CasStream)
  {
    Pos=ftell(CasStream);
    if(Pos&7)
      if(fseek(CasStream,8-(Pos&7),SEEK_CUR))
      {
        if(Verbose&0x04) puts("FAILED");
        rewind(CasStream);return;
      }

    while(fread(Buf,1,8,CasStream)==8)
      if(!memcmp(Buf,TapeHeader,8))
      {
        if(Verbose&0x04) puts("OK");
        R->AF.B.l&=~C_FLAG;return;
      }

    rewind(CasStream);
  }

  if(Verbose&0x04) puts("FAILED");
  return;
}

case 0x00E4:
/** TAPIN: Read tape ********************************************
****************************************************************/
{
  R->AF.B.l|=C_FLAG;

  if(CasStream)
  {
    J=fgetc(CasStream);
    if(J<0) rewind(CasStream);
    else { R->AF.B.h=J;R->AF.B.l&=~C_FLAG; }
  }

  return;
}

case 0x00E7:
/** TAPIOF: *****************************************************
****************************************************************/
  R->AF.B.l&=~C_FLAG;
  return;

case 0x00EA:
/** TAPOON: *****************************************************
****************************************************************/
{
  long Pos;

  R->AF.B.l|=C_FLAG;

  if(CasStream)
  {
    Pos=ftell(CasStream);
    if(Pos&7)
      if(fseek(CasStream,8-(Pos&7),SEEK_CUR))
      { R->AF.B.l|=C_FLAG;return; }

    fwrite(TapeHeader,1,8,CasStream);
    R->AF.B.l&=~C_FLAG;
  }   

  return;
}

case 0x00ED:
/** TAPOUT: Write tape ******************************************
****************************************************************/
  R->AF.B.l|=C_FLAG;

  if(CasStream)
  {
    fputc(R->AF.B.h,CasStream);
    R->AF.B.l&=~C_FLAG;
  }

  return;

case 0x00F0:
/** TAPOOF: *****************************************************
****************************************************************/
  R->AF.B.l&=~C_FLAG;
  return;

case 0x00F3:
/** STMOTR: *****************************************************
****************************************************************/
  R->AF.B.l&=~C_FLAG;
  return;

default:
  printf("Unknown BIOS trap called at PC=%04Xh\n",R->PC.W-2);

  }
}

#else

void SSlot(byte Value); /* Used to switch secondary slots */
void PSlot(byte Value); /* Used to switch primary slots   */

/** DiskPresent() ********************************************/
/** Return 1 if disk drive with a given ID is present.      **/
/*************************************************************/
byte DiskPresent(byte ID)
{
    return((ID < MAXDRIVES) && FDD[ID].Data);
}

/** DiskRead() ***********************************************/
/** Read requested sector from the drive into a buffer.     **/
/*************************************************************/
byte DiskRead(byte ID, byte* Buf, int N)
{
    byte* P;

    if (ID < MAXDRIVES)
    {
        /* Get data pointer to requested sector */
        P = LinearFDI(&FDD[ID], N);
        /* If seek operation succeeded, read sector */
        if (P) memcpy(Buf, P, FDD[ID].SecSize);
        /* Done */
        return(!!P);
    }

    return(0);
}

/** DiskWrite() **********************************************/
/** Write contents of the buffer into a given sector of the **/
/** disk.                                                   **/
/*************************************************************/
byte DiskWrite(byte ID, const byte* Buf, int N)
{
    byte* P;

    if (ID < MAXDRIVES)
    {
        /* Get data pointer to requested sector */
        P = LinearFDI(&FDD[ID], N);
        /* If seek operation succeeded, write sector */
        if (P) memcpy(P, Buf, FDD[ID].SecSize);
        /* Done */
        return(!!P);
    }

    return(0);
}

/** PatchZ80() ***********************************************/
/** Emulate BIOS calls. This function is called on an ED FE **/
/** instruction to emulate disk/tape access, etc.           **/
/*************************************************************/
void PatchZ80(Z80* R)
{
    static const byte TapeHeader[8] = { 0x1F,0xA6,0xDE,0xBA,0xCC,0x13,0x7D,0x74 };

    static const struct
    {
        int Sectors; byte Heads, Names, PerTrack, PerFAT, PerCluster;
    }
    Info[8] =
    {
      {  720,1,112,9,2,2 },
      { 1440,2,112,9,3,2 },
      {  640,1,112,8,1,2 },
      { 1280,2,112,8,2,2 },
      {  360,1, 64,9,2,1 },
      {  720,2,112,9,2,2 },
      {  320,1, 64,8,1,1 },
      {  640,2,112,8,1,2 }
    };

    byte Buf[512], Count, PS, SS, N, * P;
    int J, I, Sector;
    word Addr;

    switch (R->PC.W - 2)
    {

    case 0x4010:
        /** PHYDIO: Read/write sectors to disk **************************
        *** Input:                                                    ***
        *** [F] CARRY=WRITE                  [A] Drive number (0=A:)  ***
        *** [B] Number of sectors to write   [C] Media descriptor     ***
        *** [DE] Logical sector number (starts at 0)                  ***
        *** [HL] Transfer address                                     ***
        *** Output:                                                   ***
        *** [F] CARRY=ERROR                  [A] If error: errorcode  ***
        *** [B] Number of sectors remaining (not read/written)        ***
        *** Error codes in [A] can be:                                ***
        *** 0 Write protected      8  Record not found                ***
        *** 2 Not ready            10 Write fault                     ***
        *** 4 Data (CRC) error     12 Other errors                    ***
        *** 6 Seek error                                              ***
        ****************************************************************/
    {
        if (Verbose & 0x04)
            printf
            (
                "%s DISK %c: %d sectors starting from %04Xh [buffer at %04Xh]\n",
                R->AF.B.l & C_FLAG ? "WRITE" : "READ", R->AF.B.h + 'A', R->BC.B.h,
                R->DE.W, R->HL.W
                );

        R->IFF |= 1;
        Addr = R->HL.W;
        Count = R->BC.B.h;

        if (!DiskPresent(R->AF.B.h))
        {
            R->AF.W = 0x0201; return;
        }  /* No disk      -> "Not ready"        */
        if ((int)(R->DE.W) + Count > Info[R->BC.B.l - 0xF8].Sectors)
        {
            R->AF.W = 0x0801; return;
        }  /* Wrong sector -> "Record not found" */

/* If data does not fit into 64kB address space, trim it */
        if ((int)(R->HL.W) + Count * 512 > 0x10000) Count = (0x10000 - R->HL.W) / 512;

        /* Save slot states */
        PS = PSLReg;
        SS = SSLReg[3];

        /* Turn on RAM in all slots */
        OutZ80(0xA8, 0xFF);
        SSlot(0xAA);

        if (R->AF.B.l & C_FLAG)
            for (Sector = R->DE.W; Count--; Sector++) /* WRITE */
            {
                for (J = 0; J < 512; J++) Buf[J] = RdZ80(Addr++);

                if (DiskWrite(R->AF.B.h, Buf, Sector)) R->BC.B.h--;
                else
                {
                    R->AF.W = 0x0A01;
                    SSlot(SS);
                    OutZ80(0xA8, PS);
                    return;
                }
            }
        else
            for (Sector = R->DE.W; Count--; Sector++) /* READ */
            {
                if (DiskRead(R->AF.B.h, Buf, Sector)) R->BC.B.h--;
                else
                {
                    R->AF.W = 0x0401;
                    SSlot(SS);
                    OutZ80(0xA8, PS);
                    return;
                }

                for (J = 0; J < 512; J++) WrZ80(Addr++, Buf[J]);
            }

        /* Restore slot states */
        SSlot(SS);
        OutZ80(0xA8, PS);

        /* Return "Success" */
        R->AF.B.l &= ~C_FLAG;
        return;
    }

    case 0x4013:
        /** DSKCHG: Check if the disk was changed ***********************
        *** Input:                                                    ***
        *** [A] Drive number (0=A:)       [B]  Media descriptor       ***
        *** [C] Media descriptor          [HL] Base address of DPB    ***
        *** Output:                                                   ***
        *** [F] CARRY=ERROR       [A] If error: errorcode (see DSKIO) ***
        *** [B] If success: 1=Unchanged, 0=Unknown, -1=Changed        ***
        *** Note:                                                     ***
        *** If the disk has been changed or may have been  changed    ***
        *** (unknown) read the boot sector or the FAT sector for disk ***
        *** media descriptor and transfer a new DPB as with GETDPB.   ***
        ****************************************************************/
    {
        if (Verbose & 0x04) printf("CHECK DISK %c\n", R->AF.B.h + 'A');

        R->IFF |= 1;

        /* If no disk, return "Not ready": */
        if (!DiskPresent(R->AF.B.h)) { R->AF.W = 0x0201; return; }

        /* This requires some major work to be done: */
        R->BC.B.h = 0; R->AF.B.l &= ~C_FLAG;

        /* We continue with GETDPB now... */
    }

    case 0x4016:
        /** GETDPB: Disk format *****************************************
        *** Input:                                                    ***
        *** [A] Drive number   [B] 1st byte of FAT (media descriptor) ***
        *** [C] Media descriptor  [HL] Base address of DPB            ***
        *** Output:                                                   ***
        *** [HL+1] .. [HL+18] = DPB for specified drive               ***
        *** DPB consists of:                                          ***
        *** Name   Offset Size Description                            ***
        *** MEDIA    0     1   Media type (F8..FF)                    ***
        *** SECSIZ   1     2   Sector size (must be 2^n)              ***
        *** DIRMSK   3     1   (SECSIZE/32)-1                         ***
        *** DIRSHFT  4     1   Number of one bits in DIRMSK           ***
        *** CLUSMSK  5     1   (Sectors per cluster)-1                ***
        *** CLUSSHFT 6     1   (Number of one bits in CLUSMSK)+1      ***
        *** FIRFAT   7     2   Logical sector number of first FAT     ***
        *** FATCNT   8     1   Number of FATs                         ***
        *** MAXENT   A     1   Number of directory entries (max 254)  ***
        *** FIRREC   B     2   Logical sector number of first data    ***
        *** MAXCLUS  D     2   Number of clusters (not including      ***
        ***                    reserved, FAT and directory sectors)+1 ***
        *** FATSIZ   F     1   Number of sectors used                 ***
        *** FIRDIR   10    2   FAT logical sector number of start of  ***
        ***                    directory                              ***
        ****************************************************************/
    {
        int BytesPerSector, SectorsPerDisk, SectorsPerFAT, ReservedSectors;

        /* If no disk, return "Not ready": */
        if (!DiskPresent(R->AF.B.h)) { R->AF.W = 0x0201; return; }
        /* If can't read, return "Other error": */
        if (!DiskRead(R->AF.B.h, Buf, 0)) { R->AF.W = 0x0C01; return; }

        BytesPerSector = (int)Buf[0x0C] * 256 + Buf[0x0B];
        SectorsPerDisk = (int)Buf[0x14] * 256 + Buf[0x13];
        SectorsPerFAT = (int)Buf[0x17] * 256 + Buf[0x16];
        ReservedSectors = (int)Buf[0x0F] * 256 + Buf[0x0E];

        Addr = R->HL.W + 1;
        WrZ80(Addr++, Buf[0x15]);             /* Format ID [F8h-FFh] */
        WrZ80(Addr++, Buf[0x0B]);             /* Sector size         */
        WrZ80(Addr++, Buf[0x0C]);
        J = (BytesPerSector >> 5) - 1;
        for (I = 0; J & (1 << I); I++);
        WrZ80(Addr++, J);                     /* Directory mask/shft */
        WrZ80(Addr++, I);
        J = Buf[0x0D] - 1;
        for (I = 0; J & (1 << I); I++);
        WrZ80(Addr++, J);                     /* Cluster mask/shift  */
        WrZ80(Addr++, I + 1);
        WrZ80(Addr++, Buf[0x0E]);             /* Sector # of 1st FAT */
        WrZ80(Addr++, Buf[0x0F]);
        WrZ80(Addr++, Buf[0x10]);             /* Number of FATs      */
        WrZ80(Addr++, Buf[0x11]);             /* Number of dirent-s  */
        J = ReservedSectors + Buf[0x10] * SectorsPerFAT;
        J += 32 * Buf[0x11] / BytesPerSector;
        WrZ80(Addr++, J & 0xFF);                /* Sector # of data    */
        WrZ80(Addr++, (J >> 8) & 0xFF);
        J = (SectorsPerDisk - J) / Buf[0x0D];
        WrZ80(Addr++, J & 0xFF);                /* Number of clusters  */
        WrZ80(Addr++, (J >> 8) & 0xFF);
        WrZ80(Addr++, Buf[0x16]);             /* Sectors per FAT     */
        J = ReservedSectors + Buf[0x10] * SectorsPerFAT;
        WrZ80(Addr++, J & 0xFF);                /* Sector # of dir.    */
        WrZ80(Addr, (J >> 8) & 0xFF);

        /* Return success      */
        R->AF.B.l &= ~C_FLAG;
        return;
    }

    case 0x401C:
        /** DSKFMT: Disk format *****************************************
        *** Input:                                                    ***
        *** [A]  Specified choice (1-9)      [D]  Drive number (0=A:) ***
        *** [HL] Begin address of work area  [BC] Length of work area ***
        *** Output:                                                   ***
        *** [F] CARRY=ERROR                                           ***
        *** Notes:                                                    ***
        *** 1) Also writes a MSX boot sector at sector 0, clears all  ***
        ***    FATs (media descriptor at first byte, 0FFh at second/  ***
        ***    third byte and rest zero) and clears the directory     ***
        ***    filling it with zeros.                                 ***
        *** 2) Error codes are:                                       ***
        ***    0 Write protected       10 Write fault                 ***
        ***    2 Not ready             12 Bad parameter               ***
        ***    4 Data (CRC) error      14 Insufficient memory         ***
        ***    6 Seek error            16 Other errors                ***
        ***    8 Record not found                                     ***
        ****************************************************************/
    {
        R->IFF |= 1;

        /* If invalid choice, return "Bad parameter": */
        if (!R->AF.B.h || (R->AF.B.h > 2)) { R->AF.W = 0x0C01; return; }
        /* If no disk, return "Not ready": */
        if (!DiskPresent(R->DE.B.h)) { R->AF.W = 0x0201; return; }

        /* Fill bootblock with data: */
        P = BootBlock + 3;
        N = 2 - R->AF.B.h;
        memcpy(P, "fMSXdisk", 8); P += 10;    /* Manufacturer's ID   */
        *P = Info[N].PerCluster; P += 4;      /* Sectors per cluster */
        *P++ = Info[N].Names; *P++ = 0x00;    /* Number of names     */
        *P++ = Info[N].Sectors & 0xFF;       /* Number of sectors   */
        *P++ = (Info[N].Sectors >> 8) & 0xFF;
        *P++ = N + 0xF8;                     /* Format ID [F8h-FFh] */
        *P++ = Info[N].PerFAT; *P++ = 0x00;   /* Sectors per FAT     */
        *P++ = Info[N].PerTrack; *P++ = 0x00; /* Sectors per track   */
        *P++ = Info[N].Heads; *P = 0x00;      /* Number of heads     */

        /* If can't write bootblock, return "Write protected": */
        if (!DiskWrite(R->DE.B.h, BootBlock, 0)) { R->AF.W = 0x0001; return; };

        /* Writing FATs: */
        for (Sector = 1, J = 0; J < 2; J++)
        {
            Buf[0] = N + 0xF8;
            Buf[1] = Buf[2] = 0xFF;
            memset(Buf + 3, 0x00, 509);

            if (!DiskWrite(R->DE.B.h, Buf, Sector++)) { R->AF.W = 0x0A01; return; }

            memset(Buf, 0x00, 512);

            for (I = Info[N].PerFAT; I > 1; I--)
                if (!DiskWrite(R->DE.B.h, Buf, Sector++)) { R->AF.W = 0x0A01; return; }
        }

        J = Info[N].Names / 16;                     /* Directory size */
        I = Info[N].Sectors - 2 * Info[N].PerFAT - J - 1; /* Data size */

        for (memset(Buf, 0x00, 512); J; J--)
            if (!DiskWrite(R->DE.B.h, Buf, Sector++)) { R->AF.W = 0x0A01; return; }
        for (memset(Buf, 0xFF, 512); I; I--)
            if (!DiskWrite(R->DE.B.h, Buf, Sector++)) { R->AF.W = 0x0A01; return; }

        /* Return success      */
        R->AF.B.l &= ~C_FLAG;
        return;
    }

    case 0x401F:
        /** DRVOFF: Stop drives *****************************************
        *** Input:  None                                              ***
        *** Output: None                                              ***
        ****************************************************************/
        return;

    case 0x00E1:
        /** TAPION: Open for read and read header ***********************
        ****************************************************************/
    {
        long Pos;

        if (Verbose & 0x04) printf("TAPE: Looking for header...");

        R->AF.B.l |= C_FLAG;
        if (CasStream)
        {
            Pos = ftell(CasStream);
            if (Pos & 7)
                if (fseek(CasStream, 8 - (Pos & 7), SEEK_CUR))
                {
                    if (Verbose & 0x04) puts("FAILED");
                    rewind(CasStream); return;
                }

            while (fread(Buf, 1, 8, CasStream) == 8)
                if (!memcmp(Buf, TapeHeader, 8))
                {
                    if (Verbose & 0x04) puts("OK");
                    R->AF.B.l &= ~C_FLAG; return;
                }

            rewind(CasStream);
        }

        if (Verbose & 0x04) puts("FAILED");
        return;
    }

    case 0x00E4:
        /** TAPIN: Read tape ********************************************
        ****************************************************************/
    {
        R->AF.B.l |= C_FLAG;

        if (CasStream)
        {
            J = fgetc(CasStream);
            if (J < 0) rewind(CasStream);
            else { R->AF.B.h = J; R->AF.B.l &= ~C_FLAG; }
        }

        return;
    }

    case 0x00E7:
        /** TAPIOF: *****************************************************
        ****************************************************************/
        R->AF.B.l &= ~C_FLAG;
        return;

    case 0x00EA:
        /** TAPOON: *****************************************************
        ****************************************************************/
    {
        long Pos;

        R->AF.B.l |= C_FLAG;

        if (CasStream)
        {
            Pos = ftell(CasStream);
            if (Pos & 7)
                if (fseek(CasStream, 8 - (Pos & 7), SEEK_CUR))
                {
                    R->AF.B.l |= C_FLAG; return;
                }

            fwrite(TapeHeader, 1, 8, CasStream);
            R->AF.B.l &= ~C_FLAG;
        }

        return;
    }

    case 0x00ED:
        /** TAPOUT: Write tape ******************************************
        ****************************************************************/
        R->AF.B.l |= C_FLAG;

        if (CasStream)
        {
            fputc(R->AF.B.h, CasStream);
            R->AF.B.l &= ~C_FLAG;
        }

        return;

    case 0x00F0:
        /** TAPOOF: *****************************************************
        ****************************************************************/
        R->AF.B.l &= ~C_FLAG;
        return;

    case 0x00F3:
        /** STMOTR: *****************************************************
        ****************************************************************/
        R->AF.B.l &= ~C_FLAG;
        return;

    default:
        printf("Unknown BIOS trap called at PC=%04Xh\n", R->PC.W - 2);

    }
}
#endif // _3DS
