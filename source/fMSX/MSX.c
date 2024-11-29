/** fMSX: portable MSX emulator ******************************/
/**                                                         **/
/**                          MSX.c                          **/
/**                                                         **/
/** This file contains implementation for the MSX-specific  **/
/** hardware: slots, memory mapper, PPIs, VDP, PSG, clock,  **/
/** etc. Initialization code and definitions needed for the **/
/** machine-dependent drivers are also here.                **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1994-2020                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/

/** NINTENDO 3DS Version modified by h.tomioka 2023         **/

#ifndef BYTE_TYPE_DEFINED
#define BYTE_TYPE_DEFINED
typedef unsigned char byte;
#endif
#ifndef WORD_TYPE_DEFINED
#define WORD_TYPE_DEFINED
typedef unsigned short word;
#endif

#ifdef __cplusplus
extern "C" {
#endif
#include "MSX.h"
#include "Sound.h"
#include "Floppy.h"
#include "SHA1.h"
#include "MCF.h"
#ifdef __cplusplus
}
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>

#ifdef _3DS
#include <3ds.h>
#include "3DSLib.h"
#include "FDIDisk.h"

#include "SDL/SDL.h"
#endif // _3DS

#ifdef __BORLANDC__
#include <dir.h>
#endif

#ifdef __WATCOMC__
#include <direct.h>
#endif

#ifdef ZLIB
//#include <zlib.h>
#endif

#ifdef ANDROID
#include "MemFS.h"
#define puts   LOGI
#define printf LOGI
#endif

#ifdef DEBUG_LOG
#define PRINTOK           if(Verbose) fputs("OK", debugFile)
#define PRINTFAILED       if(Verbose) fputs("FAILED",debugFile)
#define PRINTRESULT(R)    if(Verbose) fputs((R)? "OK":"FAILED", debugFile)
#else
#define PRINTOK           if(Verbose) puts("OK")
#define PRINTFAILED       if(Verbose) puts("FAILED")
#define PRINTRESULT(R)    if(Verbose) puts((R)? "OK":"FAILED")
#endif // DEBUG_LOG

#define RGB2INT(R,G,B)    ((B)|((int)(G)<<8)|((int)(R)<<16))

/* MSDOS chdir() is broken and has to be replaced :( */
#ifdef MSDOS
#include "LibMSDOS.h"
#define chdir(path) ChangeDir(path)
#endif

/** User-defined parameters for fMSX *************************/
#ifdef TURBO_R
//int  Mode        = MSX_MSX2P|MSX_NTSC|MSX_MSXDOS2|MSX_GUESSA|MSX_GUESSB;
int  Mode = MSX_MSX2P | MSX_NTSC | MSX_GUESSA | MSX_GUESSB;
byte Verbose = 0;              /* Debug msgs ON/OFF      */
byte UPeriod = 100;             /* % of frames to draw    */
int  VPeriod = CPU_VPERIOD;    /* CPU cycles per VBlank  */
int  HPeriod = CPU_HPERIOD;    /* CPU cycles per HBlank  */
int  RAMPages = 32;              /* Number of RAM pages    */
int  VRAMPages = 8;              /* Number of VRAM pages   */
#else
int  Mode = MSX_MSX2 | MSX_NTSC | MSX_MSXDOS2 | MSX_GUESSA | MSX_GUESSB;
byte Verbose = 1;              /* Debug msgs ON/OFF      */
byte UPeriod = 75;             /* % of frames to draw    */
int  VPeriod = CPU_VPERIOD;    /* CPU cycles per VBlank  */
int  HPeriod = CPU_HPERIOD;    /* CPU cycles per HBlank  */
int  RAMPages = 4;              /* Number of RAM pages    */
int  VRAMPages = 2;              /* Number of VRAM pages   */
#endif // TURBO_R
byte ExitNow = 0;              /* 1 = Exit the emulator  */

/** Main hardware: CPU, RAM, VRAM, mappers *******************/
Z80 CPU;                           /* Z80 CPU state and regs */

byte* VRAM, * VPAGE;                 /* Video RAM              */

byte* RAM[8];                      /* Main RAM (8x8kB pages) */
byte* EmptyRAM;                    /* Empty RAM page (8kB)   */
byte SaveCMOS;                     /* Save CMOS.ROM on exit  */
byte* MemMap[4][4][8];   /* Memory maps [PPage][SPage][Addr] */

byte* RAMData;                     /* RAM Mapper contents    */
byte RAMMapper[4];                 /* RAM Mapper state       */
byte RAMMask;                      /* RAM Mapper mask        */

byte* ROMData[MAXSLOTS];           /* ROM Mapper contents    */
byte ROMMapper[MAXSLOTS][4];       /* ROM Mappers state      */
byte ROMMask[MAXSLOTS];            /* ROM Mapper masks       */
byte ROMType[MAXSLOTS];            /* ROM Mapper types       */

byte EnWrite[4];                   /* 1 if write enabled     */
byte PSL[4], SSL[4];                /* Lists of current slots */
byte PSLReg, SSLReg[4];   /* Storage for A8h port and (FFFFh) */

/** Memory blocks to free in TrashMSX() **********************/
void* Chunks[MAXCHUNKS];           /* Memory blocks to free  */
int NChunks;                       /* Number of memory blcks */

/** Working directory names **********************************/
const char* ProgDir = 0;           /* Program directory      */
const char* WorkDir;               /* Working directory      */

/** Cartridge files used by fMSX *****************************/
const char* ROMName[MAXCARTS] = { "CARTA.ROM","CARTB.ROM" };

/** On-cartridge SRAM data ***********************************/
char* SRAMName[MAXSLOTS] = { 0,0,0,0,0,0 };/* Filenames (gen-d)*/
byte SaveSRAM[MAXSLOTS] = { 0,0,0,0,0,0 }; /* Save SRAM on exit*/
byte* SRAMData[MAXSLOTS];          /* SRAM (battery backed)  */

/** Disk images used by fMSX *********************************/
const char* DSKName[MAXDRIVES] = { "DRIVEA.DSK","DRIVEB.DSK" };

/** Soundtrack logging ***************************************/
const char* SndName = "LOG.MID";   /* Sound log file         */

/** Emulation state saving ***********************************/
const char* STAName = "DEFAULT.STA";/* State file (autogen-d)*/

/** Fixed font used by fMSX **********************************/
const char* FNTName = "DEFAULT.FNT"; /* Font file for text   */
byte* FontBuf;                     /* Font for text modes    */

/** Printer **************************************************/
const char* PrnName = 0;           /* Printer redirect. file */
FILE* PrnStream;

/** Cassette tape ********************************************/
const char* CasName = "DEFAULT.CAS";  /* Tape image file     */
FILE* CasStream;

/** Serial port **********************************************/
const char* ComName = 0;           /* Serial redirect. file  */
FILE* ComIStream;
FILE* ComOStream;

/** Kanji font ROM *******************************************/
byte* Kanji;                       /* Kanji ROM 4096x32      */
int  KanLetter;                    /* Current letter index   */
byte KanCount;                     /* Byte count 0..31       */

/** Keyboard, joystick, and mouse ****************************/
volatile byte KeyState[16];        /* Keyboard map state     */
#ifndef _3DS
word JoyState;                     /* Joystick states        */
#endif
int  MouState[2];                  /* Mouse states           */
byte MouseDX[2], MouseDY[2];        /* Mouse offsets          */
byte OldMouseX[2], OldMouseY[2];    /* Old mouse coordinates  */
byte MCount[2];                    /* Mouse nibble counter   */

/** General I/O registers: i8255 *****************************/
I8255 PPI;                         /* i8255 PPI at A8h-ABh   */
byte IOReg;                        /* Storage for AAh port   */

/** Disk controller: WD1793 **********************************/
WD1793 FDC;                        /* WD1793 at 7FF8h-7FFFh  */
FDIDisk FDD[4];                    /* Floppy disk images     */

/** Sound hardware: PSG, SCC, OPLL ***************************/
AY8910 PSG;                        /* PSG registers & state  */
YM2413 OPLL;                       /* OPLL registers & state */
SCC  SCChip;                       /* SCC registers & state  */
byte SCCOn[2];                     /* 1 = SCC page active    */
word FMPACKey;                     /* MAGIC = SRAM active    */

/** Serial I/O hardware: i8251+i8253 *************************/
I8251 SIO;                         /* SIO registers & state  */

/** Real-time clock ******************************************/
byte RTCReg, RTCMode;               /* RTC register numbers   */
byte RTC[4][13];                   /* RTC registers          */

/** Video processor ******************************************/
byte* ChrGen, * ChrTab, * ColTab;      /* VDP tables (screen)    */
byte* SprGen, * SprTab;              /* VDP tables (sprites)   */
int  ChrGenM, ChrTabM, ColTabM;      /* VDP masks (screen)     */
int  SprTabM;                      /* VDP masks (sprites)    */
word VAddr;                        /* VRAM address in VDP    */
byte VKey, PKey;                    /* Status keys for VDP    */
byte FGColor, BGColor;              /* Colors                 */
byte XFGColor, XBGColor;            /* Second set of colors   */
byte ScrMode;                      /* Current screen mode    */
byte VDP[64], VDPStatus[16];        /* VDP registers          */
byte IRQPending;                   /* Pending interrupts     */
int  ScanLine;                     /* Current scanline       */
byte VDPData;                      /* VDP data buffer        */
byte PLatch;                       /* Palette buffer         */
byte ALatch;                       /* Address buffer         */
int  Palette[16];                  /* Current palette        */

/** Cheat entries ********************************************/
int MCFCount = 0;              /* Size of MCFEntries[]   */
MCFEntry MCFEntries[MAXCHEATS];    /* Entries from .MCF file */

/** Cheat codes **********************************************/
byte CheatsON = 0;              /* 1: Cheats are on       */
int  CheatCount = 0;              /* # cheats, <=MAXCHEATS  */
CheatCode CheatCodes[MAXCHEATS];

/** Places in DiskROM to be patched with ED FE C9 ************/
static const word DiskPatches[] =
{ 0x4010,0x4013,0x4016,0x401C,0x401F,0 };

/** Places in BIOS to be patched with ED FE C9 ***************/
static const word BIOSPatches[] =
#ifdef TURBO_R
{ 0x00DE, 0x00E1,0x00E4,0x00E7,0x00EA,0x00ED,0x00F0,0x00F3,0 };
#else
{ 0x00E1, 0x00E4, 0x00E7, 0x00EA, 0x00ED, 0x00F0, 0x00F3, 0 };
#endif // TURBO_R

/** Cartridge map, by primary and secondary slots ************/
static const byte CartMap[4][4] =
//{ { 255,3,4,5 },{ 0,0,0,0 },{ 1,255,6,7 },{ 2,8,9,255 } };
#ifdef TURBO_R
//{ { 255, 3, 4, 5 }, { 0,0,0,0 }, { 1,6,7,8 }, { 2,9,255,255 } };
//{ { 255,3,4,5 },{ 0,0,0,0 },{ 1,6,7,8 },{ 2,255,255,255 } };
{ { 255,3,4,5 },{ 0,0,0,0 },{ 1,255,6,7 },{ 2,8,255,255 } };
//{ { 255,3,4,5 },{ 0,0,0,0 },{ 1,255,6,7 },{ 2,8,255,9 } };

//{ { 255,3,4,5 },{ 0,0,0,0 },{ 1,6,255,7 },{ 2,255,255,255 } };
#else
{ { 255, 3, 4, 5 }, { 0,0,0,0 }, { 1,1,1,1 }, { 2,255,255,255 } };
#endif // TURBO_R

/** Screen Mode Handlers [number of screens + 1] *************/
void (*RefreshLine[MAXSCREEN + 2])(byte Y) =
{
  RefreshLine0,   /* SCR 0:  TEXT 40x24  */
  RefreshLine1,   /* SCR 1:  TEXT 32x24  */
  RefreshLine2,   /* SCR 2:  BLK 256x192 */
  RefreshLine3,   /* SCR 3:  64x48x16    */
  RefreshLine4,   /* SCR 4:  BLK 256x192 */
  RefreshLine5,   /* SCR 5:  256x192x16  */
  RefreshLine6,   /* SCR 6:  512x192x4   */
  RefreshLine7,   /* SCR 7:  512x192x16  */
  RefreshLine8,   /* SCR 8:  256x192x256 */
  0,              /* SCR 9:  NONE        */
  RefreshLine10,  /* SCR 10: YAE 256x192 */
  RefreshLine10,  /* SCR 11: YAE 256x192 */
  RefreshLine12,  /* SCR 12: YJK 256x192 */
  RefreshLineTx80 /* SCR 0:  TEXT 80x24  */
};

/** VDP Address Register Masks *******************************/
static const struct { byte R2, R3, R4, R5, M2, M3, M4, M5; } MSK[MAXSCREEN + 2] =
{
  { 0x7F,0x00,0x3F,0x00,0x00,0x00,0x00,0x00 }, /* SCR 0:  TEXT 40x24  */
  { 0x7F,0xFF,0x3F,0xFF,0x00,0x00,0x00,0x00 }, /* SCR 1:  TEXT 32x24  */
  { 0x7F,0x80,0x3C,0xFF,0x00,0x7F,0x03,0x00 }, /* SCR 2:  BLK 256x192 */
  { 0x7F,0x00,0x3F,0xFF,0x00,0x00,0x00,0x00 }, /* SCR 3:  64x48x16    */
  { 0x7F,0x80,0x3C,0xFC,0x00,0x7F,0x03,0x03 }, /* SCR 4:  BLK 256x192 */
  { 0x60,0x00,0x00,0xFC,0x1F,0x00,0x00,0x03 }, /* SCR 5:  256x192x16  */
  { 0x60,0x00,0x00,0xFC,0x1F,0x00,0x00,0x03 }, /* SCR 6:  512x192x4   */
  { 0x20,0x00,0x00,0xFC,0x1F,0x00,0x00,0x03 }, /* SCR 7:  512x192x16  */
  { 0x20,0x00,0x00,0xFC,0x1F,0x00,0x00,0x03 }, /* SCR 8:  256x192x256 */
  { 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 }, /* SCR 9:  NONE        */
  { 0x20,0x00,0x00,0xFC,0x1F,0x00,0x00,0x03 }, /* SCR 10: YAE 256x192 */
  { 0x20,0x00,0x00,0xFC,0x1F,0x00,0x00,0x03 }, /* SCR 11: YAE 256x192 */
  { 0x20,0x00,0x00,0xFC,0x1F,0x00,0x00,0x03 }, /* SCR 12: YJK 256x192 */
  { 0x7C,0xF8,0x3F,0x00,0x03,0x07,0x00,0x00 }  /* SCR 0:  TEXT 80x24  */
};

#ifdef TURBO_R
char DiskWrited[] = { 0,0 };
char DiskAccess[] = { 0,0 };
byte IsZeminaSram = 0;
int VRAMPageInt = 0;
byte* SRAMData32[MAXSLOTS][4];
byte* ExternalRAMData[MAXCARTS];
byte NewScrMode;
int firstScanLine = 0;
unsigned char IsShowFPS = 1;
unsigned char IsHardReset = 1;
unsigned char UseInterlace = 0;
#ifdef UPD_FDC
unsigned char FDCEmuType = 0;
#endif // UPD_FDC
int regionid = 0;
int cbiosReg = 0;
unsigned char ForceCBIOS = 0;
unsigned char ForceJPBIOS = 1;
unsigned char ReloadBIOS = 0;
unsigned char UseCBIOS = 0;
unsigned char NeedRest = 1;
unsigned char SkipBIOS = 1;
unsigned char IsSpriteColi = 0;
int currScanLine = 0;
int audioCnt = 0;
unsigned char IsJpKey = 1;
unsigned char KeyRegion = 0;
unsigned char IsJPKeyBIOS = 1;
unsigned char InvertF4Reg = 0;
unsigned char SCCEnhanced = 0;
unsigned char ForceSCCP = 0;
unsigned char AccurateAudioSync = 0;
unsigned char waitStep = 3;
byte* Kanji2;                       /* Kanji ROM Level 2 4096x32                  */
byte OldROMType[MAXSLOTS];
byte ROMLen[MAXSLOTS];
byte AllowSCC[2] = { 1, 1 };

int currJoyMode[2] = { 1, 0 };
int JoyMode[2] = { 1,0 };
int MouseDX3DS[2] = { 0,0 };
int MouseDY3DS[2] = { 0,0 };
int PrinterMode = 0;
int  PaddleState[2];
int PaddleShift[2];
byte OldPaddleVal;
byte PadCount[2];
byte HasKanjiBASIC = 0;
byte OldBIOS_GETPAD[3] = { 0,0,0 };

byte* PACData;
byte PACMapper[MAXCARTS] = { 0,0 };
byte* PACSaveData;
byte SaveSRAMPAC = 0;
int IsSCCRAM[MAXCARTS][4];
byte IsMapped[MAXCARTS][4];
byte SCCMode[MAXCARTS];
int VDPDelay = 0;
int delayedLine = 0;
byte PrinterStatus = 0;
int PrinterValue = 0;
int PrinterValue2 = 0;
int adpcmpos;
byte ADPCMData[224];
byte ScreenModeReg = 0xFF;      /* Screen Mode register for MSX2+ MSXTurboR. */

/* Scale Index Table for +PCM */
static const int SI_Table[16] =
{
    -3,-2,-1,0,0,1,2,3,
    0,3,2,1,0,0,-1,-2
};

/* ADPCM Data Table for +PCM */
static const byte PCM_Table[256] =
{
    0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0xF8,0xF9,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF,
    0x00,0x01,0x03,0x04,0x05,0x06,0x08,0x09,0xF6,0xF7,0xF8,0xFA,0xFB,0xFC,0xFD,0xFF,
    0x00,0x02,0x03,0x05,0x06,0x08,0x0A,0x0B,0xF3,0xF5,0xF6,0xF8,0xFA,0xFB,0xFD,0xFE,
    0x00,0x02,0x04,0x06,0x08,0x0A,0x0C,0x0E,0xF0,0xF2,0xF4,0xF6,0xF8,0xFA,0xFC,0xFE,
    0x00,0x03,0x05,0x08,0x0A,0x0D,0x0F,0x12,0xEC,0xEE,0xF1,0xF3,0xF6,0xF8,0xFB,0xFD,
    0x00,0x03,0x06,0x0A,0x0D,0x10,0x13,0x16,0xE7,0xEA,0xED,0xF0,0xF3,0xF6,0xFA,0xFD,
    0x00,0x04,0x08,0x0C,0x10,0x14,0x18,0x1C,0xE0,0xE4,0xE8,0xEC,0xF0,0xF4,0xF8,0xFC,
    0x00,0x05,0x0A,0x0F,0x14,0x19,0x1E,0x23,0xD8,0xDD,0xE2,0xE7,0xEC,0xF1,0xF6,0xFB,
    0x00,0x06,0x0D,0x13,0x19,0x20,0x26,0x2C,0xCD,0xD4,0xDA,0xE0,0xE7,0xED,0xF3,0xFA,
    0x00,0x08,0x10,0x18,0x20,0x28,0x30,0x38,0xC0,0xC8,0xD0,0xD8,0xE0,0xE8,0xF0,0xF8,
    0x00,0x0A,0x14,0x1E,0x28,0x32,0x3C,0x47,0xAF,0xB9,0xC4,0xCE,0xD8,0xE2,0xEC,0xF6,
    0x00,0x0D,0x19,0x26,0x33,0x3F,0x4C,0x59,0x9A,0xA7,0xB4,0xC1,0xCD,0xDA,0xE7,0xF3,
    0x00,0x10,0x20,0x30,0x40,0x50,0x60,0x70,0x80,0x90,0xA0,0xB0,0xC0,0xD0,0xE0,0xF0,
    0x00,0x14,0x28,0x3C,0x51,0x65,0x79,0x8D,0x5F,0x73,0x87,0x9B,0xAF,0xC4,0xD8,0xEC,
    0x00,0x19,0x33,0x4C,0x66,0x7F,0x98,0xB2,0x35,0x4E,0x68,0x81,0x9A,0xB4,0xCD,0xE7,
    0x00,0x20,0x40,0x60,0x80,0xA0,0xC0,0xE0,0x00,0x20,0x40,0x60,0x80,0xA0,0xC0,0xE0
};

byte S1990Latch;
byte S1990Reg6;
byte R800Pause;
byte PCMStatus;
byte PCMSample;
byte PCMPos;
byte PCMData[8];
byte* MSXDOS23Data;
byte MSXDOS2Mapper;
byte* ViewFontData;
word PanaMapper[8];
byte PanaMapper2;
byte PanaMapper3;
byte* PanasonicSRAM[4];
int R800TimerCnt;
int PCMTimerCnt;
byte PCMTimerReg;
int VDPIOTimerCnt;
unsigned char  isLoadDer = 0;
unsigned char* derBuf;
#define UpdateVDPDelay()        if(CPU.User & 0x01){                    \
                                if ((VDPIOTimerCnt - CPU.ICount) < 48)CPU.ICount = VDPIOTimerCnt - 60;  \
                                VDPIOTimerCnt = CPU.ICount;}
#endif // TURBO_R

#ifdef UPD_FDC
TC8566AF TCFDC;
int DiskChanged[2];
#endif // UPD_FDC


#ifdef VDP_V9990
typedef struct
{
    byte Active;
    byte SelectReg;
    byte SystemControlReg;
    byte IncVRAMWrite;
    byte IncVRAMRead;
    byte IncPaletteWrite;
    byte pending;
    uint VRAMReadInt;
    uint VRAMWriteInt;
}V9990;
V9990 vdpV9990;
unsigned char V9990VDP[64];
unsigned char V9990Port[16];
unsigned char V9KPal[256];

unsigned char UseV9990 = 0;
unsigned char V9990Active = 0;
unsigned char V9990Dual = 0;
unsigned char V9990DualScreen = 0;
int V9KScanLine = 0;
int V9KcurrLine = 0;
int V9kFirstLine = 0;
int V9KImgWidth = 256;
int V9KImgHeight = 2048;
unsigned int V9KYScrollOff = 0;
unsigned int V9KYScrollOff2 = 0;
unsigned int V9KYScrollOffB = 0;
unsigned int V9KYScrollOffB2 = 0;

byte V9KVDP8Val = 0;
byte V9KScrMode = 0;
byte V9KType = 9;
byte V9KPixelRes = 1;
byte* V9KVRAM;

void V9990Out(register byte R, register byte V);
byte SetScreenV9990(void);
#endif // VDP_V9990

#ifdef USE_OVERCLOCK
unsigned char overClockRatio = 0;
#endif // USE_OVERCLOCK

#ifdef AUDIO_SYNC
int audioCycleCnt = 0;
#endif // AUDIO_SYNC

#if defined(HDD_NEXTOR) || defined(HDD_IDE) || defined(MEGASCSI_HD)
FDIDisk HDD[2];
FILE* HDDStream;
int HDDSize;
char HDDWrited = 0;
#endif // HDD_NEXTOR    HDD_IDE     MEGASCSI_HD
#ifdef MEGASCSI_HD
MB89352A spc;
byte ReadMEGASCSI(word A);
#endif // MEGASCSI_HD

#ifdef LOG_ERROR
const char* ErrorChar;
#endif // LOG_ERROR

#ifdef DEBUG_LOG
FILE* debugFile;
char* debugBuf;
size_t debugBufSize;
#endif // DEBUG_LOG


/** MegaROM Mapper Names *************************************/
static const char* ROMNames[MAXMAPPERS + 1] =
{
  "GENERIC/8kB","GENERIC/16kB","KONAMI5/8kB",
  "KONAMI4/8kB","ASCII/8kB","ASCII/16kB",
#ifdef _3DS
  "GMASTER2/SRAM","FMPAC/SRAM","ASCII/16kBKOEI",
  "ASCII/16kBAlt","CrossBlaim","Harryfox",
  "Zemina","ZeminaDS2","XXin1 ",
  "126in1","MSX90","SWANGI",
  "DOOLY","LodeRunner","Pierrot",
  "RType","Wizardry","MANBOW2",
  "MAJUTSUSHI","SCCPLUS","SCCPLUS_2",
  "PlainROM","WingWarrior","ESESCC",
  "SYNTHE","ASCII/16kbBig","UNKNOWN"
#else
  "GMASTER2/SRAM","FMPAC/SRAM","UNKNOWN"
  #endif // _3DS
};

/** Keyboard Mapping *****************************************/
/** This keyboard mapping is used by KBD_SET()/KBD_RES()    **/
/** macros to modify KeyState[] bits.                       **/
/*************************************************************/
const byte Keys[][2] =
{
  { 0,0x00 },{ 8,0x10 },{ 8,0x20 },{ 8,0x80 }, /* None,LEFT,UP,RIGHT */
  { 8,0x40 },{ 6,0x01 },{ 6,0x02 },{ 6,0x04 }, /* DOWN,SHIFT,CONTROL,GRAPH */
  { 7,0x20 },{ 7,0x08 },{ 6,0x08 },{ 7,0x40 }, /* BS,TAB,CAPSLOCK,SELECT */
  { 8,0x02 },{ 7,0x80 },{ 8,0x08 },{ 8,0x04 }, /* HOME,ENTER,DELETE,INSERT */
  { 6,0x10 },{ 7,0x10 },{ 6,0x20 },{ 6,0x40 }, /* COUNTRY,STOP,F1,F2 */
  { 6,0x80 },{ 7,0x01 },{ 7,0x02 },{ 9,0x08 }, /* F3,F4,F5,PAD0 */
  { 9,0x10 },{ 9,0x20 },{ 9,0x40 },{ 7,0x04 }, /* PAD1,PAD2,PAD3,ESCAPE */
  { 9,0x80 },{ 10,0x01 },{ 10,0x02 },{ 10,0x04 }, /* PAD4,PAD5,PAD6,PAD7 */
  { 8,0x01 },{ 0,0x02 },{ 2,0x01 },{ 0,0x08 }, /* SPACE,[!],["],[#] */
  { 0,0x10 },{ 0,0x20 },{ 0,0x80 },{ 2,0x01 }, /* [$],[%],[&],['] */
  { 1,0x02 },{ 0,0x01 },{ 1,0x01 },{ 1,0x08 }, /* [(],[)],[*],[=] */
  { 2,0x04 },{ 1,0x04 },{ 2,0x08 },{ 2,0x10 }, /* [,],[-],[.],[/] */
  { 0,0x01 },{ 0,0x02 },{ 0,0x04 },{ 0,0x08 }, /* 0,1,2,3 */
  { 0,0x10 },{ 0,0x20 },{ 0,0x40 },{ 0,0x80 }, /* 4,5,6,7 */
  { 1,0x01 },{ 1,0x02 },{ 1,0x80 },{ 1,0x80 }, /* 8,9,[:],[;] */
  { 2,0x04 },{ 1,0x08 },{ 2,0x08 },{ 2,0x10 }, /* [<],[=],[>],[?] */
  { 0,0x04 },{ 2,0x40 },{ 2,0x80 },{ 3,0x01 }, /* [@],A,B,C */
  { 3,0x02 },{ 3,0x04 },{ 3,0x08 },{ 3,0x10 }, /* D,E,F,G */
  { 3,0x20 },{ 3,0x40 },{ 3,0x80 },{ 4,0x01 }, /* H,I,J,K */
  { 4,0x02 },{ 4,0x04 },{ 4,0x08 },{ 4,0x10 }, /* L,M,N,O */
  { 4,0x20 },{ 4,0x40 },{ 4,0x80 },{ 5,0x01 }, /* P,Q,R,S */
  { 5,0x02 },{ 5,0x04 },{ 5,0x08 },{ 5,0x10 }, /* T,U,V,W */
  { 5,0x20 },{ 5,0x40 },{ 5,0x80 },{ 1,0x20 }, /* X,Y,Z,[[] */
#ifdef _3DS
  { 1,0x10 },{ 1,0x40 },{ 0,0x40 },{ 2,0x20 }, /* [\],[]],[^],[(Japanese charactor 0xFB)]*/
#else
  { 1,0x10 },{ 1,0x40 },{ 0,0x40 },{ 1,0x04 }, /* [\],[]],[^],[_] */
  #endif // _3DS
  { 2,0x02 },{ 2,0x40 },{ 2,0x80 },{ 3,0x01 }, /* [`],a,b,c */
  { 3,0x02 },{ 3,0x04 },{ 3,0x08 },{ 3,0x10 }, /* d,e,f,g */
  { 3,0x20 },{ 3,0x40 },{ 3,0x80 },{ 4,0x01 }, /* h,i,j,k */
  { 4,0x02 },{ 4,0x04 },{ 4,0x08 },{ 4,0x10 }, /* l,m,n,o */
  { 4,0x20 },{ 4,0x40 },{ 4,0x80 },{ 5,0x01 }, /* p,q,r,s */
  { 5,0x02 },{ 5,0x04 },{ 5,0x08 },{ 5,0x10 }, /* t,u,v,w */
  { 5,0x20 },{ 5,0x40 },{ 5,0x80 },{ 1,0x20 }, /* x,y,z,[{] */
  { 1,0x10 },{ 1,0x40 },{ 2,0x02 },{ 8,0x08 }, /* [|],[}],[~],DEL */
  { 10,0x08 },{ 10,0x10 }                      /* PAD8,PAD9 */
#ifdef _3DS
  ,{ 9,0x01},{9,0x02},{9,0x04},{10,0x80},      /* PAD[*],PAD[+],PAD[/],PAD[.]*/
   {10,0x40},{10,0x20}                       /* PAD[,],PAD[-] */
#endif // _3DS
};

/** Internal Functions ***************************************/
/** These functions are defined and internally used by the  **/
/** code in MSX.c.                                          **/
/*************************************************************/
byte* LoadROM(const char* Name, int Size, byte* Buf);
int  GuessROM(const byte* Buf, int Size);
int  FindState(const char* Name);
void SetMegaROM(int Slot, byte P0, byte P1, byte P2, byte P3);
void MapROM(word A, byte V);       /* Switch MegaROM banks            */
void PSlot(byte V);               /* Switch primary slots            */
void SSlot(byte V);               /* Switch secondary slots          */
void VDPOut(byte R, byte V);       /* Write value into a VDP register */
void Printer(byte V);             /* Send a character to a printer   */
void PPIOut(byte New, byte Old);   /* Set PPI bits (key click, etc.)  */
#ifndef _3DS
int  CheckSprites(void);          /* Check for sprite collisions     */
#endif // !_3DS
byte RTCIn(byte R);               /* Read RTC registers              */
byte SetScreen(void);             /* Change screen mode              */
word SetIRQ(byte IRQ);            /* Set/Reset IRQ                   */
word StateID(void);               /* Compute emulation state ID      */
int  ApplyCheats(void);           /* Apply RAM-based cheats          */

static int hasext(const char* FileName, const char* Ext);
static byte* GetMemory(int Size); /* Get memory chunk                */
static void FreeMemory(const void* Ptr); /* Free memory chunk        */
static void FreeAllMemory(void);  /* Free all memory chunks          */

/** hasext() *************************************************/
/** Check if file name has given extension.                 **/
/*************************************************************/
static int hasext(const char* FileName, const char* Ext)
{
    const char* P;
    int J;

    /* Start searching from the end, terminate at directory name */
    for (P = FileName + strlen(FileName); (P >= FileName) && (*P != '/') && (*P != '\\'); --P)
    {
        /* Locate start of the next extension */
        for (--P; (P >= FileName) && (*P != '/') && (*P != '\\') && (*P != *Ext); --P);
        /* If next extension not found, return FALSE */
        if ((P < FileName) || (*P == '/') || (*P == '\\')) return(0);
        /* Compare with the given extension */
        for (J = 0; P[J] && Ext[J] && (toupper(P[J]) == toupper(Ext[J])); ++J);
        /* If extension matches, return TRUE */
        if (!Ext[J] && (!P[J] || (P[J] == *Ext))) return(1);
    }

    /* Extensions do not match */
    return(0);
}

/** GetMemory() **********************************************/
/** Allocate a memory chunk of given size using malloc().   **/
/** Store allocated address in Chunks[] for later disposal. **/
/*************************************************************/
static byte* GetMemory(int Size)
{
    byte* P;

    if ((Size <= 0) || (NChunks >= MAXCHUNKS)) return(0);
    P = (byte*)malloc(Size);
    if (P) Chunks[NChunks++] = P;

    return(P);
}

/** FreeMemory() *********************************************/
/** Free memory allocated by a previous GetMemory() call.   **/
/*************************************************************/
static void FreeMemory(const void* Ptr)
{
    int J;

    /* Special case: we do not free EmptyRAM! */
    if (!Ptr || (Ptr == (void*)EmptyRAM)) return;

    for (J = 0; (J < NChunks) && (Ptr != Chunks[J]); ++J);
    if (J < NChunks)
    {
        free(Chunks[J]);
        for (--NChunks; J < NChunks; ++J) Chunks[J] = Chunks[J + 1];
    }
}

/** FreeAllMemory() ******************************************/
/** Free all memory allocated by GetMemory() calls.         **/
/*************************************************************/
static void FreeAllMemory(void)
{
    int J;

    for (J = 0; J < NChunks; ++J) free(Chunks[J]);
    NChunks = 0;
}

/** StartMSX() ***********************************************/
/** Allocate memory, load ROM images, initialize hardware,  **/
/** CPU and start the emulation. This function returns 0 in **/
/** the case of failure.                                    **/
/*************************************************************/
int StartMSX(int NewMode, int NewRAMPages, int NewVRAMPages)
{
    /*** Joystick types: ***/
    static const char* JoyTypes[] =
    {
      "nothing","normal joystick",
      "mouse in joystick mode","mouse in real mode"
    };

    /*** CMOS ROM default values: ***/
    static const byte RTCInit[4][13] =
    {
      {  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
      {  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
      {  0, 0, 0, 0,40,80,15, 4, 4, 0, 0, 0, 0 },
      {  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
    };

    int* T, I, J, K;
    byte* P;
    word A;

    /*** STARTUP CODE starts here: ***/

    T = (int*)"\01\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
#ifdef LSB_FIRST
    if (*T != 1)
    {
        printf("********** This machine is high-endian. **********\n");
        printf("Take #define LSB_FIRST out and compile fMSX again.\n");
        return(0);
    }
#else
    if (*T == 1)
    {
        printf("********* This machine is low-endian. **********\n");
        printf("Insert #define LSB_FIRST and compile fMSX again.\n");
        return(0);
    }
#endif

    /* Zero everyting */
    CasStream = PrnStream = ComIStream = ComOStream = 0;
    FontBuf = 0;
    RAMData = 0;
    VRAM = 0;
    Kanji = 0;
    WorkDir = 0;
    SaveCMOS = 0;
    FMPACKey = 0x0000;
    ExitNow = 0;
    NChunks = 0;
    CheatsON = 0;
    CheatCount = 0;
    MCFCount = 0;

    /* Zero cartridge related data */
    for (J = 0; J < MAXSLOTS; ++J)
    {
        ROMMask[J] = 0;
        ROMData[J] = 0;
        ROMType[J] = 0;
        SRAMData[J] = 0;
        SRAMName[J] = 0;
        SaveSRAM[J] = 0;
    }

    /* UPeriod has ot be in 1%..100% range */
    UPeriod = UPeriod < 1 ? 1 : UPeriod>100 ? 100 : UPeriod;

    /* Allocate 16kB for the empty space (scratch RAM) */
    if (Verbose) printf("Allocating 16kB for empty space...\n");
    if (!(EmptyRAM = GetMemory(0x4000))) { PRINTFAILED; return(0); }
    memset(EmptyRAM, NORAM, 0x4000);

    /* Reset memory map to the empty space */
    for (I = 0; I < 4; ++I)
        for (J = 0; J < 4; ++J)
            for (K = 0; K < 8; ++K)
                MemMap[I][J][K] = EmptyRAM;

    /* Save current directory */
    if (ProgDir && (WorkDir = getcwd(0, 1024))) Chunks[NChunks++] = (void*)WorkDir;

    /* Set invalid modes and RAM/VRAM sizes before calling ResetMSX() */
    Mode = ~NewMode;
    RAMPages = 0;
    VRAMPages = 0;

    /* Try resetting MSX, allocating memory, loading ROMs */
#ifdef TURBO_R
    if ((ResetMSX(NewMode, NewRAMPages, NewVRAMPages) ^ NewMode) & MSX_MODEL)
    {
        /* If you missing MSXTurboR BIOS, change optionitem "MSX Model" to MSX2+. This fixes crash on startup. */
        if ((NewMode & MSX_MODEL) == MSX_MSXTR)
        {
            ChangeOptionMSXModel("MSX Model", 2);
            //NewMode = MSX_MSX2P;
            //ResetMSX(NewMode, NewRAMPages, NewVRAMPages);
        }
        return(0);
    }
#else
    if ((ResetMSX(NewMode, NewRAMPages, NewVRAMPages) ^ NewMode) & MSX_MODEL) return(0);
#endif // TURBO_R
    if (!RAMPages || !VRAMPages) return(0);

    /* Change to the program directory */
    if (ProgDir && chdir(ProgDir))
    {
        if (Verbose) printf("Failed changing to '%s' directory!\n", ProgDir);
    }

    /* Try loading font */
    if (FNTName)
    {
        if (Verbose) printf("Loading %s font...", FNTName);
        J = LoadFNT(FNTName);
        PRINTRESULT(J);
    }

    if (Verbose) printf("Loading optional ROMs: ");

    /* Try loading CMOS memory contents */
    if (LoadROM("CMOS.ROM", sizeof(RTC), (byte*)RTC))
    {
        if (Verbose) printf("CMOS.ROM..");
    }
    else memcpy(RTC, RTCInit, sizeof(RTC));

    //memcpy(RTC, RTCInit, sizeof(RTC));

    /* Try loading Kanji alphabet ROM */
#ifdef _3DS
    LoadKanjiROM();
#else
    if ((Kanji = LoadROM("KANJI.ROM", 0x20000, 0)))
    {
        if (Verbose) printf("KANJI.ROM..");
    }

    /* Try loading RS232 support ROM to slot */
    if ((P = LoadROM("RS232.ROM", 0x4000, 0)))
    {
        if (Verbose) printf("RS232.ROM..");
        MemMap[3][3][2] = P;
        MemMap[3][3][3] = P + 0x2000;
    }
#endif // _3DS

    PRINTOK;

    /* Start loading system cartridges */
    J = MAXCARTS;

    //  /* If MSX2 or better and DiskROM present...  */
    //  /* ...try loading MSXDOS2 cartridge into 3:0 */
#ifndef _3DS
    /* If MSX2 or better and DiskROM present...  */
    /* ...try loading MSXDOS2 cartridge into 3:0 */
    if (!MODEL(MSX_MSX1) && OPTION(MSX_MSXDOS2) && (MemMap[3][1][2] != EmptyRAM) && !ROMData[2])
        if (LoadCart("MSXDOS2.ROM", 2, MAP_GEN16))
            SetMegaROM(2, 0, 1, ROMMask[J] - 1, ROMMask[J]);
#endif // !_3DS

#ifdef _MSX0
    if (UseMSX0)
    {
        NeedRest = 0;
        LoadCart("fakeiot.rom", 3, MAP_GUESS);
        if (LoadXBASIC)
        {
            if(!LoadCart("MSX0_XBASIC.ROM", 1, MAP_GUESS))LoadCart("XBASIC2.rom", 1, MAP_GUESS);
        }
        NeedRest = 1;
    }
#endif // _MSX0

#ifdef TURBO_R
    if (MODEL(MSX_MSXTR))
    {
        LoadMSXMusicTurboR();
#ifdef UPD_FDC
        ResetTC8566AF(&TCFDC, FDD, TC8566AF_INIT);
        if (Verbose & 0x04)TCFDC.Verbose = 1;
        DiskChanged[0] = DiskChanged[1] = 0;
#endif // UPD_FDC
    }
    else
    {
        J = 4;
        if ((J < MAXSLOTS) && LoadCart("FMPAC.ROM", J, MAP_FMPAC)) ++J;
    }

    J = 5;

    ///* If MSX2 or better, load PAINTER cartridge */
    //if (!MODEL(MSX_MSX1))
    //{
    //    for (; (J < MAXSLOTS) && ROMData[J]; ++J);
    //    if ((J < MAXSLOTS) && LoadCart("PAINTER.ROM", J, 0)) ++J;
    //}
    ///* load MSX Basic-kun/Basic-kun Plus that need for some games made with BASIC. */
    //for (; (J < MAXSLOTS) && ROMData[J]; ++J);
    //if (J < MAXSLOTS)
    //{
    //    if (LoadCart("/FMSX3DS/XBASIC2.ROM", J, MAP_GUESS)) ++J;
    //    else if (LoadCart("/FMSX3DS/XBASIC.ROM", J, MAP_GUESS)) ++J;
    //}

#else
    /* If MSX2 or better, load PAINTER cartridge */
    if (!MODEL(MSX_MSX1))
    {
        for (; (J < MAXSLOTS) && ROMData[J]; ++J);
        if ((J < MAXSLOTS) && LoadCart("PAINTER.ROM", J, 0)) ++J;
    }

    /* Load FMPAC cartridge */
    for (; (J < MAXSLOTS) && ROMData[J]; ++J);
    if ((J < MAXSLOTS) && LoadCart("FMPAC.ROM", J, MAP_FMPAC)) ++J;
#endif // TURBO_R

    /* Load Konami GameMaster2/GameMaster cartridges */
    for (; (J < MAXSLOTS) && ROMData[J]; ++J);
    if (J < MAXSLOTS)
    {
        if (LoadCart("GMASTER2.ROM", J, MAP_GMASTER2)) ++J;
        else if (LoadCart("GMASTER.ROM", J, 0)) ++J;
    }

#ifdef TURBO_R
    LoadFMPAC(2, 1);

    for (J = 0; J < MAXSLOTS; ++J)
    {
        SRAMData32[J][0] = 0;
        SRAMData32[J][1] = 0;
        SRAMData32[J][2] = 0;
        SRAMData32[J][3] = 0;
        OldROMType[J] = 0;
        ROMLen[J] = 0;
    }
    ExternalRAMData[0] = ExternalRAMData[1] = 0;
    IsHardReset = 1;
#endif // TURBO_R

    /* We are now back to working directory */
    if (WorkDir && chdir(WorkDir))
    {
        if (Verbose) printf("Failed changing to '%s' directory!\n", WorkDir);
    }

    /* For each user cartridge slot, try loading cartridge */
#ifdef _3DS
    for (J = 0; J < MAXCARTS; ++J) LoadCart(ROMName[J], J, ROMGUESS(J) | OldROMType[J]);
#else
    for (J = 0; J < MAXCARTS; ++J) LoadCart(ROMName[J], J, ROMGUESS(J) | ROMTYPE(J));

    /* Open stream for a printer */
    if (Verbose)
        printf("Redirecting printer output to %s...OK\n", PrnName ? PrnName : "STDOUT");
    ChangePrinter(PrnName);
#endif // _3DS

    /* Open streams for serial IO */
    if (!ComName) { ComIStream = stdin; ComOStream = stdout; }
    else
    {
        if (Verbose) printf("Redirecting serial I/O to %s...", ComName);
        if (!(ComOStream = ComIStream = fopen(ComName, "r+b")))
        {
            ComIStream = stdin; ComOStream = stdout;
        }
        PRINTRESULT(ComOStream != stdout);
    }

    /* Open casette image */
    if (CasName && ChangeTape(CasName))
        if (Verbose) printf("Using %s as a tape\n", CasName);

    /* Initialize floppy disk controller */
    Reset1793(&FDC, FDD, WD1793_INIT);
    FDC.Verbose = Verbose & 0x04;

    /* Open disk images */
    for (J = 0; J < MAXDRIVES; ++J)
    {
        FDD[J].Verbose = Verbose & 0x04;
        if (ChangeDisk(J, DSKName[J]))
            if (Verbose) printf("Inserting %s into drive %c\n", DSKName[J], J + 'A');
    }

#ifndef ALTSOUND
    /* Initialize sound logging */
    InitMIDI(SndName);
#endif // !ALTSOUND

    /* Done with initialization */
    if (Verbose)
    {
        printf("Initializing VDP, FDC, PSG, OPLL, SCC, and CPU...\n");
        printf("  Attached %s to joystick port A\n", JoyTypes[JOYTYPE(0)]);
        printf("  Attached %s to joystick port B\n", JoyTypes[JOYTYPE(1)]);
        printf("  %d CPU cycles per HBlank\n", HPeriod);
        printf("  %d CPU cycles per VBlank\n", VPeriod);
        printf("  %d scanlines\n", VPeriod / HPeriod);
    }

    /* Start execution of the code */
    if (Verbose) printf("RUNNING ROM CODE...\n");

    A = RunZ80(&CPU);

    /* Exiting emulation... */
    if (Verbose) printf("EXITED at PC = %04Xh.\n", A);
    return(1);
}

/** TrashMSX() ***********************************************/
/** Free resources allocated by StartMSX().                 **/
/*************************************************************/
void TrashMSX(void)
{
    FILE* F;
    int J;

#ifdef  _3DS
    DoAutoSave();
    DoAutoSaveHDD();
    WriteOptionCFG();
    TrashMSX0();
#endif //  _3DS
#ifdef LOG_ERROR
    WriteErrorLog();
#endif // LOG_ERROR
#ifdef DEBUG_LOG
    writeDebugLog();
#endif // DEBUG_LOG

    /* CMOS.ROM is saved in the program directory */
    if (ProgDir && chdir(ProgDir))
    {
        if (Verbose) printf("Failed changing to '%s' directory!\n", ProgDir);
    }

    /* Save CMOS RAM, if present */
    if (SaveCMOS)
    {
        if (Verbose) printf("Writing CMOS.ROM...");
#ifdef _3DS
        if (!(F = zipfopen("CMOS.ROM", "wb"))) SaveCMOS = 0;
#else
        if (!(F = fopen("CMOS.ROM", "wb"))) SaveCMOS = 0;
#endif // _3DS
        else
        {
            if (fwrite(RTC, 1, sizeof(RTC), F) != sizeof(RTC)) SaveCMOS = 0;
            fclose(F);
        }
        PRINTRESULT(SaveCMOS);
    }

    /* Change back to working directory */
    if (WorkDir && chdir(WorkDir))
    {
        if (Verbose) printf("Failed changing to '%s' directory!\n", WorkDir);
    }

    /* Shut down sound logging */
    TrashMIDI();

    /* Eject disks, free disk buffers */
    Reset1793(&FDC, FDD, WD1793_EJECT);

#ifdef UPD_FDC
    if (MODEL(MSX_MSXTR)) ResetTC8566AF(&TCFDC, FDD, TC8566AF_EJECT);
#endif // UPD_FDC

#ifdef HDD_NEXTOR
    /* Eject Hard Disk */
    if (HDDStream) { fclose(HDDStream); HDDStream = 0; };
#endif // HDD_NEXTOR


    /* Close printer output */
    ChangePrinter(0);

    /* Close tape */
    ChangeTape(0);

    /* Close all IO streams */
    if (ComOStream && (ComOStream != stdout)) fclose(ComOStream);
    if (ComIStream && (ComIStream != stdin))  fclose(ComIStream);

    /* Eject all cartridges (will save SRAM) */
    for (J = 0; J < MAXSLOTS; ++J) LoadCart(0, J, ROMType[J]);

    /* Eject all disks */
    for (J = 0; J < MAXDRIVES; ++J) ChangeDisk(J, 0);

    /* Free all remaining allocated memory */
    FreeAllMemory();
}

/** ResetMSX() ***********************************************/
/** Reset MSX hardware to new operating modes. Returns new  **/
/** modes, possibly not the same as NewMode.                **/
/*************************************************************/
int ResetMSX(int NewMode, int NewRAMPages, int NewVRAMPages)
{
    /*** VDP status register states: ***/
    static const byte VDPSInit[16] = { 0x9F,0,0x6C,0,0,0,0,0,0,0,0,0,0,0,0,0 };

    /*** VDP control register states: ***/
    static const byte VDPInit[64] =
    {
      0x00,0x10,0xFF,0xFF,0xFF,0xFF,0x00,0x00,
      0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
      0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
      0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
      0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
      0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
      0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
    };

    /*** Initial palette: ***/
    static const unsigned int PalInit[16] =
    {
      0x00000000,0x00000000,0x0020C020,0x0060E060,
      0x002020E0,0x004060E0,0x00A02020,0x0040C0E0,
      0x00E02020,0x00E06060,0x00C0C020,0x00C0C080,
      0x00208020,0x00C040A0,0x00A0A0A0,0x00E0E0E0
    };

    byte* P1, * P2;
    int J, I;

    /* If changing hardware model, load new system ROMs */
#ifdef _3DS
    VRAMPages = 0;
    unsigned char IsCBIOS = 0;
    if (((Mode ^ NewMode) & MSX_MODEL) || ReloadBIOS)
    {
        ReloadBIOS = 0;
#else
    if ((Mode ^ NewMode) & MSX_MODEL)
    {
#endif // _3DS
        /* Change to the program directory */
        if (ProgDir && chdir(ProgDir))
        {
            if (Verbose) printf("  Failed changing to '%s' directory!\n", ProgDir);
        }

        switch (NewMode & MSX_MODEL)
        {
        case MSX_MSX1:
            if (Verbose) printf("  Opening MSX.ROM...");
#ifdef USE_CBIOS
            if (!ForceCBIOS)
            {
                if (ForceJPBIOS)
                {
                    P1 = LoadROM("MSXJ.ROM", 0x8000, 0);
                    if (!P1) P1 = LoadROM("MSX.ROM", 0x8000, 0);
                }
                else
                {
                    P1 = LoadROM("MSX.ROM", 0x8000, 0);
                }
                PRINTRESULT(P1);
            }
            if (ForceCBIOS || !P1)
            {
                switch (cbiosReg)
                {
                case CBIOS_BR:
                    P1 = LoadROM("cbios_main_msx1_br.rom", 0x8000, 0);
                    break;
                case CBIOS_EU:
                    P1 = LoadROM("cbios_main_msx1_eu.rom", 0x8000, 0);
                    break;
                case CBIOS_JP:
                    P1 = LoadROM("cbios_main_msx1_jp.rom", 0x8000, 0);
                    break;
                case CBIOS_US:
                    P1 = LoadROM("cbios_main_msx1.rom", 0x8000, 0);
                    break;
                default:
                    break;
                }
                PRINTRESULT(P1);
                if (P1)
                {
                    P2 = LoadROM("cbios_logo_msx1.rom", 0x4000, 0);
                    if (P2)
                    {
                        MemMap[0][0][4] = P2;
                        IsCBIOS = 1;
                    }
                }
            }
            else IsCBIOS = 0;
            if (UseCBIOS && !IsCBIOS)FreeMemory(MemMap[0][0][4]);
            UseCBIOS = IsCBIOS;
#else
            P1 = LoadROM("MSX.ROM", 0x8000, 0);
            PRINTRESULT(P1);
#endif // USE_CBIOS
            if (!P1) NewMode = (NewMode & ~MSX_MODEL) | (Mode & MSX_MODEL);
            else
            {
                FreeMemory(MemMap[0][0][0]);
                FreeMemory(MemMap[3][1][0]);
                MemMap[0][0][0] = P1;
                MemMap[0][0][1] = P1 + 0x2000;
                MemMap[0][0][2] = P1 + 0x4000;
                MemMap[0][0][3] = P1 + 0x6000;
                MemMap[3][1][0] = EmptyRAM;
                MemMap[3][1][1] = EmptyRAM;
            }
            break;

        case MSX_MSX2:
            if (Verbose) printf("  Opening MSX2.ROM...");
#ifdef USE_CBIOS
            if (!ForceCBIOS)
            {
                if (ForceJPBIOS)
                {
                    P1 = LoadROM("MSX2J.ROM", 0x8000, 0);
                    PRINTRESULT(P1);
                    if (P1)
                    {
                        if (Verbose) printf("  Opening MSX2EXT.ROM...");
                        P2 = LoadROM("MSX2JEXT.ROM", 0x4000, 0);
                        PRINTRESULT(P2);
                    }
                    if (!P2)
                    {
                        P1 = LoadROM("MSX2.ROM", 0x8000, 0);
                        PRINTRESULT(P1);
                        if (P1)
                        {
                            if (Verbose) printf("  Opening MSX2EXT.ROM...");
                            P2 = LoadROM("MSX2EXT.ROM", 0x4000, 0);
                            PRINTRESULT(P2);
                        }
                    }
                }
                else
                {
                    P1 = LoadROM("MSX2.ROM", 0x8000, 0);
                    PRINTRESULT(P1);
                    if (P1)
                    {
                        if (Verbose) printf("  Opening MSX2EXT.ROM...");
                        P2 = LoadROM("MSX2EXT.ROM", 0x4000, 0);
                        PRINTRESULT(P2);
                    }
                }
            }
            if (ForceCBIOS || !P1 || !P2)
            {
                switch (cbiosReg)
                {
                case CBIOS_BR:
                    P1 = LoadROM("cbios_main_msx2_br.rom", 0x8000, 0);
                    break;
                case CBIOS_EU:
                    P1 = LoadROM("cbios_main_msx2_eu.rom", 0x8000, 0);
                    break;
                case CBIOS_JP:
                    P1 = LoadROM("cbios_main_msx2_jp.rom", 0x8000, 0);
                    break;
                case CBIOS_US:
                    P1 = LoadROM("cbios_main_msx2.rom", 0x8000, 0);
                    break;
                default:
                    break;
                }
                PRINTRESULT(P1);
                P2 = LoadROM("cbios_sub.rom", 0x4000, 0);
                PRINTRESULT(P2);
                if (P2)
                {
                    byte* P3;
                    P3 = LoadROM("cbios_logo_msx2.rom", 0x4000, 0);
                    if (P3)
                    {
                        MemMap[0][0][4] = P3;
                        IsCBIOS = 1;
                    }
                }
            }
            else IsCBIOS = 0;
            if (UseCBIOS && !IsCBIOS)FreeMemory(MemMap[0][0][4]);
            UseCBIOS = IsCBIOS;
#else
            P1 = LoadROM("MSX2.ROM", 0x8000, 0);
            PRINTRESULT(P1);
            if (Verbose) printf("  Opening MSX2EXT.ROM...");
            P2 = LoadROM("MSX2EXT.ROM", 0x4000, 0);
            PRINTRESULT(P2);
#endif // USE_CBIOS
            if (!P1 || !P2)
            {
                NewMode = (NewMode & ~MSX_MODEL) | (Mode & MSX_MODEL);
                FreeMemory(P1);
                FreeMemory(P2);
            }
            else
            {
                FreeMemory(MemMap[0][0][0]);
                FreeMemory(MemMap[3][1][0]);
                MemMap[0][0][0] = P1;
                MemMap[0][0][1] = P1 + 0x2000;
                MemMap[0][0][2] = P1 + 0x4000;
                MemMap[0][0][3] = P1 + 0x6000;
                MemMap[3][1][0] = P2;
                MemMap[3][1][1] = P2 + 0x2000;
            }
            break;

        case MSX_MSX2P:
            if (Verbose) printf("  Opening MSX2P.ROM...");
#ifdef USE_CBIOS
            //if (!ForceCBIOS)
            if (!ForceCBIOS && (CartSpecial[0] != CART_NEED_CBIOS) && (CartSpecial[1] != CART_NEED_CBIOS))
            {
                P1 = LoadROM("MSX2P.ROM", 0x8000, 0);
                PRINTRESULT(P1);
                if (Verbose) printf("  Opening MSX2PEXT.ROM...");
                P2 = LoadROM("MSX2PEXT.ROM", 0x4000, 0);
                PRINTRESULT(P2);
            }
            //if (ForceCBIOS || !P1 || !P2)
            if (ForceCBIOS || !P1 || !P2 || CartSpecial[0] == CART_NEED_CBIOS || CartSpecial[1] == CART_NEED_CBIOS)
            {
                switch (cbiosReg)
                {
                case CBIOS_BR:
                    P1 = LoadROM("cbios_main_msx2+_br.rom", 0x8000, 0);
                    break;
                case CBIOS_EU:
                    P1 = LoadROM("cbios_main_msx2+_eu.rom", 0x8000, 0);
                    break;
                case CBIOS_JP:
                    P1 = LoadROM("cbios_main_msx2+_jp.rom", 0x8000, 0);
                    break;
                case CBIOS_US:
                    P1 = LoadROM("cbios_main_msx2+.rom", 0x8000, 0);
                    break;
                default:
                    break;
                }
                PRINTRESULT(P1);
                P2 = LoadROM("cbios_sub.rom", 0x4000, 0);
                PRINTRESULT(P2);
                if (P2)
                {
                    byte* P3;
                    P3 = LoadROM("cbios_logo_msx2+.rom", 0x4000, 0);
                    if (P3)
                    {
                        MemMap[0][0][4] = P3;
                        IsCBIOS = 1;
                    }
                }
            }
            else IsCBIOS = 0;
            if (UseCBIOS && !IsCBIOS)FreeMemory(MemMap[0][0][4]);
            UseCBIOS = IsCBIOS;
#else
            P1 = LoadROM("MSX2P.ROM", 0x8000, 0);
            PRINTRESULT(P1);
            if (Verbose) printf("  Opening MSX2PEXT.ROM...");
            P2 = LoadROM("MSX2PEXT.ROM", 0x4000, 0);
            PRINTRESULT(P2);
#endif // USE_CBIOS
            if (!P1 || !P2)
            {
                NewMode = (NewMode & ~MSX_MODEL) | (Mode & MSX_MODEL);
                FreeMemory(P1);
                FreeMemory(P2);
            }
            else
            {
#ifdef _3DS
                /* F4 register value is vary from manufacturer(Panasonic, Sony, and Sanyo). */
                /* BIOS with reversed F4 register has NOP(0x00) in main ROM 146Ch & 146Eh; */
                /* http://niga2.sytes.net/msx/A1Fkai2.html */
                InvertF4Reg = (P1[0x146C] == 0x00 & P1[0x146E] == 0x00) ? 1 : 0;

                if (!SkipBIOS && !UseCBIOS)
                {
                    byte* P3;
                    /* KANJI BASIC ROM contains MSX2+ boot logo.*/
                    P3 = LoadROM("MSXKANJI.ROM", 0x8000, 0);
                    if (!P3)P3 = LoadROM("KNJDRV.ROM", 0x8000, 0);
                    if (!P3)P3 = LoadROM("A1WXKDR.ROM", 0x8000, 0);
                    if (P3)
                    {
                        MemMap[3][1][2] = P3;
                        MemMap[3][1][3] = P3 + 0x2000;
                        MemMap[3][1][4] = P3 + 0x4000;
                        MemMap[3][1][5] = P3 + 0x6000;
                        HasKanjiBASIC = 1;
                    }
                    else HasKanjiBASIC = 0;
                }
#endif // _3DS
                FreeMemory(MemMap[0][0][0]);
                FreeMemory(MemMap[3][1][0]);
                MemMap[0][0][0] = P1;
                MemMap[0][0][1] = P1 + 0x2000;
                MemMap[0][0][2] = P1 + 0x4000;
                MemMap[0][0][3] = P1 + 0x6000;

                MemMap[3][1][0] = P2;
                MemMap[3][1][1] = P2 + 0x2000;
            }
            break;

#ifdef TURBO_R
        case MSX_MSXTR:
            P1 = LoadROM("MSXTR.ROM", 0x8000, 0);
            PRINTRESULT(P1);
            if (Verbose) printf("  Opening MSXTR.ROM...");
            P2 = LoadROM("MSXTREXT.ROM", 0x4000, 0);
            PRINTRESULT(P2);
            if (P2)
            {
                byte* P3;
                P3 = LoadROM("MSXTROPT.ROM", 0x4000, 0);
                if (P3)
                {
                    //MemMap[0][0][4] = P3;
                    //MemMap[0][0][5] = P3 + 0x2000;
                    MemMap[0][3][2] = P3;
                    MemMap[0][3][3] = P3 + 0x2000;
                }
            }
            if (!P1 || !P2)
            {
                NewMode = (NewMode & ~MSX_MODEL) | (Mode & MSX_MODEL);
                FreeMemory(P1);
                FreeMemory(P2);
            }
            else
            {
                P1[0x2E] &= 0xFE;       /* Disable MIDI */
                FreeMemory(MemMap[0][0][0]);
                FreeMemory(MemMap[3][1][0]);
                MemMap[0][0][0] = P1;
                MemMap[0][0][1] = P1 + 0x2000;
                MemMap[0][0][2] = P1 + 0x4000;
                MemMap[0][0][3] = P1 + 0x6000;

                MemMap[3][1][0] = P2;
                MemMap[3][1][1] = P2 + 0x2000;

                byte* P3;
                P3 = LoadROM("MSXKANJI.ROM", 0x8000, 0);
                if (!P3)P3 = LoadROM("KNJDRV.ROM", 0x8000, 0);
                if (!P3)P3 = LoadROM("A1WXKDR.ROM", 0x8000, 0);
                if (P3)
                {
                    MemMap[3][1][2] = P3;
                    MemMap[3][1][3] = P3 + 0x2000;
                    MemMap[3][1][4] = P3 + 0x4000;
                    MemMap[3][1][5] = P3 + 0x6000;
                    HasKanjiBASIC = 1;
                }
                else HasKanjiBASIC = 0;

                PanasonicSRAM[0] = GetMemory(0x2000);
                PanasonicSRAM[1] = GetMemory(0x2000);
                PanasonicSRAM[2] = GetMemory(0x2000);
                PanasonicSRAM[3] = GetMemory(0x2000);
                memset(PanasonicSRAM[0], NORAM, 0x2000);
                memset(PanasonicSRAM[1], NORAM, 0x2000);
                memset(PanasonicSRAM[2], NORAM, 0x2000);
                memset(PanasonicSRAM[3], NORAM, 0x2000);

                if (MemMap[0][0][0])
                {
                    /* http://www.yo.rim.or.jp/~anaka/AtoC/labo/view32.htm Check Unique values for FS-A1GT */
                    if (MemMap[0][0][0][0xC49] == 0 && MemMap[0][0][0][0x1A74] == 0xDB && MemMap[0][0][0][0x1A75] == 0xE9 && MemMap[0][0][0][0x1A7B] == 0xDB
                        && MemMap[0][0][0][0x1A7C] == 0xE9)
                    {
                        //NeedRest = 0;
                        //LoadCart("a1gtfirm.rom", 9, MAP_PANSONIC);
                        //NeedRest = 1;
                        /* Load pseudo View font made by A to C */
                        /* http://www.yo.rim.or.jp/~anaka/AtoC/labo/labo32.htm */
                        ViewFontData = LoadROM("kaname_v.rom", 0x3C000, 0);
                    }
                }
            }

            //if (MODEL(MSX_MSXTR))Mode &= ~MSX_PATCHBDOS;
            //else Mode |= MSX_PATCHBDOS;
            break;
#endif // TURBO_R


        default:
            /* Unknown MSX model, keep old model */
            if (Verbose) printf("ResetMSX(): INVALID HARDWARE MODEL!\n");
            NewMode = (NewMode & ~MSX_MODEL) | (Mode & MSX_MODEL);
            break;
        }

        /* Change to the working directory */
        if (WorkDir && chdir(WorkDir))
        {
            if (Verbose) printf("Failed changing to '%s' directory!\n", WorkDir);
        }
    }

#ifdef UPD_FDC
    switch (FDCEmuType)
    {
    case 0:
        NewMode |= MSX_PATCHBDOS;
        break;
    case 1:
        if ((NewMode & MSX_MODEL) == MSX_MSXTR)NewMode &= ~MSX_PATCHBDOS;
        else NewMode |= MSX_PATCHBDOS;
        break;
    default:
        break;
    }
#endif // UPD_FDC

    /* If hardware model changed ok, patch freshly loaded BIOS */
    if ((Mode ^ NewMode) & MSX_MODEL)
    {
        /* Apply patches to BIOS */
        if (Verbose) printf("  Patching BIOS: ");
#ifdef TURBO_R
        if ((NewMode & MSX_MODEL) == MSX_MSXTR)
        {
        }
        else
        {
            for (J = 0; BIOSPatches[J]; ++J)
            {
                if (Verbose) printf("%04X..", BIOSPatches[J]);
                P1 = MemMap[0][0][0] + BIOSPatches[J];
                P1[0] = 0xED; P1[1] = 0xFE; P1[2] = 0xC9;
            }
        }
#else
        for (J = 0; BIOSPatches[J]; ++J)
        {
            if (Verbose) printf("%04X..", BIOSPatches[J]);
            P1 = MemMap[0][0][0] + BIOSPatches[J];
            P1[0] = 0xED; P1[1] = 0xFE; P1[2] = 0xC9;
        }
        PRINTOK;
#endif // TURBO_R
#ifdef _MSX0
        if ((UseMSX0) || (currJoyMode[0] == JOY_TOUCHPAD) || (currJoyMode[1] == JOY_TOUCHPAD))PatchGETPAD();
#endif // _MSX0
    }

    /* If toggling BDOS patches... */
#ifdef TURBO_R
    if (((Mode ^ NewMode) & MSX_PATCHBDOS) || (((NewMode & MSX_MODEL) == MSX_MSXTR) && !(MODEL(MSX_MSXTR)))
        || ((NewMode & MSX_MODEL) != MSX_MSXTR) && (MODEL(MSX_MSXTR)))
#else
    if ((Mode ^ NewMode) & MSX_PATCHBDOS)
#endif // TURBO_R
    {
        /* Change to the program directory */
        if (ProgDir && chdir(ProgDir))
        {
            if (Verbose) printf("  Failed changing to '%s' directory!\n", ProgDir);
        }

        /* Try loading DiskROM */
        if (Verbose) printf("  Opening DISK.ROM...");
#ifdef TURBO_R
        if ((NewMode & MSX_MODEL) == MSX_MSXTR)
        {
            MSXDOS23Data = LoadROM("MSXDOS23.ROM", 0x10000, 0);
            P1 = MSXDOS23Data;

            LoadMSXMusicTurboR();
        }
        else
        {
#ifdef USE_CBIOS
            if (!IsCBIOS)P1 = LoadROM("DISK.ROM", 0x4000, 0);
#else
            P1 = LoadROM("DISK.ROM", 0x4000, 0);
#endif // USE_CBIOS
        }
#else
#ifdef USE_CBIOS
        if (!IsCBIOS)P1 = LoadROM("DISK.ROM", 0x4000, 0);
#else
        P1 = LoadROM("DISK.ROM", 0x4000, 0);
#endif // USE_CBIOS
#endif // TURBO_R
        PRINTRESULT(P1);

        /* Change to the working directory */
        if (WorkDir && chdir(WorkDir))
        {
            if (Verbose) printf("  Failed changing to '%s' directory!\n", WorkDir);
        }

        /* If failed loading DiskROM, ignore the new PATCHBDOS bit */
        if (!P1) NewMode = (NewMode & ~MSX_PATCHBDOS) | (Mode & MSX_PATCHBDOS);
        else
        {
            /* Assign new DiskROM */
#ifdef TURBO_R
            FreeMemory(MemMap[3][2][2]);
            MemMap[3][2][2] = P1;
            MemMap[3][2][3] = P1 + 0x2000;
#else
            FreeMemory(MemMap[3][1][2]);
            MemMap[3][1][2] = P1;
            MemMap[3][1][3] = P1 + 0x2000;
#endif // TURBO_R

            /* If BDOS patching requested... */
            if (NewMode & MSX_PATCHBDOS)
            {
                if (Verbose) printf("  Patching BDOS: ");
                /* Apply patches to BDOS */
#ifdef TURBO_R
                if ((NewMode & MSX_MODEL) == MSX_MSXTR)
                {
                    P1[0x7D6] = 0xED; P1[0x7D7] = 0xE0; P1[0x7D8] = 0x00;
                    P1[0x8C6] = 0xED; P1[0x8C7] = 0xE2; P1[0x8C8] = 0x00;

                    P1[0xD76F] = 0xED; P1[0xD770] = 0xE0; P1[0xD771] = 0x00;
                    P1[0xD850] = 0xED; P1[0xD851] = 0xE2; P1[0xD852] = 0x00;

                    for (J = 0; DiskPatches[J]; ++J)
                    {
                        if (Verbose) printf("%04X..", DiskPatches[J]);
                        P2 = P1 + DiskPatches[J] - 0x4000;
                        P2[0] = 0xED; P2[1] = 0xFE; P2[2] = 0xC9;
                        P2[0xC000] = 0xED; P2[0xC001] = 0xFE; P2[0xC002] = 0xC9;
                    }
                }
                else
                {
                    for (J = 0; DiskPatches[J]; ++J)
                    {
                        if (Verbose) printf("%04X..", DiskPatches[J]);
                        P2 = P1 + DiskPatches[J] - 0x4000;
                        P2[0] = 0xED; P2[1] = 0xFE; P2[2] = 0xC9;
                    }
                }
#else
                for (J = 0; DiskPatches[J]; ++J)
                {
                    if (Verbose) printf("%04X..", DiskPatches[J]);
                    P2 = P1 + DiskPatches[J] - 0x4000;
                    P2[0] = 0xED; P2[1] = 0xFE; P2[2] = 0xC9;
                }
                PRINTOK;
#endif // TURBO_R
            }
        }
    }
#ifdef TURBO_R
    if (((NewMode & MSX_MODEL) != MSX_MSXTR) && (MODEL(MSX_MSXTR)))
    {
        //FreeMemory(MemMap[3][3][2]);

        for (J = 0; J < 8; ++J)
        {
            FreeMemory(MemMap[3][3][J]);
            MemMap[3][3][J] = EmptyRAM;
        }
        FreeMemory(MemMap[0][3][2]);
        FreeMemory(MemMap[0][3][3]);
        MemMap[0][3][2] = EmptyRAM;
        MemMap[0][3][3] = EmptyRAM;

        NeedRest = 0;
        LoadCart("FMPAC.ROM", 4, MAP_FMPAC);
        NeedRest = 1;
    }
#endif // TURBO_R


    /* Assign new modes */
    Mode = NewMode;

#ifdef _3DS
    if ((ROMType[0] == MAP_ESESCC) || (ROMType[0] == MAP_MEGASCSI))
    {
        if (SaveSRAM[0] && SRAMName[0])
        {
            FILE* F;
            if (!(F = sramfopen(SRAMName[0], "wb"))) SaveSRAM[0] = 0;
            else if (fwrite(ExternalRAMData[0], 1, 0x80000, F) != 0x80000)SaveSRAM[0] = 0;
            fclose(F);
        }
    }

    /* Set ROM types for cartridges A/B */
    ROMType[0] = OldROMType[0];
    ROMType[1] = OldROMType[1];
#else
    /* Set ROM types for cartridges A/B */
    ROMType[0] = ROMTYPE(0);
    ROMType[1] = ROMTYPE(1);
#endif // _3DS

    /* Set CPU timings */
    VPeriod = (VIDEO(MSX_PAL) ? VPERIOD_PAL : VPERIOD_NTSC) / 6;
    HPeriod = HPERIOD / 6;
    CPU.IPeriod = CPU_H240;
    CPU.IAutoReset = 0;

#ifdef DEBUG
    CPU.Trace = 0;
    CPU.TrapBadOps = 0;
#endif // DEBUG


    /* Numbers of RAM/VRAM pages should be power of 2 */
    for (J = 1; J < NewRAMPages; J <<= 1);
    NewRAMPages = J;
    for (J = 1; J < NewVRAMPages; J <<= 1);
    NewVRAMPages = J;

    /* Correct RAM and VRAM sizes */
    if ((NewRAMPages < (MODEL(MSX_MSX1) ? 4 : 8)) || (NewRAMPages > 256))
        NewRAMPages = MODEL(MSX_MSX1) ? 4 : 8;
#ifdef TURBO_R
    if ((MODEL(MSX_MSXTR)) && NewRAMPages < 16)NewRAMPages = 16;
#endif // TURBO_R
    if ((NewVRAMPages < (MODEL(MSX_MSX1) ? 2 : 8)) || (NewVRAMPages > 8))
        NewVRAMPages = MODEL(MSX_MSX1) ? 2 : 8;


    /* If changing amount of RAM... */
    if (NewRAMPages != RAMPages)
    {
        if (Verbose) printf("Allocating %dkB for RAM...", NewRAMPages * 16);
        if ((P1 = GetMemory(NewRAMPages * 0x4000)))
        {
            memset(P1, NORAM, NewRAMPages * 0x4000);
            FreeMemory(RAMData);
            RAMPages = NewRAMPages;
            RAMMask = NewRAMPages - 1;
            RAMData = P1;
        }
        PRINTRESULT(P1);
    }

    /* If changing amount of VRAM... */
    if (NewVRAMPages != VRAMPages)
    {
        if (Verbose) printf("Allocating %dkB for VRAM...", NewVRAMPages * 16);
        if ((P1 = GetMemory(NewVRAMPages * 0x4000)))
        {
            memset(P1, 0x00, NewVRAMPages * 0x4000);
            FreeMemory(VRAM);
            VRAMPages = NewVRAMPages;
            VRAM = P1;
        }
        PRINTRESULT(P1);
    }

    /* For all slots... */
    for (J = 0; J < 4; ++J)
    {
        /* Slot is currently read-only */
        EnWrite[J] = 0;
        /* PSL=0:0:0:0, SSL=0:0:0:0 */
        PSL[J] = 0;
        SSL[J] = 0;
        /* RAMMap=3:2:1:0 */
#ifdef _3DS
        MemMap[3][0][J * 2] = RAMData + (3 - J) * 0x4000;
        MemMap[3][0][J * 2 + 1] = MemMap[3][0][J * 2] + 0x2000;
#else
        MemMap[3][2][J * 2] = RAMData + (3 - J) * 0x4000;
        MemMap[3][2][J * 2 + 1] = MemMap[3][2][J * 2] + 0x2000;
#endif // _3DS
        RAMMapper[J] = 3 - J;
        /* Setting address space */
        RAM[J * 2] = MemMap[0][0][J * 2];
        RAM[J * 2 + 1] = MemMap[0][0][J * 2 + 1];
    }

    /* For all MegaROMs... */
    for (J = 0; J < MAXSLOTS; ++J)
        if ((I = ROMMask[J] + 1) > 4)
        {
#ifdef _3DS
            if (ROMType[J] == MAP_ASCII8)SetMegaROM(J, 0, 0, 0, 0);
            else if (ROMType[J] == MAP_ASCII16)SetMegaROM(J, 0, 1, 0, 1);
            else if (ROMType[J] == MAP_ASCII16_2)SetMegaROM(J, 0, 1, 0, 1);
            else if (ROMType[J] == MAP_ASCII16Big)SetMegaROM(J, 0, 1, 0, 1);
            else if (ROMType[J] == MAP_MSX90)SetMegaROM(J, 0, 1, 0, 1);
            else if (ROMType[J] == MAP_RType)SetMegaROM(J, 0x2E, 0x2F, 0, 1);
            else if (ROMType[J] == MAP_LodeRunner)SetMegaROM(J, 0, 1, 0, 1);
            else if (ROMType[J] == MAP_PLAIN)SetNonMegaROM(J);
            else if (ROMType[J] == MAP_ESESCC)LoadESESCC(J, 1);
            else if (ROMType[J] == MAP_MEGASCSI)LoadESERAM(J, 1);
            //else if (ROMType[J] == MAP_PLAIN)SetMegaROM(J, 0, 1, 2, 3);
            else
#endif // _3DS

                /* For normal MegaROMs, set first four pages */
                if ((ROMData[J][0] == 'A') && (ROMData[J][1] == 'B'))
                    SetMegaROM(J, 0, 1, 2, 3);
            /* Some MegaROMs default to last pages on reset */
                else if ((ROMData[J][(I - 2) << 13] == 'A') && (ROMData[J][((I - 2) << 13) + 1] == 'B'))
                    SetMegaROM(J, I - 2, I - 1, I - 2, I - 1);
            /* If 'AB' signature is not found at the beginning or the end */
            /* then it is not a MegaROM but rather a plain 64kB ROM       */
        }

    /* Reset sound chips */
    Reset8910(&PSG, PSG_CLOCK, 0);
    ResetSCC(&SCChip, AY8910_CHANNELS);
    Reset2413(&OPLL, AY8910_CHANNELS);
    Sync8910(&PSG, AY8910_SYNC);
    SyncSCC(&SCChip, SCC_SYNC);
    Sync2413(&OPLL, YM2413_SYNC);

    /* Reset serial I/O */
    Reset8251(&SIO, ComIStream, ComOStream);

    /* Reset PPI chips and slot selectors */
    Reset8255(&PPI);
    PPI.Rout[0] = PSLReg = 0x00;
    PPI.Rout[2] = IOReg = 0x00;
    SSLReg[0] = 0x00;
    SSLReg[1] = 0x00;
    SSLReg[2] = 0x00;
    SSLReg[3] = 0x00;

    /* Reset floppy disk controller */
    Reset1793(&FDC, FDD, WD1793_KEEP);

#ifdef UPD_FDC
    if (MODEL(MSX_MSXTR)) ResetTC8566AF(&TCFDC, FDD, TC8566AF_KEEP);
#endif // UPD_FDC

    /* Reset VDP */
    memcpy(VDP, VDPInit, sizeof(VDP));
    memcpy(VDPStatus, VDPSInit, sizeof(VDPStatus));

    /* Reset keyboard */
    memset((void*)KeyState, 0xFF, 16);

    /* Set initial palette */
    for (J = 0; J < 16; ++J)
    {
        Palette[J] = PalInit[J];
        SetColor(J, (Palette[J] >> 16) & 0xFF, (Palette[J] >> 8) & 0xFF, Palette[J] & 0xFF);
    }

    /* Reset mouse coordinates/counters */
    for (J = 0; J < 2; ++J)
        MouState[J] = MouseDX[J] = MouseDY[J] = OldMouseX[J] = OldMouseY[J] = MCount[J] = 0;

    IRQPending = 0x00;                      /* No IRQs pending  */
    SCCOn[0] = SCCOn[1] = 0;                  /* SCCs off for now */
    RTCReg = RTCMode = 0;                     /* Clock registers  */
    KanCount = 0; KanLetter = 0;               /* Kanji extension  */
    ChrTab = ColTab = ChrGen = VRAM;            /* VDP tables       */
    SprTab = SprGen = VRAM;
    ChrTabM = ColTabM = ChrGenM = SprTabM = ~0;   /* VDP addr. masks  */
    VPAGE = VRAM;                           /* VRAM page        */
    FGColor = BGColor = XFGColor = XBGColor = 0;  /* VDP colors       */
    ScrMode = 0;                            /* Screen mode      */
    VKey = PKey = 1;                          /* VDP keys         */
    VAddr = 0x0000;                         /* VRAM access addr */
    ScanLine = 0;                           /* Current scanline */
    VDPData = NORAM;                        /* VDP data buffer  */
    JoyState = 0;                           /* Joystick state   */

    /* Set "V9958" VDP version for MSX2+ */
    if (MODEL(MSX_MSX2P)) VDPStatus[1] |= 0x04;
#ifdef TURBO_R
    if (MODEL(MSX_MSXTR)) VDPStatus[1] |= 0x04;
#ifdef ALTPCM
    PCMPos = 0;
    memset(PCMData, 0, 8);
#endif // ALTPCM
#endif // TURBO_R

#ifdef ALTSOUND
    if ((ROMType[0] == MAP_SCCPLUS || ROMType[0] == MAP_SCCPLUS_2 || ROMType[1]==MAP_SCCPLUS || ROMType[1]==MAP_SCCPLUS_2) || (ForceSCCP))
    {
        SCCEnhanced = 1;
    }
    else
    {
        SCCEnhanced = 0;
    }

    /* Reset soundchips */
    ResetSound();
#endif

#ifdef _3DS
    PadCount[0] = PadCount[1] = 8;
    PaddleShift[0] = PaddleShift[1] = 236;
    PaddleState[0] = PaddleState[1] = 236;
    PrinterStatus = 0;
    PrinterValue = 0;
    PrinterValue2 = 0;

    memset(ADPCMData, 0, 224);

    LoadFMPAC(2, 1);

    /*Hardware Reset. Clear RAM*/
    if (IsHardReset)memset(RAMData, NORAM, sizeof(RAMData));

    ////No need for this? BIOS automatelly do this?
    //#ifdef TURBO_R
    //  if ((NewMode & MSX_MODEL) == MSX_MSXTR)
    //  {
    //      memcpy(RAMData + ((NewRAMPages << 14) - 0x10000), MemMap[0][0][0], 0x2000);
    //      memcpy(RAMData + ((NewRAMPages << 14) - 0xE000), MemMap[0][0][1], 0x2000);
    //      memcpy(RAMData + ((NewRAMPages << 14) - 0xC000), MemMap[0][0][2], 0x2000);
    //      memcpy(RAMData + ((NewRAMPages << 14) - 0xA000), MemMap[0][0][3], 0x2000);
    //      memcpy(RAMData + ((NewRAMPages << 14) - 0x8000), MemMap[3][1][0], 0x2000);
    //      memcpy(RAMData + ((NewRAMPages << 14) - 0x6000), MemMap[3][1][1], 0x2000);
    //      memcpy(RAMData + ((NewRAMPages << 14) - 0x4000), MemMap[3][1][2], 0x2000);
    //      memcpy(RAMData + ((NewRAMPages << 14) - 0x2000), MemMap[3][1][3], 0x2000);
    //  }
    //#endif // TURBO_R

    FMPACKey = 0x0000;
    PACMapper[0] = 0;
    PACMapper[1] = 0;

    switch (regionid)
    {
    case 0:
        if (MemMap[0][0][0][0x2B] & 0x80)VDP[9] |= 0x02;
        else VDP[9] &= ~(0x02);
        break;
    case 1:
        //MemMap[0][0][0][0x2B] &= ~(0x80);
        VDP[9] &= ~(0x02);
        break;
    case 2:
        //MemMap[0][0][0][0x2B] |= 0x80;
        VDP[9] |= 0x02;
        break;
    default:
        break;
    }
    IsJPKeyBIOS = (MemMap[0][0][0][0x2C] & 0x0F) ? 0 : 1;
    IsJpKey = KeyRegion ? 1 - (KeyRegion - 1) : IsJPKeyBIOS;
    /*Reset 3DS*/
    Reset3DS();
    VRAMPageInt = 0;
    ScreenModeReg = 0xFF;
#endif // _3DS

#ifdef VDP_V9990
    V9990Active = 0;
    V9990Dual = 0;
#endif // VDP_V9990


#ifdef TURBO_R
    S1990Reg6 = 0x60;
    S1990Latch = 0;
    R800Pause = 0;
    PCMStatus = 0;
    PCMSample = 0;
    MSXDOS2Mapper = 0;
    for (J = 0; J < 7; J++)PanaMapper[J] = 0;
    PanaMapper2 = 0;
    PanaMapper3 = 0;
    R800TimerCnt = 0;
    PCMTimerCnt = 0;
    PCMTimerReg = 0;
    VDPIOTimerCnt = 0;

    if (MODEL(MSX_MSXTR))
    {
        if (!MSXDOS23Data)return(0);
        RAM[2] = MemMap[3][2][2] = MSXDOS23Data;
        RAM[3] = MemMap[3][2][3] = RAM[2] + 0x2000;
    }
    if (MODEL(MSX_MSXTR) && !IsHardReset)
    {
        /* Jump 0x416 to software reset MSXTurboR. */
        /* https://twitter.com/tiny_yarou/status/1351985437011660800 */

        CPU.PC.W = 0x0416;
        CPU.SP.W = 0xF000;
        CPU.AF.W = 0x0000;
        CPU.BC.W = 0x0000;
        CPU.DE.W = 0x0000;
        CPU.HL.W = 0x0000;
        CPU.AF1.W = 0x0000;
        CPU.BC1.W = 0x0000;
        CPU.DE1.W = 0x0000;
        CPU.HL1.W = 0x0000;
        CPU.IX.W = 0x0000;
        CPU.IY.W = 0x0000;
        CPU.I = 0x00;
        CPU.R = 0x00;
        CPU.IFF = 0x00;
        CPU.ICount = CPU.IPeriod;
        CPU.IRequest = INT_NONE;
        CPU.IBackup = 0;
        CPU.User = 0;
        JumpZ80(0x0416);
    }
    else
    {
        /* Reset CPU */
        ResetZ80(&CPU);
    }
#else
    /* Reset CPU */
    ResetZ80(&CPU);
#endif // TURBO_R

    /* Done */
    return(Mode);
    }

/** RdZ80() **************************************************/
/** Z80 emulation calls this function to read a byte from   **/
/** address A in the Z80 address space. Also see OpZ80() in **/
/** Z80.c which is a simplified code-only RdZ80() version.  **/
/*************************************************************/
byte RdZ80(word A)
{
#ifdef TURBO_R
#ifdef UPD_FDC
    if(MODEL(MSX_MSXTR))
#else
    if (CPU.User & 0x01)
#endif // UPD_FDC
    {
        byte J, I, PS, SS;
        J = A >> 14;
        PS = PSL[J];
        SS = SSL[J];
        I = CartMap[PS][SS];
        switch (I)
        {
        case 0:     /* Cartridge Slot A */
        case 1:     /* Cartridge Slot B */
            CPU.ICount -= 2;
            //if (CartSpecial[I] == CART_READSCC)
            //if (HasSpecialCart)
            //{
            int cs0 = CartSpecial[0], cs1 = CartSpecial[1];
                if ((cs0 == CART_READSCC) || (cs1 == CART_READSCC))
                {
                    if (A >= 0x9800 && A < 0xA000)
                    {
                        if (SCCOn[I])return (Read2212(A));
                        //else if (A == 0x9800) return 0xFF;
                        //if (SCCOn[I] && (CartSpecial[I] == CART_READSCC))return (Read2212(A));
                        //else if (A == 0x9800) return 0xFF;
                    }
                }
#ifdef MEGASCSI_HD
                else if ((cs0 == CART_MEGASCSI) || (cs1 == CART_MEGASCSI))
                {
                    return ReadMEGASCSI(A);
                }
#endif // MEGASCSI_HD
            //}
            break;
        case 2:
            break;
        case 8:
            /* Memory wait 2 cycle if not DRAM mode in MSXTurboR. */
            //if (S1990Reg6 & 0x40)CPU.ICount -= 2;
            break;
        default:
            switch (PS)
            {
            case 0:
                if (SS == 0)
                    //if ((SS == 0) || (SS==3))
                {
                    //if (S1990Reg6 & 0x40)CPU.ICount -= 2;
                    //if (S1990Reg6 & 0x40)CPU.ICount --;
                }
                //else CPU.ICount -= 2;
                break;
            case 3:
                switch (SS)
                {
                case 3:
                    if ((A & 0xFFF0) == 0x7FF0)
                    {
#ifndef UPD_FDC
                        if (MODEL(MSX_MSXTR))
                        {
#endif // !UPD_FDC
                            if(Verbose&0x40)printf("Panasonic Mapper Read [%04Xh]\n", A);
                            if (!(A & 0x08))
                            {
                                byte J = A & 0x07;
                                //J = J == 5 ? 6 : (J == 6 ? 5 : J);
                                return (PanaMapper3 & 0x04) ? PanaMapper[J] & 0xFF : 0xFF;
                            }
                            if (A == 0x7FF8)return (PanaMapper3 & 0x10) ? PanaMapper2 : 0xFF;
                            if (A == 0x7FF9)return (PanaMapper3 & 0x08) ? PanaMapper3 : 0xFF;
#ifndef UPD_FDC
                        }
#endif // !UPD_FDC
                    }
                    break;
                case 2:     /* Disk ROM */
#ifdef UPD_FDC
                    switch (A&0x3FFF)
                    {
                    case 0x3FF1:
                        J = 3 | (!DiskChanged[0] ? 0x10 : 0) | (!DiskChanged[1] ? 0x20 : 0);
                        DiskChanged[0] = 0;
                        DiskChanged[1] = 0;
                        return J;
                    case 0x3FF4:
                        return ReadTC8566AF(&TCFDC, 4);
                    case 0x3FF5:
                        return ReadTC8566AF(&TCFDC, 5);
                    default:
                        return(RAM[A >> 13][A & 0x1FFF]);
                    }
#endif // UPD_FDC
                    break;
                default:
                    //CPU.ICount -= 2;
                    break;
                }
                break;
            default:
                break;
            }
            break;
        }
    }
    else
    {
    int cs0 = CartSpecial[0], cs1 = CartSpecial[1];
        //if (HasSpecialCart)     /* Use global flag to speedup. */
        //{
            if (cs0 == CART_READSCC || cs1 == CART_READSCC)
            {
                if (A >= 0x9800 && A < 0xA000)
                    //if ((A & 0xE000) == 0x8000 && A >= 0x9800)
                {
                    byte J, I, PS, SS;
                    J = A >> 14;
                    PS = PSL[J];
                    SS = SSL[J];
                    I = CartMap[PS][SS];
                    if (SCCOn[I])return (Read2212(A));

                    //if ((SCCOn[0] && CartSpecial[0] == CART_READSCC) || (SCCOn[1] && CartSpecial[1] == CART_READSCC))return (Read2212(A));
                    //else if (A == 0x9800) return 0xFF;
                }
            }
#ifdef MEGASCSI_HD
            else if ((cs0 == CART_MEGASCSI) || (cs1 == CART_MEGASCSI))
            {
                return ReadMEGASCSI(A);
            }
#endif // MEGASCSI_HD
        //}
    }
#endif // TURBO_R

    /* Filter out everything but [xx11 1111 1xxx 1xxx] */
    if ((A & 0x3F88) != 0x3F88) return(RAM[A >> 13][A & 0x1FFF]);

    /* Secondary slot selector */
#ifdef TURBO_R
    /* C-BIOS doesn't support subslot in the main ROM. */
    if (A == 0xFFFF)return((PSL[3] == 3 || PSL[3] == 2 || (PSL[3] == 0 && !UseCBIOS)) ? ~SSLReg[PSL[3]] : RAM[7][0x1FFF]);
    //if (A == 0xFFFF)return((PSL[3] == 3 || PSL[3] == 2) ? ~SSLReg[PSL[3]] : RAM[7][0x1FFF]);
#else
    if (A == 0xFFFF) return(~SSLReg[PSL[3]]);
#endif // TURBO_R

    /* Floppy disk controller */
    /* 7FF8h..7FFFh Standard DiskROM  */
    /* BFF8h..BFFFh MSX-DOS BDOS      */
    /* 7F80h..7F87h Arabic DiskROM    */
    /* 7FB8h..7FBFh SV738/TechnoAhead */
#ifdef TURBO_R
    if ((PSL[A >> 14] == 3) && (SSL[A >> 14] == 2))
#else
    if ((PSL[A >> 14] == 3) && (SSL[A >> 14] == 1))
#endif // TURBO_R
        switch (A)
        {
            /* Standard      MSX-DOS       Arabic        SV738            */
        case 0x7FF8: case 0xBFF8: case 0x7F80: case 0x7FB8: /* STATUS */
        case 0x7FF9: case 0xBFF9: case 0x7F81: case 0x7FB9: /* TRACK  */
        case 0x7FFA: case 0xBFFA: case 0x7F82: case 0x7FBA: /* SECTOR */
        case 0x7FFB: case 0xBFFB: case 0x7F83: case 0x7FBB: /* DATA   */
            return(Read1793(&FDC, A & 0x0003));
        case 0x7FFF: case 0xBFFF: case 0x7F84: case 0x7FBC: /* SYSTEM */
#ifdef _3DS
            DiskAccess[FDC.Drive] = 3;
#endif // _3DS

            return(Read1793(&FDC, WD1793_READY));
        }

    /* Default to reading memory */
    return(RAM[A >> 13][A & 0x1FFF]);
}

/** WrZ80() **************************************************/
/** Z80 emulation calls this function to write byte V to    **/
/** address A of Z80 address space.                         **/
/*************************************************************/
void WrZ80(word A, byte V)
{
#ifdef _3DS
    /* Mapper Write for Super Lode Runner. */
    if (A == 0x0000)
    {
        if (CartSpecial[0] == CART_LODERUNNER || CartSpecial[1] == CART_LODERUNNER)
        {
            byte I, J, PS, SS, * P;
            J = 2;
            PS = PSL[2];          /* Primary slot number   */
            SS = SSL[2];          /* Secondary slot number */
            I = CartMap[PS][SS]; /* Cartridge number      */
            if (ROMType[I] == MAP_LodeRunner)
            {
                V = (V & ROMMask[I]) << 1;
                P = ROMData[I] + ((int)V << 13);
                if (V != ROMMapper[I][2])
                {
                    MemMap[PS][SS][4] = P;
                    MemMap[PS][SS][5] = P + 0x2000;
                    ROMMapper[I][2] = V;
                    ROMMapper[I][3] = V | 1;
                    /* Only update memory when cartridge's slot selected */
                    if ((PSL[2] == PS) && (SSL[2] == SS))
                    {
                        RAM[4] = P;
                        RAM[5] = P + 0x2000;
                    }
                }
                /* Done with page switch */
                return;
            }
        }
    }

    /* Secondary slot selector */
    if (A == 0xFFFF)
    {
        /* Secondary slot selector->wait 3 cycle for all slot. */
        /* http://d4.princess.ne.jp/msx/other/trmemwait/ */
        if (CPU.User & 0x01)CPU.ICount -= 2;

        /* Fix FM-BIOS bug. R-Type etc. */
        if (CartSpecial[0] == CART_FMBUG)
        {
            if (PSL[3] == 2)
            {
                //if (EnWrite[3])RAM[7][A & 0x1FFF] = V;
                return;
            }
        }

        if (MODEL(MSX_MSX1) || CartSpecial[0] == CART_SLOTBUG || CartSpecial[1] == CART_SLOTBUG)
        {
            if (EnWrite[3])
            {
                RAM[7][A & 0x1FFF] = V;
                return;
            }
        }
        else
        {
            /* C-BIOS doesn't support subslot in the main ROM. */
            if (PSL[3] == 3 || PSL[3] == 2 || (PSL[3] == 0 && !UseCBIOS))
            {
                SSlot(V);
                return;
            }
            else if (EnWrite[3])
            {
                RAM[7][A & 0x1FFF] = V;
                return;
            }
        }
    }
#else
    /* Secondary slot selector */
    if (A == 0xFFFF) { SSlot(V); return; }
#endif // _3DS

    /* Floppy disk controller */
    /* 7FF8h..7FFFh Standard DiskROM  */
    /* BFF8h..BFFFh MSX-DOS BDOS      */
    /* 7F80h..7F87h Arabic DiskROM    */
    /* 7FB8h..7FBFh SV738/TechnoAhead */
#ifdef _3DS
#ifdef UPD_FDC
    if (((A & 0x3F88) == 0x3F88) && (PSL[A >> 14] == 3) && (SSL[A >> 14] == 2) && !(MODEL(MSX_MSXTR)))
#else
    if (((A & 0x3F88) == 0x3F88) && (PSL[A >> 14] == 3) && (SSL[A >> 14] == 2))
#endif // UPD_FDC
#else
    if (((A & 0x3F88) == 0x3F88) && (PSL[A >> 14] == 3) && (SSL[A >> 14] == 1))
#endif // _3DS
        switch (A)
        {
            /* Standard      MSX-DOS       Arabic        SV738             */
        case 0x7FF8: case 0xBFF8: case 0x7F80: case 0x7FB8: /* COMMAND */
        case 0x7FF9: case 0xBFF9: case 0x7F81: case 0x7FB9: /* TRACK   */
        case 0x7FFA: case 0xBFFA: case 0x7F82: case 0x7FBA: /* SECTOR  */
        case 0x7FFB: case 0xBFFB: case 0x7F83: case 0x7FBB: /* DATA    */
            Write1793(&FDC, A & 0x0003, V);

#ifdef _3DS
            if (A == 0x7FF8 || A == 0xBFF8 || A == 0x7F80 || A == 0x7FB8)
            {
                if ((V & 0xF0) == 0xA0 || (V & 0xF0) == 0xB0)DiskWrited[FDC.Drive] = 1;
            }
            else if ((A & 0x0003) == 3)
            {
                if ((V & 0xF0) == 0xA0 || (V & 0xF0) == 0xB0)DiskWrited[FDC.Drive] = 1;
                //DiskWrited[FDC.Drive] = 1;
            }
#endif // _3DS

            return;
        case 0xBFFC: /* Standard/MSX-DOS */
        case 0x7FFC: /* Side: [xxxxxxxS] */
#ifdef DEBUG_LOG
            if (Verbose & 0x04)printf("Disk Write System[%02Xh] [%02Xh]\n", A, V);
#endif // DEBUG_LOG
            Write1793(&FDC, WD1793_SYSTEM, FDC.Drive | S_DENSITY | (V & 0x01 ? 0 : S_SIDE));
            return;
        case 0xBFFD: /* Standard/MSX-DOS  */
        case 0x7FFD: /* Drive: [xxxxxxxD] */
#ifdef DEBUG_LOG
            if (Verbose & 0x04)printf("Disk Write System[%02Xh] [%02Xh]\n", A, V);
#endif // DEBUG_LOG
            Write1793(&FDC, WD1793_SYSTEM, (V & 0x01) | S_DENSITY | (FDC.Side ? 0 : S_SIDE));
            return;
        case 0x7FBC: /* Arabic/SV738 */
        case 0x7F84: /* Side/Drive/Motor: [xxxxMSDD] */
            Write1793(&FDC, WD1793_SYSTEM, (V & 0x03) | S_DENSITY | (V & 0x04 ? 0 : S_SIDE));
            return;
        }

    /* Write to RAM, if enabled */
    if (EnWrite[A >> 14]) { RAM[A >> 13][A & 0x1FFF] = V; return; }

    /* Switch MegaROM pages */
    if ((A > 0x3FFF) && (A < 0xC000)) MapROM(A, V);
}

/** InZ80() **************************************************/
/** Z80 emulation calls this function to read a byte from   **/
/** a given I/O port.                                       **/
/*************************************************************/
byte InZ80(word Port)
{
    /* MSX only uses 256 IO ports */
    Port &= 0xFF;

    /* Return an appropriate port value */
    switch (Port)
    {
#ifdef _MSX0
    case 0x08:
    case 0x58:      /* MSX0 IO firmware 0.07.08 */
        return (InMSX0IOT());
#endif // _MSX0

#ifdef VDP_V9990
    case 0x60:  /* VRAM DATA  */
        if (!V9990Active)return 0x00;
        Port = V9KVRAM[vdpV9990.VRAMReadInt];
        if (vdpV9990.IncVRAMRead)vdpV9990.VRAMReadInt = (vdpV9990.VRAMReadInt + 1) & 0x7FFFF;
        return Port;
    case 0x61: /* PALETTE */
        //if (Verbose & 0x20) printf("I/O: Read V9990 Palette\n");
        if (!V9990Active)return NORAM;
        Port = V9KPal[V9990VDP[14]];
        if (!(V9990VDP[13] & 0x10))
        {
            V9990VDP[14] += (V9990VDP[14] & 0x03) == 2 ? 2 : (((V9990VDP[14] & 0x03) == 3) ? -3 : 1);
        }
        return Port;
    case 0x62:  /* COMMAND DATA */
        if (!V9990Active)return NORAM;
        Port = V9990Port[2];
        VDPReadV9990();
        return Port;
    case 0x63:  /* REGISTER DATA */
        //if (Verbose & 0x20) printf("I/O: Read V9990 Register Data VDP[%02Xh]\n", vdpV9990.SelectReg & 0x3F);
        if (!V9990Active)return NORAM;
        Port = V9990VDP[vdpV9990.SelectReg & 0x3F];
        if (!(vdpV9990.SelectReg & 0x40))
        {
            vdpV9990.SelectReg = (vdpV9990.SelectReg & ~0x3F) | ((vdpV9990.SelectReg + 1) & 0x3F);
        }
        return Port;
    case 0x65:  /* STATUS */
        //if (Verbose & 0x20) printf("I/O: Read V9990 Status\n");
        if (!V9990Active)return 0x00;
        Port = V9990Port[5];
        Port |= (vdpV9990.SystemControlReg & 0x01) << 2;
        return Port;
    case 0x66:  /* INTERRUPT FLAG */
        //if (Verbose & 0x20) printf("I/O: Read V9990 Interrupt [%d]\n", V9990Port[6] & 0x07);
        if (!V9990Active)return NORAM;
        return V9990Port[6] & 0x07;
    case 0x69: /* Kanji support */
        if (!V9990Active)return NORAM;
        Port = Kanji ? Kanji[KanLetter + KanCount] : NORAM;
        KanCount = (KanCount + 1) & 0x1F;
        return(Port);
    case 0x6B: /* Kanji support for Kanji level 2 */
        if (!V9990Active)return NORAM;
        Port = Kanji2 ? Kanji2[KanLetter + KanCount] : NORAM;
        KanCount = (KanCount + 1) & 0x1F;
        return(Port);
#endif // VDP_V9990


#ifdef ALTSOUND
    case 0x04: if (playY8950) return 2; else return(NORAM);
    case 0x05: if (playY8950) return 0; else return(NORAM);
    case 0xC0: if (playY8950) return ReadAUDIO(0); else return(NORAM);
    case 0xC1: if (playY8950) return ReadAUDIO(1); else return(NORAM);
#endif

#ifdef TURBO_R
    case 0x90:
        switch (PrinterMode)
        {
        case PRINTER_NONE:
            return 0xFF;
        case PRINTER_PRINT2FILE:
            return 0xFD;
        case PRINTER_VOICEBOX:
            if (PrinterStatus & 0x80)
            {
                switch (SoundSampRate)
                {
                case 0:
                    PrinterStatus = 3;
                    break;
                case 1:
                    PrinterStatus = 6;
                    break;
                default:
                    break;
                }
                //PrinterStatus = 3;
                //PrinterStatus = 6;
                return 0xFD;
            }
            //if (Verbose & 0x20) printf("I/O: Read from PORT[%02Xh] Value[%02Xh]\n", Port, (PrinterStatus ? 0xFF : 0xFD));
            return (PrinterStatus ? 0xFF : 0xFD);
        case PRINTER_PLUSPCM:
            return (PrinterStatus ? 0xFF : 0xFD);
        default:
            break;
        }
        /* RTC data for MSX use 0-3bit only. */
        /* https://www.msx.org/wiki/Real_Time_Clock_Programming */
        /* This fix many bugs of many kind of emulators has. Such as time error of Chikuwa Watch.  */
        /* https://chikuwa-empire.com/computer/msx0-app-watch/ */
    case 0xB5: return((RTCIn(RTCReg)&0x0F));          /* RTC registers        */
#else
    case 0x90: return(0xFD);                   /* Printer READY signal */

    case 0xB5: return(RTCIn(RTCReg));          /* RTC registers        */
#endif // TURBO_R
    //case 0xB5: return(RTCIn(RTCReg));          /* RTC registers        */

    case 0xA8: /* Primary slot state   */
    case 0xA9: /* Keyboard port        */
    case 0xAA: /* General IO register  */
    case 0xAB: /* PPI control register */
        PPI.Rin[1] = KeyState[PPI.Rout[2] & 0x0F];
        return(Read8255(&PPI, Port - 0xA8));

    case 0xFC: /* Mapper page at 0000h */
    case 0xFD: /* Mapper page at 4000h */
    case 0xFE: /* Mapper page at 8000h */
    case 0xFF: /* Mapper page at C000h */
        return(RAMMapper[Port - 0xFC] | ~RAMMask);

    case 0xD9: /* Kanji support */
        Port = Kanji ? Kanji[KanLetter + KanCount] : NORAM;
        KanCount = (KanCount + 1) & 0x1F;
        return(Port);

    case 0x80: /* SIO data */
    case 0x81:
    case 0x82:
    case 0x83:
    case 0x84:
    case 0x85:
    case 0x86:
    case 0x87:
        return(NORAM);
        /*return(Rd8251(&SIO,Port&0x07));*/

    case 0x98: /* VRAM read port */
#ifdef TURBO_R
        UpdateVDPDelay();
#endif // TURBO_R
        /* Read from VRAM data buffer */
        Port = VDPData;
        /* Reset VAddr latch sequencer */
        VKey = 1;
        /* Fill data buffer with a new value */
#ifdef TURBO_R
        if ((ScrMode == 7) || (ScrMode == 8))
        {
            int PageInt = VRAMPageInt + (int)VAddr;
            //int PageInt = (int)(VPAGE-VRAM) + (int)VAddr;
            VDPData = VRAM[(PageInt >> 1) | ((PageInt & 1) << 16)];
        }
        //else VDPData = VRAM[(int)(VPAGE-VRAM) + (int)VAddr];
        else VDPData = VRAM[VRAMPageInt + (int)VAddr];
#else
        VDPData = VPAGE[VAddr];
#endif // TURBO_R
        /* Increment VRAM address */
        VAddr = (VAddr + 1) & 0x3FFF;
        /* If rolled over, modify VRAM page# */
        if (!VAddr && (ScrMode > 3))
        {
            VDP[14] = (VDP[14] + 1) & (VRAMPages - 1);
#ifdef TURBO_R
            VRAMPageInt = VDP[14] << 14;
#else
            VPAGE = VRAM + ((int)VDP[14] << 14);
#endif // TURBO_R
        }
        return(Port);

    case 0x99: /* VDP status registers */
#ifdef TURBO_R
        UpdateVDPDelay();
#endif // TURBO_R

        /* Read an appropriate status register */
        Port = VDPStatus[VDP[15]];
        /* Reset VAddr latch sequencer */
      // @@@ This breaks Sir Lancelot on ColecoVision, so it must be wrong!
      //  VKey=1;
        /* Update status register's contents */
#ifdef TURBO_R
  //VDPSync();
    /* Update the Sprite Collision flag. This fix some games that made by BASIC.(10lines hero etc.)*/
        if (!VDP[15])
            //if (!VDP[15] && !SpritesOFF)
        {
            if (!SpritesOFF)
            {
                if (IsSpriteColi)
                {
                    Port |= 0x20;
                }
                IsSpriteColi = 0;
            }
        }
        if (VDP[15] == 5)VDPStatus[3] = VDPStatus[4] = VDPStatus[5] = VDPStatus[6] = 0;

        /* There value might differ depend on the hardware. For MSX2, some disks need this.(Art Gallery in Disk Station #9, #10, #11 ... etc.) */
        VKey = 1;
#endif // TURBO_R

        switch (VDP[15])
        {
#ifdef TURBO_R
        case 0: VDPStatus[0] &= 0x1F; SetIRQ(~INT_IE0); break;
        case 1:
            if (((VDP[0] & 0x10) != 0) && ((VDPStatus[1] & 0x01) != 0))
            {
                VDPStatus[1] &= 0xFE;
                SetIRQ(~INT_IE1);
            }
            break;
#else
        case 0: VDPStatus[0] &= 0x5F; SetIRQ(~INT_IE0); break;
        case 1: VDPStatus[1] &= 0xFE; SetIRQ(~INT_IE1); break;
#endif // TURBO_R
            //case 1: VDPStatus[1]&=0xFE;SetIRQ(~INT_IE1);break;
        case 7: VDPStatus[7] = VDP[44] = VDPRead(); break;
        }
        /* Return the status register value */
        return(Port);

    case 0xA2: /* PSG input port */
      /* PSG[14] returns joystick/mouse data */
        if (PSG.Latch == 14)
        {
            int DX, DY, L, J;

            /* Number of a joystick port */
            Port = (PSG.R[15] & 0x40) >> 6;
            L = JOYTYPE(Port);

            /* If no joystick, return dummy value */
            if (L == JOY_NONE) return(0x7F);

#ifdef TURBO_R
            if (currJoyMode[Port] == JOY_ARKANOID)
            {
                Port = (JoyState & JST_FIREA) == 0x10 ? 1 : 0;
                Port = (Port << 1) ^ 2;
                return 0x3C | ((PaddleShift[0] >> PadCount[0]) & 1) | Port;
            }
#endif // TURBO_R

#ifdef TURBO_R
            MouseDX[Port] = MouseDX3DS[Port];
            MouseDY[Port] = MouseDY3DS[Port];

#else
            /* Compute mouse offsets, if needed */
            if (MCount[Port] == 1)
            {
                /* Get new mouse coordinates */
                DX = MouState[Port] & 0xFF;
                DY = (MouState[Port] >> 8) & 0xFF;
                /* Compute offsets and store coordinates  */
                J = OldMouseX[Port] - DX; OldMouseX[Port] = DX; DX = J;
                J = OldMouseY[Port] - DY; OldMouseY[Port] = DY; DY = J;
                /* For 512-wide mode, double horizontal offset */
                if ((ScrMode == 6) || ((ScrMode == 7) && !ModeYJK) || (ScrMode == MAXSCREEN + 1)) DX <<= 1;
                /* Adjust offsets */
                MouseDX[Port] = (DX > 127 ? 127 : (DX < -127 ? -127 : DX)) & 0xFF;
                MouseDY[Port] = (DY > 127 ? 127 : (DY < -127 ? -127 : DY)) & 0xFF;
            }
#endif // TURBO_R

            /* Get joystick state */
            J = ~(Port ? (JoyState >> 8) : JoyState) & 0x3F;

            /* Determine return value */
            switch (MCount[Port])
            {
            case 0: Port = PSG.R[15] & (0x10 << Port) ? 0x3F : J; break;
            case 1: Port = (MouseDX[Port] >> 4) | (J & 0x30); break;
            case 2: Port = (MouseDX[Port] & 0x0F) | (J & 0x30); break;
            case 3: Port = (MouseDY[Port] >> 4) | (J & 0x30); break;
            case 4: Port = (MouseDY[Port] & 0x0F) | (J & 0x30); break;
            }

            /* 6th bit is always 1 */
            return(Port | 0x40);
        }

        /* PSG[15] resets mouse counters (???) */
        if (PSG.Latch == 15)
        {
            /* @@@ For debugging purposes */
            /*printf("Reading from PSG[15]\n");*/

            /*MCount[0]=MCount[1]=0;*/
            return(PSG.R[15] & 0xF0);
        }

        /* Return PSG[0-13] as they are */
#ifdef ALTSOUND
        return ReadPSG(PSG.Latch);
#else
        return(RdData8910(&PSG));
#endif

#ifdef TURBO_R
    case 0xDB: /* Kanji support for Kanji level 2 */
        Port = Kanji2 ? Kanji2[KanLetter + KanCount] : NORAM;
        KanCount = (KanCount + 1) & 0x1F;
        return(Port);

    case 0xBA: /*Lightpen Interface*/   //To be written later.
        return(NORAM);

        //case 0xF3:  /* Screen Mode Register for MSX2+ */
        // /* I fond no games or apps that using this. */
        //    if (UseCBIOS || !(MODEL(MSX_MSX2P)))return(NORAM);
        //    Port = (VDP[0] & 0x02) << 6 | (VDP[0] & 0x04) << 4 | (VDP[0] & 0x08) << 2 | (VDP[1] & 0x08) << 1 | (VDP[1] & 0x10) >> 1 | SolidColor0 >> 3 | ModeYJK >> 2
        //        | ModeYAE >> 4;
        //    return Port;

    case 0xF3:
        if (UseCBIOS || !(MODEL(MSX_MSX2P)) || !(MODEL(MSX_MSXTR)))return(NORAM);
        return ScreenModeReg;

    case 0xF4:  /* F4 Register for MSX2+ */
        if (MODEL(MSX_MSXTR))
        {
            Port = IsHardReset ? 0 : 0x80;
            Port |= (CPU.User & 0x01) ? 0x20 : 0;
            //IsHardReset = 0;
            if (Verbose & 0x20) printf("I/O: Read F4 Register [%02Xh]\n", Port);
            return Port;
            //return 0x00;
        }

        if (UseCBIOS || !(MODEL(MSX_MSX2P)))return(NORAM);
        Port = (SkipBIOS || !IsHardReset || !HasKanjiBASIC) ? (InvertF4Reg ? 0xFF : 0x00) : (InvertF4Reg ? 0x00 : 0xFF);
        IsHardReset = 0;
        if (Verbose & 0x20) printf("I/O: Read F4 Register [%02Xh]\n", Port);
        return Port;

    case 0x12:  /* Secondary PSG Port for MegaFlashROM SCC, MANBOW2 etc. */
        if (CartSpecial[0] != CART_MEGASCC && CartSpecial[1] != CART_MEGASCC)return(NORAM);
        return ReadPSG(PSG.Latch);

    case 0xA4:      /* PCM Read for MSX TurboR */
        /* https://w.atwiki.jp/msx-sdcc/pages/67.html */
        /* 15750Hz(63.5uSec) = 448 Clock. */
        //return(((PCMTimerCnt - CPU.ICount) / 448) & 0x3);
        //return PCMTimerReg;
        return(((PCMTimerCnt - CPU.ICount) / 456) & 0x3);

    case 0xA5:      /* PCM Read for MSX Turbo R */
        return((MODEL(MSX_MSXTR) ? ((~PCMSample & 0x80) | PCMStatus) : 0xFF));
    case 0xA7:      /* Pause and LED for MSX TurboR */
        return(R800Pause & 0x01);

    case 0xE4:  /* S1990 for MSX TurboR */
        if (Verbose & 0x40) printf("I/O: Read from S1990 [%02Xh] = %04Xh\n", Port, S1990Latch);
        return((MODEL(MSX_MSXTR) ? S1990Latch : 0xFF));

    case 0xE5:  /* S1990 for MSX TurboR */
        if (Verbose & 0x40) printf("I/O: Read from S1990 [%02Xh]\n", Port);
        switch (S1990Latch)
        {
        case 5:
            return 0x00;
        case 6:
            if (Verbose & 0x40) printf("S1990 Reg6 = %04Xh\n", S1990Reg6);
            return S1990Reg6;
        case 13:
            return(3);
        case 14:
            return(47);
        case 15:
            return(139);
        default:
            break;
        }
        return(NORAM);
    case 0xE6:      /* MSX TurboR System Timer */
            /* Increase every 3.911 micro sec -> 28 CPU cycle Count.(Memory Refresh) */
        return(((R800TimerCnt - CPU.ICount) / 28) & 0xFF);

    case 0xE7:      /* MSX TurboR System Timer */
        return((((R800TimerCnt - CPU.ICount) / 28) >> 8) & 0xFF);

    case 0xE9:  /* MSX TurboR MIDI */      //To be written later.
        return(0xFF);
#endif // TURBO_R


    case 0xD0: /* FDC status  */
    case 0xD1: /* FDC track   */
    case 0xD2: /* FDC sector  */
    case 0xD3: /* FDC data    */
    case 0xD4: /* FDC IRQ/DRQ */
#ifdef DEBUG_LOG
        if (Verbose & 0x04)printf("Disk IO Read[%02Xh]\n", Port);
#endif // DEBUG_LOG
#ifdef TURBO_R
        /* MSXTurboR doesn't support disk access from IO port. */
        if (MODEL(MSX_MSXTR))return(NORAM);
#endif // TURBO_R
        /* Brazilian DiskROM I/O ports */
        return(Read1793(&FDC, Port - 0xD0));

    }

    /* Return NORAM for non-existing ports */
    if (Verbose & 0x20) printf("I/O: Read from unknown PORT[%02Xh]\n", Port);
    return(NORAM);
}

/** OutZ80() *************************************************/
/** Z80 emulation calls this function to write byte V to a  **/
/** given I/O port.                                         **/
/*************************************************************/
void OutZ80(word Port, byte Value)
{
    register byte I, J;

#ifdef TURBO_R
    byte PS, SS;
    int PageInt, Val;
#endif // TURBO_R

    Port &= 0xFF;
    switch (Port)
    {

#ifdef _MSX0
    case 0x08:
    case 0x58:  /* MSX0 IO firmware 0.07.08 */
        OutMSX0IOT(Value);
        return;
#endif // _MSX0

#ifdef VDP_V9990
        /* Ymaha V9990 VDP Output. Based on infomation from msx-sdcc@wiki*/
        /* https://w.atwiki.jp/msx-sdcc/pages/50.html */
        /* And Yamaha V9990 E-VDP-III Programmers Manual by MSX BANZAI!*/
        /* https://web.archive.org/web/20230224003838/http://msxbanzai.tni.nl/v9990/manual.html */
        /* And openMSX and WebMSX.*/
        /* https://openmsx.org */
        /* http://webmsx.org */
    case 0x60:  /*  VRAM DATA  */
        if (!V9990Active)return;
        //if (Verbose & 0x20) printf("I/O: Write V9990 VRAM Data[%02Xh] : [%02Xh] \n", vdpV9990.VRAMWriteInt , Value);
        V9KVRAM[vdpV9990.VRAMWriteInt] = Value;
        if (vdpV9990.IncVRAMWrite)vdpV9990.VRAMWriteInt = (vdpV9990.VRAMWriteInt + 1) & 0x7FFFF;
        return;
    case 0x61: /* PALETTE DATA */
        if (!V9990Active)return;
        if ((V9990VDP[14] & 0x03) == 0x03)
        {
            V9990VDP[14] &= ~0x03;
            return;
        }
        Value &= !(V9990VDP[14] & 0x03) ? 0x9F : 0x1F;
        V9KPal[V9990VDP[14]] = Value;
        V9990SetPalette();
        V9990VDP[14] = (V9990VDP[14] & 0x03) == 0x02 ? (V9990VDP[14] + 2) & 0xFF : V9990VDP[14] + 1;
        return;
    case 0x62:  /* COMMAND DATA */
        if (!V9990Active)return;
        VDPWriteV9990(Value);
        return;
    case 0x63:  /* REGISTER DATA */
        if (Verbose & 0x20)
        {
            if ((((vdpV9990.SelectReg) & 0x3F) == 0x06) || (((vdpV9990.SelectReg) & 0x3F) == 0x07) || (((vdpV9990.SelectReg) & 0x3F) == 0x1B)
                //    || ((((vdpV9990.SelectReg) & 0x3F) == 44) && (Value != 0))
                //    || ((((vdpV9990.SelectReg) & 0x3F) == 0x12) && (Value & 0xC0))
                //    || (((vdpV9990.SelectReg) & 0x3F) == 0x2E) || (((vdpV9990.SelectReg) & 0x3F) == 0x2F)
                || ((((vdpV9990.SelectReg) & 0x3F) == 0x16) && (Value & 0xC0)) || ((((vdpV9990.SelectReg) & 0x3F) == 0x0D) && (Value & 0xE0)))
            {
                printf("I/O: Write V9990 Register Data VDP[%02Xh] : [%02Xh] \n", vdpV9990.SelectReg & 0x3F, Value);
            }
        }

        if (!V9990Active)return;
        V9990Out((vdpV9990.SelectReg)&0x3F, Value);
        if (!(vdpV9990.SelectReg & 0x80))
        {
            vdpV9990.SelectReg = (vdpV9990.SelectReg & ~0x3F) | ((vdpV9990.SelectReg + 1) & 0x3F);
        }
        return;
    case 0x64:  /* REGISTER SELECT */
        //if (Verbose & 0x20) printf("I/O: Write V9990 Register Select[%02Xh]\n", Value);
        if (!V9990Active)InitV9990();

        vdpV9990.SelectReg = Value;
        return;
    case 0x66:  /* INTERRUPT FLAG */
        //if (Verbose & 0x20) printf("I/O: Write V9990 Interrupt Flag[%02Xh]\n", Value);
        //if (!V9990Active)return;
        if (!V9990Active)InitV9990();   /* Support SymbOS. http://www.symbos.de/ */
        if (Value & 0x07)
        {
            V9990Port[6] &= ~Value;
            SetIRQV9K();
        }
        return;
    case 0x67:  /* SYSTEM CONTROL */
        if (Verbose & 0x20) printf("I/O: Write V9990 System Control[%02Xh]\n", Value);
        if (!V9990Active)InitV9990();
        if (!(vdpV9990.SystemControlReg & 0x02) && (Value&0x02))
        {
            V9990Port[0] = V9990Port[1] = V9990Port[2] = V9990Port[3] = V9990Port[4] = V9990Port[5] = V9990Port[6] = 0;
            vdpV9990.IncVRAMWrite = 0;
            vdpV9990.VRAMWriteInt = 0;
            vdpV9990.IncVRAMRead = 0;
            vdpV9990.VRAMReadInt = 0;
            vdpV9990.SelectReg = 0;
            vdpV9990.pending = 0;
        }
        vdpV9990.SystemControlReg = Value;
        return;
    case 0x68: /* Upper bits of Kanji ROM address */
    case 0x6A: /* Upper bits of Kanji ROM address for Kanji level 2 */
        if (!V9990Active)return;
        KanLetter = (KanLetter & 0x1F800) | ((int)(Value & 0x3F) << 5);
        KanCount = 0;
        return;
    case 0x69: /* Lower bits of Kanji ROM address */
    case 0x6B: /* Lower bits of Kanji ROM address for Kanji level 2*/
        if (!V9990Active)return;
        KanLetter = (KanLetter & 0x007E0) | ((int)(Value & 0x3F) << 11);
        KanCount = 0;
        return;
    case 0x6F:
        //if (!V9990Active)InitV9990();
        return;
#endif // VDP_V9990

#ifdef TURBO_R
        //case 0x7C: if (Use2413) OPLL.Latch = Value; EnableFMSound(0); return;              /* OPLL Register# */
    case 0x7C:
        if (Use2413)
        {
            OPLL.Latch = Value;
            WriteOPLL(Port, Value);
            ChangeFMSound(0);
            //EnableFMSound(0);
        }
        return;              /* OPLL Register# */
    case 0x7D:
        if (Use2413)
        {
            //WriteOPLL(OPLL.Latch, Value);
            WriteOPLL(Port, Value);
            IsSndRegUpd = IsSndRegUpd == 0xFF ? 0 : IsSndRegUpd + 1; return;
        }/* OPLL Data      */
        return;
    case 0xA0: PSG.Latch = Value; WritePSG(Port, Value); WrCtrl8910(&PSG, Value); return;
    case 0xC0:
        if (Use8950)
        {
            WriteAUDIO(0, Value);
            ChangeFMSound(1);
            //EnableFMSound(1);
        }/* AUDIO Register#*/
        return;
    case 0xC1: if (Use8950) { WriteAUDIO(1, Value); IsSndRegUpd = IsSndRegUpd == 0xFF ? 0 : IsSndRegUpd + 1; }return;
    case 0x10: PSG.Latch = Value; WritePSG(Port, Value); WrCtrl8910(&PSG, Value); return;    /* Secondary PSG Port for MegaFlashROM SCC, MANBOW2 */
    case  0x11:   /* Secondary PSG Port for MegaFlashROM SCC, MANBOW2 */
        if (CartSpecial[0] != CART_MEGASCC && CartSpecial[1] != CART_MEGASCC)return;
        //WritePSG(PSG.Latch, Value);
        WritePSG(Port, Value);
        WrData8910(&PSG, Value);
        IsSndRegUpd = IsSndRegUpd == 0xFF ? 0 : IsSndRegUpd + 1;
        return;
    case 0x0F:    /* Sram Mapper for Zemina bootleg Games. Dragon Slayer2 Xanadu etc(SRAM not working now). */
        for (J = 0; J < 4; ++J)
        {
            PS = PSL[J];
            SS = SSL[J];
            I = CartMap[PS][SS];
            if (ROMType[I] == MAP_ZeminaDS2)break;
        }
        if (ROMType[I] != MAP_ZeminaDS2)return;
        if (Value == 0xA0)
        {
            IsZeminaSram = 0;
        }
        else if (Value == 0x90)
        {
            IsZeminaSram = 1;
        }
        return;
    case 0x77:    /* ROM Mapper for Super Game 90*/
        for (J = 0; J < 4; ++J)
        {
            PS = PSL[J];
            SS = SSL[J];
            I = CartMap[PS][SS];
            if (ROMType[I] == MAP_MSX90)break;
        }
        if (ROMType[I] != MAP_MSX90)return;
        if (Value & 0x80)
        {
            Value = (((Value & 0x7F) << 1) & ROMMask[I]) & 0xFC;
            if (Value != ROMMapper[I][0])
            {
                RAM[2] = MemMap[PS][SS][2] = ROMData[I] + ((int)Value << 13);
                RAM[3] = MemMap[PS][SS][3] = RAM[2] + 0x2000;
                RAM[4] = MemMap[PS][SS][4] = RAM[2] + 0x4000;
                RAM[5] = MemMap[PS][SS][5] = RAM[2] + 0x6000;
                ROMMapper[I][0] = Value;
                ROMMapper[I][1] = Value + 1;
                ROMMapper[I][2] = Value + 2;
                ROMMapper[I][3] = Value + 3;
            }
            return;
        }
        else
        {
            //Value = (Value << 1) & ROMMask[I];
            Value = ((Value & 0x7F) << 1) & ROMMask[I];
            if (Value != ROMMapper[I][0])
            {
                RAM[2] = RAM[4] = MemMap[PS][SS][2] = ROMData[I] + ((int)Value << 13);
                RAM[3] = RAM[5] = MemMap[PS][SS][3] = RAM[2] + 0x2000;
                ROMMapper[I][0] = ROMMapper[I][2] = Value;
                ROMMapper[I][1] = ROMMapper[I][3] = Value | 1;
            }
            return;
        }
        return;
#elif ALTSOUND
    case 0x7C: if (Use2413) OPLL.Latch = Value; return;              /* OPLL Register# */
    case 0x7D: if (Use2413) WriteOPLL(OPLL.Latch, Value); return;    /* OPLL Data      */
    case 0xA0: PSG.Latch = Value; WrCtrl8910(&PSG, Value); return;
    case 0xC0: if (Use8950) WriteAUDIO(0, Value); return;            /* AUDIO Register#*/
    case 0xC1: if (Use8950) WriteAUDIO(1, Value); return;            /* AUDIO Data     */
#else
    case 0x7C: WrCtrl2413(&OPLL, Value); return;        /* OPLL Register# */
    case 0x7D: WrData2413(&OPLL, Value); return;        /* OPLL Data      */
    case 0xA0: WrCtrl8910(&PSG, Value); return;         /* PSG Register#  */
#endif  //TURBO_R
#ifdef TURBO_R
    case 0x91:                            /* Printer Data   */
        switch (PrinterMode)
        {
        case PRINTER_NONE:
            return;
        case PRINTER_PRINT2FILE:
            Printer(Value);
            return;

            /* Emulate Voice BOX*/
            /* http://hirosedou.sblo.jp/article/93257998.html */
        case PRINTER_VOICEBOX:
            DA8bit = (int)(((Value & 0x0F) >> 1) << 10) * ((Value & 0x01) == 0x01 ? 1 : -1);
            IsSndRegUpd = IsSndRegUpd == 0xFF ? 0 : IsSndRegUpd + 1;
            switch (SoundSampRate)
            {
            case 0:
                PrinterStatus = Value == 0 ? 0xFF : 3;
                break;
            case 1:
                PrinterStatus = Value == 0 ? 0xFF : 6;
                break;
            }
            //PrinterStatus = Value == 0 ? 0xFF : 6;
            //PrinterStatus = Value == 0 ? 0xFF : 3;
            //if (Verbose & 0x20)
            //    printf("I/O: Write to PORT[%02Xh]=%02Xh\n", Port, Value);
            return;

            /* Emulate +PCM */
            /* http://hp.vector.co.jp/authors/VA011751/MSXSR8-2.HTM */
        case PRINTER_PLUSPCM:
            if ((Value & 0xE0) == 0xE0)
            {
                PrinterStatus = 1;
                IsSndRegUpd = IsSndRegUpd == 0xFF ? 0 : IsSndRegUpd + 1;
                //adpcmpos++;
                if (adpcmpos < 224)
                {
                    ADPCMData[adpcmpos] = Value;
                    adpcmpos++;
                }
            }
            else if (!(Value & 0x80))
            {
                PrinterValue = 16;
                PrinterStatus = 0;
            }
            //if (Verbose & 0x20)
            //    printf("I/O: Write to PORT[%02Xh]=%02Xh\n", Port, Value);
            return;
        case PRINTER_COVOX:
            //DA8bit >>= 1;
            //DA8bit += ((int)Value - 0x80) << 3;
            DA8bit = ((int)Value - 0x80) << 5;
            if (DA8bit)IsSndRegUpd = IsSndRegUpd == 0xFF ? 0 : IsSndRegUpd + 1;
            break;
        default:
            break;
        }
#else
    case 0x91: Printer(Value); return;                 /* Printer Data   */
#endif // TURBO_R
    case 0xB4: RTCReg = Value & 0x0F; return;              /* RTC Register#  */

    case 0xD8: /* Upper bits of Kanji ROM address */
#ifdef TURBO_R
    case 0xDA: /* Upper bits of Kanji ROM address for Kanji level 2 */
#endif // TURBO_R
        KanLetter = (KanLetter & 0x1F800) | ((int)(Value & 0x3F) << 5);
        KanCount = 0;
        return;

    case 0xD9: /* Lower bits of Kanji ROM address */
#ifdef TURBO_R
    case 0xDB: /* Lower bits of Kanji ROM address for Kanji level 2*/
#endif // TURBO_R
        KanLetter = (KanLetter & 0x007E0) | ((int)(Value & 0x3F) << 11);
        KanCount = 0;
        return;

    case 0x80: /* SIO data */
    case 0x81:
    case 0x82:
    case 0x83:
    case 0x84:
    case 0x85:
    case 0x86:
    case 0x87:
        return;
        /*Wr8251(&SIO,Port&0x07,Value);
        return;*/

    case 0x98: /* VDP Data */
        VKey = 1;
#ifdef TURBO_R
        UpdateVDPDelay();
        if ((ScrMode == 7) || (ScrMode == 8))
        {
            PageInt = VRAMPageInt + (int)VAddr;
            //PageInt = (int)(VPAGE - VRAM) + (int)VAddr;
            VDPData = VRAM[(PageInt >> 1) | ((PageInt & 1) << 16)] = Value;
        }
        else VDPData = VRAM[VRAMPageInt + (int)VAddr] = Value;
        //else VDPData = VRAM[(int)(VPAGE-VRAM) + (int)VAddr] = Value;
        //else VDPData = VPAGE[VRAMPageInt + (int)VAddr] = Value;
#else
        VDPData = VPAGE[VAddr] = Value;
#endif // TURBO_R
        VAddr = (VAddr + 1) & 0x3FFF;
        /* If VAddr rolled over, modify VRAM page# */
        if (!VAddr && (ScrMode > 3))
        {
            VDP[14] = (VDP[14] + 1) & (VRAMPages - 1);
#ifdef TURBO_R
            VRAMPageInt = (int)VDP[14] << 14;
#else
            VPAGE = VRAM + ((int)VDP[14] << 14);
#endif // TURBO_R
        }
        return;

    case 0x99: /* VDP Address Latch */
#ifdef TURBO_R
        UpdateVDPDelay();
#endif // TURBO_R

        if (VKey) { ALatch = Value; VKey = 0; }
        else
        {
            VKey = 1;
            switch (Value & 0xC0)
            {
            case 0x80:
                /* Writing into VDP registers */
                VDPOut(Value & 0x3F, ALatch);
                break;
            case 0x00:
            case 0x40:

                /* Set the VRAM access address */
#ifdef TURBO_R
                VAddr = (((word)(Value & 0x3F) << 8) | ALatch) & 0x3FFF;
                //VAddr = (((word)(Value&0x3F) << 8) | ALatch | VDP[14]<<14) & 0x3FFF;

                //VAddr = Port | ((word)(Value & 0x3F) << 8) | VDP[14] << 14;
#else
                VAddr = (((word)Value << 8) + ALatch) & 0x3FFF;
#endif // TURBO_R
                /* When set for reading, perform first read */
                if (!(Value & 0x40))
                {
#ifdef TURBO_R
                    if ((ScrMode == 7) || (ScrMode == 8))
                    {
                        PageInt = VRAMPageInt + (int)VAddr;
                        //PageInt = (int)(VPAGE - VRAM) + (int)VAddr;
                        VDPData = VRAM[(PageInt >> 1) | ((PageInt & 1) << 16)];
                    }
                    //else VDPData = VRAM[(int)(VPAGE - VRAM) + (int)VAddr];
                    else VDPData = VRAM[VRAMPageInt + (int)VAddr];
#else
                    VDPData = VPAGE[VAddr];
#endif // TURBO_R
                    VAddr = (VAddr + 1) & 0x3FFF;
                    if (!VAddr && (ScrMode > 3))
                    {
                        VDP[14] = (VDP[14] + 1) & (VRAMPages - 1);
#ifdef TURBO_R
                        PageInt = (int)VDP[14] << 14;
                        //VRAMPageInt = (int)VDP[14] << 14;
#else
                        VPAGE = VRAM + ((int)VDP[14] << 14);
#endif // _3DS
                    }
                }
                break;
            }
        }
        return;

    case 0x9A: /* VDP Palette Latch */
#ifdef TURBO_R
        UpdateVDPDelay();
#endif // TURBO_R
        if (PKey) { PLatch = Value; PKey = 0; }
        else
        {
            byte R, G, B;
            /* New palette entry written */
            PKey = 1;
            J = VDP[16];
            /* Compute new color components */
            R = (PLatch & 0x70) * 255 / 112;
            G = (Value & 0x07) * 255 / 7;
            B = (PLatch & 0x07) * 255 / 7;
            /* Set new color for palette entry J */
            Palette[J] = RGB2INT(R, G, B);
            SetColor(J, R, G, B);
            /* Next palette entry */
            VDP[16] = (J + 1) & 0x0F;
        }
        return;

    case 0x9B: /* VDP Register Access */
#ifdef TURBO_R
        UpdateVDPDelay();
#endif // TURBO_R
        J = VDP[17] & 0x3F;
        if (J != 17) VDPOut(J, Value);
        if (!(VDP[17] & 0x80)) VDP[17] = (J + 1) & 0x3F;
        return;

    case 0xA1: /* PSG Data */
#ifdef TURBO_R
        if (PSG.Latch != 14 && PSG.Latch != 15)IsSndRegUpd = IsSndRegUpd == 0xFF ? 0 : IsSndRegUpd + 1;
        if (PSG.Latch == 8 || PSG.Latch == 9 || PSG.Latch == 10)IsSndRegUpd = IsSndRegUpd == 0xFF ? 0 : IsSndRegUpd + 1;
        if (currJoyMode[0] == JOY_ARKANOID)
        {
            if (PSG.Latch == 15)
            {
                Val = (Value & 0x03) | ((Value >> 2) & 4); /* Port 1*/
                //Val =(Value>>2&0x03)|(Value>>3&0x04); /* Port 2*/
                J = Val & (Val ^ OldPaddleVal);
                OldPaddleVal = Val;
                if (J & 0x01)PadCount[0] -= (PadCount[0] != 0);
                if (J & 0x04)
                {
                    PadCount[0] = 8;
                    PaddleShift[0] = PaddleState[0];
                }
            }
        }
        else
#endif // TURBO_R
            /* PSG[15] is responsible for joystick/mouse */
            if (PSG.Latch == 15)
            {
                /* @@@ For debugging purposes */
                /*printf("Writing PSG[15] <- %02Xh\n",Value);*/

#ifdef TURBO_R
    /* For mouse, update nibble counter      */
    /* For joystick, set nibble counter to 0 */
                if ((JOYTYPE(1) == JOY_MOUSE) && ((Value ^ PSG.R[15]) & 0x20))MCount[1] += MCount[1] == 4 ? -3 : 1;
                else if ((Value & 0x0C) == 0x0C) MCount[1] = 0;
                /* Set nibble counter to 0 only no mouse. This fix some homebrew that use mouse on MSX1 */
                /* msx-windows etc. https://github.com/albs-br/msx-windows */

                /* For mouse, update nibble counter      */
                /* For joystick, set nibble counter to 0 */
                if ((JOYTYPE(0) == JOY_MOUSE) && ((Value ^ PSG.R[15]) & 0x10))MCount[0] += MCount[0] == 4 ? -3 : 1;
                else if ((Value & 0x03) == 0x03) MCount[0] = 0;
#else
    /* For mouse, update nibble counter      */
    /* For joystick, set nibble counter to 0 */
                if ((Value & 0x0C) == 0x0C) MCount[1] = 0;
                else if ((JOYTYPE(1) == JOY_MOUSE) && ((Value ^ PSG.R[15]) & 0x20))
                    MCount[1] += MCount[1] == 4 ? -3 : 1;

                /* For mouse, update nibble counter      */
                /* For joystick, set nibble counter to 0 */
                if ((Value & 0x03) == 0x03) MCount[0] = 0;
                else if ((JOYTYPE(0) == JOY_MOUSE) && ((Value ^ PSG.R[15]) & 0x10))
                    MCount[0] += MCount[0] == 4 ? -3 : 1;
#endif // TURBO_R
            }

#ifdef ALTSOUND
        //WritePSG(PSG.Latch, Value);
        WritePSG(Port, Value);
        WrData8910(&PSG, Value);
#else
        /* Put value into a register */
        WrData8910(&PSG, Value);
#endif
        return;

    case 0xA8: /* Primary slot state   */
    case 0xA9: /* Keyboard port        */
    case 0xAA: /* General IO register  */
    case 0xAB: /* PPI control register */
      /* Write to PPI */
        Write8255(&PPI, Port - 0xA8, Value);
        /* If general I/O register has changed... */
        if (PPI.Rout[2] != IOReg) { PPIOut(PPI.Rout[2], IOReg); IOReg = PPI.Rout[2]; }
        /* If primary slot state has changed... */
        if (PPI.Rout[0] != PSLReg) PSlot(PPI.Rout[0]);
        /* Done */
#ifdef TURBO_R
  /* Generate 1bit PCM DA Sound. Many japanese games use this for voice sampling.
   Ninja Kage(Hudson) , Indian no bouken(Hudson) ,and so on.
   Taken from LibKSS
    https://github.com/digital-sound-antiques/libkss
    */
        if (Port == 0xAA)
        {
            DA1bit >>= 1;
            DAVal = (Value & 0x80) ? 256 << 3 : 0;
            DA1bit += DAVal;
            if (DA1bit)IsSndRegUpd = IsSndRegUpd == 0xFF ? 0 : IsSndRegUpd + 1;
        }
        else if (Port == 0xAB)
        {
            DA1bit >>= 1;
            DAVal = (Value & 0x01) ? 256 << 3 : 0;
            DA1bit += DAVal;
            if (DA1bit)IsSndRegUpd = IsSndRegUpd == 0xFF ? 0 : IsSndRegUpd + 1;
        }
#endif // TURBO_R

        return;

    case 0xB5: /* RTC Data */
        if (RTCReg < 13)
        {
            /* J = register bank# now */
            J = RTCMode & 0x03;
            /* Store the value */
            RTC[J][RTCReg] = Value;
            /* If CMOS modified, we need to save it */
            if (J > 1) SaveCMOS = 1;
            return;
        }
        /* RTC[13] is a register bank# */
        if (RTCReg == 13) RTCMode = Value;
        return;

#ifdef TURBO_R
    case 0xA4:      /* PCM register for MSX TurboR */
        PCMTimerCnt = CPU.ICount;
        PCMTimerReg = 0;
        if (PCMStatus & 0x02)
        {
#ifdef ALTPCM
            PCMSample = Value;
            if (PCMPos < 8)
            {
                PCMData[PCMPos] = PCMSample;
                PCMPos++;
            }
#else
            PCMSample = Value;
            DA8bit = ((int)PCMSample - 0x80) << 5;
#endif // ALTPCM
            IsSndRegUpd = IsSndRegUpd == 0xFF ? 0 : IsSndRegUpd + 1;
        }
        else
        {
            PCMSample = 0;
            DA8bit = 0;
        }
        return;
    case 0xA5:      /* PCM register for MSX TurboR */
        if ((Value & 0x03) == 0x03 && (~PCMStatus & 0x01))
        {
#ifdef ALTPCM
            if (PCMPos < 8)
            {
                PCMData[PCMPos] = PCMSample;
                PCMPos++;
            }
#else
            DA8bit = ((int)PCMSample - 0x80) << 5;
#endif // ALTPCM
            IsSndRegUpd = IsSndRegUpd == 0xFF ? 0 : IsSndRegUpd + 1;
        }
        PCMStatus = Value & 0x1F;
        if (Verbose & 0x40) printf("I/O: Write PCM Register [%02Xh] : [%02Xh] \n", Port, Value);
        return;

    case 0xA7:
        if (Verbose & 0x40) printf("I/O: Write R800 Pause [%02Xh] : [%02Xh] \n", Port, Value);
        //R800Pause = Value;
        //R800Pause = (Value & 0xFE) | ((R800Pause & 0x01) ? 0: 1);
        R800Pause = Value & 0xFC;
        if (Value & 0x02) R800Pause |= (R800Pause & 0x02) ? 0 : 2;
        R800Pause |= (R800Pause & 0x02) ? Value & 0x01 : 0;
        return;

    case 0xBA: /*Lightpen Interface*/   //To be written later.
        return;

    case 0xF3: /* Screen Mode management for MSX2+, MSXTurboR */
        //if (!(Mode & MSX_MSX2P))return;
        //if (((VDP[0] & 0x0E) == ((Value & 0x07) << 1)) && ((VDP[1] & 0x18) == (Value & 0x18)))return;
        //ScreenModeReg = Value;
        //VDP[0] = (VDP[0] & 0xF1) | ((Value & 0x07)<<1);
        //VDP[1] = (VDP[1] & 0xE7) | (Value & 0x18);
        //SetScreen();

        ScreenModeReg = Value;
        return;
    case 0xF4:  /* F4 Register for MSX2+, MSXTurboR */
        if (Verbose & 0x20) printf("I/O: Write F4 Register [%02Xh] : [%02Xh] \n", Port, Value);
#ifdef TURBO_R
        if (MODEL(MSX_MSXTR))
        {
            if (Value & 0x80)IsHardReset = 0;
            return;
        }
#endif // TURBO_R
        if (UseCBIOS || !(MODEL(MSX_MSX2P)))return;
        if (Value & 0x80)IsHardReset = 0;
        return;
#endif // _3DS

#ifdef TURBO_R
    case 0xE4:  /* S1990 for MSX Turbo R*/
        if (MODEL(MSX_MSXTR))S1990Latch = Value & 0x0F;
        if (Verbose & 0x40) printf("I/O: Write S1990 [%02Xh] : [%02Xh] \n", Port, Value);
        return;

    case 0xE5:  /* S1990 for MSX Turbo R*/
        if (Verbose & 0x40) printf("I/O: Write S1990 [%02Xh] : [%02Xh] \n", Port, Value);
        if (S1990Latch == 6 && MODEL(MSX_MSXTR))
        {
            Value &= 0x60;
            if (S1990Reg6 != Value)
            {
                S1990Reg6 = Value;
                DoCPUChange();
            }
        }
        return;

    case 0xE6:
    case 0xE7:
        R800TimerCnt = CPU.ICount;
        return;
#endif // TURBO_R

    case 0xD0: /* FDC command */
    case 0xD1: /* FDC track   */
    case 0xD2: /* FDC sector  */
    case 0xD3: /* FDC data    */
#ifdef TURBO_R
    /* MSXTurboR doesn't support disk access from IO port. */
        if (MODEL(MSX_MSXTR))return;
#endif // TURBO_R
        /* Brazilian DiskROM I/O ports */
        Write1793(&FDC, Port - 0xD0, Value);
        if (Verbose & 0x04)printf("Disk IO Write[%02Xh] : [%02Xh] \n", Port, Value);
        return;

    case 0xD4: /* FDC system  */
#ifdef TURBO_R
/* MSXTurboR doesn't support disk access from IO port. */
        if (MODEL(MSX_MSXTR))return;
#endif // TURBO_R
        /* Brazilian DiskROM drive/side: [xxxSxxDx] */
        Value = ((Value & 0x02) >> 1) | S_DENSITY | (Value & 0x10 ? 0 : S_SIDE);
        Write1793(&FDC, WD1793_SYSTEM, Value);
        if (Verbose & 0x04)printf("Disk IO Write[%02Xh] : [%02Xh] \n", Port, Value);
        return;

    case 0xFC: /* Mapper page at 0000h */
    case 0xFD: /* Mapper page at 4000h */
    case 0xFE: /* Mapper page at 8000h */
    case 0xFF: /* Mapper page at C000h */
        J = Port - 0xFC;
        Value &= RAMMask;
        if (RAMMapper[J] != Value)
        {
            //if(Verbose&0x08) printf("RAM-MAPPER: block %d at %Xh\n",Value,J*0x4000);
            I = J << 1;
            RAMMapper[J] = Value;
#ifdef _3DS
            MemMap[3][0][I] = RAMData + ((int)Value << 14);
            MemMap[3][0][I + 1] = MemMap[3][0][I] + 0x2000;
            if ((PSL[J] == 3) && (SSL[J] == 0))
            {
#ifdef TURBO_R
                /* In MSX TurboR DRAM mode, last 64k RAM is system ROM. */
                if (MODEL(MSX_MSXTR))
                {
                    if (!(S1990Reg6 & 0x40))
                    {
                        if (Value <= RAMMask - 4)EnWrite[J] = 1;
                    }
                    else EnWrite[J] = 1;
                }
                else EnWrite[J] = 1;
#else
                EnWrite[J] = 1;
#endif // TURBO_R
                RAM[I] = MemMap[3][0][I];
                RAM[I + 1] = MemMap[3][0][I + 1];
            }
#else
            MemMap[3][2][I] = RAMData + ((int)Value << 14);
            MemMap[3][2][I + 1] = MemMap[3][2][I] + 0x2000;
            if ((PSL[J] == 3) && (SSL[J] == 2))
            {
                EnWrite[J] = 1;
                RAM[I] = MemMap[3][2][I];
                RAM[I + 1] = MemMap[3][2][I + 1];
            }
#endif // _3DS
        }
        return;

    }

    /* Unknown port */
    if (Verbose & 0x20)
        printf("I/O: Write to unknown PORT[%02Xh]=%02Xh\n", Port, Value);
}

/** MapROM() *************************************************/
/** Switch ROM Mapper pages. This function is supposed to   **/
/** be called when ROM page registers are written to.       **/
/*************************************************************/
void MapROM(register word A, register byte V)
{
    byte I, J, PS, SS, * P;

    /* @@@ For debugging purposes
    printf("(%04Xh) = %02Xh at PC=%04Xh\n",A,V,CPU.PC.W);
    */

    J = A >> 14;           /* 16kB page number 0-3  */
    PS = PSL[J];          /* Primary slot number   */
    SS = SSL[J];          /* Secondary slot number */
    I = CartMap[PS][SS]; /* Cartridge number      */

  //#ifdef TURBO_R
  //  /* R800 memory access wait.internal ROM:2wait, external ROM:3 wait. */
  //  if (CPU.User & 0x01)
  //  {
  //      if (I == 0 || I == 1)
  //      {
  //          SlotRAM |= 1<<J | (1<<(J+1));
  //          CPU.ICount -= 3;
  //      }
  //      else
  //      {
  //          SlotRAM |= 1 << J;
  //          CPU.ICount -= 2;
  //      }
  //  }
  //#endif // TURBO_R


#ifdef TURBO_R
    if (I >= MAXSLOTS)
    {
        if ((PS == 3) && MODEL(MSX_MSXTR))
        {
            switch (SS)
            {
                /* MSXDOS2.3 for MSXTurboR */
            case 2:
                if (Verbose)
                {
                    if (A == 0x7FF0 && V == 0)
                    {

                    }
                    else
                    {
                        if (Verbose & 0x40)printf("MEMORY: Disk Access(%d:%d:%04Xh) = %02Xh\n", PS, SS, A, V);
                    }
                }
                if ((A < 0x4000) || (A > 0xBFFF)) return;
#ifdef UPD_FDC
                switch (A&0x3FFF)
                {
                case 0x3FF2:
                    DiskAccess[V & 0x01] = 3;
                    WriteTC8566AF(&TCFDC, 2, V);
                    return;
                case 0x3FF3:
                    return;
                case 0x3FF5:
                    WriteTC8566AF(&TCFDC, 5, V);
                    return;
                default:
                    break;
                }
#endif // UPD_FDC
                J = (A & 0x8000) >> 14;
                /* Switch ROM pages */
                V = (V << 1) & 0x07;

                //if (A == 0x7FF0 || A == 0x7FFE || A == 0x6000)
                //only 0x7FF0 ? This fix Breaker(Jast) to work on MSXTurboR. 
                if (A == 0x7FF0)
                {
                    //if (MODEL(MSX_MSXTR))
                    if (V != MSXDOS2Mapper)
                    {
                        RAM[2] = MemMap[3][2][2] = MSXDOS23Data + ((int)V << 13);
                        RAM[3] = MemMap[3][2][3] = RAM[2] + 0x2000;
                        MSXDOS2Mapper = V;
                        if (Verbose & 0x40)printf("MSX-DOS Change %d\n", V);
                        //ResetTC8566AF(&TCFDC, FDD, TC8566AF_KEEP);
                    }
                }
                //}
                break;

            case 3:
                /* Panasonic ROM. Some MSXTurboR game/application need this(MSX-View etc). */
                //if (Verbose & 0x40)printf("MEMORY: PanasonicROM(%d:%d:%04Xh) = %02Xh\n", PS, SS, A, V);
                switch (A)
                {
                case 0x7FF8:
                    if (PanaMapper3 & 0x10)
                    {
                        if (PanaMapper2 != V)
                        {
                            if (Verbose & 0x40)printf("MEMORY: PanasonicROM(%d:%d:%04Xh) = %02Xh\n", PS, SS, A, V);
                            for (J = 0; J < 8; J++)
                            {
                                if (((PanaMapper2 >> J) & 0x01) != ((V >> J) & 0x01))
                                {
                                    word WMapper = (PanaMapper[J] & 0xFF) | ((V >> J) & 0x01 ? 0x100 : 0);
                                    if ((WMapper >= 128) && (WMapper <= 131))
                                    {
                                        PanaMapper[J] = WMapper;
                                        WMapper = (WMapper - 128) & 0x03;
                                        P = PanasonicSRAM[WMapper];
                                        //EnWrite[J >> 1] = 1;
                                        RAM[J] = MemMap[3][3][J] = P;
                                        continue;
                                    }
                                    if (WMapper >= 384)
                                    {
                                        PanaMapper[J] = WMapper;
                                        WMapper = (WMapper - 384) & 0x3F;
                                        MemMap[3][3][J] = RAMData + ((uint)WMapper << 13);
                                        //EnWrite[J >> 1] = 1;
                                        RAM[J] = MemMap[3][3][J];
                                        continue;
                                    }
                                }
                            }
                            PanaMapper2 = V;
                        }
                    }
                    break;
                    if (Verbose & 0x40)printf("MEMORY: PanasonicROM(%d:%d:%04Xh) = %02Xh\n", PS, SS, A, V);
                    if (PanaMapper3 & 0x10)PanaMapper2 = V;
                    break;
                case 0x7FF9:
                    if (Verbose & 0x40)printf("MEMORY: PanasonicROM(%d:%d:%04Xh) = %02Xh\n", PS, SS, A, V);
                    PanaMapper3 = V;
                    break;
                default:
                    if (A > 0x6000 && A < 0x7FF0)
                    {
                        J = (A & 0x1C00) >> 10;
                        J = (J == 5) ? 6 : ((J == 6) ? 5 : J);
                        word WMapper = (((PanaMapper2 >> J) & 0x01) ? 0x100 : 0) | V;
                        if (Verbose & 0x40)printf("Panasonic ROM Mapper[%d] Value:[%d]\n", WMapper, V);
                        if (PanaMapper[J] != WMapper)
                        {
                            if ((WMapper >= 128) && (WMapper <= 131))
                            {
                                PanaMapper[J] = WMapper;
                                WMapper = (WMapper - 128) & 0x03;
                                P = PanasonicSRAM[WMapper];
                                //EnWrite[J >> 1] = 1;
                                RAM[J] = MemMap[3][3][J] = P;
                                return;
                            }
                            if ((WMapper >= 1) && (WMapper < 32))
                            {
                                /* Pseudo View font made by A to C (12x12 font). */
                                /* http://www.yo.rim.or.jp/~anaka/AtoC/labo/labo32.htm */
                                PanaMapper[J] = WMapper;
                                WMapper--;
                                P = ViewFontData + ((int)WMapper << 13);
                                RAM[J] = MemMap[3][3][J] = P;
                                return;
                            }
                            if ((WMapper >= 32) && (WMapper < 64))
                            {
                                /* Pseudo View font made by A to C (12x8 font). */
                                PanaMapper[J] = WMapper;
                                WMapper = WMapper < 14 ? 0 : WMapper - 14;
                                P = ViewFontData + ((int)WMapper << 13);
                                RAM[J] = MemMap[3][3][J] = P;
                                return;
                            }
                            if (WMapper >= 384)
                            {
                                PanaMapper[J] = WMapper;
                                WMapper = (WMapper - 384) & 0x3F;
                                MemMap[3][3][J] = RAMData + ((int)WMapper << 13);
                                //EnWrite[J >> 1] = 1;
                                RAM[J] = MemMap[3][3][J];
                                return;
                            }
                        }
                    }
                }
                if ((PanaMapper[A >> 13] >= 128) && (PanaMapper[A >> 13] <= 131))
                {
                    PanasonicSRAM[PanaMapper[A >> 13] - 128][A & 0x1FFF] = V;
                }
                if (PanaMapper[A >> 13] >= 384)RAM[A >> 13][A & 0x1FFF] = V;
                break;

            default:
                break;
            }
        }

        /* Drop out if no cartridge in that slot */
        if (PS != 2 || SS != 1)return;
        /* Support FM-PAC with no cartridge. */
        switch (A)
        {
        case 0x7FF7: /* ROM page select */
            V &= 3;
            PACMapper[0] = V;
            PACMapper[1] = V | 1;
            /* 4000h-5FFFh contains SRAM when correct FMPACKey supplied */
            if (FMPACKey != FMPAC_MAGIC)
            {
                P = PACData + ((int)V << 13);
                RAM[2] = MemMap[PS][SS][2] = P;
                RAM[3] = MemMap[PS][SS][3] = P + 0x2000;
            }
            return;
        case 0x7FF6: /* OPL1 enable/disable? */
            V &= 0x11;
            return;
        case 0x5FFE: /* Write 4Dh, then (5FFFh)=69h to enable SRAM */
        case 0x5FFF: /* (5FFEh)=4Dh, then write 69h to enable SRAM */
            FMPACKey = A & 1 ? ((FMPACKey & 0x00FF) | ((int)V << 8))
                : ((FMPACKey & 0xFF00) | V);
            P = FMPACKey == FMPAC_MAGIC ?
                PACSaveData : (PACData + ((int)PACMapper[0] << 13));
            RAM[2] = MemMap[PS][SS][2] = P;
            RAM[3] = MemMap[PS][SS][3] = P + 0x2000;
            return;
        }
        /* Write to SRAM */
        if ((A >= 0x4000) && (A < 0x5FFE) && (FMPACKey == FMPAC_MAGIC))
        {
            RAM[A >> 13][A & 0x1FFF] = V;
            SaveSRAMPAC = 1;
            return;
        }
        return;
    }
#else
  /* Drop out if no cartridge in that slot */
    if (I >= MAXSLOTS) return;
#endif // TURBO_R

#ifdef ALTSOUND
	//Write2212(A, V);
	//if (CheckSCC(A))IsSndRegUpd = IsSndRegUpd == 0xFF ? 0 : IsSndRegUpd + 1;
	////if (CartSpecial[I] != CART_READSCC)SCCOn[I] = SCCEnabled();
	//if ((AllowSCC[I]) && (CartSpecial[I] != CART_READSCC))SCCOn[I] = SCCEnabled();
	//if (SCCOn[I])playSCC = 1;
	////SCCOn[I] = SCCEnabled();

    if (AllowSCC[I])
    {
        Write2212(A, V);
        if (CheckSCC(A))IsSndRegUpd = IsSndRegUpd == 0xFF ? 0 : IsSndRegUpd + 1;
        SCCOn[I] = SCCEnabled();
        //if (CartSpecial[I] != CART_READSCC)SCCOn[I] = SCCEnabled();
        if (SCCOn[I])playSCC = 1;
    }
#else
    /* SCC: enable/disable for no cart */
    if (!ROMData[I] && (A == 0x9000)) SCCOn[I] = (V == 0x3F) ? 1 : 0;
#endif  //ALTSOUND

    /* If writing to SCC... */
    if (SCCOn[I] && ((A & 0xDF00) == 0x9800))
    {
        /* Compute SCC register number */
        J = A & 0x00FF;

        /* If using SCC+... */
        if (A & 0x2000)
        {
            /* When no MegaROM present, we allow the program */
            /* to write into SCC wave buffer using EmptyRAM  */
            /* as a scratch pad.                             */
            if (!ROMData[I] && (J < 0xA0)) EmptyRAM[0x1800 + J] = V;

            /* Output data to SCC chip */
            WriteSCCP(&SCChip, J, V);
            //#ifdef ALTSOUND
                  //Write2212(J, V);
            //#endif

        }
        else
        {
            /* When no MegaROM present, we allow the program */
            /* to write into SCC wave buffer using EmptyRAM  */
            /* as a scratch pad.                             */
            if (!ROMData[I] && (J < 0x80)) EmptyRAM[0x1800 + J] = V;

            /* Output data to SCC chip */
            WriteSCC(&SCChip, J, V);
            //#ifdef ALTSOUND
                  //Write2212(J, V);
            //#endif
        }

        /* Done writing to SCC */
        return;
    }

    /* If no cartridge or no mapper, exit */
    if (!ROMData[I] || !ROMMask[I]) return;

    switch (ROMType[I])
    {
    case MAP_GEN8: /* Generic 8kB cartridges (Konami, etc.) */
      /* Only interested in writes to 4000h-BFFFh */
        if ((A < 0x4000) || (A > 0xBFFF)) break;
        J = (A - 0x4000) >> 13;
        /* Turn SCC on/off on writes to 8000h-9FFFh */
        if (J == 2) SCCOn[I] = (V == 0x3F) ? 1 : 0;
        /* Switch ROM pages */
        V &= ROMMask[I];
        if (V != ROMMapper[I][J])
        {
            RAM[J + 2] = MemMap[PS][SS][J + 2] = ROMData[I] + ((int)V << 13);
            ROMMapper[I][J] = V;
        }
        if (Verbose & 0x08)
            printf("ROM-MAPPER %c: 8kB ROM page #%d at %d:%d:%04Xh\n", I + 'A', V, PS, SS, J * 0x2000 + 0x4000);
        return;

    case MAP_GEN16: /* Generic 16kB cartridges (MSXDOS2, HoleInOneSpecial) */
      /* Only interested in writes to 4000h-BFFFh */
        if ((A < 0x4000) || (A > 0xBFFF)) break;
        J = (A & 0x8000) >> 14;
        /* Switch ROM pages */
        V = (V << 1) & ROMMask[I];
        if (V != ROMMapper[I][J])
        {
            RAM[J + 2] = MemMap[PS][SS][J + 2] = ROMData[I] + ((int)V << 13);
            RAM[J + 3] = MemMap[PS][SS][J + 3] = RAM[J + 2] + 0x2000;
            ROMMapper[I][J] = V;
            ROMMapper[I][J + 1] = V | 1;
        }
        if (Verbose & 0x08)
            printf("ROM-MAPPER %c: 16kB ROM page #%d at %d:%d:%04Xh\n", I + 'A', V >> 1, PS, SS, J * 0x2000 + 0x4000);
        return;

#ifdef TURBO_R
    case MAP_SCCPLUS:   /* Konami SCC Plus Hacked ROM    */
    case MAP_WingWarr:  /* Wing Warrior(Kitmaker)      */
#endif // TURBO_R
    case MAP_KONAMI5: /* KONAMI5 8kB cartridges */
      /* Only interested in writes to 5000h/7000h/9000h/B000h */
        if ((A < 0x5000) || (A > 0xB000) || ((A & 0x1FFF) != 0x1000)) break;
        J = (A - 0x5000) >> 13;
        /* Turn SCC on/off on writes to 9000h */
        if (J == 2) SCCOn[I] = (V == 0x3F) ? 1 : 0;

        /* Switch ROM pages */
        V &= ROMMask[I];
        if (V != ROMMapper[I][J])
        {
            RAM[J + 2] = MemMap[PS][SS][J + 2] = ROMData[I] + ((int)V << 13);
            ROMMapper[I][J] = V;
        }
        if (Verbose & 0x08)
            printf("ROM-MAPPER %c: 8kB ROM page #%d at %d:%d:%04Xh\n", I + 'A', V, PS, SS, J * 0x2000 + 0x4000);
        return;

    case MAP_KONAMI4: /* KONAMI4 8kB cartridges */
      /* Only interested in writes to 6000h/8000h/A000h */
      /* (page at 4000h is fixed) */
        if ((A < 0x6000) || (A > 0xA000) || (A & 0x1FFF)) break;
        J = (A - 0x4000) >> 13;
        /* Switch ROM pages */
        V &= ROMMask[I];
        if (V != ROMMapper[I][J])
        {
            RAM[J + 2] = MemMap[PS][SS][J + 2] = ROMData[I] + ((int)V << 13);
            ROMMapper[I][J] = V;
        }
        if (Verbose & 0x08)
            printf("ROM-MAPPER %c: 8kB ROM page #%d at %d:%d:%04Xh\n", I + 'A', V, PS, SS, J * 0x2000 + 0x4000);
        return;

    case MAP_ASCII8: /* ASCII 8kB cartridges */
      /* If switching pages... */
        if ((A >= 0x6000) && (A < 0x8000))
        {
            J = (A & 0x1800) >> 11;
            /* If selecting SRAM... */
            if (V & (ROMMask[I] + 1))
            {
                /* Select SRAM page */
                V = 0xFF;
                P = SRAMData[I];
                if (Verbose & 0x08)
                    printf("ROM-MAPPER %c: 8kB SRAM at %d:%d:%04Xh\n", I + 'A', PS, SS, J * 0x2000 + 0x4000);
            }
            else
            {
                /* Select ROM page */
                V &= ROMMask[I];
                P = ROMData[I] + ((int)V << 13);
                if (Verbose & 0x08)
                    printf("ROM-MAPPER %c: 8kB ROM page #%d at %d:%d:%04Xh\n", I + 'A', V, PS, SS, J * 0x2000 + 0x4000);
            }
            /* If page was actually changed... */
            if (V != ROMMapper[I][J])
            {
                MemMap[PS][SS][J + 2] = P;
                ROMMapper[I][J] = V;
                /* Only update memory when cartridge's slot selected */
                if ((PSL[(J >> 1) + 1] == PS) && (SSL[(J >> 1) + 1] == SS)) RAM[J + 2] = P;
            }
            /* Done with page switch */
            return;
        }
        /* Write to SRAM */
        if ((A >= 0x8000) && (A < 0xC000) && (ROMMapper[I][((A >> 13) & 1) + 2] == 0xFF))
        {
            RAM[A >> 13][A & 0x1FFF] = V;
            SaveSRAM[I] = 1;
            /* Done with SRAM write */
            return;
        }
        break;

    case MAP_ASCII16: /*** ASCII 16kB cartridges ***/
      /* NOTE: Vauxall writes garbage to to 7xxxh */
      /* NOTE: Darwin writes valid data to 6x00h (ASCII8 mapper) */
      /* NOTE: Androgynus writes valid data to 77FFh */
      /* If switching pages... */
        if ((A >= 0x6000) && (A < 0x8000) && ((V <= ROMMask[I] + 1) || !(A & 0x0FFF)))
        {
            J = (A & 0x1000) >> 11;
            /* If selecting SRAM... */
            if (V & (ROMMask[I] + 1))
            {
                /* Select SRAM page */
                V = 0xFF;
                P = SRAMData[I];
                if (Verbose & 0x08)
                    printf("ROM-MAPPER %c: 2kB SRAM at %d:%d:%04Xh\n", I + 'A', PS, SS, J * 0x2000 + 0x4000);
            }
            else
            {
                /* Select ROM page */
                V = (V << 1) & ROMMask[I];
                P = ROMData[I] + ((int)V << 13);
                if (Verbose & 0x08)
                    printf("ROM-MAPPER %c: 16kB ROM page #%d at %d:%d:%04Xh\n", I + 'A', V >> 1, PS, SS, J * 0x2000 + 0x4000);
            }
            /* If page was actually changed... */
            if (V != ROMMapper[I][J])
            {
                MemMap[PS][SS][J + 2] = P;
                MemMap[PS][SS][J + 3] = P + 0x2000;
                ROMMapper[I][J] = V;
                ROMMapper[I][J + 1] = V | 1;
                /* Only update memory when cartridge's slot selected */
                if ((PSL[(J >> 1) + 1] == PS) && (SSL[(J >> 1) + 1] == SS))
                {
                    RAM[J + 2] = P;
                    RAM[J + 3] = P + 0x2000;
                }
            }
            /* Done with page switch */
            return;
        }
        /* Write to SRAM */
        if ((A >= 0x8000) && (A < 0xC000) && (ROMMapper[I][2] == 0xFF))
        {
            P = RAM[A >> 13];
            A &= 0x07FF;
            P[A + 0x0800] = P[A + 0x1000] = P[A + 0x1800] =
                P[A + 0x2000] = P[A + 0x2800] = P[A + 0x3000] =
                P[A + 0x3800] = P[A] = V;
            SaveSRAM[I] = 1;
            /* Done with SRAM write */
            return;
        }
        break;

    case MAP_GMASTER2: /* Konami GameMaster2+SRAM cartridge */
      /* Switch ROM and SRAM pages, page at 4000h is fixed */
        if ((A >= 0x6000) && (A <= 0xA000) && !(A & 0x1FFF))
        {
            /* Figure out which ROM page gets switched */
            J = (A - 0x4000) >> 13;
            /* If changing SRAM page... */
            if (V & 0x10)
            {
                /* Select SRAM page */
                RAM[J + 2] = MemMap[PS][SS][J + 2] = SRAMData[I] + (V & 0x20 ? 0x2000 : 0);
                /* SRAM is now on */
                ROMMapper[I][J] = 0xFF;
                if (Verbose & 0x08)
                    printf("GMASTER2 %c: 4kB SRAM page #%d at %d:%d:%04Xh\n", I + 'A', (V & 0x20) >> 5, PS, SS, J * 0x2000 + 0x4000);
            }
            else
            {
                /* Compute new ROM page number */
                V &= ROMMask[I];
                /* If ROM page number has changed... */
                if (V != ROMMapper[I][J])
                {
                    RAM[J + 2] = MemMap[PS][SS][J + 2] = ROMData[I] + ((int)V << 13);
                    ROMMapper[I][J] = V;
                }
                if (Verbose & 0x08)
                    printf("GMASTER2 %c: 8kB ROM page #%d at %d:%d:%04Xh\n", I + 'A', V, PS, SS, J * 0x2000 + 0x4000);
            }
            /* Done with page switch */
            return;
        }
        /* Write to SRAM */
        if ((A >= 0xB000) && (A < 0xC000) && (ROMMapper[I][3] == 0xFF))
        {
            RAM[5][(A & 0x0FFF) | 0x1000] = RAM[5][A & 0x0FFF] = V;
            SaveSRAM[I] = 1;
            /* Done with SRAM write */
            return;
        }
        break;

    case MAP_FMPAC: /* Panasonic FMPAC+SRAM cartridge */
      /* See if any switching occurs */
        switch (A)
        {
        case 0x7FF7: /* ROM page select */
            V = (V << 1) & ROMMask[I];
            ROMMapper[I][0] = V;
            ROMMapper[I][1] = V | 1;
            /* 4000h-5FFFh contains SRAM when correct FMPACKey supplied */
            if (FMPACKey != FMPAC_MAGIC)
            {
                P = ROMData[I] + ((int)V << 13);
                RAM[2] = MemMap[PS][SS][2] = P;
                RAM[3] = MemMap[PS][SS][3] = P + 0x2000;
            }
            if (Verbose & 0x08)
                printf("FMPAC %c: 16kB ROM page #%d at %d:%d:4000h\n", I + 'A', V >> 1, PS, SS);
            return;
        case 0x7FF6: /* OPL1 enable/disable? */
            if (Verbose & 0x08)
                printf("FMPAC %c: (7FF6h) = %02Xh\n", I + 'A', V);
            V &= 0x11;
            return;
        case 0x5FFE: /* Write 4Dh, then (5FFFh)=69h to enable SRAM */
        case 0x5FFF: /* (5FFEh)=4Dh, then write 69h to enable SRAM */
            FMPACKey = A & 1 ? ((FMPACKey & 0x00FF) | ((int)V << 8))
                : ((FMPACKey & 0xFF00) | V);
            P = FMPACKey == FMPAC_MAGIC ?
                SRAMData[I] : (ROMData[I] + ((int)ROMMapper[I][0] << 13));
            RAM[2] = MemMap[PS][SS][2] = P;
            RAM[3] = MemMap[PS][SS][3] = P + 0x2000;
            if (Verbose & 0x08)
                printf("FMPAC %c: 8kB SRAM %sabled at %d:%d:4000h\n", I + 'A', FMPACKey == FMPAC_MAGIC ? "en" : "dis", PS, SS);
            return;
        }
        /* Write to SRAM */
        if ((A >= 0x4000) && (A < 0x5FFE) && (FMPACKey == FMPAC_MAGIC))
        {
            RAM[A >> 13][A & 0x1FFF] = V;
            SaveSRAM[I] = 1;
            return;
        }
        break;

#ifdef TURBO_R
    case MAP_ASCII8SRM32: /* ASCII 8kB cartridges  with 32kb sram. Many kinds of KOEI games etc.*/
/* If switching pages... */
        if ((A >= 0x6000) && (A < 0x8000))
        {
            J = (A & 0x1800) >> 11;
            /* If selecting SRAM... */
            if (V & (ROMMask[I] + 1))
            {
                /* Select SRAM page */
                P = SRAMData32[I][V & 3];
                V |= 0x80;
                SaveSRAM[I] = 1;
                if (Verbose & 0x08)
                    printf("ROM-MAPPER %c: 8kB SRAM at %d:%d:%04Xh\n", I + 'A', PS, SS, J * 0x2000 + 0x4000);
            }
            else
            {
                /* Select ROM page */
                V &= ROMMask[I];
                P = ROMData[I] + ((int)V << 13);
                if (Verbose & 0x08)
                    printf("ROM-MAPPER %c: 8kB ROM page #%d at %d:%d:%04Xh\n", I + 'A', V, PS, SS, J * 0x2000 + 0x4000);
            }
            /* If page was actually changed... */
            if (V != ROMMapper[I][J])
            {
                MemMap[PS][SS][J + 2] = P;
                ROMMapper[I][J] = V;
                /* Only update memory when cartridge's slot selected */
                //if ((PSL[(J >> 1) + 1] == PS) && (SSL[(J >> 1) + 1] == SS)) RAM[J + 2] = P;
                if ((PSL[(J >> 1) + 1] == PS) && (SSL[(J >> 1) + 1] == SS))
                {
                    RAM[J + 2] = P;
                    if (J != 1 && ((V & (ROMMask[I] + 1)) != 0))EnWrite[J] = 1;
                    //if (J != 1)EnWrite[J] = 1;
                }
            }
            /* Done with page switch */
            return;
        }
        /* Write to SRAM */
        if ((((A >= 0x8000) && (A < 0xC000)) || ((A >= 0x4000) && (A < 0x6000))) && (ROMMapper[I][((A >> 13) & 1) + 2] & 0x80))
        {
            RAM[A >> 13][A & 0x1FFF] = V;
            SaveSRAM[I] = 1;
            /* Done with SRAM write */
            return;
        }
        break;

    case MAP_Wizardry: /* Wizardry */
/* If switching pages... */
        if ((A >= 0x6000) && (A < 0x8000))
        {
            J = (A & 0x1800) >> 11;
            /* If selecting SRAM... */
            if (V & 0x80)
            {
                /* Select SRAM page */
                V = 0xFF;
                P = SRAMData[I];
                if (Verbose & 0x08)
                    printf("ROM-MAPPER %c: 8kB SRAM at %d:%d:%04Xh\n", I + 'A', PS, SS, J * 0x2000 + 0x4000);
            }
            else
            {
                /* Select ROM page */
                V &= ROMMask[I];
                P = ROMData[I] + ((int)V << 13);
                if (Verbose & 0x08)
                    printf("ROM-MAPPER %c: 8kB ROM page #%d at %d:%d:%04Xh\n", I + 'A', V, PS, SS, J * 0x2000 + 0x4000);
            }
            /* If page was actually changed... */
            if (V != ROMMapper[I][J])
            {
                MemMap[PS][SS][J + 2] = P;
                ROMMapper[I][J] = V;
                /* Only update memory when cartridge's slot selected */
                if ((PSL[(J >> 1) + 1] == PS) && (SSL[(J >> 1) + 1] == SS)) RAM[J + 2] = P;
            }
            /* Done with page switch */
            return;
        }
        /* Write to SRAM */
        if (A >= 0x8000 && A < 0xC000 && (ROMMapper[I][((A >> 13) & 1) + 2] == 0xFF))
        {
            RAM[A >> 13][A & 0x1FFF] = V;
            SaveSRAM[I] = 1;
            /* Done with SRAM write */
            return;
        }
        break;

    case MAP_ASCII16_2: /* ASCII 16k cartridges with another SRAM. Super Daisenryaku etc. */
        if ((A >= 0x6000) && (A < 0x8000) && ((V <= ROMMask[I] + 1) || !(A & 0x0FFF)))
        {
            //J = (A & 0x1000) >> 11;
            J = (A >> 11) & 0x03;
            /* If selecting SRAM... */
            //if (V & (ROMMask[I] + 1))
            if (V & 0x80)
            {
                /* Select SRAM page */
                V = 0xFF;
                P = SRAMData[I];
                if (Verbose & 0x08)
                    printf("ROM-MAPPER %c: 2kB SRAM at %d:%d:%04Xh\n", I + 'A', PS, SS, J * 0x2000 + 0x4000);
            }
            else
            {
                /* Select ROM page */
                V = (V << 1) & ROMMask[I];
                P = ROMData[I] + ((int)V << 13);
                if (Verbose & 0x08)
                    printf("ROM-MAPPER %c: 16kB ROM page #%d at %d:%d:%04Xh\n", I + 'A', V >> 1, PS, SS, J * 0x2000 + 0x4000);
            }
            /* If page was actually changed... */
            if (V != ROMMapper[I][J])
            {
                MemMap[PS][SS][J + 2] = P;
                MemMap[PS][SS][J + 3] = P + 0x2000;
                ROMMapper[I][J] = V;
                ROMMapper[I][J + 1] = V | 1;
                /* Only update memory when cartridge's slot selected */
                if ((PSL[(J >> 1) + 1] == PS) && (SSL[(J >> 1) + 1] == SS))
                {
                    RAM[J + 2] = P;
                    RAM[J + 3] = P + 0x2000;
                }
            }
            /* Done with page switch */
            return;
        }
        /* Write to SRAM */
        if (A >= 0x8000 && A < 0xC000 && (ROMMapper[I][((A >> 13) & 1) + 2] == 0xFF))
        {
            P = RAM[A >> 13];
            A &= 0x07FF;
            P[A + 0x0800] = P[A + 0x1000] = P[A + 0x1800] =
                P[A + 0x2000] = P[A + 0x2800] = P[A + 0x3000] =
                P[A + 0x3800] = P[A] = V;
            SaveSRAM[I] = 1;
            /* Done with SRAM write */
            return;
        }
        break;

    case MAP_ASCII16Big: /*** ASCII 16kB cartridges over 2MB size ***/
        /* Some hombrew ROMs has that size(9Finger Demo by NOP, MSX-Wings etc.))*/
        /* https://nopmsx.nl/9fingers/ */
        /* https://github.com/albs-br/msx-wings */
        /* If switching pages... */
        if ((A >= 0x6000) && (A < 0x8000) && ((V <= ROMMask[I] + 1) || !(A & 0x0FFF)))
        {
            J = (A & 0x1000) >> 11;
            /* If selecting SRAM... */
            if (V & (ROMMask[I] + 1))
            {
                /* Select SRAM page */
                V = 0xFF;
                P = SRAMData[I];
                if (Verbose & 0x08)
                    printf("ROM-MAPPER %c: 2kB SRAM at %d:%d:%04Xh\n", I + 'A', PS, SS, J * 0x2000 + 0x4000);
            }
            else
            {
                /* Select ROM page */
                P = ROMData[I] + ((int)V << 14);
                if (Verbose & 0x08)
                    printf("ROM-MAPPER %c: 16kB ROM page #%d at %d:%d:%04Xh\n", I + 'A', V >> 1, PS, SS, J * 0x2000 + 0x4000);
            }
            /* If page was actually changed... */
            if (V != ROMMapper[I][J])
            {
                MemMap[PS][SS][J + 2] = P;
                MemMap[PS][SS][J + 3] = P + 0x2000;
                ROMMapper[I][J] = V;
                ROMMapper[I][J + 1] = V | 1;
                /* Only update memory when cartridge's slot selected */
                if ((PSL[(J >> 1) + 1] == PS) && (SSL[(J >> 1) + 1] == SS))
                {
                    RAM[J + 2] = P;
                    RAM[J + 3] = P + 0x2000;
                }
            }
            /* Done with page switch */
            return;
        }
        /* Write to SRAM */
        if ((A >= 0x8000) && (A < 0xC000) && (ROMMapper[I][2] == 0xFF))
        {
            P = RAM[A >> 13];
            A &= 0x07FF;
            P[A + 0x0800] = P[A + 0x1000] = P[A + 0x1800] =
                P[A + 0x2000] = P[A + 0x2800] = P[A + 0x3000] =
                P[A + 0x3800] = P[A] = V;
            SaveSRAM[I] = 1;
            /* Done with SRAM write */
            return;
        }
        break;

    case MAP_HarryFox16:    /* Harryfox - Yuki no Maouhen (Microcabin) */
        if (((A >= 0x6000) && (A < 0x8000)) && ((V <= ROMMask[I] + 1) || !(A & 0x0FFF)))
        {
            J = (A >> 11) & 2;
            V = ((V << 2) & 4 | J) & ROMMask[I];
            if (V != ROMMapper[I][J])
            {
                RAM[J + 2] = MemMap[PS][SS][J + 2] = ROMData[I] + ((int)V << 13);
                RAM[J + 3] = MemMap[PS][SS][J + 3] = RAM[J + 2] + 0x2000;
                ROMMapper[I][J] = V;
                ROMMapper[I][J + 1] = V + 1;
            }
        }
        if (Verbose & 0x08)
            printf("ROM-MAPPER %c: 16kB ROM page #%d at %d:%d:%04Xh\n", I + 'A', V >> 1, PS, SS, J * 0x2000 + 0x4000);
        break;

    case MAP_Zemina:    /* Many kind of bootleg games by Zemina */
        /* Only interested in writes to 4000h/6000h/8000h/A000h */
        /* (page at 4000h is fixed) */
        if ((A < 0x4000) || (A > 0xC000)) break;
        J = ((A - 0x4000) >> 13);
        /* Switch ROM pages */
        V &= ROMMask[I];
        if (V != ROMMapper[I][J])
        {
            RAM[J + 2] = MemMap[PS][SS][J + 2] = ROMData[I] + ((int)V << 13);
            ROMMapper[I][J] = V;
        }
        if (Verbose & 0x08)
            printf("ROM-MAPPER %c: 8kB ROM page #%d at %d:%d:%04Xh\n", I + 'A', V, PS, SS, J * 0x2000 + 0x4000);
        return;

    case MAP_ZeminaDS2:     /* Bootleg version of DragonSlayer2 Xanadu by Zemina */
        if (IsZeminaSram)
        {
            RAM[A >> 13][A & 0x1FFF] = SRAMData[I][A & 0x1FFF] = V;
            SaveSRAM[I] = 1;
            return;
        }
        if (((A >= 0x6000) && (A <= 0x8000)) || A == 0x4000 || A == 0xA000)
        {
            J = (A - 0x4000) >> 13;
            if (V == 7)
            {
                V = 0xFF;
                P = SRAMData[I];
                if (Verbose & 0x08)
                    printf("ROM-MAPPER %c: 8kB SRAM at %d:%d:%04Xh\n", I + 'A', PS, SS, J * 0x2000 + 0x4000);
            }
            else
            {
                V &= ROMMask[I];
                P = ROMData[I] + ((int)V << 13);
                if (Verbose & 0x08)
                    printf("ROM-MAPPER %c: 8kB ROM page #%d at %d:%d:%04Xh\n", I + 'A', V, PS, SS, J * 0x2000 + 0x4000);
            }
            if (V != ROMMapper[I][J])
            {
                MemMap[PS][SS][J + 2] = P;
                ROMMapper[I][J] = V;
                if ((PSL[(J >> 1) + 1] == PS) && (SSL[(J >> 1) + 1] == SS)) RAM[J + 2] = P;
            }
            return;
        }
        break;

    case MAP_SWANGI:    /* Super Swangi(Super Altered Beast ) by Clover Soft*/
        if (A != 0x8000) break;
        J = (A & 0x8000) >> 14;
        /* Switch ROM pages */
        V = ((V >> 1) << 1) & ROMMask[I];
        if (V != ROMMapper[I][J])
        {
            RAM[J + 2] = MemMap[PS][SS][J + 2] = ROMData[I] + ((int)V << 13);
            RAM[J + 3] = MemMap[PS][SS][J + 3] = RAM[J + 2] + 0x2000;
            ROMMapper[I][J] = V;
            ROMMapper[I][J + 1] = V | 1;
        }
        if (Verbose & 0x08)
            printf("ROM-MAPPER %c: 16kB ROM page #%d at %d:%d:%04Xh\n", I + 'A', V >> 1, PS, SS, J * 0x2000 + 0x4000);
        return;

    case MAP_DOOLY:     /* Baby Dinosaur Dooly(Dooly The Little Dinosaur) by Daou Infosys*/
        V &= 0x07;
        if (V != ROMMapper[I][0])
        {
            if (!V)
            {
                RAM[2] = MemMap[PS][SS][2] = ROMData[I];
                RAM[3] = MemMap[PS][SS][3] = RAM[2] + 0x2000;
                RAM[4] = MemMap[PS][SS][4] = RAM[2] + 0x4000;
                RAM[5] = MemMap[PS][SS][5] = RAM[2] + 0x6000;
                ROMMapper[I][0] = 0;
                ROMMapper[I][1] = 1;
                ROMMapper[I][2] = 2;
                ROMMapper[I][3] = 3;
            }
            else if (V == 0x04)
            {
                RAM[2] = MemMap[PS][SS][2] = ExternalRAMData[I];
                RAM[3] = MemMap[PS][SS][3] = RAM[2] + 0x2000;
                RAM[4] = MemMap[PS][SS][4] = RAM[2] + 0x4000;
                RAM[5] = MemMap[PS][SS][5] = RAM[2] + 0x6000;
                ROMMapper[I][0] = 4;
                ROMMapper[I][1] = 5;
                ROMMapper[I][2] = 6;
                ROMMapper[I][3] = 7;
            }
        }
        return;

    case MAP_XXin1:     /* Super Game World 30/64/80(Zemmix 30/64/80 Games) by Screen Software */
        if (A < 0x4000 || A>0x7FFF)break;
        J = A & 3;
        V &= ROMMask[I];
        if (V != ROMMapper[I][J])
        {
            RAM[J + 2] = MemMap[PS][SS][J + 2] = ROMData[I] + ((int)V << 13);
            ROMMapper[I][J] = V;
        }
        if (Verbose & 0x08)
            printf("ROM-MAPPER %c: 8kB ROM page #%d at %d:%d:%04Xh\n", I + 'A', V, PS, SS, J * 0x2000 + 0x4000);
        return;

    case MAP_126in1:    /* Super Game World 126(Zemmix 126 Games) bt Screen Software */
        if (A < 0x4000 || A>0x7FFF)break;
        J = (A << 1) & 2;
        V = (V << 1) & ROMMask[I];
        if (V != ROMMapper[I][J])
        {
            RAM[J + 2] = MemMap[PS][SS][J + 2] = ROMData[I] + ((int)V << 13);
            RAM[J + 3] = MemMap[PS][SS][J + 3] = RAM[J + 2] + 0x2000;
            ROMMapper[I][J] = V;
            ROMMapper[I][J + 1] = V + 1;
        }
        if (Verbose & 0x08)
            printf("ROM-MAPPER %c: 16kB ROM page #%d at %d:%d:%04Xh\n", I + 'A', V >> 1, PS, SS, J * 0x2000 + 0x4000);
        return;

    case MAP_CrossBlaim:    /* Cross Blaim(db-Soft) */
        /* Switch ROM pages */
        V = ((V << 1) & 7) & ROMMask[I];
        if (V != ROMMapper[I][2])
        {
            RAM[4] = MemMap[PS][SS][4] = ROMData[I] + ((int)V << 13);
            RAM[5] = MemMap[PS][SS][5] = RAM[4] + 0x2000;
            ROMMapper[I][2] = V;
            ROMMapper[I][3] = V | 1;
        }
        if (Verbose & 0x08)
            printf("ROM-MAPPER %c: 16kB ROM page #%d at %d:%d:%04Xh\n", I + 'A', V >> 1, PS, SS, 2 * 0x2000 + 0x4000);
        return;

    case MAP_Pierrot:   /* Super Pierrot(Taito) */
        /* Only interested in writes to 4000h-BFFFh */
        if ((A < 0x4000) || (A > 0xBFFF)) break;
        J = (A & 0x1000) >> 11;
        /* Switch ROM pages */
        V = (V << 1) & ROMMask[I];
        if (V != ROMMapper[I][J])
        {
            RAM[J + 2] = MemMap[PS][SS][J + 2] = ROMData[I] + ((int)V << 13);
            RAM[J + 3] = MemMap[PS][SS][J + 3] = RAM[J + 2] + 0x2000;
            ROMMapper[I][J] = V;
            ROMMapper[I][J + 1] = V + 1;
        }
        if (Verbose & 0x08)
            printf("ROM-MAPPER %c: 16kB ROM page #%d at %d:%d:%04Xh\n", I + 'A', V >> 1, PS, SS, J * 0x2000 + 0x4000);
        return;

    case MAP_RType:     /* R-Type(Irem) */
        if ((A < 0x4000) || (A > 0xBFFF)) break;
        V &= (V & 0x10) ? 0x17 : 0x0F;
        if (V != ROMMapper[I][2])
        {
            P = ROMData[I] + ((int)V << 14);
            MemMap[I + 1][0][4] = P;
            MemMap[I + 1][0][5] = P + 0x2000;
            ROMMapper[I][2] = V;
            /* Only update memory when cartridge's slot selected */
            if (PSL[2] == I + 1)
            {
                RAM[4] = P;
                RAM[5] = P + 0x2000;
            }
        }
        return;

    case MAP_MANBOW2:     /* Manbow2(Sunrise) Use MegaFlashROM SCC. Use secondary PSG port. */
        if (A >= 0x5000 && A <= 0xB000)
        {
            J = (A - 0x4000) >> 13;
            if ((A & 0x1800) == 0x1000)
            {
                //V &= 0x3F;
                if (J == 2)
                {
                    SCCOn[I] = (V & 0x3F == 0x3F) ? 1 : 0;
                    //if (V == 0x3F) return;
                }
                V &= ROMMask[I];
                if (V >= 56)
                {
                    if ((PSL[(J >> 1) + 1] == PS) && (SSL[(J >> 1) + 1] == SS))
                    {
                        RAM[J + 2] = MemMap[PS][SS][J + 2] = ExternalRAMData[I] + ((int)V << 13);
                        ROMMapper[I][J] = V;
                    }
                    return;

                }
                if (V != ROMMapper[I][J])
                {
                    RAM[J + 2] = MemMap[PS][SS][J + 2] = ROMData[I] + ((int)V << 13);
                    ROMMapper[I][J] = V;
                }
            }
            if (Verbose & 0x08)
                printf("ROM-MAPPER %c: 8kB ROM page #%d at %d:%d:%04Xh\n", I + 'A', V, PS, SS, J * 0x2000 + 0x4000);
            return;
        }
        else if (ROMMapper[I][((A >> 13) & 1) + 2] >= 56)
        {
            RAM[A >> 13][A & 0x1FFF] = V;
            SaveSRAM[I] = 1;
        }
        return;

    case MAP_MAJUTSUSHI: /* Hai no Majutsushi by Konami. That use DAC-8bit Sound for voice. */
        /* Only interested in writes to 6000h/8000h/A000h */
        /* (page at 4000h is fixed) */
        if (A >= 0x5000 && A <= 0x5FFF)
        {
            //DA8bit >>= 1;
            //DA8bit += ((int)V - 0x80) << 3;
            DA8bit = ((int)V - 0x80) << 5;
            if (DA8bit)IsSndRegUpd = IsSndRegUpd == 0xFF ? 0 : IsSndRegUpd + 1;
            return;
        }
        if (A < 0x6000 || A >= 0xC000) break;
        J = (A - 0x4000) >> 13;
        /* Switch ROM pages */
        V &= ROMMask[I];
        if (V != ROMMapper[I][J])
        {
            RAM[J + 2] = MemMap[PS][SS][J + 2] = ROMData[I] + ((int)V << 13);
            ROMMapper[I][J] = V;
        }
        if (Verbose & 0x08)
            printf("ROM-MAPPER %c: 8kB ROM page #%d at %d:%d:%04Xh\n", I + 'A', V, PS, SS, J * 0x2000 + 0x4000);
        return;

    case MAP_SCCPLUS_2:     /* Konmai SCC Enhanced ROM. Snatcher, SD Snatcher, and Konami Game Collection. */
        /* Taken from BlueMSX. */
        if ((A | 1) == 0xBFFF)
        {
            SCCMode[I] = V;
            IsSCCRAM[I][0] = (V & 0x10) | (V & 0x01);
            IsSCCRAM[I][1] = (V & 0x10) | (V & 0x02);
            IsSCCRAM[I][2] = (V & 0x10) | ((V & 0x24) == 0x24);
            IsSCCRAM[I][3] = (V & 0x10);
            if ((SCCMode[I] & 0x20) && (ROMMapper[I][3] & 0x80))
            {
                SCCOn[I] = 1;
                SetSCCEnhanced(1);
            }
            else if (!(SCCMode[I] & 0x20) && (ROMMapper[I][2] & 0x3F) == 0x3F)
            {
                SCCOn[I] = 1;
                SetSCCEnhanced(0);
            }
            else SCCOn[I] = 0;
            return;
        }
        J = (A - 0x4000) >> 13;
        if (IsSCCRAM[I][J])
        {
            RAM[A >> 13][A & 0x1FFF] = V;
            return;
        }
        if ((A & 0x1800) == 0x1000)
        {
            //if (V!=ROMMapper[I][J])
            //{
            ROMMapper[I][J] = V;
            V &= ROMMask[I];
            IsMapped[I][J] = 1;
            if (IsMapped)
            {
                RAM[J + 2] = MemMap[PS][SS][J + 2] = ExternalRAMData[I] + ((int)V << 13);
            }
            else
            {
                RAM[J + 2] = MemMap[PS][SS][J + 2] = ExternalRAMData[I] + 0x20000;
            }
            if ((SCCMode[I] & 0x20) && (ROMMapper[I][3] & 0x80))
            {
                SCCOn[I] = 1;
                SetSCCEnhanced(1);
            }
            else if (!(SCCMode[I] & 0x20) && (ROMMapper[I][2] & 0x3F) == 0x3F)
            {
                SCCOn[I] = 1;
                SetSCCEnhanced(0);
            }
            else SCCOn[I] = 0;
            return;
        }
        return;

    case MAP_ESESCC:
        J = (A - 0x4000) >> 13;
        if ((ROMMapper[I][1] == 0xFF) && (A >= 0x4000) && (A < 0x7FFE))
        {
            RAM[A >> 13][A & 0x1FFF] = V;
            SaveSRAM[I] = 1;
            //if (Verbose & 0x08)
            //    printf("SRAM Write %d at %d \n", V, A);
            return;
        }
        if (A == 0x7FFE || A == 0x7FFF)
        {
            if ((V & 0x10) == 0x10)V = 0xFF;
            else V = 0x00;
            if ((PSL[(J >> 1) + 1] == PS) && (SSL[(J >> 1) + 1] == SS))
            {
                ROMMapper[I][J] = V;
                if (Verbose & 0x08)
                    printf("ROM-MAPPER SRAM val:%d at:%d \n", V, A);
            }
            return;
        }
        if ((A & 0x1800) == 0x1000)
        {
            //V &= 0x3F;
            if (J == 2)
            {
                SCCOn[I] = (V & 0x3F == 0x3F) ? 1 : 0;
            }
            V &= ROMMask[I];
            if (V != ROMMapper[I][J])
            {
                RAM[J + 2] = MemMap[PS][SS][J + 2] = ExternalRAMData[I] + ((int)V << 13);
                ROMMapper[I][J] = V;
            }
            //if (Verbose & 0x08)
            //    printf("ROM-MAPPER %c: 8kB ROM page #%d at %d:%d:%04Xh\n", I + 'A', V, PS, SS, J * 0x2000 + 0x4000);
            //if (Verbose & 0x08)
            //    printf("ESESCC at %d val %d \n", A, V);
            return;
        }
        break;
        //if (Verbose & 0x08)
        //    printf("ESESCC at %d val %d \n",A, V);
        //return;

    case MAP_SYNTHE:
        if ((A & 0xC010) == 0x4000)
        {
            //DA8bit >>= 1;
            //DA8bit += ((int)V - 0x80) << 3;
            DA8bit = ((int)V - 0x80) << 5;
            if (DA8bit)IsSndRegUpd = IsSndRegUpd == 0xFF ? 0 : IsSndRegUpd + 1;
            if (Verbose & 0x08)
                printf("ROM-MAPPER SYNTHE val:%d at:%d \n", V, A);
            return;
        }
        break;

    /* ESE-RAM and Mega-SCSI */
    /* http://www.hat.hi-ho.ne.jp/tujikawa/ese/megascsi.html */
    case MAP_MEGASCSI:
        if ((A >= 0x6000) && (A < 0x8000))
        {
            J = (A & 0x1800) >> 11;
            if (V != ROMMapper[I][J])
            {
                RAM[J + 2] = MemMap[PS][SS][J + 2] = ExternalRAMData[I] + ((int)(V&0x3F) << 13);
                ROMMapper[I][J] = V;
                if (Verbose & 0x08)
                    printf("ROM-MAPPER Mega-SCSI val:%02Xh at:%04Xh \n", V, A);
                //if ((J != 1) && (V >= 0x80))ROMMapper[I][J] = 0xFF;
            }
            return;
        }
        //else if ((A >= 0x4000) && (A < 0xC000) && (ROMMapper[I][((A-0x4000) >> 13)] == 0xFF))
        else if ((A >= 0x4000) && (A < 0xC000) && (ROMMapper[I][((A - 0x4000) >> 13)] >=0x80))
        {
            RAM[A >> 13][A & 0x1FFF] = V;
            SaveSRAM[I] = 1;
            return;
        }
#ifdef MEGASCSI_HD
        else if((ROMMapper[I][((A - 0x4000) >> 13)] == 0x7F) && ((A>=0x4000) && (A<0x6000)))       /* SCSI */
        {
            if ((A & 0x1FFF) < 0x1000)      /* 0x4000-0x4FFF:Data Register */
            {
                MB89352A_WriteDREG(&spc, V);
                if (Verbose & 0x08)
                    printf("Mega-SCSI Write Data Register %4Xh : %2Xh \n", A, V);
            }
            else        /* 0x5000-0x5FFF:spc register */
            {
                MB89352A_WriteReg(&spc, A & 0x0F, V);
                if (Verbose & 0x08)
                    printf("Mega-SCSI Write spc Register[%d] : %2Xh \n", A&0x0F, V);
            }
            return;
        }
#endif // MEGASCSI_HD
        break;
    case MAP_QURAN:
        if ((A >= 0x5000) && (A < 0x6000))
        {
            J = (A - 0x5000) >> 10;
            /* Switch ROM pages */
            V &= ROMMask[I];
            if (V != ROMMapper[I][J])
            {
                RAM[J + 2] = MemMap[PS][SS][J + 2] = ROMData[I] + ((int)V << 13);
                ROMMapper[I][J] = V;
            }
            if (Verbose & 0x08)
                printf("ROM-MAPPER %c: 8kB ROM page #%d at %d:%d:%04Xh\n", I + 'A', V, PS, SS, J * 0x2000 + 0x4000);
            return;
        }
        break;
#endif //   TURBO_R
    }

    /* No MegaROM mapper or there is an incorrect write */
  //#ifdef DEBUG_LOG
  //  if (debugLogType & 0x08)fprintf("MEMORY: Bad write (%d:%d:%04Xh) = %02Xh\n", PS, SS, A, V);
  //#else
    if (Verbose & 0x08) printf("MEMORY: Bad write (%d:%d:%04Xh) = %02Xh\n", PS, SS, A, V);
    //#endif // DEBUG
}

/** PSlot() **************************************************/
/** Switch primary memory slots. This function is called    **/
/** when value in port A8h changes.                         **/
/*************************************************************/
void PSlot(register byte V)
{
    register byte J, I;

    if (PSLReg != V)
        for (PSLReg = V, J = 0; J < 4; ++J, V >>= 2)
        {
            I = J << 1;
            PSL[J] = V & 3;
            SSL[J] = (SSLReg[PSL[J]] >> I) & 3;
            RAM[I] = MemMap[PSL[J]][SSL[J]][I];
            RAM[I + 1] = MemMap[PSL[J]][SSL[J]][I + 1];
#ifdef _3DS
            EnWrite[J] = (PSL[J] == 3) && (SSL[J] == 0) && (MemMap[3][0][I] != EmptyRAM);
#else
            EnWrite[J] = (PSL[J] == 3) && (SSL[J] == 2) && (MemMap[3][2][I] != EmptyRAM);
#endif // _3DS
        }
}

/** SSlot() **************************************************/
/** Switch secondary memory slots. This function is called  **/
/** when value in (FFFFh) changes.                          **/
/*************************************************************/
void SSlot(register byte V)
{
    register byte J, I;

    /* Cartridge slots do not have subslots, fix them at 0:0:0:0 */
#ifdef _3DS
    if (CartSpecial[0] == CART_YAKSA || CartSpecial[1] == CART_YAKSA)
    {
        if (PSL[3] == 0 || PSL[3] == 1 || PSL[3] == 2)V = 0x00;
    }
    else if (PSL[3] == 1) V = 0x00;
#else
    if ((PSL[3] == 1) || (PSL[3] == 2)) V = 0x00;
#endif // _3DS
    /* In MSX1, slot 0 does not have subslots either */
    if (!PSL[3] && ((Mode & MSX_MODEL) == MSX_MSX1)) V = 0x00;

    if (SSLReg[PSL[3]] != V)
        for (SSLReg[PSL[3]] = V, J = 0; J < 4; ++J, V >>= 2)
        {
            if (PSL[J] == PSL[3])
            {
                I = J << 1;
                SSL[J] = V & 3;
                RAM[I] = MemMap[PSL[J]][SSL[J]][I];
                RAM[I + 1] = MemMap[PSL[J]][SSL[J]][I + 1];
#ifdef _3DS
                EnWrite[J] = (PSL[J] == 3) && (SSL[J] == 0) && (MemMap[3][0][I] != EmptyRAM);
#else
                EnWrite[J] = (PSL[J] == 3) && (SSL[J] == 2) && (MemMap[3][2][I] != EmptyRAM);
#endif // _3DS
            }
        }
}

/** SetIRQ() *************************************************/
/** Set or reset IRQ. Returns IRQ vector assigned to        **/
/** CPU.IRequest. When upper bit of IRQ is 1, IRQ is reset. **/
/*************************************************************/
word SetIRQ(register byte IRQ)
{
    if (IRQ & 0x80) IRQPending &= IRQ; else IRQPending |= IRQ;
    CPU.IRequest = IRQPending ? INT_IRQ : INT_NONE;
    return(CPU.IRequest);
}

/** SetScreen() **********************************************/
/** Change screen mode. Returns new screen mode.            **/
/*************************************************************/
byte SetScreen(void)
{
    register byte I, J;

    switch (((VDP[0] & 0x0E) >> 1) | (VDP[1] & 0x18))
    {
    case 0x10: J = 0; break;
    case 0x00: J = 1; break;
    case 0x01: J = 2; break;
    case 0x08: J = 3; break;
    case 0x02: J = 4; break;
    case 0x03: J = 5; break;
    case 0x04: J = 6; break;
    case 0x05: J = 7; break;
    case 0x07: J = 8; break;
    case 0x12: J = MAXSCREEN + 1; break;
    default:   J = ScrMode; break;
    }

    /* Recompute table addresses */
#ifdef _3DS
    ChrTab = VRAM + ((int)(VDP[2] & MSK[J].R2) << 10);
#else
    I = (J > 6) && (J != MAXSCREEN + 1) ? 11 : 10;
    ChrTab = VRAM + ((int)(VDP[2] & MSK[J].R2) << I);
#endif // _3DS
    ChrGen = VRAM + ((int)(VDP[4] & MSK[J].R4) << 11);
    ColTab = VRAM + ((int)(VDP[3] & MSK[J].R3) << 6) + ((int)VDP[10] << 14);
    SprTab = VRAM + ((int)(VDP[5] & MSK[J].R5) << 7) + ((int)VDP[11] << 15);
    SprGen = VRAM + ((int)VDP[6] << 11);
#ifdef _3DS
    ChrTabM = ((int)(VDP[2] | ~MSK[J].M2) << 10) | ((1 << 10) - 1);
#else
    ChrTabM = ((int)(VDP[2] | ~MSK[J].M2) << I) | ((1 << I) - 1);
#endif // _3DS
    ChrGenM = ((int)(VDP[4] | ~MSK[J].M4) << 11) | 0x007FF;
    ColTabM = ((int)(VDP[3] | ~MSK[J].M3) << 6) | 0x1C03F;
    SprTabM = ((int)(VDP[5] | ~MSK[J].M5) << 7) | 0x1807F;

#ifdef _3DS
    if (VDPStatus[2] & 0x01)NewScrMode = J;
    else
    {
        NewScrMode = J;
        ScrMode = J;
    }
#else
    /* Return new screen mode */
    ScrMode = J;
#endif // _3DS

    return(J);
}

/** SetMegaROM() *********************************************/
/** Set MegaROM pages for a given slot. SetMegaROM() always **/
/** assumes 8kB pages.                                      **/
/*************************************************************/
void SetMegaROM(int Slot, byte P0, byte P1, byte P2, byte P3)
{
    byte PS, SS;

    /* @@@ ATTENTION: MUST ADD SUPPORT FOR SRAM HERE!   */
    /* @@@ The FFh value must be treated as a SRAM page */

    /* Slot number must be valid */
    if ((Slot < 0) || (Slot >= MAXSLOTS)) return;
    /* Find primary/secondary slots */
    for (PS = 0; PS < 4; ++PS)
    {
        for (SS = 0; (SS < 4) && (CartMap[PS][SS] != Slot); ++SS);
        if (SS < 4) break;
    }
    /* Drop out if slots not found */
    if (PS >= 4) return;

    /* Apply masks to ROM pages */
    P0 &= ROMMask[Slot];
    P1 &= ROMMask[Slot];
    P2 &= ROMMask[Slot];
    P3 &= ROMMask[Slot];
#ifdef _3DS
    if (ROMType[Slot] == MAP_LodeRunner)
    {
        FreeMemory(MemMap[PS][SS][0]);
        FreeMemory(MemMap[PS][SS][1]);
        FreeMemory(MemMap[PS][SS][2]);
        FreeMemory(MemMap[PS][SS][3]);

        MemMap[PS][SS][4] = ROMData[Slot] + P0 * 0x2000;
        MemMap[PS][SS][5] = ROMData[Slot] + P1 * 0x2000;
        MemMap[PS][SS][6] = ROMData[Slot] + P2 * 0x2000;
        MemMap[PS][SS][7] = ROMData[Slot] + P3 * 0x2000;
    }
    else
    {
        MemMap[PS][SS][2] = ROMData[Slot] + P0 * 0x2000;
        MemMap[PS][SS][3] = ROMData[Slot] + P1 * 0x2000;
        MemMap[PS][SS][4] = ROMData[Slot] + P2 * 0x2000;
        MemMap[PS][SS][5] = ROMData[Slot] + P3 * 0x2000;
    }
#else
    /* Set memory map */
    MemMap[PS][SS][2] = ROMData[Slot] + P0 * 0x2000;
    MemMap[PS][SS][3] = ROMData[Slot] + P1 * 0x2000;
    MemMap[PS][SS][4] = ROMData[Slot] + P2 * 0x2000;
    MemMap[PS][SS][5] = ROMData[Slot] + P3 * 0x2000;
#endif // _3DS
    /* Set ROM mappers */
    ROMMapper[Slot][0] = P0;
    ROMMapper[Slot][1] = P1;
    ROMMapper[Slot][2] = P2;
    ROMMapper[Slot][3] = P3;
}

/** VDPOut() *************************************************/
/** Write value into a given VDP register.                  **/
/*************************************************************/
void VDPOut(register byte R, register byte V)
{
    register byte J;
    //#ifdef _3DS
    //  VDPSync();
    //#endif // _3DS

    switch (R)
    {
#ifdef TURBO_R
    case  0: /* Reset HBlank interrupt if disabled */
        if (VDPStatus[1] & 0x01)SetIRQ(V & 0x10 ? INT_IE1 : ~INT_IE1);
        /* Set screen mode */
        if (VDP[0] != V) { VDP[0] = V; SetScreen(); }
        break;
    case  1: /* Set/Reset VBlank interrupt if enabled or disabled */
        if (VDPStatus[0] & 0x80) SetIRQ(V & 0x20 ? INT_IE0 : ~INT_IE0);
        /* Set screen mode */
        if (VDP[1] != V) { VDP[1] = V; SetScreen(); }
        break;
    case  2: ChrTab = VRAM + ((int)(V & MSK[ScrMode].R2) << 10);
        ChrTabM = ((int)(V | ~MSK[ScrMode].M2) << 10) | ((1 << 10) - 1);
#else
    case  0: /* Reset HBlank interrupt if disabled */
        if ((VDPStatus[1] & 0x01) && !(V & 0x10))
        {
            VDPStatus[1] &= 0xFE;
            SetIRQ(~INT_IE1);
        }
        /* Set screen mode */
        if (VDP[0] != V) { VDP[0] = V; SetScreen(); }
        break;
    case  1: /* Set/Reset VBlank interrupt if enabled or disabled */
        if (VDPStatus[0] & 0x80) SetIRQ(V & 0x20 ? INT_IE0 : ~INT_IE0);
        /* Set screen mode */
        if (VDP[1] != V) { VDP[1] = V; SetScreen(); }
        break;
    case  2: J = (ScrMode > 6) && (ScrMode != MAXSCREEN + 1) ? 11 : 10;
        ChrTab = VRAM + ((int)(V & MSK[ScrMode].R2) << J);
        ChrTabM = ((int)(V | ~MSK[ScrMode].M2) << J) | ((1 << J) - 1);
#endif // TURBO_R
        break;
    case  3: ColTab = VRAM + ((int)(V & MSK[ScrMode].R3) << 6) + ((int)VDP[10] << 14);
        ColTabM = ((int)(V | ~MSK[ScrMode].M3) << 6) | 0x1C03F;
        break;
    case  4: ChrGen = VRAM + ((int)(V & MSK[ScrMode].R4) << 11);
        ChrGenM = ((int)(V | ~MSK[ScrMode].M4) << 11) | 0x007FF;
        break;
    case  5: SprTab = VRAM + ((int)(V & MSK[ScrMode].R5) << 7) + ((int)VDP[11] << 15);
        SprTabM = ((int)(V | ~MSK[ScrMode].M5) << 7) | 0x1807F;
        break;
    case  6: V &= 0x3F; SprGen = VRAM + ((int)V << 11); break;
    case  7: FGColor = V >> 4; BGColor = V & 0x0F; break;
    case 10: V &= 0x07;
        ColTab = VRAM + ((int)(VDP[3] & MSK[ScrMode].R3) << 6) + ((int)V << 14);
        break;
    case 11: V &= 0x03;
        SprTab = VRAM + ((int)(VDP[5] & MSK[ScrMode].R5) << 7) + ((int)V << 15);
        break;
#ifdef TURBO_R
    case 14: V &= VRAMPages - 1; VRAMPageInt = (int)V << 14;
#else
    case 14: V &= VRAMPages - 1; VPAGE = VRAM + ((int)V << 14);
#endif // TURBO_R
        break;
    case 15: V &= 0x0F; break;
    case 16: V &= 0x0F; PKey = 1; break;
    case 17: V &= 0xBF; break;
#ifdef TURBO_R
    case 25: V &= 0x7F; VDP[25] = V;
        if (V & 0x08)WideScreenOff();
#else
    case 25: VDP[25] = V;
#endif // TURBO_R
        SetScreen();
        break;
    case 44: VDPWrite(V); break;
    case 46: VDPDraw(V); break;


#ifdef TURBO_R
    case 8: V &= 0xFB; break;
    case 9: V &= 0xBF; break;
        //case 19: SetIRQ(~INT_IE1); break;
        //case 23: if(!(VDP[0]&0x10))SetIRQ(~INT_IE1); break;
    case 20: V &= 0x3F; break;
    case 21: V &= 0x3F; break;
    case 22: V &= 0x3F; break;
    case 26: V &= 0x3F; break;
    case 27: V &= 0x07; break;
#endif // TURBO_R

    }

    /* Write value into a register */
    VDP[R] = V;
}

/** Printer() ************************************************/
/** Send a character to the printer.                        **/
/*************************************************************/
void Printer(byte V)
{
    if (!PrnStream)
    {
        PrnStream = PrnName ? fopen(PrnName, "ab") : 0;
        PrnStream = PrnStream ? PrnStream : stdout;
    }
    fputc(V, PrnStream);
}

/** PPIOut() *************************************************/
/** This function is called on each write to PPI to make    **/
/** key click sound, motor relay clicks, and so on.         **/
/*************************************************************/
void PPIOut(register byte New, register byte Old)
{
#ifndef ALTSOUND
    /* Keyboard click bit */
    if ((New ^ Old) & 0x80) Drum(DRM_CLICK, 64);
    /* Motor relay bit */
    if ((New ^ Old) & 0x10) Drum(DRM_CLICK, 255);
#endif // !ALTSOUND
}

/** RTCIn() **************************************************/
/** Read value from a given RTC register.                   **/
/*************************************************************/
byte RTCIn(register byte R)
{
    static time_t PrevTime;
    static struct tm TM;
    register byte J;
    time_t CurTime;

    /* Only 16 registers/mode */
    R &= 0x0F;

    /* Bank mode 0..3 */
    J = RTCMode & 0x03;

    if (R > 12) J = R == 13 ? RTCMode : NORAM;
    else
        if (J) J = RTC[J][R];
        else
        {
            /* Retrieve system time if any time passed */
            CurTime = time(NULL);
            if (CurTime != PrevTime)
            {
                TM = *localtime(&CurTime);
                PrevTime = CurTime;
            }

            /* Parse contents of last retrieved TM */
            switch (R)
            {
            case 0:  J = TM.tm_sec % 10; break;
            case 1:  J = TM.tm_sec / 10; break;
            case 2:  J = TM.tm_min % 10; break;
            case 3:  J = TM.tm_min / 10; break;
            case 4:  J = TM.tm_hour % 10; break;
            case 5:  J = TM.tm_hour / 10; break;
            case 6:  J = TM.tm_wday; break;
            case 7:  J = TM.tm_mday % 10; break;
            case 8:  J = TM.tm_mday / 10; break;
            case 9:  J = (TM.tm_mon + 1) % 10; break;
            case 10: J = (TM.tm_mon + 1) / 10; break;
            case 11: J = (TM.tm_year - 80) % 10; break;
            case 12: J = ((TM.tm_year - 80) / 10) % 10; break;
            default: J = 0x0F; break;
            }
        }

    /* Four upper bits are always high */
    return(J | 0xF0);
}



word LoopZ80(Z80 * R)
{
#ifdef VDP_V9990
    if (V9990Active)
    {
        LoopZ80_V9990(R);
    }
#endif // VDP_V9990
    static byte BFlag = 0;
    static byte BCount = 0;
    static int  UCount = 0;
    static byte ACount = 0;
    static byte Drawing = 0;
    register int J;

    /* Flip HRefresh bit */
    VDPStatus[2] ^= 0x20;
#ifdef TURBO_R
    currScanLine = ScanLine - firstScanLine;

    //SyncAudio();
#endif // TURBO_R

  /* If HRefresh is now in progress... */
    if (!(VDPStatus[2] & 0x20))
    {
        /* HRefresh takes most of the scanline */
        R->IPeriod = !ScrMode || (ScrMode == MAXSCREEN + 1) ? (CPU_H240 + 5) : (CPU_H256 + 4);
#ifdef TURBO_R
        if ((R->User) & 0x01)
        {
#ifdef USE_OVERCLOCK
            R->IPeriod <<= (1+overClockRatio);
#else
            R->IPeriod <<= 1;
#endif // USE_OVERCLOCK

            /* Add memory refresh wait if HRefresh. */
            R->ICount -= 29;
            //R->IPeriod -= 29;
        }
        //if (MODEL(MSX_MSXTR))
        //{
        //    R800TimerCnt += R->IPeriod;
        //    PCMTimerCnt += R->IPeriod;
        //    VDPIOTimerCnt += R->IPeriod;
        //    PCMTimerReg = (PCMTimerReg & 0x04) ? 0 : PCMTimerReg + 1;
        //    if ((R->User) & 0x01)
        //    {
        //        R800TimerCnt += 29;
        //        PCMTimerCnt += 29;
        //        VDPIOTimerCnt += 29;
        //    }
        //}
#ifdef ALTPCM
        updatePCM();
#endif // ALTPCM

#ifndef AUDIO_SYNC
        switch (SoundSampRate)
        {
        case 0:
            CalcAudio();
            break;
        case 1:
            CalcAudio();
            CalcAudio();
            break;
        default:
            break;
        }
#endif // !AUDIO_SYNC
        if (AccurateAudioSync)WaitSyncLineStep();

        /* If first scanline of the screen... */
        if (!ScanLine)
        {
            //firstScanLine = PALVideo ? ((ScanLines212 ? 60 : 70) + VAdjust) : ((ScanLines212 ? 33 : 43) + VAdjust);
            firstScanLine = PALVideo ? ((ScanLines212 ? 59 : 69) + VAdjust) : ((ScanLines212 ? 32 : 42) + VAdjust);

            //VDPDelay = 0;
            delayedLine = 0;

            if (!AccurateAudioSync)WaitSync();
            else CheckPALVideo();
            if (IsShowFPS)CalcFPS();
            DrawDiskLamp();
            SetFirstLineTime();
            checkAutoFrameSkip();

#ifdef _3DS
            /* This fixes crashes when you close Nintendo 3DS's shell.*/
            svcSleepThread(1);
#endif // _3DS
#else
        R->IPeriod = !ScrMode || (ScrMode == MAXSCREEN + 1) ? CPU_H240 : CPU_H256;

        /* New scanline */
        ScanLine = ScanLine < (PALVideo ? 312 : 261) ? ScanLine + 1 : 0;

        /* If first scanline of the screen... */
        if (!ScanLine)
        {
            /* Drawing now... */
            Drawing = 1;

            /* Reset VRefresh bit */
            VDPStatus[2] &= 0xBF;
#endif // TURBO_R

            /* If first scanline of the screen... */
#ifdef LOG_ERROR
            if (ErrorChar != NULL)ErrorLogUpdate();
#endif // LOG_ERROR

            /* Refresh display */
            if (UCount >= 100) { UCount -= 100; RefreshScreen(); }
            UCount += UPeriod;

            /* Blinking for TEXT80 */
            if (BCount) BCount--;
            else
            {
                BFlag = !BFlag;
                if (!VDP[13]) { XFGColor = FGColor; XBGColor = BGColor; }
                else
                {
                    BCount = (BFlag ? VDP[13] & 0x0F : VDP[13] >> 4) * 10;
                    if (BCount)
                    {
                        if (BFlag) { XFGColor = FGColor; XBGColor = BGColor; }
                        else { XFGColor = VDP[12] >> 4; XBGColor = VDP[12] & 0x0F; }
                    }
                }
            }
        }
#ifdef TURBO_R
        if (ScanLine == firstScanLine - 1)
        {
            /* Reset VRefresh bit */
            VDPStatus[2] &= 0xBF;
        }
        else if (currScanLine == (ScanLines212 ? 212 : 192))
        {
            /* Set VBlank bit, set VRefresh bit */
            //VDPStatus[0] |= 0x80;
            VDPStatus[2] |= 0x40;

            if (!(VDPStatus[0] & 0x80))
            {
                VDPStatus[0] |= 0x80;

                /* Generate VBlank interrupt */
                if (VDP[1] & 0x20) SetIRQ(INT_IE0);
                else SetIRQ(~INT_IE0);
            }
        }

        if ((VDPStatus[1] & 0x01) && (!(VDP[0] & 0x10)))VDPStatus[1] &= 0xFE;


        //Update ScreenMode if only VDP command is inactive. This fixes some graphical glitches  such as Feedback(Techno Soft)'s intro etc.
        if (!(VDPStatus[2] & 0x01))ScrMode = NewScrMode;

        /* Run V9938 engine */
        LoopVDP();

        /* Refresh scanline, possibly with the overscan */
#ifdef VDP_V9990
        //if ((UCount >= 100) && Drawing && (!V9990Active) && (currScanLine < 230))
        if ((UCount >= 100) && Drawing && ((!V9990Active) || (V9990Dual)) && (currScanLine < 230))
#else
        if ((UCount >= 100) && Drawing && (currScanLine < 230))
#endif // VDP_V9990
        {
            //Support games that supports both MSX2 and MSX2+(Shin Maou Golvellius(Golvellius II) etc).
            if ((MODEL(MSX_MSX2)) || !ModeYJK || (ScrMode < 7) || (ScrMode > 8))
                (RefreshLine[ScrMode])(currScanLine);
            else
                if (ModeYAE) RefreshLine[10](currScanLine);
                else RefreshLine[12](currScanLine);
        }
#else
        /* Line coincidence is active at 0..255 */
        /* in PAL and 0..234/244 in NTSC        */
        J = PALVideo ? 256 : ScanLines212 ? 245 : 235;

        /* When reaching end of screen, reset line coincidence */
        if (ScanLine == J)
        {
            VDPStatus[1] &= 0xFE;
            SetIRQ(~INT_IE1);
        }

        /* When line coincidence is active... */
        if (ScanLine < J)
        {
            /* Line coincidence processing */
            J = (((ScanLine + VScroll) & 0xFF) - VDP[19]) & 0xFF;
            if (J == 2)
            {
                /* Set HBlank flag on line coincidence */
                VDPStatus[1] |= 0x01;
                /* Generate IE1 interrupt */
                if (VDP[0] & 0x10) SetIRQ(INT_IE1);
            }
            else
            {
                /* Reset flag immediately if IE1 interrupt disabled */
                if (!(VDP[0] & 0x10)) VDPStatus[1] &= 0xFE;
            }
        }
#endif // TURBO_R

        /* Return whatever interrupt is pending */
        R->IRequest = IRQPending ? INT_IRQ : INT_NONE;
        return(R->IRequest);
        }

    /*********************************/
    /* We come here for HBlanks only */
    /*********************************/

    /* HBlank takes HPeriod-HRefresh */
#ifdef TURBO_R
    R->IPeriod = !ScrMode || (ScrMode == MAXSCREEN + 1) ? 68 : 57;
    //if ((R->User) & 0x01)R->IPeriod <<= 1;

    if ((R->User) & 0x01)
    {
        R->IPeriod <<= 1;
        //R->IPeriod -= 29;
        R->ICount -= 29;
    }
    //if (MODEL(MSX_MSXTR))
    //{
    //    R800TimerCnt += R->IPeriod;
    //    PCMTimerCnt += R->IPeriod;
    //    VDPIOTimerCnt += R->IPeriod;
    //    if ((R->User) & 0x01)
    //    {
    //        R800TimerCnt += 29;
    //        PCMTimerCnt += 29;
    //        VDPIOTimerCnt += 29;
    //    }
    //}

    /* New scanline */
    ScanLine = ScanLine < (PALVideo ? 312 : 261) ? ScanLine + 1 : 0;

    currScanLine = ScanLine - firstScanLine;

    if (!(VDP[0] & 0x10)) VDPStatus[1] &= 0xFE;
#else
    R->IPeriod = !ScrMode || (ScrMode == MAXSCREEN + 1) ? CPU_H240 : CPU_H256;
    R->IPeriod = HPeriod - R->IPeriod;
#endif // TURBO_R

    /* If last scanline of VBlank, see if we need to wait more */
    J = PALVideo ? 313 : 262;
    if (ScanLine >= J - 1)
    {
#ifdef TURBO_R
        J *= CPU_HPERIOD;
        if (R->User & 0x01)
        {
            J <<= 1;
            if (VPeriod * 2 > J) R->IPeriod += (VPeriod - J) * 2;
        }
        else
        {
            if (VPeriod > J) R->IPeriod += VPeriod - J;
        }
#else
        J *= CPU_HPERIOD;
        if (VPeriod > J) R->IPeriod += VPeriod - J;
#endif // TURBO_R
    }

    /* If first scanline of the bottom border... */

    /* If first scanline of VBlank... */
#ifdef TURBO_R
    if (currScanLine == (ScanLines212 ? 212 : 192))Drawing = 0;

    /* Drawing now... */
    if (ScanLine == firstScanLine)Drawing = 1;

    /* Line coincidence processing */
    J = (((currScanLine + VScroll) & 0xFF) - VDP[19]) & 0xFF;
    if (J == 1)
    {
        if (!(VDPStatus[1] & 0x01))
        {
            VDPStatus[1] |= 0x01;
            /* Generate IE1 interrupt */
            if (VDP[0] & 0x10) SetIRQ(INT_IE1);
            else SetIRQ(~INT_IE1);
        }
    }
#else
    if (ScanLine == (ScanLines212 ? 212 : 192)) Drawing = 0;

    J = PALVideo ? (ScanLines212 ? 212 + 42 : 192 + 52) : (ScanLines212 ? 212 + 18 : 192 + 28);
    if (!Drawing && (ScanLine == J))
    {
        /* Set VBlank bit, set VRefresh bit */
        VDPStatus[0] |= 0x80;
        VDPStatus[2] |= 0x40;

        /* Generate VBlank interrupt */
        if (VDP[1] & 0x20) SetIRQ(INT_IE0);
    }

    /* Run V9938 engine */
    LoopVDP();

    /* Refresh scanline, possibly with the overscan */
    if ((UCount >= 100) && Drawing && (ScanLine < 256))
    {
        if (!ModeYJK || (ScrMode < 7) || (ScrMode > 8))
            (RefreshLine[ScrMode])(ScanLine);
        else
            if (ModeYAE) RefreshLine10(ScanLine);
            else RefreshLine12(ScanLine);
    }
#endif // TURBO_R

#ifndef AUDIO_SYNC
#ifdef ALTSOUND
    switch (SoundSampRate)
    {
    case 0:
        /* Additional audio calculation on no audio overtflow to get rid of audio underflow. CheckIsVoice() is true -> maybe overflow. */
        if (((currScanLine & 0x03) == 0x03) || ((currScanLine & 0x05) == 0x05) || ((currScanLine & 0x07) == 0x07) || ((currScanLine & 0x09) == 0x09)
            || (!CheckIsVoice()))CalcAudio();
        audioCnt++;
        if (audioCnt > 40)
        {
            audioCnt = 0;
            IsSndRegUpd = 0;
        }
        break;
    case 1:
        if (((currScanLine & 0x07) != 0x07 && (currScanLine & 0x0F) != 0x0F) || (!CheckIsVoice()))CalcAudio();
        audioCnt++;
        if (audioCnt > 40)
        {
            audioCnt = 0;
            IsSndRegUpd = 0;
        }
        break;
    default:
        break;
    }
    //CalcAudio();
#else
    /* Every few scanlines, update sound */
    if (!(ScanLine & 0x07))
    {
        /* Compute number of microseconds */
        J = (int)(1000000L * (CPU_HPERIOD << 3) / CPU_CLOCK);

        /* Update AY8910 state */
        Loop8910(&PSG, J);

        /* Flush changes to sound channels, only hit drums once a frame */
        Sync8910(&PSG, AY8910_FLUSH | (!ScanLine && OPTION(MSX_DRUMS) ? AY8910_DRUMS : 0));
        SyncSCC(&SCChip, SCC_FLUSH);
        Sync2413(&OPLL, YM2413_FLUSH);

        /* Render and play all sound now */
        PlayAllSound(J);
    }
#endif // ALTSOUND
#endif // !AUDIO_SYNC

    /* Keyboard, sound, and other stuff always runs at line 192    */
    /* This way, it can't be shut off by overscan tricks (Maarten) */
#ifdef TURBO_R
    if (currScanLine == 192)
    {
        if (UseInterlace && InterlaceON)VDPStatus[2] ^= 0x02; /* for Interlace */

        LoadCartAtStart();

        /* Clear 5thSprite fields (wrong place to do it?) */
        /* Without this error occurs (The Goblin(MSXdev23) etc.) */
        VDPStatus[0] = (VDPStatus[0] & ~0x40) | 0x1F;
#else
    if (ScanLine == 192)
    {
        /* Clear 5thSprite fields (wrong place to do it?) */
        VDPStatus[0] = (VDPStatus[0] & ~0x40) | 0x1F;

        /* Check sprites and set Collision bit */
        if (!(VDPStatus[0] & 0x20) && CheckSprites()) VDPStatus[0] |= 0x20;

        /* Count MIDI ticks */
        //MIDITicks(1000*VPeriod/CPU_CLOCK);
#endif // TURBO_R

    /* Apply RAM-based cheats */
        if (CheatsON && CheatCount) ApplyCheats();

        /* Check joystick */
        //JoyState=Joystick();

        /* Check keyboard */
        Keyboard();

        /* Exit emulation if requested */
        if (ExitNow) return(INT_QUIT);

        if (currJoyMode[0] == JOY_ARKANOID)
        {
            PaddleState[0] = ArkanoidPaddle(0);
        }

        /* Check mouse in joystick port #1 */
        if (JOYTYPE(0) >= JOY_MOUSTICK)
        {
            /* Get new mouse state */
            MouState[0] = Mouse(0);
            /* Merge mouse buttons into joystick buttons */
            JoyState |= (MouState[0] >> 12) & 0x0030;
            /* If mouse-as-joystick... */
            if (JOYTYPE(0) == JOY_MOUSTICK)
            {
                J = MouState[0] & 0xFF;
                JoyState |= J > OldMouseX[0] ? 0x0008 : J < OldMouseX[0] ? 0x0004 : 0;
                OldMouseX[0] = J;
                J = (MouState[0] >> 8) & 0xFF;
                JoyState |= J > OldMouseY[0] ? 0x0002 : J < OldMouseY[0] ? 0x0001 : 0;
                OldMouseY[0] = J;
            }
        }

        /* Check mouse in joystick port #2 */
        if (JOYTYPE(1) >= JOY_MOUSTICK)
        {
            /* Get new mouse state */
            MouState[1] = Mouse(1);
            /* Merge mouse buttons into joystick buttons */
            JoyState |= (MouState[1] >> 4) & 0x3000;
            /* If mouse-as-joystick... */
            if (JOYTYPE(1) == JOY_MOUSTICK)
            {
                J = MouState[1] & 0xFF;
                JoyState |= J > OldMouseX[1] ? 0x0800 : J < OldMouseX[1] ? 0x0400 : 0;
                OldMouseX[1] = J;
                J = (MouState[1] >> 8) & 0xFF;
                JoyState |= J > OldMouseY[1] ? 0x0200 : J < OldMouseY[1] ? 0x0100 : 0;
                OldMouseY[1] = J;
            }
        }

        /* If any autofire options selected, run autofire counter */
        if (OPTION(MSX_AUTOSPACE | MSX_AUTOFIREA | MSX_AUTOFIREB))
            if ((ACount = (ACount + 1) & 0x07) > 3)
            {
                /* Autofire spacebar if needed */
                if (OPTION(MSX_AUTOSPACE)) KBD_RES(' ');
                /* Autofire FIRE-A if needed */
                if (OPTION(MSX_AUTOFIREA)) JoyState &= ~(JST_FIREA | (JST_FIREA << 8));
                /* Autofire FIRE-B if needed */
                if (OPTION(MSX_AUTOFIREB)) JoyState &= ~(JST_FIREB | (JST_FIREB << 8));
            }
    }

    /* Return whatever interrupt is pending */
    R->IRequest = IRQPending ? INT_IRQ : INT_NONE;
    return(R->IRequest);
    }


#ifdef _3DS
/* For fMSX3DS, Checking sprites Directly in Sprites(), ColorSprites(), ColorSpritesScr78() int the Common3DS.h */
/* This fixes many games that needs accurate sprite collision. 10Lines hero, Manbow2 etc. And enables to show the MSX2+ boot screeen. */
#else
/** CheckSprites() *******************************************/
/** Check for sprite collisions.                            **/
/*************************************************************/
int CheckSprites(void)
{
    unsigned int I, J, LS, LD;
    byte DH, DV, * S, * D, * PS, * PD, * T;

    /* Must be showing sprites */
    if (SpritesOFF || !ScrMode || (ScrMode >= MAXSCREEN + 1)) return(0);

    /* Find bottom/top scanlines */
    DH = ScrMode > 3 ? 216 : 208;
    LD = 255 - (Sprites16x16 ? 16 : 8);
    LS = ScanLines212 ? 211 : 191;

    /* Find valid, displayed sprites */
    for (I = J = 0, S = SprTab; (I < 32) && (S[0] != DH); ++I, S += 4)
        if ((S[0] < LS) || (S[0] > LD)) J |= 1 << I;

    if (Sprites16x16)
    {
        for (S = SprTab; J; J >>= 1, S += 4)
            if (J & 1)
                for (I = J >> 1, D = S + 4; I; I >>= 1, D += 4)
                    if (I & 1)
                    {
                        DV = S[0] - D[0];
                        if ((DV < 16) || (DV > 240))
                        {
                            DH = S[1] - D[1];
                            if ((DH < 16) || (DH > 240))
                            {
                                PS = SprGen + ((int)(S[2] & 0xFC) << 3);
                                PD = SprGen + ((int)(D[2] & 0xFC) << 3);
                                if (DV < 16) PD += DV; else { DV = 256 - DV; PS += DV; }
                                if (DH > 240) { DH = 256 - DH; T = PS; PS = PD; PD = T; }
                                while (DV < 16)
                                {
                                    LS = ((unsigned int)*PS << 8) + *(PS + 16);
                                    LD = ((unsigned int)*PD << 8) + *(PD + 16);
                                    if (LD & (LS >> DH)) break;
                                    else { ++DV; ++PS; ++PD; }
                                }
                                if (DV < 16) return(1);
                            }
                        }
                    }
    }
    else
    {
        for (S = SprTab; J; J >>= 1, S += 4)
            if (J & 1)
                for (I = J >> 1, D = S + 4; I; I >>= 1, D += 4)
                    if (I & 1)
                    {
                        DV = S[0] - D[0];
                        if ((DV < 8) || (DV > 248))
                        {
                            DH = S[1] - D[1];
                            if ((DH < 8) || (DH > 248))
                            {
                                PS = SprGen + ((int)S[2] << 3);
                                PD = SprGen + ((int)D[2] << 3);
                                if (DV < 8) PD += DV; else { DV = 256 - DV; PS += DV; }
                                if (DH > 248) { DH = 256 - DH; T = PS; PS = PD; PD = T; }
                                while ((DV < 8) && !(*PD & (*PS >> DH))) { ++DV; ++PS; ++PD; }
                                if (DV < 8) return(1);
                            }
                        }
                    }
    }

    /* No collisions */
    return(0);
}
#endif // _3DS

/** StateID() ************************************************/
/** Compute 16bit emulation state ID used to identify .STA  **/
/** files.                                                  **/
/*************************************************************/
word StateID(void)
{
    word ID;
    int J, I;

    ID = 0x0000;

    /* Add up cartridge ROMs, BIOS, BASIC, ExtBIOS, and DiskBIOS bytes */
    for (I = 0; I < MAXSLOTS; ++I)
        if (ROMData[I]) for (J = 0; J < (ROMMask[I] + 1) * 0x2000; ++J) ID += I ^ ROMData[I][J];
    if (MemMap[0][0][0] && (MemMap[0][0][0] != EmptyRAM))
        for (J = 0; J < 0x8000; ++J) ID += MemMap[0][0][0][J];
    if (MemMap[3][1][0] && (MemMap[3][1][0] != EmptyRAM))
        for (J = 0; J < 0x4000; ++J) ID += MemMap[3][1][0][J];
#ifdef TURBO_R
    if (MemMap[3][2][2] && (MemMap[3][2][2] != EmptyRAM))
        for (J = 0; J < 0x4000; ++J) ID += MemMap[3][2][2][J];
#else
    if (MemMap[3][1][2] && (MemMap[3][1][2] != EmptyRAM))
        for (J = 0; J < 0x4000; ++J) ID += MemMap[3][1][2][J];
#endif TURBO_R
    return(ID);
}

/** MakeFileName() *******************************************/
/** Make a copy of the file name, replacing the extension.  **/
/** Returns allocated new name or 0 on failure.             **/
/*************************************************************/
char* MakeFileName(const char* FileName, const char* Extension)
{
    char* Result, * P;

    //Result = malloc(strlen(FileName)+strlen(Extension)+1);
    Result = (char*)malloc(strlen(FileName) + strlen(Extension) + 1);
    if (!Result) return(0);

    strcpy(Result, FileName);
    if ((P = strrchr(Result, '.'))) strcpy(P, Extension); else strcat(Result, Extension);
    return(Result);
}

/** ChangeTape() *********************************************/
/** Change tape image. ChangeTape(0) closes current image.  **/
/** Returns 1 on success, 0 on failure.                     **/
/*************************************************************/
byte ChangeTape(const char* FileName)
{
    /* Close previous tape image, if open */
    if (CasStream) { fclose(CasStream); CasStream = 0; }

    /* If opening a new tape image... */
    if (FileName)
    {
        /* Try read+append first, then read-only */
#ifdef _3DS
        CasStream = zipfopen(FileName, "r+b");
        CasStream = CasStream ? CasStream : zipfopen(FileName, "rb");
#else
        CasStream = fopen(FileName, "r+b");
        CasStream = CasStream ? CasStream : fopen(FileName, "rb");
#endif // _3DS
    }

    /* Done */
    return(!FileName || CasStream);
}

/** RewindTape() *********************************************/
/** Rewind currenly open tape.                              **/
/*************************************************************/
void RewindTape(void) { if (CasStream) rewind(CasStream); }

/** ChangePrinter() ******************************************/
/** Change printer output to a given file. The previous     **/
/** file is closed. ChangePrinter(0) redirects output to    **/
/** stdout.                                                 **/
/*************************************************************/
void ChangePrinter(const char* FileName)
{
    if (PrnStream && (PrnStream != stdout)) fclose(PrnStream);
    PrnName = FileName;
    PrnStream = 0;
}

/** ChangeDisk() *********************************************/
/** Change disk image in a given drive. Closes current disk **/
/** image if Name=0 was given. Creates a new disk image if  **/
/** Name="" was given. Returns 1 on success or 0 on failure.**/
/*************************************************************/
byte ChangeDisk(byte N, const char* FileName)
{
    int NeedState;
    byte* P;

    /* We only have MAXDRIVES drives */
    if (N >= MAXDRIVES) return(0);

    /* Load state when inserting first disk into drive A: */
    NeedState = FileName && *FileName && !N && !FDD[N].Data;

    /* Reset FDC, in case it was running a command */
    Reset1793(&FDC, FDD, WD1793_KEEP);

#ifdef UPD_FDC
    if (MODEL(MSX_MSXTR)) ResetTC8566AF(&TCFDC,FDD,TC8566AF_KEEP);
#endif // UPD_FDC

    /* Eject disk if requested */
    if (!FileName) { EjectFDI(&FDD[N]); return(1); }

    /* If FileName not empty, try loading disk image */
    if (*FileName && LoadFDI(&FDD[N], FileName, FMT_AUTO))
    {
        /* If first disk, also try loading state */
        if (NeedState) FindState(FileName);
#ifdef UPD_FDC
        DiskChanged[N] = 1;
#endif // UPD_FDC
        /* Done */
        return(1);
    }

    /* If failed opening existing image, create a new 720kB disk image */
    P = FormatFDI(&FDD[N], FMT_MSXDSK);

    /* If FileName not empty, treat it as directory, otherwise new disk */
    if (P && !(*FileName ? DSKLoad(FileName, P, "MSX-DISK") : DSKCreate(P, "MSX-DISK")))
    {
        EjectFDI(&FDD[N]); return(0);
    }

    /* Done */
    return(!!P);
}

/** LoadFile() ***********************************************/
/** Simple utility function to load cartridge, state, font  **/
/** or a disk image, based on the file extension, etc.      **/
/*************************************************************/
int LoadFile(const char* FileName)
{
    int J;

    /* Try loading as a disk */
    if (hasext(FileName, ".DSK") || hasext(FileName, ".FDI"))
    {
        /* Change disk image in drive A: */
        if (!ChangeDisk(0, FileName)) return(0);
        /* Eject all user cartridges if successful */
        for (J = 0; J < MAXCARTS; ++J) LoadCart(0, J, ROMType[J]);
        /* Done */
        return(1);
    }

    /* Try loading as a cartridge */
    if (hasext(FileName, ".ROM") || hasext(FileName, ".MX1") || hasext(FileName, ".MX2"))
        return(!!LoadCart(FileName, 0, ROMGUESS(0) | ROMTYPE(0)));

    /* Try loading as a tape */
    if (hasext(FileName, ".CAS")) return(!!ChangeTape(FileName));
    /* Try loading as a font */
    if (hasext(FileName, ".FNT")) return(!!LoadFNT(FileName));
    /* Try loading as palette */
    if (hasext(FileName, ".PAL")) return(!!LoadPAL(FileName));
    /* Try loading as cheats */
    if (hasext(FileName, ".CHT")) return(!!LoadCHT(FileName));
    if (hasext(FileName, ".MCF")) return(!!LoadMCF(FileName));
    /* Try loading as state */
    if (hasext(FileName, ".STA")) return(!!LoadSTA(FileName));

    /* Unknown file type */
    return(0);
}

/** SaveCHT() ************************************************/
/** Save cheats to a given text file. Returns the number of **/
/** cheats on success, 0 on failure.                        **/
/*************************************************************/
int SaveCHT(const char* Name)
{
    FILE* F;
    int J;

    /* Open .CHT text file with cheats */
    F = fopen(Name, "wb");
    if (!F) return(0);

    /* Save cheats */
    for (J = 0; J < CheatCount; ++J)
        fprintf(F, "%s\n", CheatCodes[J].Text);

    /* Done */
    fclose(F);
    return(CheatCount);
}

/** ApplyMCFCheat() ******************************************/
/** Apply given MCF cheat entry. Returns 0 on failure or 1  **/
/** on success.                                             **/
/*************************************************************/
int ApplyMCFCheat(int N)
{
    int Status;

    /* Must be a valid MSX-specific entry */
    if ((N < 0) || (N >= MCFCount) || (MCFEntries[N].Addr > 0xFFFF) || (MCFEntries[N].Size > 2))
        return(0);

    /* Switch cheats off for now and remove all present cheats */
    Status = Cheats(CHTS_QUERY);
    Cheats(CHTS_OFF);
    ResetCheats();

    /* Insert cheat codes from the MCF entry */
    CheatCodes[0].Addr = MCFEntries[N].Addr;
    CheatCodes[0].Data = MCFEntries[N].Data;
    CheatCodes[0].Size = MCFEntries[N].Size;
    sprintf(
    (char*)CheatCodes[0].Text,
        CheatCodes[0].Size > 1 ? "%04X-%04X" : "%04X-%02X",
        CheatCodes[0].Addr,
        CheatCodes[0].Data
        );

    /* Have one cheat code now */
    CheatCount = 1;

    /* Turn cheats back on, if they were on */
    Cheats(Status);

    /* Done */
    return(CheatCount);
}

/** AddCheat() ***********************************************/
/** Add a new cheat. Returns 0 on failure or the number of  **/
/** cheats on success.                                      **/
/*************************************************************/
int AddCheat(const char* Cheat)
{
    static const char* Hex = "0123456789ABCDEF";
    unsigned int A, D;
    char* P;
    int J, N;

    /* Table full: no more cheats */
    if (CheatCount >= MAXCHEATS) return(0);

    /* Check cheat length and decode */
    N = strlen(Cheat);

    if (((N == 13) || (N == 11)) && (Cheat[8] == '-'))
    {
        for (J = 0, A = 0; J < 8; J++)
        {
            P = strchr(Hex, toupper(Cheat[J]));
            if (!P) return(0); else A = (A << 4) | (P - Hex);
        }
        for (J = 9, D = 0; J < N; J++)
        {
            P = strchr(Hex, toupper(Cheat[J]));
            if (!P) return(0); else D = (D << 4) | (P - Hex);
        }
    }
    else if (((N == 9) || (N == 7)) && (Cheat[4] == '-'))
    {
        for (J = 0, A = 0x0100; J < 4; J++)
        {
            P = strchr(Hex, toupper(Cheat[J]));
            if (!P) return(0); else A = (A << 4) | (P - Hex);
        }
        for (J = 5, D = 0; J < N; J++)
        {
            P = strchr(Hex, toupper(Cheat[J]));
            if (!P) return(0); else D = (D << 4) | (P - Hex);
        }
    }
    else
    {
        /* Cannot parse this cheat */
        return(0);
    }

    /* Add cheat */
    strcpy((char*)CheatCodes[CheatCount].Text, Cheat);
    if (N == 13)
    {
        CheatCodes[CheatCount].Addr = A;
        CheatCodes[CheatCount].Data = D & 0xFFFF;
        CheatCodes[CheatCount].Size = 2;
    }
    else
    {
        CheatCodes[CheatCount].Addr = A;
        CheatCodes[CheatCount].Data = D & 0xFF;
        CheatCodes[CheatCount].Size = 1;
    }

    /* Successfully added a cheat! */
    return(++CheatCount);
}

/** DelCheat() ***********************************************/
/** Delete a cheat. Returns 0 on failure, 1 on success.     **/
/*************************************************************/
int DelCheat(const char* Cheat)
{
    int I, J;

    /* Scan all cheats */
    for (J = 0; J < CheatCount; ++J)
    {
        /* Match cheat text */
        for (I = 0; Cheat[I] && CheatCodes[J].Text[I]; ++I)
            if (CheatCodes[J].Text[I] != toupper(Cheat[I])) break;
        /* If cheat found... */
        if (!Cheat[I] && !CheatCodes[J].Text[I])
        {
            /* Shift cheats by one */
            if (--CheatCount != J)
                memcpy(&CheatCodes[J], &CheatCodes[J + 1], (CheatCount - J) * sizeof(CheatCodes[0]));
            /* Cheat deleted */
            return(1);
        }
    }

    /* Cheat not found */
    return(0);
}

/** ResetCheats() ********************************************/
/** Remove all cheats.                                      **/
/*************************************************************/
void ResetCheats(void) { Cheats(CHTS_OFF); CheatCount = 0; }

/** ApplyCheats() ********************************************/
/** Apply RAM-based cheats. Returns the number of applied   **/
/** cheats.                                                 **/
/*************************************************************/
int ApplyCheats(void)
{
    int J, I;

    /* For all current cheats that look like 01AAAAAA-DD/DDDD... */
    for (J = I = 0; J < CheatCount; ++J)
        if ((CheatCodes[J].Addr >> 24) == 0x01)
        {
            WrZ80(CheatCodes[J].Addr & 0xFFFF, CheatCodes[J].Data & 0xFF);
            if (CheatCodes[J].Size > 1)
                WrZ80((CheatCodes[J].Addr + 1) & 0xFFFF, CheatCodes[J].Data >> 8);
            ++I;
        }

    /* Return number of applied cheats */
    return(I);
}

/** Cheats() *************************************************/
/** Toggle cheats on (1), off (0), inverse state (2) or     **/
/** query (3).                                              **/
/*************************************************************/
int Cheats(int Switch)
{
    byte* P, * Base;
    int J, Size;

    switch (Switch)
    {
    case CHTS_ON:
    case CHTS_OFF:    if (Switch == CheatsON) return(CheatsON);
    case CHTS_TOGGLE: Switch = !CheatsON; break;
    default:          return(CheatsON);
    }

    /* Find valid cartridge */
    for (J = 1; (J <= 2) && !ROMData[J]; ++J);

    /* Must have ROM */
    if (J > 2) return(Switch = CHTS_OFF);

    /* Compute ROM address and size */
    Base = ROMData[J];
    Size = ((int)ROMMask[J] + 1) << 14;

    /* If toggling cheats... */
    if (Switch != CheatsON)
    {
        /* If enabling cheats... */
        if (Switch)
        {
            /* Patch ROM with the cheat values */
            for (J = 0; J < CheatCount; ++J)
                if (!(CheatCodes[J].Addr >> 24) && (CheatCodes[J].Addr + CheatCodes[J].Size <= Size))
                {
                    P = Base + CheatCodes[J].Addr;
                    CheatCodes[J].Orig = P[0];
                    P[0] = CheatCodes[J].Data;
                    if (CheatCodes[J].Size > 1)
                    {
                        CheatCodes[J].Orig |= (int)P[1] << 8;
                        P[1] = CheatCodes[J].Data >> 8;
                    }
                }
        }
        else
        {
            /* Restore original ROM values */
            for (J = 0; J < CheatCount; ++J)
                if (!(CheatCodes[J].Addr >> 24) && (CheatCodes[J].Addr + CheatCodes[J].Size <= Size))
                {
                    P = Base + CheatCodes[J].Addr;
                    P[0] = CheatCodes[J].Orig;
                    if (CheatCodes[J].Size > 1)
                        P[1] = CheatCodes[J].Orig >> 8;
                }
        }

        /* Done toggling cheats */
        CheatsON = Switch;
    }

    /* Done */
    if (Verbose) printf("Cheats %s\n", CheatsON ? "ON" : "OFF");
    return(CheatsON);
}


#ifdef TURBO_R
void LoadFMPAC(byte PS, byte SS)
{
    byte* P0, * P1;
    FILE* F;
    P0 = LoadROM("FMPAC.ROM", 0x4000, 0);
#ifdef USE_CBIOS
    if (!P0) P0 = LoadROM("cbios_music.rom", 0x4000, 0);
#endif // USE_CBIOS
    if (P0)
    {
        //if (!Use2413)
        if (!Use2413 || (PS != 0))
        {
            P0[0x1C] = 0x20;
            P0[0x1D] = 0x20;
            P0[0x1E] = 0x20;
            P0[0x1F] = 0x20;
        }
        MemMap[PS][SS][2] = P0;
        MemMap[PS][SS][3] = P0 + 0x2000;
        //MemMap[3][3][2] = P0;
        //MemMap[3][3][3] = P0 + 0x2000;
        //ROMType[J] = MAP_FMPAC;

        PACData = P0;
        PACSaveData = GetMemory(0x4000);
        if (F = fopen("/FMSX3DS/SRAM/FMPAC.SAV", "rb"))
        {
            fread(PACSaveData, 1, 0x2000, F);
        }
        P1 = PACSaveData;
        if (P1)
        {
            memset(P1 + 0x2000, NORAM, 0x2000);
            P1[0x1FFE] = FMPAC_MAGIC & 0xFF;
            P1[0x1FFF] = FMPAC_MAGIC >> 8;
        }
    }
    NeedRest = 1;
    fclose(F);
}


void PatchFMPAC(byte PS, byte SS)
{
    /* FM-PAC to PAC for S-RAM. */
    if (MemMap[PS][SS][2])
    {
        if (MemMap[PS][SS][2][0x1C] == 'O' && MemMap[PS][SS][2][0x1D] == 'P' && MemMap[PS][SS][2][0x1E] == 'L'
            && MemMap[PS][SS][2][0x1F] == 'L')
        {
            MemMap[PS][SS][2][0x1C] = 0x20;
            MemMap[PS][SS][2][0x1D] = 0x20;
            MemMap[PS][SS][2][0x1E] = 0x20;
            MemMap[PS][SS][2][0x1F] = 0x20;
        }
    }
    if (MemMap[PS][SS][4])
    {
        if (MemMap[PS][SS][4][0x1C] == 'O' && MemMap[PS][SS][4][0x1D] == 'P' && MemMap[PS][SS][4][0x1E] == 'L'
            && MemMap[PS][SS][4][0x1F] == 'L')
        {
            MemMap[PS][SS][4][0x1C] = 0x20;
            MemMap[PS][SS][4][0x1D] = 0x20;
            MemMap[PS][SS][4][0x1E] = 0x20;
            MemMap[PS][SS][4][0x1F] = 0x20;
        }
    }
}


void PatchGETPAD()
{
    if (MemMap[0][0][0] == NULL)return;
    byte B0 = MemMap[0][0][0][0xDB];
    byte B1 = MemMap[0][0][0][0xDC];
    byte B2 = MemMap[0][0][0][0xDD];
    OldBIOS_GETPAD[0] = B0;
    OldBIOS_GETPAD[1] = B1;
    OldBIOS_GETPAD[2] = B2;
    MemMap[0][0][0][0xDB] = 0xED;
    MemMap[0][0][0][0xDC] = 0xFE;
    MemMap[0][0][0][0xDD] = 0xC9;
}


void UnPatchGETPAD()
{
    if (MemMap[0][0][0] == NULL)return;
    if ((OldBIOS_GETPAD[0] != 0) && (OldBIOS_GETPAD[1] != 0) && (OldBIOS_GETPAD[2] != 0))
    {
        if ((MemMap[0][0][0][0xDB] == 0xED) && (MemMap[0][0][0][0xDC] == 0xFE) && (MemMap[0][0][0][0xDD] == 0xC9))
        {
            MemMap[0][0][0][0xDB] = OldBIOS_GETPAD[0];
            MemMap[0][0][0][0xDC] = OldBIOS_GETPAD[1];
            MemMap[0][0][0][0xDD] = OldBIOS_GETPAD[2];
        }
    }
}


void LoadMSXMusicTurboR()
{
    NeedRest = 0;
    LoadCart("MSXTRMUS.ROM", 4, MAP_FMPAC);
    NeedRest = 1;
}


#ifdef HDD_NEXTOR
unsigned char LoadPatchedNEXTOR(const char* nextorPath)
{
    byte J, * P;
    //NeedRest = 0;
    /* If already loaded NEXTOR driver ROM, return.  */
    if (ROMData[7])
    {
        if (ROMLen[7] >= 0x1C11A)
        {
            if ((ROMData[7][0x1C116] == 'N') && (ROMData[7][0x1C117] == 'e') && (ROMData[7][0x1C118] == 'x') && (ROMData[7][0x1C119] == 't')
                && (ROMData[7][0x1C11A] == 'o') && (ROMData[7][0x1C11B] == 'r'))
            {
                ResetMSX(Mode, RAMPages, VRAMPages);
                return 0;
            }
        }
    }
    if (LoadCart(nextorPath, 7, MAP_ASCII16))
    {
        ROMData[7][0x1C10E] = 0x01;
        //const word NEXTORPatches[] = { 0x4130, 0x4133, 0x4136, 0x4139, 0x413C, 0x413F, 0x4142, 0x4145, 0x4148, 0x414B, 0x414E, 0x4151,
        //0x4160, 0x4163, 0x4166, 0x4169 };
        const word NEXTORPatches[] = { 0x4130, 0x4133, 0x4136, 0x4139, 0x413C, 0x413F, 0x4142, 0x4145, 0x4148, 0x414B, 0x414E, 0x4160,
            0x4163, 0x4166, 0x4169 };
        for (J = 0; NEXTORPatches[J]; ++J)
        {
            P = ROMData[7] + (0x18000 + NEXTORPatches[J]);
            P[0] = 0xED; P[1] = 0xFE; P[2] = 0xC9;
        }
        P = ROMData[7] + 0x1C110;
        const char DriveName[] = "fMSX Nextor Device Driver";
        memcpy(P, DriveName, sizeof(DriveName));
        return 1;       /* Load NEXTOR driver scucess. */
    }
    return 0;       /* Load NEXTOR driver failed. */
    //NeedRest = 1;
}
#endif // HDD_NEXTOR
#ifdef HDD_IDE
void LoadNEXTOR()
{
    LoadCart("Nextor-2.1.2.StandaloneASCII16.ROM", 7, MAP_ASCII16);
}
#endif // HDD_IDE



void LoadKanjiROM()
{
    byte* P;
    FILE* F;
    F = zipfopen("KANJI.ROM", "rb");
    if (F)
    {
        if (!fseek(F, 0, SEEK_END))
        {
            if (ftell(F) == 0x40000)P = LoadROM("KANJI.ROM", 0x40000, 0);
            else P = LoadROM("romfs:/FMSX3DS/KANJI.ROM", 0x40000, 0);
            if (P)
            {
                Kanji = GetMemory(0x20000);
                memcpy(Kanji, P, 0x20000);
                Kanji2 = GetMemory(0x20000);
                memcpy(Kanji2, P + 0x20000, 0x20000);
            }
            else
            {
                Kanji = LoadROM("KANJI.ROM", 0x20000, 0);
            }
        }
    }
    fclose(F);
}


//void LoadMSXDOS2()
//{
//    if (!MODEL(MSX_MSX1) && OPTION(MSX_MSXDOS2) && (MemMap[3][2][2] != EmptyRAM) && !ROMData[3])
//        if (LoadCart("MSXDOS2.ROM", 3, MAP_GEN16))
//            SetMegaROM(3, 0, 1, ROMMask[1] - 1, ROMMask[1]);
//}
//
//
//void UnLoadMSXDOS2()
//{
//    LoadCart(0, 3, MAP_GEN16);
//}


void LoadSCCPLUS(int Slot)
{
    byte* P, PS, SS;
    /* Slot number must be valid */
    if ((Slot < 0) || (Slot >= MAXSLOTS)) return;
    /* Find primary/secondary slots */
    for (PS = 0; PS < 4; ++PS)
    {
        for (SS = 0; (SS < 4) && (CartMap[PS][SS] != Slot); ++SS);
        if (SS < 4) break;
    }
    /* Drop out if slots not found */
    if (PS >= 4) return;

    ROMData[Slot] = P = ResizeMemory(ROMData[Slot], 0x20000);
    //free(ROMData[Slot]);
    //ROMData[Slot] = P = GetMemory(0x20000);
    memset(P, 0x00, 0x20000);
    P[0] = 'A';
    P[1] = 'B';

    ROMType[Slot] = MAP_SCCPLUS_2;
    ROMName[Slot] = "SCCPLUS";

    //ExternalRAMData[Slot] = (byte*)malloc(0x22000);
    //free(ExternalRAMData[Slot]);
    //ExternalRAMData[Slot] = GetMemory(0x22000);
    /* Use ResizeMemory() insted of GetMemory() for large size data because of error in Old3DS.*/
    ExternalRAMData[Slot] = ResizeMemory(ExternalRAMData[Slot], 0x22000);

    memset(ExternalRAMData[Slot], NORAM, 0x22000);
    memcpy(ExternalRAMData[Slot], P, 0x20000);

    MemMap[PS][SS][2] = ExternalRAMData[Slot];
    MemMap[PS][SS][3] = ExternalRAMData[Slot] + 0x2000;
    MemMap[PS][SS][4] = ExternalRAMData[Slot] + 0x4000;
    MemMap[PS][SS][5] = ExternalRAMData[Slot] + 0x6000;

    ROMMapper[Slot][0] = 0;
    ROMMapper[Slot][1] = 1;
    ROMMapper[Slot][2] = 2;
    ROMMapper[Slot][3] = 3;
    SCCMode[Slot] = 0x20;
    IsSCCRAM[Slot][0] = IsSCCRAM[Slot][1] = IsSCCRAM[Slot][2] = IsSCCRAM[Slot][3] = 0;
    IsMapped[Slot][0] = IsMapped[Slot][1] = IsMapped[Slot][2] = IsMapped[Slot][3] = 1;
    ROMMask[Slot] = 0x0F;
    //SCCEnhanced = 1;

    SetROMType(Slot, MAP_SCCPLUS_2);
}


void LoadESESCC(int Slot, int RamSize)
{
    byte* P, PS, SS;
    FILE* F;
    /* Slot number must be valid */
    if ((Slot < 0) || (Slot >= MAXSLOTS)) return;
    /* Find primary/secondary slots */
    for (PS = 0; PS < 4; ++PS)
    {
        for (SS = 0; (SS < 4) && (CartMap[PS][SS] != Slot); ++SS);
        if (SS < 4) break;
    }
    /* Drop out if slots not found */
    if (PS >= 4) return;

    ROMData[Slot] = P = ResizeMemory(ROMData[Slot], 0x80000);
    memset(P, 0x00, 0x80000);
    P[0] = 'A';
    P[1] = 'B';

    ROMType[Slot] = MAP_ESESCC;
    ROMName[Slot] = "esescc512A";
    SRAMName[Slot] = "esescc512A.sav";
    //SRAMName[Slot] = "esescc512A.sram";

    /* Use ResizeMemory() insted of GetMemory() for large size data because of error in Old3DS.*/
    ExternalRAMData[Slot] = ResizeMemory(ExternalRAMData[Slot], 0x80000);

    memset(ExternalRAMData[Slot], NORAM, 0x80000);
    memcpy(ExternalRAMData[Slot], P, 0x80000);
    if (F = sramfopen("esescc512A.sav", "rb"))
    {
        fread(ExternalRAMData[Slot], 1, 0x80000, F);
        fclose(F);
    }
    else if (F = sramfopen("esescc512A.sram", "rb"))    /* Comapible with other emulator. */
    {
        fread(ExternalRAMData[Slot], 1, 0x80000, F);
        fclose(F);
    }

    MemMap[PS][SS][2] = ExternalRAMData[Slot];
    MemMap[PS][SS][3] = ExternalRAMData[Slot] + 0x2000;
    MemMap[PS][SS][4] = ExternalRAMData[Slot] + 0x4000;
    MemMap[PS][SS][5] = ExternalRAMData[Slot] + 0x6000;

    ROMMapper[Slot][0] = 0;
    ROMMapper[Slot][1] = 1;
    ROMMapper[Slot][2] = 2;
    ROMMapper[Slot][3] = 3;
    SCCMode[Slot] = 0x20;
    ROMMask[Slot] = 0x3F;

    SetROMType(Slot, MAP_ESESCC);
}


void LoadESERAM(int Slot, int RamSize)
{
    byte* P, PS, SS;
    FILE* F;
    /* Slot number must be valid */
    if ((Slot < 0) || (Slot >= MAXSLOTS)) return;
    /* Find primary/secondary slots */
    for (PS = 0; PS < 4; ++PS)
    {
        for (SS = 0; (SS < 4) && (CartMap[PS][SS] != Slot); ++SS);
        if (SS < 4) break;
    }
    /* Drop out if slots not found */
    if (PS >= 4) return;

    ROMData[Slot] = P = ResizeMemory(ROMData[Slot], 0x80000);
    memset(P, 0x00, 0x80000);
    P[0] = 'A';
    P[1] = 'B';

    ROMType[Slot] = MAP_MEGASCSI;
    ROMName[Slot] = "eseram512A";
    SRAMName[Slot] = "eseram512A.sav";
    //SRAMName[Slot] = "eseram512A.sram";

    /* Use ResizeMemory() insted of GetMemory() for large size data because of error in Old3DS.*/
    ExternalRAMData[Slot] = ResizeMemory(ExternalRAMData[Slot], 0x80000);

    memset(ExternalRAMData[Slot], NORAM, 0x80000);
    memcpy(ExternalRAMData[Slot], P, 0x80000);
    if (F = sramfopen("eseram512A.sav", "rb"))
    {
        fread(ExternalRAMData[Slot], 1, 0x80000, F);
        fclose(F);
    }
    else if (F = sramfopen("eseram512A.sram", "rb"))         /* Comapible with other emulator. */
    {
        fread(ExternalRAMData[Slot], 1, 0x80000, F);
        fclose(F);
    }

    MemMap[PS][SS][2] = ExternalRAMData[Slot];
    MemMap[PS][SS][3] = ExternalRAMData[Slot] + 0x2000;
    MemMap[PS][SS][4] = ExternalRAMData[Slot] + 0x4000;
    MemMap[PS][SS][5] = ExternalRAMData[Slot] + 0x6000;

    ROMMapper[Slot][0] = 0;
    ROMMapper[Slot][1] = 0;
    ROMMapper[Slot][2] = 0;
    ROMMapper[Slot][3] = 0;
    ROMMask[Slot] = 0x3F;

    SetROMType(Slot, MAP_MEGASCSI);

    CartSpecial[Slot] = CART_MEGASCSI;
}


/* Emulate a hardware that connected to printer port. */
void updatePrinter()
{
    /* Emulate Voice BOX*/
    /* http://hirosedou.sblo.jp/article/93257998.html */
    /* Emulate with the infomation from http://mydocuments.g2.xrea.com/html/p8/soundinfo.html*/
    if (PrinterMode == PRINTER_VOICEBOX)
    {
        PrinterStatus = PrinterStatus == 0 ? 0 : PrinterStatus - 1;
    }

    /* Emulate +PCM */
    /* http://hp.vector.co.jp/authors/VA011751/MSXSR8-2.HTM */
    /* Emulate with the infomation from https://web.archive.org/web/20010424170723/http://www.a1.st/tomo/hard/gallery/ */
    if (PrinterMode == PRINTER_PLUSPCM)
    {
        if (PrinterStatus)
        {
            PrinterStatus++;
            switch (SoundSampRate)
            {
            case 0:
                if (PrinterStatus < 4)return;
                break;
            case 1:
                if (PrinterStatus < 6)return;
                break;
            default:
                break;
            }
            //if (PrinterStatus < 6)return;
            PrinterStatus = 1;
            if (adpcmpos > 0)
            {
                int Val = ADPCMData[0] & 0x0F;
                PrinterValue += (Sint8)(PCM_Table[((PrinterValue2 + Val) > 0 ? (PrinterValue2 + Val) : 0)] & 0xFF);
                //PrinterValue = (Sint16)PrinterValue;
                if (!(PrinterValue & 0x80))
                {
                    if (Val & 0x08)PrinterValue = 0xFF;
                }
                else
                {
                    if (!(Val & 0x08))PrinterValue = 0;
                }
                PrinterValue2 += SI_Table[Val];
                PrinterValue2 = ((PrinterValue2 > 0) ? ((PrinterValue2 > 0x15) ? 15 : PrinterValue2) : 0);
                DA8bit = PrinterValue << 4;
                memmove(ADPCMData, ADPCMData + 1, adpcmpos - 1);
                adpcmpos = adpcmpos <= 0 ? 0 : adpcmpos - 1;
            }
        }
    }
}


#ifdef ALTPCM
/* TurboR PCM voice sampling */
void updatePCM()
{
    if (PCMPos > 0)
    {
        int Val = PCMData[0];
        DA8bit = (Val - 0x80) << 5;
        memmove(PCMData, PCMData + 1, PCMPos - 1);
        PCMPos = PCMPos <= 0 ? 0 : PCMPos - 1;
    }
}
#endif // ALTPCM

void ResetPPI()
{
    Reset8255(&PPI);
    PPI.Rout[0] = PSLReg = 0x00;
    PPI.Rout[2] = IOReg = 0x00;
}


static byte* ResizeMemory(byte * Buf, int Size)
{
    byte* P;
    if (Buf != NULL)
    {
        P = (byte*)realloc(Buf, Size);
        if (P == NULL)
        {
            free(Buf);
            P = (byte*)malloc(Size);
        }
    }
    else
    {
        P = (byte*)malloc(Size);
    }

    //else if(Buf != P)
    //{
    //    Buf = P;
    //}
    return(P);
}


byte ReadRAM(word A)
{
    return RAM[A >> 13][A & 0x1FFF];
}


void WriteRAM(word A, byte Value)
{
    RAM[A >> 13][A & 0x1FFF] = Value;
}


byte ChangeDiskWithFormat(byte N, const char* FileName, int Format)
{
    int NeedState;
    byte* P;

    /* We only have MAXDRIVES drives */
    if (N >= MAXDRIVES) return(0);

    /* Load state when inserting first disk into drive A: */
    NeedState = FileName && *FileName && !N && !FDD[N].Data;

    /* Reset FDC, in case it was running a command */
    Reset1793(&FDC, FDD, WD1793_KEEP);

#ifdef UPD_FDC
    if (MODEL(MSX_MSXTR))ResetTC8566AF(&TCFDC, FDD, TC8566AF_KEEP);
#endif // UPD_FDC

    /* Eject disk if requested */
    if (!FileName) { EjectFDI(&FDD[N]); return(1); }

#ifdef HDD_NEXTOR
    IsHardDisk = 0;
#endif // HDD_NEXTOR

    /* If FileName not empty, try loading disk image */
    if (*FileName && LoadFDI(&FDD[N], FileName, Format))
    {
        /* If first disk, also try loading state */
        if (NeedState) FindState(FileName);
#ifdef UPD_FDC
        DiskChanged[N] = 1;
#endif // UPD_FDC
        /* Done */
        return(1);
    }

#ifdef HDD_NEXTOR
    if (IsHardDisk)
    {
        return(0);
    }
#endif // HDD_NEXTOR

    /* If failed opening existing image, create a new 720kB disk image */
    P = FormatFDI(&FDD[N], FMT_MSXDSK);

    /* If FileName not empty, treat it as directory, otherwise new disk */
    if (P && !(*FileName ? DSKLoad(FileName, P, "MSX-DISK") : DSKCreate(P, "MSX-DISK")))
    {
        EjectFDI(&FDD[N]); return(0);
    }

    /* Done */
    return(!!P);
}


void SetNonMegaROM(byte Slot)
{
    byte PS, SS, * P, ROM64;
    int BASIC;

    if ((Slot < 0) || (Slot >= MAXSLOTS)) return;
    if (!ROMData[Slot])return;
    /* Find primary/secondary slots */
    for (PS = 0; PS < 4; ++PS)
    {
        for (SS = 0; (SS < 4) && (CartMap[PS][SS] != Slot); ++SS);
        if (SS < 4) break;
    }
    /* Drop out if slots not found */
    if (PS >= 4) return;

    P = ROMData[Slot] + 0;
    BASIC = (P[0] == 'A') && (P[1] == 'B') && !(P[2] || P[3]) && (P[8] || P[9]);
    switch (ROMLen[Slot])
    {
    case 1:
        /* 8kB ROMs are mirrored 8 times: 0:0:0:0:0:0:0:0 */
        if (!BASIC)
        {
            MemMap[PS][SS][0] = P;
            MemMap[PS][SS][1] = P;
            MemMap[PS][SS][2] = P;
            MemMap[PS][SS][3] = P;
        }
        else
        {
            MemMap[PS][SS][0] = EmptyRAM;
            MemMap[PS][SS][1] = EmptyRAM;
            MemMap[PS][SS][2] = EmptyRAM;
            MemMap[PS][SS][3] = EmptyRAM;
        }

        MemMap[PS][SS][4] = P;
        MemMap[PS][SS][5] = P;
        if (!BASIC)
        {
            MemMap[PS][SS][6] = P;
            MemMap[PS][SS][7] = P;
        }
        MemMap[PS][SS][6] = EmptyRAM;
        MemMap[PS][SS][7] = EmptyRAM;
        break;

    case 2:
        /* 16kB ROMs are mirrored 4 times: 0:1:0:1:0:1:0:1 */
        if (!BASIC)
        {
            MemMap[PS][SS][0] = P;
            MemMap[PS][SS][1] = P + 0x2000;
            MemMap[PS][SS][2] = P;
            MemMap[PS][SS][3] = P + 0x2000;
        }
        else
        {
            MemMap[PS][SS][0] = EmptyRAM;
            MemMap[PS][SS][1] = EmptyRAM;
            MemMap[PS][SS][2] = EmptyRAM;
            MemMap[PS][SS][3] = EmptyRAM;
        }

        MemMap[PS][SS][4] = P;
        MemMap[PS][SS][5] = P + 0x2000;
        if (!BASIC)
        {
            MemMap[PS][SS][6] = P;
            MemMap[PS][SS][7] = P + 0x2000;
        }
        else
        {
            MemMap[PS][SS][6] = EmptyRAM;
            MemMap[PS][SS][7] = EmptyRAM;
        }
        break;

    case 3:
    case 4:
        /* 24kB and 32kB ROMs are mirrored twice: 0:1:0:1:2:3:2:3 */
        MemMap[PS][SS][0] = P;
        MemMap[PS][SS][1] = P + 0x2000;
        MemMap[PS][SS][2] = P;
        MemMap[PS][SS][3] = P + 0x2000;
        MemMap[PS][SS][4] = P + 0x4000;
        MemMap[PS][SS][5] = P + 0x6000;
        MemMap[PS][SS][6] = P + 0x4000;
        MemMap[PS][SS][7] = P + 0x6000;
        break;

    case 6:
        if ((P[0] == 'A') && (P[1] == 'B') && (P[0x4000] != 'A') && (P[0x4001] != 'B'))
        {
            MemMap[PS][SS][0] = P;
            MemMap[PS][SS][1] = P + 0x2000;
            MemMap[PS][SS][2] = P;
            MemMap[PS][SS][3] = P + 0x2000;
            MemMap[PS][SS][4] = P + 0x4000;
            MemMap[PS][SS][5] = P + 0x6000;
            MemMap[PS][SS][6] = P + 0x8000;
            MemMap[PS][SS][7] = P + 0xA000;
        }
        else
        {
            MemMap[PS][SS][0] = P;
            MemMap[PS][SS][1] = P + 0x2000;
            MemMap[PS][SS][2] = P + 0x4000;
            MemMap[PS][SS][3] = P + 0x6000;
            MemMap[PS][SS][4] = P + 0x8000;
            MemMap[PS][SS][5] = P + 0xA000;
            MemMap[PS][SS][6] = P;
            MemMap[PS][SS][7] = P + 0x2000;
        }
        break;

    default:
        if (ROMLen[Slot] > 4)
            if ((P[0] != 'A') && (P[1] != 'B') && (P[0x4000] == 'A') && (P[0x4001] == 'B'))
            {
                MemMap[PS][SS][0] = P + 0x0000;
                MemMap[PS][SS][1] = P + 0x2000;
                MemMap[PS][SS][2] = P + 0x4000;
                MemMap[PS][SS][3] = P + 0x6000;

                MemMap[PS][SS][4] = P + 0x8000;
                MemMap[PS][SS][5] = P + 0xA000;
                MemMap[PS][SS][6] = P + 0xC000;
                MemMap[PS][SS][7] = P + 0xE000;
            }
        break;
    }
}


void ToggleVKey()
{
    VKey++;
    VKey &= 1;
}

byte PeekPSGReg(byte R)
{
    byte OldLatch = PSG.Latch;
    byte Val = 0xFF;
    PSG.Latch = R;
    Val = InZ80(0xA2);
    PSG.Latch = OldLatch;
    return Val;
}


byte GetROMType(int Slot)
{
    return OldROMType[Slot];
}


void SetROMType(int Slot, int Value)
{
    OldROMType[Slot] = Value;
}


void VDPSync()
{
    int currScanTime = GetCurrScanLine();
    //while (ScanLine && (ScanLine<currScanTime))
    while (currScanLine && (currScanLine < currScanTime))
    {
        if (currScanLine && (currScanLine < 192))
        {
            ScanLine = ScanLine < (PALVideo ? 312 : 261) ? ScanLine + 1 : 0;
            //currScanLine = ScanLine - (ScanLines212 ? 32 : 42) - VAdjust;
            currScanLine = ScanLine - firstScanLine;
            if ((MODEL(MSX_MSX2)) || !ModeYJK || (ScrMode < 7) || (ScrMode > 8))(RefreshLine[ScrMode])(currScanLine);
            else if (ModeYAE) RefreshLine[10](currScanLine);
            else RefreshLine[12](currScanLine);

            LoopVDP();
        }
        else break;
        //ScanLine++;
    }
}

void verboseFDC()
{
    FDC.Verbose = 1;
}

void ChangeFMSound(int FMType)
{
    switch (FMType)
    {
    case 0:
        if (MODEL(MSX_MSXTR) && !Use8950TurboR)
        {
            playY8950 = 0;
        }
        playYM2413 = 1;
        break;
    case 1:
        if (MODEL(MSX_MSXTR) && !Use8950TurboR)
        {
            if (playYM2413)return;
        }
        playY8950 = 1;
        break;
    default:
        break;
    }
}

void DoCPUChange()
{
    CPU.PC.W++;

    byte oldCPUUSer = CPU.User;
    /* Use CPU.User to store CPU mode value. 0=Z80, 1=R800. */
    if (S1990Reg6 & 0x20)CPU.User &= ~0x01;
    else CPU.User |= 0x01;
    if (CPU.User != oldCPUUSer)
    {
        if (CPU.User & 0x01)
        {
            /* CPU.ICount = (CPU.IPeriod<<1) - (CPU.IPeriod - CPU.ICount) */
            CPU.ICount += CPU.IPeriod;
            CPU.IBackup += CPU.IPeriod;

            //CPU.PC.W++;
            //CPU.IFF = 0x00;
            //CPU.IRequest = INT_NONE;

            //VDPIOTimerCnt += CPU.IPeriod;
            //R800TimerCnt += CPU.IPeriod;
            //PCMTimerCnt += CPU.IPeriod;
            CPU.IPeriod <<= 1;
        }
        else
        {
            /* CPU.ICount = (CPU.IPeriod >> 1) - (CPU.IPeriod - CPU.ICount) */
            CPU.ICount -= (CPU.IPeriod >> 1);
            CPU.IBackup -= (CPU.IPeriod >> 1);

            //CPU.PC.W++;
            //CPU.IFF = 0x00;
            //CPU.IRequest = INT_NONE;

            //VDPIOTimerCnt -= (CPU.IPeriod >> 1);
            //R800TimerCnt -= (CPU.IPeriod >> 1);
            //PCMTimerCnt -= (CPU.IPeriod >> 1);
            CPU.IPeriod >>= 1;
        }
    }
    if (Verbose & 0x40) printf("CPU Changed :%s ROMMode:%s \n", (S1990Reg6 & 0x20) ? "Z80" : "R800", (S1990Reg6 & 0x40) ? "ROM" : "DRAM");
}


void UpdateTurboRTimer(int val)
{
    R800TimerCnt += val;
    PCMTimerCnt += val + 59;
    VDPIOTimerCnt += val;
#ifdef AUDIO_SYNC
    audioCycleCnt += val;
#endif // AUDIO_SYNC

}
#endif  //TURBO_R


#ifdef VDP_V9990
void V9990Out(register byte R, register byte V)
{
    register byte J;
    switch (R)
    {
    case 0:
        vdpV9990.VRAMWriteInt = (((uint)(V9990VDP[2] & 0x07) << 16) | ((uint)V9990VDP[1] << 8) | (uint)V) & 0x7FFFF;
        break;
    case 1:
        vdpV9990.VRAMWriteInt = (((uint)(V9990VDP[2] & 0x07) << 16) | ((uint)V << 8) | (uint)V9990VDP[0]) & 0x7FFFF;
        break;
    case 2:
        vdpV9990.VRAMWriteInt = (((uint)(V & 0x07) << 16) | ((uint)V9990VDP[1] << 8) | (uint)V9990VDP[0]) & 0x7FFFF;
        vdpV9990.IncVRAMWrite = !(V & 0x080) ? 1 : 0;
        break;
    case 3:
        vdpV9990.VRAMReadInt = (((uint)(V9990VDP[5] & 0x07) << 16) | ((uint)V9990VDP[4] << 8) | (uint)V) & 0x7FFFF;
        break;
    case 4:
        vdpV9990.VRAMReadInt = (((uint)(V9990VDP[5] & 0x07) << 16) | ((uint)V << 8) | (uint)V9990VDP[3]) & 0x7FFFF;
        break;
    case 5:
        vdpV9990.VRAMReadInt = (((uint)(V & 0x07) << 16) | ((uint)V9990VDP[4] << 8) | (uint)V9990VDP[3]) & 0x7FFFF;
        vdpV9990.IncVRAMRead = !(V & 0x80) ? 1 : 0;
        break;
    case 6:
        V9990VDP[6] = V;
        SetScreenV9990();
        break;
    case 7:
        V9990VDP[R] = V;
        SetScreenV9990();
        break;
    case 8:
        if (V & 0x20)V9990Dual = 1; /* Super Impose */
        else V9990Dual &= 0xFE;
        break;
    case 13:
        if (V & 0xC0)
        {
            V9990VDP[R] = V;
            SetScreenV9990();
        }
        break;
        /* For 17,18,21,22 Taken from WebMSX. */
        /* https://webmsx.org/ */
        /* Update V9990 Y Scroll H value and Display enable/disable  and Sprite enable/disable only at frame start. */
        /* This fix many graphic glitches. Ghosts'n Goblins port from ASM Team , Golden Power Disc #12 etc. */
    case 17:
        V9KYScrollOff = (V9KYScrollOff & 0x1F00) | (unsigned int)V;
        V9KYScrollOff2 = V9KScanLine > V9kFirstLine ? V9KYScrollOff - V9KcurrLine : V9KYScrollOff;
        vdpV9990.pending |= 0x01;
        break;
    case 18:
        if (!(V9990VDP[18] & 0x1F) && (V & 0x1F))vdpV9990.pending |= 0x01;
        if ((V9990VDP[18] & 0x1F) && !(V & 0x1F))vdpV9990.pending |= 0x01;
        break;
    case 21:
        V9KYScrollOffB = (V9KYScrollOffB & 0x0100) | (unsigned int)V;
        V9KYScrollOffB2 = V9KScanLine > V9kFirstLine ? V9KYScrollOffB - V9KcurrLine : V9KYScrollOffB;
        vdpV9990.pending |= 0x02;
        break;
    case 22:
        if (!(V9990VDP[22] & 0x01) && (V & 0x01))vdpV9990.pending |= 0x02;
        if ((V9990VDP[22] & 0x01) && !(V & 0x01))vdpV9990.pending |= 0x02;
        break;
    case 45:
        V &= 0x1F;
        break;
    case 52:
        //VDPDrawV9990(0x40);
        VDPDrawV9990(V);
        break;
    default:
        break;
    }
    V9990VDP[R] = V;
}

word LoopZ80_V9990(Z80* R)
{
    static byte Drawing = 0;
    register int J;

    /* Flip HRefresh bit */
    V9990Port[5] ^= 0x20;
    V9KcurrLine = V9KScanLine - V9kFirstLine;

  /* If HRefresh is now in progress... */
    if(!(V9990Port[5]&0x20))
    {
        if(!(V9KScanLine))
        {
            V9kFirstLine = 32;
            
            /* Update V9990 Y Scroll H value and Display enable/disable  and Sprite enable/disable only at frame start. */
            /* Info from WebMSX. */
            /* https://webmsx.org */
            /* This fix some graphic glitches. Ghosts'n Goblins port from ASM Team etc. */
            if(vdpV9990.pending & 0x01)V9KYScrollOff2 = V9KYScrollOff = ((unsigned int)(V9990VDP[18] & 0x1F) << 8) | (V9KYScrollOff & 0xFF);
            if(vdpV9990.pending & 0x02)V9KYScrollOffB2 = V9KYScrollOffB = ((unsigned int)(V9990VDP[22] & 0x01) << 8) | (V9KYScrollOffB & 0xFF);
            vdpV9990.pending = 0;
            V9KVDP8Val = V9990VDP[8];
        }
        //if (ScanLine == firstScanLine - 1)
        if (V9KScanLine == V9kFirstLine - 1)
        {
            /* Reset VRefresh bit */
            V9990Port[5] &= 0xBF;

            if ((V9990Port[6] & 0x01) && (V9990VDP[9] & 0x01))
            {
                V9990Port[6] &= 0xFE;
                SetIRQV9K();
            }
            else V9990Port[6] &= 0xFE;

            /* Interlace */
            V9990Port[5] ^= 0x02;
        }
        else if (V9KcurrLine == 212)
        {
            /* Set VBlank bit, set VRefresh bit */
            V9990Port[5] |= 0x40;

            if (!(V9990Port[6] & 0x01) && (V9990VDP[9]&0x01))
            {
                V9990Port[6] |= 1;
                SetIRQV9K();
            }
            else V9990Port[6] |= 1;
        }

        /* Run V9938 engine */
        LoopVDPV9990();

        /* Refresh scanline, possibly with the overscan */
        if (Drawing && (V9KcurrLine < 230))
        {
            switch (V9KScrMode)
            {
            case 0:
                RefreshLineP1AB(V9KcurrLine);
                break;
            case 1:
                RefreshLineP2(V9KcurrLine);
                break;
            case 3:
                switch (V9KType)
                {
                case 4:
                    RefreshLineBYUV(V9KcurrLine);
                    break;
                case 6:
                    RefreshLineBD16(V9KcurrLine);
                    break;
                case 8:
                    RefreshLineBP6(V9KcurrLine);
                    break;
                case 9:
                    RefreshLineB1(V9KcurrLine);
                    break;
                default:
                    RefreshLineB1(V9KcurrLine);
                    break;
                }
                break;
            case 5:
                switch (V9KType)
                {
                case 4:
                    RefreshLineBYUV(V9KcurrLine);
                    break;
                case 6:
                    RefreshLineBD16Wide(V9KcurrLine);
                    break;
                case 8:
                    RefreshLineBP6Wide(V9KcurrLine);
                    break;
                case 9:
                    RefreshLineB3(V9KcurrLine);
                    break;
                default:
                    RefreshLineB3(V9KcurrLine);
                    break;
                }
                break;
            case 8:
                RefreshLineB6(V9KcurrLine);
                break;
            default:
                RefreshLineB1(V9KcurrLine);
                break;
            }
        }
        return 0;
        }

    /*********************************/
    /* We come here for HBlanks only */
    /*********************************/
        
    V9KScanLine = V9KScanLine < 261 ? V9KScanLine + 1 : 0;
    V9KcurrLine = V9KScanLine - V9kFirstLine;

    ///* If first scanline of the bottom border... */

    /* If first scanline of VBlank... */
    if (V9KcurrLine == 212)Drawing = 0;

    /* Drawing now... */
    if (V9KScanLine == V9kFirstLine)Drawing = 1;

    /* Line coincidence processing */
    //if (((V9KcurrLine + V9990VDP[17]) & 0xFF) == (V9990VDP[10] | ((V9990VDP[11] & 0x03) << 8)))
    if ((V9KcurrLine & 0xFF) == (V9990VDP[10] | ((V9990VDP[11] & 0x03) << 8)))
    {
        if (!(V9990Port[6] & 0x02))
        {
            V9990Port[6] |= 2;
            SetIRQV9K();
        }
    }
    return 0;
    }


void V9990SetPalette()
{
    int Idx = (V9990VDP[14]>>2)<<2;
    byte r = V9KPal[Idx] & 0x1F;
    byte g = V9KPal[Idx + 1] & 0x1F;
    byte b =V9KPal[Idx + 2] & 0x1F;
    V9990SetColor(Idx, r, g, b);
    //SetColor(Idx >> 2, r, g, b);
}

word SetIRQV9K(void)
{
    if (V9990VDP[9] & V9990Port[6])IRQPending |= 0x08; else IRQPending &= ~0x08;
    CPU.IRequest = (V9990VDP[9] & V9990Port[6]) ? INT_IRQ : INT_NONE;
    return(CPU.IRequest);
}


void InitV9990()
{
    if (!UseV9990)return;
    int J, VSize;
    byte* P, P1;
    vdpV9990.Active = 1;
    V9990Active = 1;
    if (V9990DualScreen)V9990Dual |= 2;

    for (J = 0; J < 64; J++)V9990VDP[J] = 0;
    for (J = 0; J < 16; J++)V9990Port[J] = 0;

    for (J = 0; J < 256; J +=4) V9990SetColor(J, 0x1F, 0x1F, 0x1F);

    InitXbuf();

    VSize = 32 * 0x4000;
    P = ResizeMemory(V9KVRAM, VSize);
    for (J = 0; J < VSize; J += 1024)
    {
        //memset(P + J, 0x00, 512);
        //memset(P + (J + 512), NORAM, 512);
        memset(P + J, 0x00, 1024);
    }
    V9KVRAM = P;
    
    InitV9990CMD();
    SetIRQV9K();
}

/* Change Screen Mode/Type */
byte SetScreenV9990(void)
{
    byte V = V9990VDP[6];
    if (!(V & 0xC0))    /* P1 Mode */
    {
        V9KScrMode = 0;
        V9KImgWidth = 256;
        V9KImgHeight = 2048;
        V9KType = 1;
        V9KPixelRes = 1;
        //WideScreenOff();
    }
    else if (!(V & 0x80)) /* P2 Mode */
    {
        V9KScrMode = 1;
        V9KImgWidth = 512;
        V9KImgHeight = 2048;
        V9KType = 1;
        V9KPixelRes = 1;
        //WideScreenOn();
    }
    else if ((V9990VDP[7] & 0x41) == 0x41)  /* B6 Mode(640x480) */
    {
        V9KScrMode = 8;
        V9KImgWidth = 1024;
        V9KImgHeight = 1024;
        //WideScreenOn();
    }
    else if ((V & 0x30) == 0x00)  /* B1 Mode(256x212) */
    {
        V9KScrMode = 3;
        V9KImgWidth = 256 << ((V & 0x0C) >> 2);
        switch (V & 0x03)
        {
        case 1:
            V9KType = 9;
            V9KPixelRes = 1;
            break;
        case 2:
            switch ((V9990VDP[13] >> 5) & 0xFF)
            {
            case 6:
                V9KType = 4;
                V9KPixelRes = 2;
                break;
            default:
                V9KType = 8;
                V9KPixelRes = 2;
                break;
            }
            break;
        case 3:
            V9KType = 6;
            V9KPixelRes = 3;
            break;
        default:
            break;
        }
        V9KImgHeight = ((0x80000 >> V9KPixelRes)<<2) / V9KImgWidth;
        //WideScreenOff();
    }
    else if ((V & 0x30) == 0x10)    /* B3 Mode(512x212) */
    {
        V9KScrMode = 5;
        V9KImgWidth = 256 << ((V & 0x0C) >> 2);
        switch (V & 0x03)
        {
        case 1:
            V9KType = 9;
            V9KPixelRes = 1;
            break;
        case 2:
            switch ((V9990VDP[13] >> 5) & 0xFF)
            {
            case 6:
                V9KType = 4;
                V9KPixelRes = 2;
                break;
            default:
                V9KType = 8;
                V9KPixelRes = 2;
                break;
            }
            break;
        case 3:
            V9KType = 6;
            V9KPixelRes = 3;
            break;
        default:
            break;
        }
        V9KImgHeight = ((0x80000 >> V9KPixelRes) << 2) / V9KImgWidth;
        //WideScreenOn();
    }
    else if ((V & 0x20) == 0x20)
    {
        V9KScrMode = 8;
        V9KImgWidth = 256 << ((V & 0x0C) >> 2);
        V9KImgHeight = 4096 >> ((V & 0x0C) >> 2);
        //WideScreenOn();
    }
    if (Verbose & 0x80)
    {
        printf("V9990 Screen Mode[%d]  Screen Type[%d]  Resolution[%d]\n", V9KScrMode, V9KType, V9KPixelRes);
        printf("V9990 Image Width[%d]  V9990 Image Height[%d]\n", V9KImgWidth, V9KImgHeight);
    }
}
#endif // VDP_V9990


#ifdef MEGASCSI_HD
byte ReadMEGASCSI(word A)
{
    byte J, I, PS, SS;
    J = A >> 14;
    PS = PSL[J];
    SS = SSL[J];
    I = CartMap[PS][SS];
    //J = A >> 13;
    J = (A - 0x4000) >> 13;
    if ((!J) && (ROMMapper[I][0] == 0x7F))
    {
        if ((A & 0x1FFF) < 0x1000)      /* 0x4000-0x4FFF: Data Regsiter */
        {
            if (Verbose & 0x08)
                printf("Mega-SCSI Read Data Register %4Xh  PC:%4Xh\n", A, CPU.PC.W);
            return MB89352A_ReadDREG(&spc);
        }
        /* 0x5000-0x5FFF: spc register */
        A &= 0x0F;
        if (Verbose & 0x08)
            printf("Mega-SCSI read spc Register:%d  PC:%4Xh \n", A, CPU.PC.W);
        return MB89352A_ReadReg(&spc, A);
    }
    //else if(ROMMapper[I][J]>=0x80)
    else
    {
        //if (Verbose & 0x08)
        //    printf("Mega-SCSI Read SRAM %4Xh\n", A);
        //return NORAM;
        return(RAM[A >> 13][A & 0x1FFF]);
    }
    //else
    //{
    //   return NORAM;
    //}
}
#endif // MEGASCSI_HD

#if defined(HDD_NEXTOR) || defined(MEGASCSI_HD)
byte ChangeHDDWithFormat(byte N, const char* FileName, int Format)
{
    int NeedState, J, C2, Size;
    byte* P;
    FILE* F;

    /* Load state when inserting first disk into drive A: */
    NeedState = FileName && *FileName && !N && !HDD[N].Data;

    /* Eject disk if requested */
    if (!FileName) { EjectFDI(&HDD[N]); return(1); }

    ///* If FileName not empty, try loading disk image */
    //if (!(F = zipfopen(FileName, "rb+"))) return(0);

    EjectFDI(&HDD[N]);

    //if (!fseek(F, 0, SEEK_END)) HDDSize = ftell(F);
    //else
    //{
    //    HDDSize = 0;
    //    /* Read file in 16kB increments */
    //    while ((J = fread(EmptyRAM, 1, 0x4000, F)) == 0x4000) HDDSize += J;
    //    if (J > 0) HDDSize += J;
    //    /* Clean up the EmptyRAM! */
    //    memset(EmptyRAM, NORAM, 0x4000);
    //}
    // /* Rewind file to the beginning */
    //rewind(F);

    //fclose(HDDStream);
    //HDDStream = F;

    //fclose(F);
    if (!(P = LoadROM(FileName, HDDSize, 0)))return(0);
    HDD[N].Data = P;
    HDD[N].DataSize = HDDSize;
    //HDD[N].Format = Format;
    if (Verbose)printf("Load HardDisk Image Size %d", HDDSize);

#ifdef MEGASCSI_HD
    MB89352A_Init(&spc);
    MB89352A_Reset(&spc);
#endif // MEGASCSI_HD

    return(1);
}
#endif // HDD_NEXTOR     MEGASCSI_HD

#ifdef HDD_IDE
byte ChangeHDDIDEWithFormat(byte N, const char* FileName, int Format)
{
    int NeedState, J, C2, Size;
    byte* P;
    FILE* F;

    /* Load state when inserting first disk into drive A: */
    NeedState = FileName && *FileName && !N && !HDD[N].Data;

    /* Eject disk if requested */
    if (!FileName) { EjectFDI(&HDD[N]); return(1); }

    /* If FileName not empty, try loading disk image */
    if (!(F = zipfopen(FileName, "rb"))) return(0);

    EjectFDI(&HDD[N]);

    if (!fseek(F, 0, SEEK_END)) Size = ftell(F);
    else
    {
        /* Read file in 16kB increments */
        while ((J = fread(EmptyRAM, 1, 0x4000, F)) == 0x4000) Size += J;
        if (J > 0) Size += J;
        /* Clean up the EmptyRAM! */
        memset(EmptyRAM, NORAM, 0x4000);
    }
    /* Rewind file to the beginning */
    rewind(F);

    if (!(P = LoadROM(FileName, Size, 0)))return(0);
    HDD[N].Data = P;
    HDD[N].DataSize = Size;
    HDD[N].Format = Format;
    if (Verbose)printf("Load HardDisk Image Size %d", Size);
    return(1);
}
#endif // HDD_IDE


#if defined(ANDROID)
#undef  feof
#define fopen           mopen
#define fclose          mclose
#define fread           mread
#define fwrite          mwrite
#define fgets           mgets
#define fseek           mseek
#define rewind          mrewind
#define fgetc           mgetc
#define ftell           mtell
#define feof            meof
#elif defined(ZLIB)
#undef  feof
#define fopen(N,M)      (FILE *)gzopen(N,M)
#define fclose(F)       gzclose((gzFile)(F))
#define fread(B,L,N,F)  gzread((gzFile)(F),B,(L)*(N))
#define fwrite(B,L,N,F) gzwrite((gzFile)(F),B,(L)*(N))
#define fgets(B,L,F)    gzgets((gzFile)(F),B,L)
#define fseek(F,O,W)    gzseek((gzFile)(F),O,W)
#define rewind(F)       gzrewind((gzFile)(F))
#define fgetc(F)        gzgetc((gzFile)(F))
#define ftell(F)        gztell((gzFile)(F))
#define feof(F)         gzeof((gzFile)(F))
#endif

/** GuessROM() ***********************************************/
/** Guess MegaROM mapper of a ROM.                          **/
/*************************************************************/
int GuessROM(const byte * Buf, int Size)
{
    int J, I, K, Result, ROMCount[MAXMAPPERS];
    char S[256];
    FILE* F;

    /* No result yet */
    Result = -1;

    /* Change to the program directory */
    if (ProgDir && chdir(ProgDir))
    {
        if (Verbose) printf("Failed changing to '%s' directory!\n", ProgDir);
    }

#ifdef _3DS
    //Result = CalcCRC32(Buf, Size);
    Result = CalcCRC32((void*)Buf, "/FMSX3DS/CARTS32.CSV", Size);
    if (Result < 0)Result = CalcCRC32((void*)Buf, "/FMSX3DS/CARTS.CRC", Size);

    /* Try opening file with SHA1 sums */
  //for 3DS -> use romfs
    if ((Result < 0) && (F = zipfopen("CARTS.SHA", "rb")))
#else
    /* Try opening file with CRCs */
    if ((F = fopen("CARTS.CRC", "rb")))
    {
        /* Compute ROM's CRC */
        for (J = K = 0; J < Size; ++J) K += Buf[J];

        /* Scan file comparing CRCs */
        while (fgets(S, sizeof(S) - 4, F))
            if (sscanf(S, "%08X %d", &J, &I) == 2)
                if (K == J) { Result = I; break; }

        /* Done with the file */
        fclose(F);
    }

    /* Try opening file with SHA1 sums */
    if ((Result < 0) && (F = fopen("CARTS.SHA", "rb")))
#endif // _3DS
    {
        char S1[41], S2[41];
        SHA1 C;

        /* Compute ROM's SHA1 */
        ResetSHA1(&C);
        InputSHA1(&C, Buf, Size);
        if (ComputeSHA1(&C) && OutputSHA1(&C, S1, sizeof(S1)))
        {
            while (fgets(S, sizeof(S) - 4, F))
                if ((sscanf(S, "%40s %d", S2, &J) == 2) && !strcmp(S1, S2))
                {
                    Result = J; break;
                }
        }

        /* Done with the file */
        fclose(F);
    }

    /* We are now back to working directory */
    if (WorkDir && chdir(WorkDir))
    {
        if (Verbose) printf("Failed changing to '%s' directory!\n", WorkDir);
    }

    /* If found ROM by CRC or SHA1, we are done */
    if (Result >= 0) return(Result);

    /* Clear all counters */
    for (J = 0; J < MAXMAPPERS; ++J) ROMCount[J] = 1;
#ifdef _3DS
    ROMCount[MAP_ASCII8] += 1;
    ROMCount[MAP_ASCII8SRM32] -= 1;

    /* Count occurences of characteristic addresses */
    for (J = 0; J < Size - 2; ++J)
    {
        I = Buf[J] + ((int)Buf[J + 1] << 8) + ((int)Buf[J + 2] << 16);
        switch (I)
        {
        case 0x500032: ROMCount[MAP_KONAMI5] += 2; break;
        case 0x900032: ROMCount[MAP_KONAMI5] += 2; break;
        case 0xB00032: ROMCount[MAP_KONAMI5] += 2; break;
            //case 0x400032: ROMCount[MAP_KONAMI4] +=3; break;
        case 0x800032: ROMCount[MAP_KONAMI4] += 3; break;
        case 0xA00032: ROMCount[MAP_KONAMI4] += 3; break;
        case 0x680032:
            ROMCount[MAP_ASCII8] += 3;
            ROMCount[MAP_ASCII8SRM32] += 3;
            break;
        case 0x780032:
            ROMCount[MAP_ASCII8] += 3;
            ROMCount[MAP_ASCII8SRM32] += 3;
            break;
        case 0x600032: ROMCount[MAP_KONAMI4] += 2;
            ROMCount[MAP_ASCII8] += 3;
            ROMCount[MAP_ASCII16] += 4;
            ROMCount[MAP_ASCII8SRM32] += 3;
            break;
        case 0x700032: ROMCount[MAP_KONAMI5] += 2;
            ROMCount[MAP_ASCII8] += 3;
            ROMCount[MAP_ASCII16] += 4;
            ROMCount[MAP_ASCII8SRM32] += 3;
            break;
        case 0x77FF32: ROMCount[MAP_ASCII16]++; break;
        case 0x574F4E:
            /* Many KOEI games contains "NOW DEVICE CHECKING" text.*/
            if (J > 0x10 && J < 0x1000)
            {
                if (Buf[J + 3] == 0x20 && Buf[J + 4] == 0x44 && Buf[J + 5] == 0x45 && Buf[J + 6] == 0x56 && Buf[J + 7] == 0x49 && Buf[J + 8] == 0x43
                    && Buf[J + 9] == 0x45 && Buf[J + 10] == 0x20 && Buf[J + 11] == 0x43 && Buf[J + 12] == 0x48 && Buf[J + 13] == 0x45
                    && Buf[J + 14] == 0x43 && Buf[J + 15] == 0x4B && Buf[J + 16] == 0x49 && Buf[J + 17] == 0x4E && Buf[J + 18] == 0x47)
                {
                    ROMCount[MAP_ASCII8SRM32] += 10;
                }
            }
            break;
        }
    }
    /* The ROM created with MSXBAS2ROM contains "MSXB2R" signature at 0x000A or 0x400A position. */
    /* https://github.com/amaurycarvalho/msxbas2rom */
    if ((Buf[0x000A] == 'M' && Buf[0x000B] == 'S' && Buf[0x000C] == 'X' && Buf[0x000D] == 'B' && Buf[0x000E] == '2' && Buf[0x000F] == 'R')
        || (Buf[0x400A] == 'M' && Buf[0x400B] == 'S' && Buf[0x400C] == 'X' && Buf[0x400D] == 'B' && Buf[0x400E] == '2' && Buf[0x400F] == 'R'))
    {
        ROMCount[MAP_KONAMI5] += 100;
        if (Verbose)printf("You load the ROM that crated with the MSXBAS2ROM.\n");
    }

    /* The ROM created with DSK2ROM contains "DSK2ROM" signature.*/
    /* DSK2ROM uses either KonamiSCC mapper  or ASCII8 mapper . */
    /* http://home.kabelfoon.nl/~vincentd/ */
    /* https://github.com/joyrex2001/dsk2rom */
    if (Buf[0x3FE0] == 'D' && Buf[0x3FE1] == 'S' && Buf[0x3FE2] == 'K' && Buf[0x3FE3] == '2' && Buf[0x3FE4] == 'R' && Buf[0x3FE5] == 'O' && Buf[0x3FE6] == 'M' && Buf[0x3FE7] == 0x20)
    {
        ROMCount[MAP_ASCII8] += 100;
        ROMCount[MAP_KONAMI5] += 100;
        if (Verbose)printf("You load the ROM that crated with the DSK2ROM.\n");
    }
#else
    /* Generic 8kB mapper is default */
    ROMCount[MAP_GEN8] += 1;
    /* ASCII 16kB preferred over ASCII 8kB */
    ROMCount[MAP_ASCII16] -= 1;

    /* Count occurences of characteristic addresses */
    for (J = 0; J < Size - 2; ++J)
    {
        I = Buf[J] + ((int)Buf[J + 1] << 8) + ((int)Buf[J + 2] << 16);
        switch (I)
        {
        case 0x500032: ROMCount[MAP_KONAMI5]++; break;
        case 0x900032: ROMCount[MAP_KONAMI5]++; break;
        case 0xB00032: ROMCount[MAP_KONAMI5]++; break;
        case 0x400032: ROMCount[MAP_KONAMI4]++; break;
        case 0x800032: ROMCount[MAP_KONAMI4]++; break;
        case 0xA00032: ROMCount[MAP_KONAMI4]++; break;
        case 0x680032: ROMCount[MAP_ASCII8]++; break;
        case 0x780032: ROMCount[MAP_ASCII8]++; break;
        case 0x600032: ROMCount[MAP_KONAMI4]++;
            ROMCount[MAP_ASCII8]++;
            ROMCount[MAP_ASCII16]++;
            break;
        case 0x700032: ROMCount[MAP_KONAMI5]++;
            ROMCount[MAP_ASCII8]++;
            ROMCount[MAP_ASCII16]++;
            break;
        case 0x77FF32: ROMCount[MAP_ASCII16]++; break;
        }
    }
#endif // _3DS

    /* Find which mapper type got more hits */
    for (I = 0, J = 0; J < MAXMAPPERS; ++J)
        if (ROMCount[J] > ROMCount[I]) I = J;

#ifdef _3DS
    if ((I == MAP_ASCII16) && (Size>0x200000))
    {
        I = MAP_ASCII16Big;
    }
#endif // _3DS


    /* Return the most likely mapper type */
    return(I);
}

/** LoadFNT() ************************************************/
/** Load fixed 8x8 font used in text screen modes when      **/
/** MSX_FIXEDFONT option is enabled. LoadFNT(0) frees the   **/
/** font buffer. Returns 1 on success, 0 on failure.        **/
/*************************************************************/
byte LoadFNT(const char* FileName)
{
    FILE* F;

    /* Drop out if no new font requested */
    if (!FileName) { FreeMemory(FontBuf); FontBuf = 0; return(1); }
    /* Try opening font file */
    if (!(F = fopen(FileName, "rb"))) return(0);
    /* Allocate memory for 256 8x8 characters, if needed */
    if (!FontBuf) FontBuf = GetMemory(256 * 8);
    /* Drop out if failed memory allocation */
    if (!FontBuf) { fclose(F); return(0); }
    /* Read font, ignore short reads */
    fread(FontBuf, 1, 256 * 8, F);
    /* Done */
    fclose(F);
    return(1);
}

/** LoadROM() ************************************************/
/** Load a file, allocating memory as needed. Returns addr. **/
/** of the alocated space or 0 if failed.                   **/
/*************************************************************/
byte* LoadROM(const char* Name, int Size, byte * Buf)
{
    FILE* F;
    byte* P;
    int J;

    /* Can't give address without size! */
    if (Buf && !Size) return(0);

    /* Open file */
#ifdef _3DS
    if (!(F = zipfopen(Name, "rb"))) return(0);
#else
    if (!(F = fopen(Name, "rb"))) return(0);
#endif // _3DS

    /* Determine data size, if wasn't given */
    if (!Size)
    {
        /* Determine size via ftell() or by reading entire [GZIPped] stream */
        if (!fseek(F, 0, SEEK_END)) Size = ftell(F);
        else
        {
            /* Read file in 16kB increments */
            while ((J = fread(EmptyRAM, 1, 0x4000, F)) == 0x4000) Size += J;
            if (J > 0) Size += J;
            /* Clean up the EmptyRAM! */
            memset(EmptyRAM, NORAM, 0x4000);
        }
        /* Rewind file to the beginning */
        rewind(F);
    }

    /* Allocate memory */
    P = Buf ? Buf : GetMemory(Size);
    if (!P)
    {
        fclose(F);
        return(0);
    }

    /* Read data */
    if ((J = fread(P, 1, Size, F)) != Size)
    {
#ifdef _3DS
        if (Size == 0x2000)
        {
            fclose(F);
            return(P);
        }
#endif // _3DS

        if (!Buf) FreeMemory(P);
        fclose(F);
        return(0);
    }

    /* Done */
    fclose(F);
    return(P);
}

/** FindState() **********************************************/
/** Compute state file name corresponding to given filename **/
/** and try loading state. Returns 1 on success, 0 on       **/
/** failure.                                                **/
/*************************************************************/
int FindState(const char* Name)
{
    int J, I;
    char* P;

    /* No result yet */
    J = 0;

    /* Remove old state name */
    FreeMemory(STAName);

    /* If STAName gets created... */
    if ((STAName = MakeFileName(Name, ".sta")))
    {
        /* Try loading state */
        if (Verbose) printf("Loading state from %s...", STAName);
        J = LoadSTA(STAName);
        PRINTRESULT(J);
    }

    /* Generate .CHT cheat file name and try loading it */
    if ((P = MakeFileName(Name, ".cht")))
    {
        I = LoadCHT(P);
        if (I && Verbose) printf("Loaded %d cheats from %s\n", I, P);
        FreeMemory(P);
    }

    /* Generate .MCF cheat file name and try loading it */
    if ((P = MakeFileName(Name, ".mcf")))
    {
        I = LoadMCF(P);
        if (I && Verbose) printf("Loaded %d cheat entries from %s\n", I, P);
        FreeMemory(P);
    }

    /* Generate palette file name and try loading it */
    if ((P = MakeFileName(Name, ".pal")))
    {
        I = LoadPAL(P);
        if (I && Verbose) printf("Loaded palette from %s\n", P);
        FreeMemory(P);
    }

    /* Done */
    return(J);
}

/** LoadCart() ***********************************************/
/** Load cartridge into given slot. Returns cartridge size  **/
/** in 16kB pages on success, 0 on failure.                 **/
/*************************************************************/
int LoadCart(const char* FileName, int Slot, int Type)
{
#ifdef _3DS
    int C1, C2, Len, Pages, ROM64, BASIC, X, OldLen, AddtionalSize;
#else
    int C1, C2, Len, Pages, ROM64, BASIC;
#endif // _3DS
    byte* P, PS, SS;
    char* T;
    FILE* F;

#ifdef _3DS
    if (SaveSRAMPAC)
    {
        if (!(F = fopen("/FMSX3DS/SRAM/FMPAC.SAV", "wb")))SaveSRAMPAC = 0;
        else
        {
            if (fwrite(PACSaveData, 1, 0x2000, F) != 0x2000)SaveSRAMPAC = 0;
        }
        fclose(F);
    }
#endif // _3DS

    /* Slot number must be valid */
    if ((Slot < 0) || (Slot >= MAXSLOTS)) return(0);
    /* Find primary/secondary slots */
    for (PS = 0; PS < 4; ++PS)
    {
        for (SS = 0; (SS < 4) && (CartMap[PS][SS] != Slot); ++SS);
        if (SS < 4) break;
    }
    /* Drop out if slots not found */
    if (PS >= 4) return(0);

    /* If there is a SRAM in this cartridge slot... */
#ifdef _3DS
    //if (SaveSRAM[Slot] && SRAMName[Slot]
    //    && (SRAMData[Slot] || (ROMType[Slot] == MAP_ASCII8SRM32) || (ROMType[Slot]==MAP_MANBOW2)))
    if (SaveSRAM[Slot] && SRAMName[Slot])
#else
    if (SRAMData[Slot] && SaveSRAM[Slot] && SRAMName[Slot])
#endif // _3DS
    {
        /* Open .SAV file */
        if (Verbose) printf("Writing %s...", SRAMName[Slot]);
#ifdef _3DS
        if (!(F = sramfopen(SRAMName[Slot], "wb"))) SaveSRAM[Slot] = 0;
#else
        if (!(F = fopen(SRAMName[Slot], "wb"))) SaveSRAM[Slot] = 0;
#endif // _3DS
        else
        {
            /* Write .SAV file */
            switch (ROMType[Slot])
            {
            case MAP_ASCII8:
            case MAP_FMPAC:
                if (fwrite(SRAMData[Slot], 1, 0x2000, F) != 0x2000) SaveSRAM[Slot] = 0;
                break;
            case MAP_ASCII16:
#ifdef _3DS
            case MAP_ASCII16Big:
#endif // _3DS
                if (fwrite(SRAMData[Slot], 1, 0x0800, F) != 0x0800) SaveSRAM[Slot] = 0;
                break;
            case MAP_GMASTER2:
                if (fwrite(SRAMData[Slot], 1, 0x1000, F) != 0x1000)        SaveSRAM[Slot] = 0;
                if (fwrite(SRAMData[Slot] + 0x2000, 1, 0x1000, F) != 0x1000) SaveSRAM[Slot] = 0;
                break;
#ifdef _3DS
            case MAP_ASCII8SRM32:
                if (fwrite(SRAMData32[Slot][0], 1, 0x2000, F) != 0x2000) SaveSRAM[Slot] = 0;
                if (fwrite(SRAMData32[Slot][1], 1, 0x2000, F) != 0x2000) SaveSRAM[Slot] = 0;
                if (fwrite(SRAMData32[Slot][2], 1, 0x2000, F) != 0x2000) SaveSRAM[Slot] = 0;
                if (fwrite(SRAMData32[Slot][3], 1, 0x2000, F) != 0x2000) SaveSRAM[Slot] = 0;
                break;
            case MAP_ASCII16_2:
                if (fwrite(SRAMData[Slot], 1, 0x0800, F) != 0x0800) SaveSRAM[Slot] = 0;
                break;
            case MAP_Wizardry:
            case MAP_ZeminaDS2:
                if (fwrite(SRAMData[Slot], 1, 0x2000, F) != 0x2000) SaveSRAM[Slot] = 0;
                break;
            case MAP_MANBOW2:
            case MAP_ESESCC:
            case MAP_MEGASCSI:
                if (fwrite(ExternalRAMData[Slot], 1, 0x80000, F) != 0x80000)SaveSRAM[Slot] = 0;
                break;
#endif // _3DS

            }

            /* Done with .SAV file */
            fclose(F);
        }

        /* Done saving SRAM */
        PRINTRESULT(SaveSRAM[Slot]);
#ifdef _3DS
        FreeMemory(SRAMName[Slot]);
#endif // _3DS

    }

    /* If ejecting cartridge... */
    if (!FileName)
    {
        if (ROMData[Slot])
        {
#ifdef _3DS
            CartSpecial[Slot] = 0;
            HasSpecialCart = 0;
            AllowSCC[Slot] = 1;
#endif // _3DS

            /* Free memory if present */
            FreeMemory(ROMData[Slot]);
            ROMData[Slot] = 0;
            ROMMask[Slot] = 0;
            /* Set memory map to dummy RAM */
            for (C1 = 0; C1 < 8; ++C1) MemMap[PS][SS][C1] = EmptyRAM;
#ifdef _3DS
            if (!NeedRest)return(0);
#endif // _3DS

            /* Restart MSX */
            ResetMSX(Mode, RAMPages, VRAMPages);
            /* Cartridge ejected */
            if (Verbose) printf("Ejected cartridge from slot %c\n", Slot + 'A');
        }

        /* Nothing else to do */
        return(0);
    }

#ifdef _3DS
    if (strcasecmp(FileName, "SCCPLUS") == 0)LoadSCCPLUS(Slot);
    else if (strcasecmp(FileName, "esescc512A") == 0)LoadESESCC(Slot, 1);
    else if (strcasecmp(FileName, "eseram512A") == 0)LoadESERAM(Slot, 1);
    /* Try opening file */
    if (!(F = zipfopen(FileName, "rb"))) return(0);
#else
    /* Try opening file */
    if (!(F = fopen(FileName, "rb"))) return(0);
#endif // _3DS
    if (Verbose) printf("Found %s:\n", FileName);

    /* Determine size via ftell() or by reading entire [GZIPped] stream */
    if (!fseek(F, 0, SEEK_END)) Len = ftell(F);
    else
    {
        /* Read file in 16kB increments */
        for (Len = 0; (C2 = fread(EmptyRAM, 1, 0x4000, F)) == 0x4000; Len += C2);
        if (C2 > 0) Len += C2;
        /* Clean up the EmptyRAM! */
        memset(EmptyRAM, NORAM, 0x4000);
    }

    /* Rewind file */
    rewind(F);

#ifdef _3DS
    OldLen = Len;
#endif // _3DS

    /* Length in 8kB pages */
    Len = Len >> 13;

#ifdef _3DS
    /*  Support ROMs that has small size or invalid size.(Some homebrew ROMs has that).  */
    AddtionalSize = OldLen - (Len << 13);
    if (AddtionalSize > 0)Len++;

    ROMLen[Slot] = Len;
#endif // _3DS


    /* Calculate 2^n closest to number of 8kB pages */
    for (Pages = 1; Pages < Len; Pages <<= 1);

    /* Check "AB" signature in a file */
    ROM64 = 0;
    C1 = fgetc(F);
    C2 = fgetc(F);

    /* Maybe this is a flat 64kB ROM? */
    if ((C1 != 'A') || (C2 != 'B'))
        if (fseek(F, 0x4000, SEEK_SET) >= 0)
        {
            C1 = fgetc(F);
            C2 = fgetc(F);
            ROM64 = (C1 == 'A') && (C2 == 'B');
        }

    /* Maybe it is the last 16kB page that contains "AB" signature? */
    if ((Len >= 2) && ((C1 != 'A') || (C2 != 'B')))
        if (fseek(F, 0x2000 * (Len - 2), SEEK_SET) >= 0)
        {
            C1 = fgetc(F);
            C2 = fgetc(F);
        }

    /* If we can't find "AB" signature, drop out */
    if ((C1 != 'A') || (C2 != 'B'))
    {
        if (Verbose) puts("  Not a valid cartridge ROM");
        fclose(F);
        return(0);
    }

    if (Verbose) printf("  Cartridge %c: ", 'A' + Slot);

    /* Done with the file */
    fclose(F);

    /* Show ROM type and size */
    if (Verbose)
        printf
        (
            "%dkB %s ROM..", Len * 8,
            ROM64 || (Len <= 0x8000) ? "NORMAL" : Type >= MAP_GUESS ? "UNKNOWN" : ROMNames[Type]
            );

    /* Assign ROMMask for MegaROMs */
    ROMMask[Slot] = !ROM64 && (Len > 4) ? (Pages - 1) : 0x00;
    //#ifdef _3DS
    //   FreeMemory(ROMData[Slot]);
    //#endif // _3DS

        /* Allocate space for the ROM */
    ROMData[Slot] = P = GetMemory(Pages << 13);
    if (!P) { PRINTFAILED; return(0); }

    /* Try loading ROM */
#ifdef _3DS
    if (AddtionalSize > 0)
    {
        memset(P, NORAM, Len << 13);
        if (!LoadROM(FileName, OldLen, P)) { PRINTFAILED; return(0); }
    }
    else
#endif // _3DS
        if (!LoadROM(FileName, Len << 13, P)) { PRINTFAILED; return(0); }

    /* Mirror ROM if it is smaller than 2^n pages */
    if (Len < Pages)
        memcpy(P + Len * 0x2000, P + (Len - Pages / 2) * 0x2000, (Pages - Len) * 0x2000);

    /* Detect ROMs containing BASIC code */
    BASIC = (P[0] == 'A') && (P[1] == 'B') && !(P[2] || P[3]) && (P[8] || P[9]);

    /* Set memory map depending on the ROM size */
    switch (Len)
    {
    case 1:
        /* 8kB ROMs are mirrored 8 times: 0:0:0:0:0:0:0:0 */
        if (!BASIC)
        {
            MemMap[PS][SS][0] = P;
            MemMap[PS][SS][1] = P;
            MemMap[PS][SS][2] = P;
            MemMap[PS][SS][3] = P;
        }
#ifdef _3DS
        else
        {
            MemMap[PS][SS][0] = EmptyRAM;
            MemMap[PS][SS][1] = EmptyRAM;
            MemMap[PS][SS][2] = EmptyRAM;
            MemMap[PS][SS][3] = EmptyRAM;
        }
#endif // _3DS

        MemMap[PS][SS][4] = P;
        MemMap[PS][SS][5] = P;
        if (!BASIC)
        {
            MemMap[PS][SS][6] = P;
            MemMap[PS][SS][7] = P;
        }
#ifdef _3DS
        MemMap[PS][SS][6] = EmptyRAM;
        MemMap[PS][SS][7] = EmptyRAM;
#endif // _3DS

        break;

    case 2:
        /* 16kB ROMs are mirrored 4 times: 0:1:0:1:0:1:0:1 */
        if (!BASIC)
        {
            MemMap[PS][SS][0] = P;
            MemMap[PS][SS][1] = P + 0x2000;
            MemMap[PS][SS][2] = P;
            MemMap[PS][SS][3] = P + 0x2000;
        }
#ifdef _3DS
        else
        {
            MemMap[PS][SS][0] = EmptyRAM;
            MemMap[PS][SS][1] = EmptyRAM;
            MemMap[PS][SS][2] = EmptyRAM;
            MemMap[PS][SS][3] = EmptyRAM;
        }
#endif // _3DS

        MemMap[PS][SS][4] = P;
        MemMap[PS][SS][5] = P + 0x2000;
        if (!BASIC)
        {
            MemMap[PS][SS][6] = P;
            MemMap[PS][SS][7] = P + 0x2000;
        }
#ifdef _3DS
        else
        {
            MemMap[PS][SS][6] = EmptyRAM;
            MemMap[PS][SS][7] = EmptyRAM;
        }
#endif // _3DS

        break;

    case 3:
    case 4:
        /* 24kB and 32kB ROMs are mirrored twice: 0:1:0:1:2:3:2:3 */
        MemMap[PS][SS][0] = P;
        MemMap[PS][SS][1] = P + 0x2000;
        MemMap[PS][SS][2] = P;
        MemMap[PS][SS][3] = P + 0x2000;
        MemMap[PS][SS][4] = P + 0x4000;
        MemMap[PS][SS][5] = P + 0x6000;
        MemMap[PS][SS][6] = P + 0x4000;
        MemMap[PS][SS][7] = P + 0x6000;
        break;

#ifdef _3DS
    case 6:
        Type = MAP_PLAIN;
        if (Slot < MAXCARTS)OldROMType[Slot] = Type;
        if ((P[0] == 'A') && (P[1] == 'B') && (P[0x4000] != 'A') && (P[0x4001] != 'B'))
        {
            MemMap[PS][SS][0] = P;
            MemMap[PS][SS][1] = P + 0x2000;
            MemMap[PS][SS][2] = P;
            MemMap[PS][SS][3] = P + 0x2000;
            MemMap[PS][SS][4] = P + 0x4000;
            MemMap[PS][SS][5] = P + 0x6000;
            MemMap[PS][SS][6] = P + 0x8000;
            MemMap[PS][SS][7] = P + 0xA000;
        }
        else
        {
            MemMap[PS][SS][0] = P;
            MemMap[PS][SS][1] = P + 0x2000;
            MemMap[PS][SS][2] = P + 0x4000;
            MemMap[PS][SS][3] = P + 0x6000;
            MemMap[PS][SS][4] = P + 0x8000;
            MemMap[PS][SS][5] = P + 0xA000;
            MemMap[PS][SS][6] = P;
            MemMap[PS][SS][7] = P + 0x2000;
        }
        break;
#endif // _3DS

    default:
        if (ROM64)
        {
            /* 64kB ROMs are loaded to fill slot: 0:1:2:3:4:5:6:7 */
            MemMap[PS][SS][0] = P;
            MemMap[PS][SS][1] = P + 0x2000;
            MemMap[PS][SS][2] = P + 0x4000;
            MemMap[PS][SS][3] = P + 0x6000;
            MemMap[PS][SS][4] = P + 0x8000;
            MemMap[PS][SS][5] = P + 0xA000;
            MemMap[PS][SS][6] = P + 0xC000;
            MemMap[PS][SS][7] = P + 0xE000;
        }
        break;
    }

    /* Show starting address */
    if (Verbose)
        printf
        (
            "starts at %04Xh..",
            MemMap[PS][SS][2][2] + 256 * MemMap[PS][SS][2][3]
            );

    /* Guess MegaROM mapper type if not given */
    if ((Type >= MAP_GUESS) && (ROMMask[Slot] + 1 > 4))
    {
        Type = GuessROM(P, Len << 13);
        if (Verbose) printf("guessed %s..", ROMNames[Type]);
#ifdef _3DS
        //Ugly fix for fMSX3DS that cann't use SETROMTYPE(N,T) macro because it has more than 15(0x0F) ROM Mappers.
        if (Slot < MAXCARTS)OldROMType[Slot] = Type;
#else
        if (Slot < MAXCARTS) SETROMTYPE(Slot, Type);
#endif // _3DS
    }

    /* Save MegaROM type */
    ROMType[Slot] = Type;

#ifdef _3DS
    if (Slot == 0 || Slot == 1)CheckSpecialCart(P, Len << 13, Type, Slot);

    /* R-Type for MSX has bug that it miss internal FM BIOS. So, insert FM-PAC ROM in Slot2 to enable FM Sound. */
    /* http://www7b.biglobe.ne.jp/~leftyserve/delusion/del_rtyp.htm */
    if (CartSpecial[Slot] == CART_FMBUG && Use2413)LoadCart("FMPAC.ROM", 1, MAP_FMPAC);

    if (CartSpecial[Slot] == CART_SMALL)
    {
        Type = GuessROM(P, Len << 13);
        if (Slot < MAXCARTS)OldROMType[Slot] = Type;
        ROMType[Slot] = Type;
    }

    if (Type == MAP_FMPAC && !Use2413)
    {
        P[0x1C] = 0x20;
        P[0x1D] = 0x20;
        P[0x1E] = 0x20;
        P[0x1F] = 0x20;
    }
    if (Type == MAP_FMPAC && Slot == 6)
    {
        for (X = 0; X < 4; X++)
        {
            if (P[(X << 14) + 0x1C] == 'O' && P[(X << 14) + 0x1D] == 'P' && P[(X << 14) + 0x1E] == 'L' && P[(X << 14) + 0x1F] == 'L')
            {
                P[(X << 14) + 0x1C] = 0x20;
                P[(X << 14) + 0x1D] = 0x20;
                P[(X << 14) + 0x1E] = 0x20;
                P[(X << 14) + 0x1F] = 0x20;
            }
        }
    }

    if (Type == MAP_FMPAC && Slot != 0 && Slot != 1 && Slot != 6)
    {
        //FM PAC in Cartridge Slot must have "PAC2". else has "APRL".
        for (X = 0; X < 4; X++)
        {
            if (P[0x18] == 'P' && P[0x19] == 'A' && P[0x1A] == 'C' && P[0x1B] == '2')
            {
                P[0x18] = 'A';
                P[0x19] = 'P';
                P[0x1A] = 'R';
                P[0x1B] = 'L';
            }
            if (P[(X << 14) + 0x18] == 'P' && P[(X << 14) + 0x19] == 'A' && P[(X << 14) + 0x1A] == 'C' && P[(X << 14) + 0x1B] == '2')
            {
                P[(X << 14) + 0x18] = 'A';
                P[(X << 14) + 0x19] = 'P';
                P[(X << 14) + 0x1A] = 'R';
                P[(X << 14) + 0x1B] = 'L';
            }
        }
    }
    if (Type == MAP_SCCPLUS_2)
    {
        if (Slot >= MAXCARTS)return(0);
        /* Use ResizeMemory() insted of GetMemory() for large size data because of error in Old3DS.*/
        ExternalRAMData[Slot] = ResizeMemory(ExternalRAMData[Slot], 0x22000);
        //ExternalRAMData[Slot] = (byte*)malloc(0x22000);

        //FreeMemory(ExternalRAMData[Slot]);
        //ExternalRAMData[Slot] = GetMemory(0x22000);
        memset(ExternalRAMData[Slot], NORAM, 0x22000);
        memcpy(ExternalRAMData[Slot], P, Len << 13);

        MemMap[PS][SS][2] = ExternalRAMData[Slot];
        MemMap[PS][SS][3] = ExternalRAMData[Slot] + 0x2000;
        MemMap[PS][SS][4] = ExternalRAMData[Slot] + 0x4000;
        MemMap[PS][SS][5] = ExternalRAMData[Slot] + 0x6000;

        ROMMapper[Slot][0] = 0;
        ROMMapper[Slot][1] = 1;
        ROMMapper[Slot][2] = 2;
        ROMMapper[Slot][3] = 3;
        SCCMode[Slot] = 0x20;
        IsSCCRAM[Slot][0] = IsSCCRAM[Slot][1] = IsSCCRAM[Slot][2] = IsSCCRAM[Slot][3] = 0;
        IsMapped[Slot][0] = IsMapped[Slot][1] = IsMapped[Slot][2] = IsMapped[Slot][3] = 1;
        ROMMask[Slot] = 0x0F;
        //SCCEnhanced = 1;
    }
    else if (Type == MAP_ESESCC)
    {
        if (Slot >= MAXCARTS)return(0);
        /* Use ResizeMemory() insted of GetMemory() for large size data because of error in Old3DS.*/
        ExternalRAMData[Slot] = ResizeMemory(ExternalRAMData[Slot], 0x80000);

        memset(ExternalRAMData[Slot], NORAM, 0x80000);
        memcpy(ExternalRAMData[Slot], P, Len << 13);

        if (F = sramfopen("esescc512A.sram", "rb"))
        {
            fread(ExternalRAMData[Slot], 1, 0x80000, F);
            fclose(F);
        }

        MemMap[PS][SS][2] = ExternalRAMData[Slot];
        MemMap[PS][SS][3] = ExternalRAMData[Slot] + 0x2000;
        MemMap[PS][SS][4] = ExternalRAMData[Slot] + 0x4000;
        MemMap[PS][SS][5] = ExternalRAMData[Slot] + 0x6000;

        ROMMapper[Slot][0] = 0;
        ROMMapper[Slot][1] = 1;
        ROMMapper[Slot][2] = 2;
        ROMMapper[Slot][3] = 3;
        SCCMode[Slot] = 0x20;
        ROMMask[Slot] = 0x3F;
    }
    else if (Type == MAP_MEGASCSI)
    {
        if (Slot >= MAXCARTS)return(0);
        /* Use ResizeMemory() insted of GetMemory() for large size data because of error in Old3DS.*/
        ExternalRAMData[Slot] = ResizeMemory(ExternalRAMData[Slot], 0x80000);

        memset(ExternalRAMData[Slot], NORAM, 0x80000);
        memcpy(ExternalRAMData[Slot], P, Len << 13);

        if (F = sramfopen("eseram512A.sram", "rb"))
        {
            fread(ExternalRAMData[Slot], 1, 0x80000, F);
            fclose(F);
        }

        MemMap[PS][SS][2] = ExternalRAMData[Slot];
        MemMap[PS][SS][3] = ExternalRAMData[Slot] + 0x2000;
        MemMap[PS][SS][4] = ExternalRAMData[Slot] + 0x4000;
        MemMap[PS][SS][5] = ExternalRAMData[Slot] + 0x6000;

        ROMMapper[Slot][0] = 0;
        ROMMapper[Slot][1] = 0;
        ROMMapper[Slot][2] = 0;
        ROMMapper[Slot][3] = 0;
        ROMMask[Slot] = 0x3F;
    }
    else if (Type == MAP_DOOLY)
    {
        /* Use external RAM for decript encoded ROM. Real cartridge doesn't have this feature, but fMSX3DS use this for speed and safety. */
        //ExternalRAMData[Slot] = (byte*)malloc(0x8000);
        ExternalRAMData[Slot] = ResizeMemory(ExternalRAMData[Slot], 0x8000);
        memset(ExternalRAMData[Slot], NORAM, 0x8000);
        byte Val;
        for (X = 0; X < 0x8000; X++)
        {
            Val = P[X];
            ExternalRAMData[Slot][X] = (Val & 0xF8) | (Val >> 2 & 0x01) | (Val << 1 & 0x06);
        }
        RAM[2] = MemMap[PS][SS][2];
        RAM[3] = MemMap[PS][SS][3];
        RAM[4] = MemMap[PS][SS][4];
        RAM[5] = MemMap[PS][SS][5];

        ROMMapper[Slot][0] = 0;
        ROMMapper[Slot][1] = 1;
        ROMMapper[Slot][2] = 2;
        ROMMapper[Slot][3] = 3;

        ROMMask[Slot] = 3;
    }
    else if (Type == MAP_SYNTHE)
    {
        RAM[2] = MemMap[PS][SS][2];
        RAM[3] = MemMap[PS][SS][3];
        RAM[4] = MemMap[PS][SS][4];
        RAM[5] = MemMap[PS][SS][5];

        ROMMapper[Slot][0] = 0;
        ROMMapper[Slot][1] = 1;
        ROMMapper[Slot][2] = 2;
        ROMMapper[Slot][3] = 3;

        ROMMask[Slot] = 3;
    }

    switch (Type)
    {
    case MAP_ASCII8:
    case MAP_Wizardry:
    case MAP_ZeminaDS2:
        SetMegaROM(Slot, 0, 0, 0, 0);
        break;
    case MAP_ASCII16:
    case MAP_ASCII16_2:
    case MAP_ASCII16Big:
    case MAP_MSX90:
        SetMegaROM(Slot, 0, 1, 0, 1);
        break;
    case MAP_RType:
        SetMegaROM(Slot, 0x2E, 0x2F, 0, 1);
        break;
    case MAP_LodeRunner:
        SetMegaROM(Slot, 0, 1, 0, 1);
        break;
    case MAP_MANBOW2:
        /* Use ResizeMemor() insted of GetMemory() for large size data because of error in Old3DS.*/
        ExternalRAMData[Slot] = ResizeMemory(ExternalRAMData[Slot], 0x80000);
        //ExternalRAMData[Slot] = (byte*)malloc(0x80000);
        //FreeMemory(ExternalRAMData[Slot]);
        //ExternalRAMData[Slot] = GetMemory(0x80000);
        memset(ExternalRAMData[Slot], NORAM, 0x80000);
        break;
    default:
        break;
    }

    SCCEnhanced = Type == MAP_SCCPLUS ? 1 : 0;

    if ((Slot == 0) || (Slot == 1))
    {
        if ((Type == MAP_KONAMI5) || (Type == MAP_SCCPLUS) || (Type == MAP_SCCPLUS_2) || (Type == MAP_ESESCC) || (Type == MAP_WingWarr)
            || (!ROMData[Slot]))
        {
            AllowSCC[Slot] = 1;
        }
        else AllowSCC[Slot] = 0;

        if ((!ROMData[0]) && (AllowSCC[1]))AllowSCC[0] = 0;
        if ((!ROMData[1]) && (AllowSCC[0]))AllowSCC[1] = 0;
    }
#endif // _3DS

    /* For Generic/16kB carts, set ROM pages as 0:1:N-2:N-1 */
    if ((Type == MAP_GEN16) && (ROMMask[Slot] + 1 > 4))
        SetMegaROM(Slot, 0, 1, ROMMask[Slot] - 1, ROMMask[Slot]);

    /* If cartridge may need a SRAM... */
    if (MAP_SRAM(Type))
    {
        /* Free previous SRAM resources */
        FreeMemory(SRAMData[Slot]);
        FreeMemory(SRAMName[Slot]);

        /* Get SRAM memory */
#ifdef _3DS
        FreeMemory(SRAMData32[Slot][0]);
        FreeMemory(SRAMData32[Slot][1]);
        FreeMemory(SRAMData32[Slot][2]);
        FreeMemory(SRAMData32[Slot][3]);
        //FreeMemory(MegaFlashData);

        SRAMData[Slot] = GetMemory(0x4000);
        SRAMData32[Slot][0] = GetMemory(0x2000);
        SRAMData32[Slot][1] = GetMemory(0x2000);
        SRAMData32[Slot][2] = GetMemory(0x2000);
        SRAMData32[Slot][3] = GetMemory(0x2000);
        //MegaFlashData = GetMemory(0x80000);
#else
        SRAMData[Slot] = GetMemory(0x4000);
#endif // _3DS
        if (!SRAMData[Slot])
        {
            if (Verbose) printf("scratch SRAM..");
            SRAMData[Slot] = EmptyRAM;
        }
        else
        {
            if (Verbose) printf("got 16kB SRAM..");
            memset(SRAMData[Slot], NORAM, 0x4000);
        }

#ifdef _3DS
        for (int i = 0; i < 4; i++)
        {
            if (!SRAMData32[Slot][i])SRAMData32[Slot][i] = EmptyRAM;
            else memset(SRAMData32[Slot][i], NORAM, 0x2000);
        }
#endif // _3DS


        /* Generate SRAM file name and load SRAM contents */
        if ((SRAMName[Slot] = (char*)GetMemory(strlen(FileName) + 5)))
        {
            /* Compose SRAM file name */
            strcpy(SRAMName[Slot], FileName);
            T = strrchr(SRAMName[Slot], '.');
            if (T) strcpy(T, ".sav"); else strcat(SRAMName[Slot], ".sav");
#ifdef _3DS
            /* Try opening file... */
            if ((F = sramfopen(SRAMName[Slot], "rb")))
            {
                /* Read SRAM file */
                if (Type == MAP_ASCII8SRM32)
                {
                    Len = fread(SRAMData32[Slot][0], 1, 0x2000, F);
                    Len += fread(SRAMData32[Slot][1], 1, 0x2000, F);
                    Len += fread(SRAMData32[Slot][2], 1, 0x2000, F);
                    Len += fread(SRAMData32[Slot][3], 1, 0x2000, F);
                }
                else if (Type == MAP_MANBOW2)
                {
                    Len = fread(ExternalRAMData[Slot], 1, 0x80000, F);
                }
                else Len = fread(SRAMData[Slot], 1, 0x4000, F);
#else
            if ((F = fopen(SRAMName[Slot], "rb")))
            {
                Len = fread(SRAMData[Slot], 1, 0x4000, F);
#endif // _3DS
                fclose(F);
                /* Print information if needed */
                if (Verbose) printf("loaded %d bytes from %s..", Len, SRAMName[Slot]);
                /* Mirror data according to the mapper type */
                P = SRAMData[Slot];
                switch (Type)
                {
                case MAP_FMPAC:
                    memset(P + 0x2000, NORAM, 0x2000);
                    P[0x1FFE] = FMPAC_MAGIC & 0xFF;
                    P[0x1FFF] = FMPAC_MAGIC >> 8;
                    break;
                case MAP_GMASTER2:
                    memcpy(P + 0x2000, P + 0x1000, 0x1000);
                    memcpy(P + 0x3000, P + 0x1000, 0x1000);
                    memcpy(P + 0x1000, P, 0x1000);
                    break;
                case MAP_ASCII16:
#ifdef _3DS
                case MAP_ASCII16Big:
#endif // _3DS

                    memcpy(P + 0x0800, P, 0x0800);
                    memcpy(P + 0x1000, P, 0x0800);
                    memcpy(P + 0x1800, P, 0x0800);
                    memcpy(P + 0x2000, P, 0x0800);
                    memcpy(P + 0x2800, P, 0x0800);
                    memcpy(P + 0x3000, P, 0x0800);
                    memcpy(P + 0x3800, P, 0x0800);
                    break;
                }
            }
            }
        }
#ifdef _3DS
    if (!NeedRest)return(Pages);
    IsHardReset = 1;
    if (CartSpecial[0] == CART_SOFT_RESET || CartSpecial[1] == CART_SOFT_RESET)IsHardReset = 0;
    //IsHardReset = 0;
#endif // _3DS
#ifdef TURBO_R
    if (MODEL(MSX_MSXTR))
    {
        if (CartSpecial[0] == CART_NEED_CBIOS || CartSpecial[1] == CART_NEED_CBIOS || CartSpecial[0] == CART_SLOTBUG || CartSpecial[1] == CART_SLOTBUG)
        {
            ShowMessage3DS("This ROM doesn't work on MSXTurboR.", "Chnage MSX Hardware Model and try again.");
        }
    }
#endif // TURBO_R


    /* Done setting up cartridge */
    ResetMSX(Mode, RAMPages, VRAMPages);
    PRINTOK;

    /* If first used user slot, try loading state */
    if (!Slot || ((Slot == 1) && !ROMData[0])) FindState(FileName);

    /* Done loading cartridge */
    return(Pages);
    }

/** LoadCHT() ************************************************/
/** Load cheats from .CHT file. Cheat format is either      **/
/** 00XXXXXX-XX (one byte) or 00XXXXXX-XXXX (two bytes) for **/
/** ROM-based cheats and XXXX-XX or XXXX-XXXX for RAM-based **/
/** cheats. Returns the number of cheats on success, 0 on   **/
/** failure.                                                **/
/*************************************************************/
int LoadCHT(const char* Name)
{
    char Buf[256], S[16];
    int Status;
    FILE* F;

    /* Open .CHT text file with cheats */
    F = fopen(Name, "rb");
    if (!F) return(0);

    /* Switch cheats off for now and remove all present cheats */
    Status = Cheats(CHTS_QUERY);
    Cheats(CHTS_OFF);
    ResetCheats();

    /* Try adding cheats loaded from file */
    while (!feof(F))
        if (fgets(Buf, sizeof(Buf), F) && (sscanf(Buf, "%13s", S) == 1))
            AddCheat(S);

    /* Done with the file */
    fclose(F);

    /* Turn cheats back on, if they were on */
    Cheats(Status);

    /* Done */
    return(CheatCount);
}

/** LoadPAL() ************************************************/
/** Load new palette from .PAL file. Returns number of      **/
/** loaded colors on success, 0 on failure.                 **/
/*************************************************************/
int LoadPAL(const char* Name)
{
    static const char* Hex = "0123456789ABCDEF";
    char S[256], * P, * T, * H;
    FILE* F;
    int J, I;

    if (!(F = fopen(Name, "rb"))) return(0);

    for (J = 0; (J < 16) && fgets(S, sizeof(S), F); ++J)
    {
        /* Skip white space and optional '#' character */
        for (P = S; *P && (*P <= ' '); ++P);
        if (*P == '#') ++P;
        /* Parse six hexadecimal digits */
        for (T = P, I = 0; *T && (H = strchr(Hex, toupper(*T))); ++T) I = (I << 4) + (H - Hex);
        /* If we have got six digits, parse and set color */
        if (T - P == 6) SetColor(J, I >> 16, (I >> 8) & 0xFF, I & 0xFF);
    }

    fclose(F);
    return(J);
}

/** LoadMCF() ************************************************/
/** Load cheats from .MCF file. Returns number of loaded    **/
/** cheat entries or 0 on failure.                          **/
/*************************************************************/
int LoadMCF(const char* Name)
{
    MCFCount = LoadFileMCF(Name, MCFEntries, sizeof(MCFEntries) / sizeof(MCFEntries[0]));
    return(MCFCount);
}

/** SaveMCF() ************************************************/
/** Save cheats to .MCF file. Returns number of loaded      **/
/** cheat entries or 0 on failure.                          **/
/*************************************************************/
int SaveMCF(const char* Name)
{
    return(MCFCount > 0 ? SaveFileMCF(Name, MCFEntries, MCFCount) : 0);
}

/** State.h **************************************************/
/** SaveState(), LoadState(), SaveSTA(), and LoadSTA()      **/
/** functions are implemented here.                         **/
/*************************************************************/
#include "State.h"

#if defined(ZLIB) || defined(ANDROID)
#undef fopen
#undef fclose
#undef fread
#undef fwrite
#undef fgets
#undef fseek
#undef ftell
#undef fgetc
#undef feof
#endif
