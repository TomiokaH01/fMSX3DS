#ifdef __cplusplus
extern "C" {
#endif

#define SND_CHANNELS    16     /* Number of sound channels   */
#define SND_BITS        8
#define SND_BUFSIZE     (1<<SND_BITS)

//#include <string.h>
#include <3ds.h>
#include "3DSConfig.h"
//#include <citro3d.h>
extern touchPosition oldtp;
extern touchPosition tp;

extern unsigned char IsShowFPS;
extern bool IsFrameLimit;
extern unsigned char IsHardReset;
extern unsigned char SkipBIOS;
extern unsigned char UseInterlace;
extern bool IsDoubleBuffer;
extern int currJoyMode[2];
extern int JoyMode[2];
extern int PrinterMode;
extern int regionid;
extern int cbiosReg;
extern unsigned char ForceCBIOS;
extern unsigned char ForceJPBIOS;
extern unsigned char ReloadBIOS;
extern unsigned char NeedRest;
extern int playYM2413;
extern int playY8950;
extern unsigned char playSCC;
extern float FactorPSG;
extern float FactorSCC;
extern float Factor2413;
extern float Factor8950;
extern float FactorPCM;
extern float MasterVol;
extern int DAVal;
extern int DA1bit;
extern int DA8bit;
extern unsigned char IsSndRegUpd;
extern int MouseDX3DS[2];
extern int MouseDY3DS[2];
extern int CartSpecial[2];
extern int HasSpecialCart;
extern int ScreenShotOffx;
extern int ScreenShotOffy;
extern unsigned char IsStartLoadFile;
extern unsigned char IsJpKey;
extern unsigned char KeyRegion;
extern unsigned char IsJPKeyBIOS;
extern unsigned char SCCEnhanced;
extern unsigned char ForceSCCP;
extern unsigned char IsSpriteColi;
extern unsigned char AccurateAudioSync;
extern unsigned char waitStep;
extern int currScanLine;
extern int audioCnt;
extern int TextDelay;
extern int IPSPatchSize;
extern unsigned char* IPSPatchBuf;
extern unsigned char ScreenFilter;
extern unsigned char TurboNow;
extern int fpsval;
extern int zipMessage;

#ifdef USE_3D
extern unsigned char OldStereo3DMode;
extern unsigned char Stereo3DMode;
extern unsigned char OldIs3DNow;
extern unsigned char Is3DNow;
#endif // USE_3D

#ifdef TURBO_R
extern unsigned char MSXDOS2Mapper;
#endif // TURBO_R

#ifdef _MSX0
extern unsigned char UseMSX0;
extern unsigned char LoadXBASIC;
extern unsigned char MSX0_I2CA;
extern unsigned char MSX0_ANALOGOUT;
//extern unsigned char MSX0_GPIO;
//extern unsigned char MSX0_UART;
#endif // _MSX0

#ifdef HFE_DISK
unsigned char loadHFE_File(int slotid, const char* filename, unsigned char isSavedDisk);
unsigned short GetHfeShiftedVal(unsigned char val0, unsigned char val1, unsigned char val2, unsigned char val3, int shiftv);
unsigned char GetHfeV3Val(unsigned char* Buf, int index, int shift, int skip);
#endif // HFE_DISK

#ifdef VDP_V9990
extern unsigned char UseV9990;
extern unsigned char V9990Active;
extern unsigned char V9990Dual;
extern unsigned char V9990DualScreen;
extern int V9KcurrLine;
#endif // VDP_V9990

#ifdef USE_OVERCLOCK
extern unsigned char overClockRatio;
#endif // USE_OVERCLOCK

#ifdef HDD_NEXTOR
#define HDD_DETECT_SIZE		0x100000
extern unsigned char IsHardDisk;
#endif // HDD_NEXTOR


#ifdef AUDIO_SYNC
extern int audioCycleCnt;
#endif // AUDIO_SYNC


#define CART_NONE 0			/* No Specila Cartridge */
#define CART_ARKANOID	1	/* Arkanoid(Support Arkanoid Paddle), Arkanoid2(Support Arkanoid Paddle) */
#define CART_LODERUNNER 2	/* Super Lode Runner. Write to RAM 0000h */
#define CART_ZEMINADS2	3	/* Zemina bootleg games with SRAM(Dragon Slayer2 Xanadu etc.)  I/O 0Fh */
#define CART_MSX90		4	/* Zemina 90in1. I/O 77h */
#define CART_YAKSA		5	/* Yaksa (Wolf Team) */
#define CART_MEGASCC	6	/* Mega Frash ROM SCC, MANBOW2 Secondary PSG I/O 10h,11h,12h */
#define CART_FMBUG		7	/* Cart that Has FM BIOS detect bug. R-Type etc. */
#define CART_SMALL		8	/* Small size MegaROM. Baby Dinosaur Dooly etc.*/
#define	CART_READSCC	9	/* Read from SCC register. Wing Warrior(2021 ver.) etc. */
#define CART_SOFT_RESET 10	/* Need software reset on MSX2+. Zombie Hunter etc. */
#define CART_NEED_CBIOS	11	/* Need C-BIOS to wrok on MSX2+. */
#define CART_SLOTBUG	12	/* Does'nt work on MSX with Slot Expanded. */
#define CART_MEGASCSI	13	/* MEGA-SCSI cartridge for Hard Disk etc. */
#define CART_ARABIC		14	/* Need Arabic BIOS ROM. */

	
#define CBIOS_BR	0	/* C-BIOS Brazil */
#define	CBIOS_EU	1	/* C-BIOS European */
#define CBIOS_JP	2	/* C-BIOS Japanese */
#define CBIOS_US	3	/* C-BIOS United States */

#define STEREO3D_NONE		0	/* No Stereoscopic 3D*/
#define STEREO3D_ANAGLYPH	1	/* Anaglyph  */
#define STEREO3D_ANA_COLOR	2	/* Anaglyph(Color) */

#ifdef ALTSOUND
extern int Use2413;
extern int Use8950;
extern bool IsPSGHQ;
extern  bool IsSCCHQ;
extern bool Is2413HQ;
extern unsigned char PSGType;	/* 0:YM2149  1:AY-3-8910 */
extern unsigned char SoundChannels;		/* 0:monaural 1:stereo */
extern unsigned char UseDCFilter;
extern unsigned char UseRCFilter;
extern unsigned char UseFIRFilter;
extern unsigned char SoundSampRate;
extern unsigned char Use8950TurboR;
extern unsigned char ReadSCCPlus;
#endif

#ifdef DEBUG_LOG
extern FILE* debugFile;
extern char* debugBuf;
extern size_t debugBufSize;
//#define printf printfToFile
#endif // DEBUG_LOG

#ifdef LOG_ERROR
extern const char* ErrorChar;
#endif // LOG_ERROR


#include <SDL/SDL.h>

void CalcAudio(void);
static inline short compressBuf(int32_t wav);
unsigned char CheckIsVoice(void);
void updatePrinter();
void ResetSound(void);
void Reset3DS(void);
#ifdef ALTPCM
void updatePCM();
#endif // ALTPCM
#ifdef _MSX0
unsigned short InMSX0IOT(void);
unsigned char ReadIOTGET(int val);
void OutMSX0IOT(unsigned char val);
void ResetIOTData(void);
void TrashMSX0(void);
#endif // _MSX0


/** InitAudio() **********************************************/
/** Initialize sound. Returns rate (Hz) on success, else 0. **/
/** Rate=0 to skip initialization (will be silent).         **/
/*************************************************************/
unsigned int InitAudio(unsigned int Rate, unsigned int Latency);

/** TrashAudio() *********************************************/
/** Free resources allocated by InitAudio().                **/
/*************************************************************/
void TrashAudio(void);

/** PauseAudio() *********************************************/
/** Pause/resume audio playback.                            **/
/*************************************************************/
int PauseAudio(int Switch);

void WriteOPLL(int R, int V);
void WriteAUDIO(int R, int V);
void Write2212(int R, int V);
int CheckSCC(int R);
void WritePSG(int R, int V);
int  ReadAUDIO(int R);
int  ReadPSG(int R);
int Read2212(int R);
int SCCEnabled(void);
void SetSCCEnhanced(int IsEnhance);

void OPLLChangeOption(bool IsEnable);
void OPLChangeOption(bool IsEnable);
void DCFilterChangeOption(bool IsEnable);
void RCFilterChangeOption(bool IsEnable);
void FIRFilterChangeOption(bool IsEnable);
static inline int16_t compressBuf(int32_t wav);
void EnableFMSound(int FMType);

void DoAutoSave();
void WriteOptionCFG();
void ReadOptionCFG();
void ChangeOptionMSXModel(const char* optionName, int val);
void WriteErrorLog();
#ifdef DEBUG_LOG
void printfToFile(const char* format, ...);
void writeDebugLog();
#endif // DEBUG_LOG
void LoadOption(bool IsInit);

void CalcFPS();
void DrawDiskLamp();
int WaitSync();
int WaitSyncLine();
int WaitSyncLineStep();
void CheckPALVideo();
void SetFirstLineTime();
void checkAutoFrameSkip();
int GetCurrScanLine();
void ErrorLogUpdate();
//void SyncAudio();
void SaveScrrenShot(const char* pathchr);
void StartMenu();
void EndMenu();
unsigned char* ResizeMemory(unsigned char* Buf, int Size);
FILE* zipfopen(const char* _name, const char* _mode);
FILE* zipfopenExtract(const char* _name, const char* _savepath, const char* _mode);
FILE* gzipfopenExtract(const char* _name, const char* _savepath, const char* _mode);
FILE* sramfopen(const char* _name, const char* _mode);
int CalcCRC32(void* Buf, const char* filePath, int Size);
void CheckSpecialCart(void* Buf, int Size, int Type, int Slot);
void LoadCartAtStart();
void InitXbuf();
void InitWbuf();
#ifdef VDP_V9990
void InitV9KXbuf();
#endif // VDP_V9990
#ifdef HDD_NEXTOR
void DoAutoSaveHDD();
void AutoSaveHDD();
#endif // HDD_NEXTOR
#ifdef SUPERIMPOSE
void InitScreenShotTexture(SDL_Surface* ssurface);
void ChangeScreenImposeTransparent(int alpha);
#endif // SUPERIMPOSE
#ifdef MSX0_OLED
void DrawOLED_Display(int pos, unsigned char val);
#endif // MSX0_OLED
#ifdef DEBUGGER_3DS
void DoPutImage();
#endif // DEBUGGER_3DS


void ShowMessage3DS(char* msg, char* msg2);
void DoReloadFMPAC();
int Debug_CalcBPalVlue(int R, int G, int B);
void Show_3DS_BreakPoint(const char* message);
void Show_3DS_BreakPointVal(int val);
void Show_3DS_BreakPointArg(const char* format, ...);
void  SetupWideScreen(bool isWide);
void WideScreenOn();
void WideScreenOff();
void SetScreenFilter(void);
#ifdef __cplusplus
}
#endif