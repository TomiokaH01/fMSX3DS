#include "MSX.h"
#include "EMULib.h"
#include "Sound.h"

#include <string.h>
//#include <string>
#include <stdio.h>
#include <3ds.h>
#include<stdlib.h>
#include <unordered_set>

#include <citro2d.h>

#include "3DSMenu.h"
#include "3DSLib.h"

extern "C"
{
#include <SDL/SDL.h>
#include "unzip.h"
#include <zlib.h>
}

#include <sstream>

#define WIDTH       272                   /* Buffer width    */
#define HEIGHT      228                   /* Buffer height   */
/* Press/Release keys in the background KeyState */
#define XKBD_SET(K) XKeyState[Keys[K][0]]&=~Keys[K][1]
#define XKBD_RES(K) XKeyState[Keys[K][0]]|=Keys[K][1]
//static volatile unsigned int JoyState = 0; /* Joystick state */
volatile unsigned int JoyState = 0; /* Joystick state */
static volatile unsigned int LastKey  = 0; /* Last key prsd  */
static volatile unsigned int KeyModes = 0; /* SHIFT/CTRL/ALT */
byte MousePos[2] = { 0,0 };
int ArkanoidPos = 238;

/* Combination of EFF_* bits */
int UseEffects  = EFF_SCALE|EFF_SAVECPU;

int InMenu;                /* 1: In MenuMSX(), ignore keys   */
int UseZoom     = 1;       /* Zoom factor (1=no zoom)        */
int UseSound = 44100;   /* Audio sampling frequency (Hz)  */
int SyncFreq    = 60;      /* Sync frequency (0=sync off)    */
int FastForward;           /* Fast-forwarded UPeriod backup  */
int SndSwitch;             /* Mask of enabled sound channels */
int SndVolume;             /* Master volume for audio        */
int OldScrMode;            /* fMSX "ScrMode" variable storage*/
#ifdef VDP_V9990
int OldV9KScrMode;
static unsigned int V9kXPal[80];
static unsigned short* V9KXBuf;
void PutImageSuperImpose(void);
#ifdef DUAL_SCREEN
void PutImageDualScreen(void);
#endif // DUAL_SCREEN
#endif // VDP_V9990


const char *Title     = "fMSX 6.0";       /* Program version */

//Image NormScreen;          /* Main screen image              */
//Image WideScreen;          /* Wide screen image              */
//static pixel *WBuf;        /* From Wide.h                    */
static unsigned short *WBuf;        /* From Wide.h                    */
static unsigned short *XBuf;        /* From Common.h                  */
//static char *XBuf;        /* From Common.h                  */
static unsigned int XPal[80];
static unsigned int BPal[256];
static unsigned int XPal0;
	
const char *Disks[2][MAXDISKS+1];         /* Disk names      */
volatile byte XKeyState[20]; /* Temporary KeyState array     */

C2D_Image ScreenImage;
C3D_Tex ScreenTex;
Tex3DS_SubTexture ScreenSubTex;
C2D_Image WideImage;
C3D_Tex WideTex;
Tex3DS_SubTexture WideSubTex;
extern C3D_RenderTarget *TopRenderTarget;
extern C3D_RenderTarget* WideRenderTarget;
extern C3D_RenderTarget* BottomRenderTartget;
unsigned char* zipBuf;

#ifdef USE_3D
C2D_Image ScreenImageR;
C3D_Tex ScreenTexR;
Tex3DS_SubTexture ScreenSubTexR;
extern C3D_RenderTarget* TopRenderTargetR;
#endif // USE_3D
#ifdef DUAL_SCREEN
C2D_Image ScreenImageBottom;
C3D_Tex	ScreenTexBottom;
Tex3DS_SubTexture ScreenSubTexBottom;
#endif DUAL_SCREEN
#ifdef SUPERIMPOSE
C2D_Image ScreenImageImpose;
C3D_Tex ScreenTexImpose;
Tex3DS_SubTexture ScreenSubTexImpose;
#endif // SUPERIMPOSE


//#define TopTexWidth  400
//#define TopTexHeight 240
#define TopTexWidth 512
#define TopTexHeight 256
#define WideTextWidth (TopTexWidth*2)

void HandleKeys(unsigned int Key);
void PutImage(void);
#ifdef USE_3D
void PutImage3D(void);
#endif // USE_3D
void checkRapidFire(int idx);
void SetWait(void);

static u64 NextTime;
static u64 NextLineTime;
static u64 NextAudioTime;
static u64 NextLineStepTime;
long dt;
static	u64	TimePerFrame;
static int WaitCount = 0;
static int WaitStepCnt = 0;
static u64 FirstLineTime;
static u64 LineTimePerFrame;
static u64 AudioTimePerFrame;
static u64 LineStepTimePerFrame;
int FPSDelay = 0;
int FPSNoDelay = 0;
int FrameSkipChangeCnt = 0;
int fpscount = 0;
u32 ticksarr[6] = { 0,0,0,0,0,0 };
unsigned int OldJ;
extern bool AllowWide;
extern bool IsWide;
extern bool IsOld2DS;
extern bool IsScreenShot;
#ifdef SUPERIMPOSE
extern bool IsImposeScreenShot;
extern float ImposeRefX;
extern float ImposeRefY;
extern float ImposeRefSize;
extern int ImposeTransP;
#endif // SUPERIMPOSE
extern bool ShowDebugMsg3DS;
extern bool IsSmallScrShot;
extern unsigned char ScreenRes;
extern bool AutoFrameSkip;
extern byte KeyMaps3DS[20];
extern u8 rapidCounts[10];
extern u8 rapidInterVals[10];
extern int ZipIndex;
#ifdef LOG_ERROR
extern std::vector<const char*> ErrorVec;
#endif // LOG_ERROR
byte OldPALVidedo = 0;
int OldJoyModes[2] = { 1,0 };
int OldX = 0, OldY = 0;
int PadDX = 0, PadDY = 0, PadButton = 0, PaddleVal = 0;
bool isShellOpen = true;

bool IsCapsReady = false;
bool IsKanaReady = false;
bool IsStart = true;

#ifdef HFE_DISK
#include <map>
static std::map<int, byte*> MultiSecMap;
static std::map<int, byte*> IllegalSecMap;
#endif // HFE_DISK


/** CommonMux.h **********************************************/
/** Display drivers for all possible screen depths.         **/
/*************************************************************/
//#include "CommonMux.h"
#include "CommonMux3DS.h"

#ifdef DEBUGGER_3DS
#include "CommonDebug3DS.h"
#endif // DEBUGGER_3DS


static const int KeyLayout1Line[] =
{
	0,32, KBD_STOP,32,63,KBD_F1,63,94,KBD_F2,94,123,KBD_F3,125,156,KBD_F4
	,156,187,KBD_F5,190,221,KBD_SELECT,221,254,KBD_INSERT,254,287,KBD_DELETE,287,320,KBD_HOME
};

static const int KeyLayout2Line[] =
{
	0,22,KBD_ESCAPE,22,43,'1',43,64,'2',64,85,'3',85,107,'4',107,128,'5'
	,128,149,'6',149,170,'7',170,192,'8',192,213,'9',213,234,'0',234,256,'-'
	,256,277,'+',277,298,0x5c,298,320,KBD_BS
};

static const int KeyLayout3Line[] =
{
	0,30,KBD_TAB,30,51,'q',51,72,'w',72,93,'e',93,115,'r'
	,115,136,'t',136,157,'y',157,179,'u',179,200,'i',200,222,'o',222,243,'p'
	,243,264,'{',264,285,'}',285,320,KBD_ENTER
};

static const int KeyLayout4Line[] =
{
	0,38,KBD_CONTROL,38,60,'a',60,81,'s',81,102,'d',102,124,'f',124,145,'g'
	,145,166,'h',166,187,'j',187,208,'k',208,230,'l',230,251,';'
	,251,273,'"',273,294,0x7E,294,320,KBD_ENTER
};

static const int KeyLayout5Line[] =
{
	0,45,KBD_SHIFT,45,67,'z',67,88,'x',88,109,'c',109,130,'v',130,152,'b',
	152,173,'n',173,194,'m',194,215,',',215,237,'.'
	,237,258,'/',258,280,'_',280,320,KBD_SHIFT
};

static const int KeyLayout6Line[] =
{
	0,45,KBD_CAPSLOCK,45,67,KBD_GRAPH,67,230,KBD_SPACE,230,252,KBD_COUNTRY
};

static const int KeyLayout7Line[] =
{
	182,202,KBD_NUMPAD7,203,223,KBD_NUMPAD8, 224,244,KBD_NUMPAD9, 245,265,KBD_NUMSLA
};

static const int KeyLayout8Line[] =
{
	182,202,KBD_NUMPAD4,203,223,KBD_NUMPAD5,224,244,KBD_NUMPAD6,245,265,KBD_NUMASTA
};

static const int KeyLayout9Line[] =
{
	182,202,KBD_NUMPAD1,203,223,KBD_NUMPAD2,224,244,KBD_NUMPAD3,245,265,KBD_NUMMINUS
};

static const int KeyLayout10Line[] =
{
	182,202,KBD_NUMPAD0,203,223,KBD_NUMDOT,224,244,KBD_NUMCOMMA,245,265,KBD_NUMPLUS
};

void SoftwareKeyboardCheck(bool flag0, int px, int py);

//void ReDrawKeyboard();

/** InitMachine() ********************************************/
/** Allocate resources needed by machine-dependent code.    **/
/*************************************************************/
int InitMachine(void)
{
  int J,I;

  /* Initialize variables */
  UseZoom         = UseZoom<1? 1:UseZoom>5? 5:UseZoom;
  InMenu          = 0;
  FastForward     = 0;
  OldScrMode      = 0;
#ifdef VDP_V9990
  OldV9KScrMode = 0;
#endif // VDP_V9990

  //NormScreen.Data = 0;
  //WideScreen.Data = 0;

  /* Set visual effects */
  //SetEffects(UseEffects);

  /* Create main image buffer */
  SetScreenDepth(16);
//#define BPP32
//#define  BPP16
  XBuf = (unsigned short*)malloc(WIDTH * HEIGHT * sizeof(unsigned short) * 2);
  WBuf = (unsigned short*)malloc(512 * HEIGHT * sizeof(unsigned short) * 2);

  /* Set all colors to black */
  for (J = 0; J < 80; J++) SetColor(J, 0, 0, 0);
	/* Reset the palette */
  //for (J = 0; J < 16; J++) XPal[J] = 0;
  //XPal0 = 0;

  /* Create SCREEN8 palette (GGGRRRBB) */
  for (J = 0; J < 256; J++)
  {
	  unsigned long LongR = (((unsigned long)J >> 2) & 0x07) * 255 / 7;
	  unsigned long LongG = (((unsigned long)J >> 5) & 0x07) * 255 / 7;
	  unsigned long LongB = ((unsigned long)J & 0x03) * 255 / 3;
	  BPal[J] = ((LongR / 8) << 11 | (LongG / 8) << 6 | (LongB / 8));
  }

  /* Initialize temporary keyboard array */
  memset((void*)XKeyState, 0xFF, sizeof(XKeyState));

  Init3DS();

  /* Attach keyboard handler */
  SetKeyHandler(HandleKeys);
  UseSound = SoundSampRate ? 44100 : 22050;
  //UseSound = 22050;
  //UseSound = 44100;
  /* Initialize sound */
  InitSound(UseSound,150);
  SndSwitch = (1 << MAXCHANNELS) - 1;
  SndVolume = 255 / MAXCHANNELS;
  SetChannels(SndVolume,SndSwitch);
  playYM2413 = 0;
  playY8950 = 0;
  DAVal = 0;
  DA1bit = 0;
  DA8bit = 0;

  //Init3DS();

  IsCapsReady = false;
  IsKanaReady = false;

  SetWait();
  switch (regionid)
  {
  case 1:
	  VDP[9] &= ~(0x02);
	  break;
  case 2:
	  VDP[9] |= 0x02;
	  break;
  default:
	  break;
  }

  //std::srand(time(NULL));

  C3D_TexInit(&ScreenTex, TopTexWidth, TopTexHeight, GPU_RGB565);
  ScreenSubTex.width = TopTexWidth;
  ScreenSubTex.height = TopTexHeight;
  ScreenSubTex.left = 0.0f;
  ScreenSubTex.right = 1.0f;
  ScreenSubTex.top = 1.0f;
  ScreenSubTex.bottom = 0.0f;

  ScreenImage.tex = &ScreenTex;
  ScreenImage.subtex = &ScreenSubTex;

  //C3D_TexSetFilter(&ScreenTex, GPU_LINEAR, GPU_NEAREST);
  //C3D_TexSetFilter(&ScreenTex, GPU_NEAREST, GPU_NEAREST);

#ifdef USE_3D
  C3D_TexInit(&ScreenTexR, TopTexWidth, TopTexHeight, GPU_RGB565);
  ScreenSubTexR.width = TopTexWidth;
  ScreenSubTexR.height = TopTexHeight;
  ScreenSubTexR.left = 0.0f;
  ScreenSubTexR.right = 1.0f;
  ScreenSubTexR.top = 1.0f;
  ScreenSubTexR.bottom = 0.0f;

  ScreenImageR.tex = &ScreenTexR;
  ScreenImageR.subtex = &ScreenSubTexR;
#endif // USE_3D

#ifdef DUAL_SCREEN
  C3D_TexInit(&ScreenTexBottom, TopTexWidth, TopTexHeight, GPU_RGB565);
  ScreenSubTexBottom.width = TopTexWidth;
  ScreenSubTexBottom.height = TopTexHeight;
  ScreenSubTexBottom.left = 0.0f;
  ScreenSubTexBottom.right = 1.0f;
  ScreenSubTexBottom.top = 1.0f;
  ScreenSubTexBottom.bottom = 0.0f;

  ScreenImageBottom.tex = &ScreenTexBottom;
  ScreenImageBottom.subtex = &ScreenSubTexBottom;
#endif // DUAL_SCREEN

  C3D_TexInit(&WideTex, WideTextWidth, TopTexHeight, GPU_RGB565);
  WideSubTex.width = WideTextWidth;
  WideSubTex.height = TopTexHeight;
  WideSubTex.left = 0.0f;
  WideSubTex.right = 1.0f;
  WideSubTex.top = 1.0f;
  WideSubTex.bottom = 0.0f;

  WideImage.tex = &WideTex;
  WideImage.subtex = &WideSubTex;

  //C3D_TexInit(&WideBufTex, WideTextWidth, TopTexHeight, GPU_RGB565);

  SetScreenFilter();

  C3D_CullFace(GPU_CULL_NONE);
  C3D_DepthTest(false, GPU_ALWAYS, GPU_WRITE_ALL);
  C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA, GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA);
  C3D_AlphaTest(false, GPU_ALWAYS, 0);
  C3D_BlendingColor(0);

  //printf("Init OK\n");

  /* Done */
  return(1);
}

/** TrashMachine() *******************************************/
/** Deallocate all resources taken by InitMachine().        **/
/*************************************************************/
void TrashMachine(void)
{

	//FreeImage(&NormScreen);
	TrashSound();

	Quit3DS();

	free(XBuf);
	free(WBuf);

#ifdef VDP_V9990
	free(V9KXBuf);
#endif // VDP_V9990

	//SDL_WaitThread(thread1, NULL);
}

/** PutImage() ***********************************************/
/** Put an image on the screen.                             **/
/*************************************************************/
void PutImage(void)
{
#ifdef VDP_V9990
	if (V9990Dual & 0x01)
	{
		PutImageSuperImpose();
		return;
	}
#ifdef DUAL_SCREEN
	if (V9990Dual & 0x02)
	{
		PutImageDualScreen();
		return;
	}
#endif // DUAL_SCREEN
#endif // VDP_V9990

#ifdef USE_3D
	if (Stereo3DMode>0)
	{
		if (osGet3DSliderState() > 0)
		{
			Is3DNow = 1;
			PutImage3D();
			return;
		}
	}
	Is3DNow = 0;
	if (OldIs3DNow)
	{
		OldIs3DNow = Is3DNow;
#ifdef VDP_V9990
		if (V9990Active)
		{
			if((V9KScrMode==1) || (V9KScrMode > 3))SetupWideScreen(true);
			else SetupWideScreen(false);
		}
		else
		{
			if ((ScrMode == 6) || ((ScrMode == 7) && !ModeYJK) || (ScrMode == MAXSCREEN + 1))SetupWideScreen(true);
			else SetupWideScreen(false);
		}
#else
		if ((ScrMode == 6) || ((ScrMode == 7) && !ModeYJK) || (ScrMode == MAXSCREEN + 1))SetupWideScreen(true);
		else SetupWideScreen(false);
#endif // VDP_V9990
	}
#endif // USE_3D

	if (AllowWide && !IsOld2DS)
	{
#ifdef VDP_V9990
		if (V9990Active)
		{
			if ((V9KScrMode != OldV9KScrMode))
			{
				OldV9KScrMode = V9KScrMode;
				if ((V9KScrMode == 1) || (V9KScrMode > 3))SetupWideScreen(true);
				else SetupWideScreen(false);
			}
		}
		else
#endif // VDP_V9990
		if (ScrMode != OldScrMode)
		{
			OldScrMode = ScrMode;
			/* Depending on the new screen width... */
			if ((ScrMode == 6) || ((ScrMode == 7) && !ModeYJK) || (ScrMode == MAXSCREEN + 1))SetupWideScreen(true);
			else SetupWideScreen(false);
		}
	}
	if (!IsWide)
	{
#ifdef INLINE_ASM
		/* Currently, only very very little speed up. */
		const int dwidth = WIDTH, dheight = HEIGHT;
		const int swidth = 512, sheight = 256;
		int i, j, vx0, vx1, v0;

		u32* srcbuf0;

		u32* scrbuf = (u32*)ScreenTex.data;
		u32* scrtexbf;
		for (j = 0; j < dheight; j += 4)
		{
			v0 = (j >> 3) * (swidth >> 3);
			vx1 = (j * dwidth) >> 1;
			srcbuf0 = (u32*)XBuf + vx1;
			i = 0;
			__asm__  __volatile__(
				"ldr r5, %[srcbuf0]\n"
				"ldr r6, %[i]\n"
				"loopX:\n"
				"add r0, %[v0], r6\n"
				"lsl r0, r0, #6\n"	//(v0+i)<<6

				"lsl r1, r6,#3\n"	//
				"and r1, r1 ,#4\n"	//
				"lsl r1, r1, #2\n"	// ((i<<3) & 0x04) << 2
				"add r0,r0,r1\n"

				"and r2, %[j], #4\n"
				"lsl r2, r2, #3\n"  //(j & 0x04) << 3
				"orr r0,r0,r2\n"

				"lsl  r0,  r0, #1\n"	//(vx0>>1)*4 equal vx0<<1
				"add %[scrtexbf], %[scrbuf], r0\n"
				"mov r4, r5\n"

				"ldm r4,{r0-r3} \n"
				"str r0,[%[scrtexbf],#0]\n"
				"str r1,[%[scrtexbf],#8]\n"
				"str r2,[%[scrtexbf],#32]\n"
				"str r3,[%[scrtexbf],#40]\n"
				"add r4, r4, #512\n"
				"add r4, r4, #32\n"

				"ldm r4,{r0-r3} \n"
				"str r0,[%[scrtexbf],#4]\n"
				"str r1,[%[scrtexbf],#12]\n"
				"str r2,[%[scrtexbf],#36]\n"
				"str r3,[%[scrtexbf],#44]\n"
				"add r4, r4, #512\n"
				"add r4, r4, #32\n"

				"ldm r4,{r0-r3} \n"
				"str r0,[%[scrtexbf],#16]\n"
				"str r1,[%[scrtexbf],#24]\n"
				"str r2,[%[scrtexbf],#48]\n"
				"str r3,[%[scrtexbf],#56]\n"
				"add r4, r4, #512\n"
				"add r4, r4, #32\n"

				"ldm r4,{r0-r3} \n"
				"str r0,[%[scrtexbf],#20]\n"
				"str r1,[%[scrtexbf],#28]\n"
				"str r2,[%[scrtexbf],#52]\n"
				"str r3,[%[scrtexbf],#60]\n"

				"add r5, r5 ,#16\n"
				//"subs r6, r6, #1\n"
				"add r6, r6, #1\n"
				"cmp r6,#34\n"
				"bne loopX\n"

				//: [v0]"+r"(v0), [i]"+r"(i), [j]"+r"(j)
				:
			: [srcbuf0] "m"(srcbuf0), [scrtexbf] "r"(scrtexbf), [scrbuf]"r"(scrbuf), [v0] "r"(v0), [i]"m"(i), [j]"r"(j)
				: "r0", "r1", "r2", "r3", "r4", "r5", "r6"
				);
		}
#else
		const int dwidth = WIDTH, dheight = HEIGHT;
		const int swidth = 512, sheight = 256;
		int i, j, vx0, vx1, v0;

		u32* srcbuf0, * srcbuf1, * srcbuf2, * srcbuf3;

		u32* scrbuf = (u32*)ScreenTex.data;
		u32* scrtexbf;
#ifdef VDP_V9990
		u32* XXBuf = V9990Active ? (u32*)V9KXBuf : (u32*)XBuf;
#endif // VDP_V9990

		for (j = 0; j < dheight; j += 4)
		{
			v0 = (j >> 3) * (swidth >> 3);
			vx1 = (j * dwidth) >> 1;
#ifdef VDP_V9990
			srcbuf0 = XXBuf + vx1;
#else
			srcbuf0 = (u32*)XBuf + vx1;
#endif // VDP_V9990
			srcbuf1 = srcbuf0 + (dwidth >> 1);
			srcbuf2 = srcbuf1 + (dwidth >> 1);
			srcbuf3 = srcbuf2 + (dwidth >> 1);
			//for (i = 0; i < dwidth; i += 8)
			for (i = 0; i < (dwidth>>3); ++i, srcbuf0 += 4, srcbuf1 += 4, srcbuf2 += 4, srcbuf3 += 4)
			{
				//vx0 = (((v0 + (i >> 3)) << 6) + ((i & 0x01) | ((j & 0x01) << 1)
				//	| ((i & 0x02) << 1) | ((j & 0x02) << 2) | ((i & 0x04) << 2) | ((j & 0x04) << 3)));
				vx0 = (((v0 + i) << 6) + (((i<<3) & 0x04) << 2) | ((j & 0x04) << 3));

				scrtexbf = scrbuf + (vx0 >> 1);

				scrtexbf[0] = srcbuf0[0];
				scrtexbf[1] = srcbuf1[0];
				scrtexbf[2] = srcbuf0[1];
				scrtexbf[3] = srcbuf1[1];
				scrtexbf[4] = srcbuf2[0];
				scrtexbf[5] = srcbuf3[0];
				scrtexbf[6] = srcbuf2[1];
				scrtexbf[7] = srcbuf3[1];

				scrtexbf[8] = srcbuf0[2];
				scrtexbf[9] = srcbuf1[2];
				scrtexbf[10] = srcbuf0[3];
				scrtexbf[11] = srcbuf1[3];
				scrtexbf[12] = srcbuf2[2];
				scrtexbf[13] = srcbuf3[2];
				scrtexbf[14] = srcbuf2[3];
				scrtexbf[15] = srcbuf3[3];
			}
		}
#endif // INLINE_ASM
		
		C3D_FrameBegin(C3D_FRAME_NONBLOCK);
		C2D_TargetClear(TopRenderTarget, C2D_Color32(0, 0, 0, 255));

		C2D_SceneBegin(TopRenderTarget);
		switch (ScreenRes)
		{
		case 0:		/* No Scale */
			//C2D_DrawImageAt(ScreenImage, 0.0f, 0.0f, 1.0f, NULL, 1.0f, 1.0f);
			C2D_DrawImageAt(ScreenImage, 65.0f, 10.0f, 1.0f, NULL, 1.0f, 1.0f);
			break;
		case 1:		/* Wide */
			C2D_DrawImageAt(ScreenImage, 37.0f, 0.0f, 0.5f, NULL, 1.2f, 1.06f);
			break;
		case 2:		/* Full Screen */
			C2D_DrawImageAt(ScreenImage, 0.0f, 0.0f, 0.5f, NULL, 1.46f, 1.06f);
			break;
		case 3:		/* Keep Aspect */
			//C2D_DrawImageAt(ScreenImage, 45.0f, 0.0f, 0.5f, NULL, 1.06f, 1.06f);
			C2D_DrawImageAt(ScreenImage, 55.0f, 0.0f, 0.5f, NULL, 1.06f, 1.06f);
			break;
		case 4:		/* ExtremelyLarge */
			C2D_DrawImageAt(ScreenImage, -12.0f, -21.0f, 0.5f, NULL, 1.56f, 1.24f);
			break;
		default:
			break;
		}
		if (IsImposeScreenShot)
		{
			C2D_DrawImageAt(ScreenImageImpose, ImposeRefX, ImposeRefY, 0.2f, NULL, ImposeRefSize, ImposeRefSize);
		}
		if (IsShowFPS)
		{
			DrawText(std::to_string(fpsval).c_str(), 350, 0, 1.0f, 0.5f, 0.5f, C2D_Color32(255, 255, 255, 255));
			DrawText(("Skip"+std::to_string(4-(UPeriod/25))).c_str(), 310, 0, 1.0f, 0.5f, 0.5f, C2D_Color32(255, 255, 255, 255));
		}
#ifdef _MSX0
		if (UseMSX0)
		{
			if ((currJoyMode[0] >= HIDE_KEYBOARD) || (currJoyMode[1] >= HIDE_KEYBOARD))
			{
				C2D_TargetClear(BottomRenderTartget, C2D_Color32(0, 0, 0, 255));

				C2D_SceneBegin(BottomRenderTartget);
				C2D_DrawImageAt(ScreenImage, 0.0f, 0.0f, 1.0f, NULL, 1.0f, 1.0f);
				DrawHUD();
			}
		}
#endif // _MSX0

		C3D_FrameEnd(0);
	}
	else
	{

#ifdef INLINE_ASM
		/* Currently, only very very little speed up. */
		const int dwidth = WIDTH * 2, dheight = HEIGHT;
		const int swidth = 1024, sheight = 256;
		int i, j, vx0, vx1, v0;
		u32* srcbuf0;
		u32* scrbuf = (u32*)WideTex.data;
		u32* scrtexbf;
		for (j = 0; j < dheight; j += 4)
		{
			v0 = (j >> 3) * (swidth >> 3);
			vx1 = (j * dwidth) >> 1;
			srcbuf0 = (u32*)WBuf + vx1;
			i = 0;
			//i =  (dwidth >> 3);
			__asm__  __volatile__(
				"ldr r5, %[srcbuf0]\n"
				"ldr r6, %[i]\n"
				"loopW:\n"
				"add r0, %[v0], r6\n"
				"lsl r0, r0, #6\n"	//(v0+i)<<6

				"lsl r1, r6,#3\n"	//
				"and r1, r1 ,#4\n"	//
				"lsl r1, r1, #2\n"	// ((i<<3) & 0x04) << 2
				"add r0,r0,r1\n"

				"and r2, %[j], #4\n"
				"lsl r2, r2, #3\n"  //(j & 0x04) << 3
				"orr r0,r0,r2\n"

				"lsl  r0,  r0, #1\n"	//(vx0>>1)*4 equal vx0<<1
				"add %[scrtexbf], %[scrbuf], r0\n"
				"mov r4, r5\n"

				"ldm r4,{r0-r3} \n"
				"str r0,[%[scrtexbf],#0]\n"
				"str r1,[%[scrtexbf],#8]\n"
				"str r2,[%[scrtexbf],#32]\n"
				"str r3,[%[scrtexbf],#40]\n"
				"add r4, r4, #512\n"
				"add r4, r4, #512\n"
				"add r4, r4, #64\n"

				"ldm r4,{r0-r3} \n"
				"str r0,[%[scrtexbf],#4]\n"
				"str r1,[%[scrtexbf],#12]\n"
				"str r2,[%[scrtexbf],#36]\n"
				"str r3,[%[scrtexbf],#44]\n"
				"add r4, r4, #512\n"
				"add r4, r4, #512\n"
				"add r4, r4, #64\n"

				"ldm r4,{r0-r3} \n"
				"str r0,[%[scrtexbf],#16]\n"
				"str r1,[%[scrtexbf],#24]\n"
				"str r2,[%[scrtexbf],#48]\n"
				"str r3,[%[scrtexbf],#56]\n"
				"add r4, r4, #512\n"
				"add r4, r4, #512\n"
				"add r4, r4, #64\n"

				"ldm r4,{r0-r3} \n"
				"str r0,[%[scrtexbf],#20]\n"
				"str r1,[%[scrtexbf],#28]\n"
				"str r2,[%[scrtexbf],#52]\n"
				"str r3,[%[scrtexbf],#60]\n"

				"add r5, r5 ,#16\n"
				//"subs r6, r6, #1\n"
				"add r6, r6, #1\n"
				"cmp r6,#68\n"
				"bne loopW\n"

				//: [v0]"+r"(v0), [i]"+r"(i), [j]"+r"(j)
				:
			: [srcbuf0] "m"(srcbuf0), [scrtexbf] "r"(scrtexbf), [scrbuf]"r"(scrbuf), [v0] "r"(v0), [i]"m"(i), [j]"r"(j)
				: "r0", "r1", "r2", "r3", "r4", "r5", "r6"
				);
		}

		//const int dwidth = WIDTH * 2, dheight = HEIGHT;
		//const int swidth = 1024, sheight = 256;
		//int i, j, vx0, vx1, v0;
		//u32* srcbuf0;
		//u32* scrbuf = (u32*)WideTex.data;
		//u32* scrtexbf;
		//for (j = 0; j < dheight; j += 4)
		//{
		//	v0 = (j >> 3) * (swidth >> 3);
		//	vx1 = (j * dwidth) >> 1;
		//	srcbuf0 = (u32*)WBuf + vx1;
		//	for (i = 0; i < (dwidth >> 3); ++i, srcbuf0 += 4)
		//	{
		//		//vx0 = (((v0 + (i >> 3)) << 6) + ((i & 0x01) | ((j & 0x01) << 1)
		//		//	| ((i & 0x02) << 1) | ((j & 0x02) << 2) | ((i & 0x04) << 2) | ((j & 0x04) << 3)));
		//		__asm__  __volatile__(

		//			//"	mov r0, %[v0]\n"
		//			//"	mov r1, %[i]\n"
		//			//"	mov r2, %[j]\n"
		//			"	add r0, %[v0], %[i]\n"
		//			"	lsl r0, r0, #6\n"		//(v0+i)<<6

		//			"	lsl	r1,%[i],#3\n"
		//			"	and r1, r1 ,#4\n"
		//			"	lsl	r1, r1, #2\n"	//(((i<<3) & 0x04) << 2)

		//			"	add r0,r0,r1\n"

		//			"	and r2, %[j], #4\n"
		//			"	lsl r2, r2, #3\n"	//(j & 0x04) << 3)
		//			"	orr r0,r0,r2\n"

		//			"	lsl r0, r0, #1\n"
		//			"	add %[scrtexbf], %[scrbuf],r0\n" //scrtexbf = scrbuf + (vx0 >> 1);
		//			"	mov r4, %[srcbuf0]\n"

		//			"ldm r4,{r0-r3} \n"
		//			"str r0,[%[scrtexbf],#0]\n"
		//			"str r1,[%[scrtexbf],#8]\n"
		//			"str r2,[%[scrtexbf],#32]\n"
		//			"str r3,[%[scrtexbf],#40]\n"
		//			"add r4, r4, #512\n"
		//			"add r4, r4, #512\n"
		//			"add r4, r4, #64\n"

		//			"ldm r4,{r0-r3} \n"
		//			"str r0,[%[scrtexbf],#4]\n"
		//			"str r1,[%[scrtexbf],#12]\n"
		//			"str r2,[%[scrtexbf],#36]\n"
		//			"str r3,[%[scrtexbf],#44]\n"
		//			"add r4, r4, #512\n"
		//			"add r4, r4, #512\n"
		//			"add r4, r4, #64\n"

		//			"ldm r4,{r0-r3} \n"
		//			"str r0,[%[scrtexbf],#16]\n"
		//			"str r1,[%[scrtexbf],#24]\n"
		//			"str r2,[%[scrtexbf],#48]\n"
		//			"str r3,[%[scrtexbf],#56]\n"
		//			"add r4, r4, #512\n"
		//			"add r4, r4, #512\n"
		//			"add r4, r4, #64\n"

		//			"ldm r4,{r0-r3} \n"
		//			"str r0,[%[scrtexbf],#20]\n"
		//			"str r1,[%[scrtexbf],#28]\n"
		//			"str r2,[%[scrtexbf],#52]\n"
		//			"str r3,[%[scrtexbf],#60]\n"
		//			: [srcbuf0] "+r"(srcbuf0)
		//			: [v0] "r" (v0), [i] "r" (i), [j] "r"(j), [scrtexbf] "r"(scrtexbf), [scrbuf]"r"(scrbuf)
		//			: "r0", "r1", "r2", "r3", "r4"
		//			);
		//	}
		//}

		//const int dwidth = WIDTH * 2, dheight = HEIGHT;
		//const int swidth = 1024, sheight = 256;
		//int i, j, vx0, vx1, v0;
		//u32* srcbuf0;
		//u32* scrbuf = (u32*)WideTex.data;
		//u32* scrtexbf;
		//for (j = 0; j < dheight; j += 4)
		//{
		//	v0 = (j >> 3) * (swidth >> 3);
		//	vx1 = (j * dwidth) >> 1;
		//	srcbuf0 = (u32*)WBuf + vx1;
		//	for (i = 0; i < (dwidth >> 3); ++i, srcbuf0 += 4)
		//	{
		//		//vx0 = (((v0 + (i >> 3)) << 6) + ((i & 0x01) | ((j & 0x01) << 1)
		//		//	| ((i & 0x02) << 1) | ((j & 0x02) << 2) | ((i & 0x04) << 2) | ((j & 0x04) << 3)));
		//		__asm__  __volatile__(

		//			"	mov r0, %[v0]\n"
		//			"	mov r1, %[i]\n"
		//			"	mov r2, %[j]\n"
		//			"	add r0, r0, r1\n" 	//v0 = v0+i;
		//			"	lsl r0, r0, #6\n"		//(v0+i)<<6

		//			"	lsl	r3,r1,#3\n"		//(i<<3)
		//			"	and r3, r3 ,#4\n"	//(i<<3)&0x04
		//			"	lsl	r3, r3, #2\n"	//(((i<<3) & 0x04) << 2)

		//			"	add r0,r0,r3\n"

		//			"	and r2, r2, #4\n"
		//			"	lsl r2, r2, #3\n"
		//			"	add r0,r0,r2\n"
		//			"	lsl r0, r0, #1\n"	//(vx0>>1)*4
		//			"	add %[scrtexbf], %[scrbuf],r0\n"
		//			"	mov r4, %[srcbuf0]\n"

		//			"ldm r4,{r0-r3} \n"
		//			"str r0,[%[scrtexbf],#0]\n"
		//			"str r1,[%[scrtexbf],#8]\n"
		//			"str r2,[%[scrtexbf],#32]\n"
		//			"str r3,[%[scrtexbf],#40]\n"
		//			"add r4, r4, #512\n"
		//			"add r4, r4, #512\n"
		//			"add r4, r4, #64\n"

		//			"ldm r4,{r0-r3} \n"
		//			"str r0,[%[scrtexbf],#4]\n"
		//			"str r1,[%[scrtexbf],#12]\n"
		//			"str r2,[%[scrtexbf],#36]\n"
		//			"str r3,[%[scrtexbf],#44]\n"
		//			"add r4, r4, #512\n"
		//			"add r4, r4, #512\n"
		//			"add r4, r4, #64\n"

		//			"ldm r4,{r0-r3} \n"
		//			"str r0,[%[scrtexbf],#16]\n"
		//			"str r1,[%[scrtexbf],#24]\n"
		//			"str r2,[%[scrtexbf],#48]\n"
		//			"str r3,[%[scrtexbf],#56]\n"
		//			"add r4, r4, #512\n"
		//			"add r4, r4, #512\n"
		//			"add r4, r4, #64\n"

		//			"ldm r4,{r0-r3} \n"
		//			"str r0,[%[scrtexbf],#20]\n"
		//			"str r1,[%[scrtexbf],#28]\n"
		//			"str r2,[%[scrtexbf],#52]\n"
		//			"str r3,[%[scrtexbf],#60]\n"
		//			: [srcbuf0] "+r"(srcbuf0)
		//			: [v0] "r" (v0), [i] "r" (i), [j] "r"(j), [scrtexbf] "r"(scrtexbf), [scrbuf]"r"(scrbuf)
		//			: "r0", "r1", "r2", "r3", "r4"
		//			);
		//	}
		//}

	//	const int dwidth = WIDTH * 2, dheight = HEIGHT;
	//	const int swidth = 1024, sheight = 256;
	//	int i, j, vx0, vx1, v0;
	//	u32* srcbuf0;
	//	u32* scrbuf = (u32*)WideTex.data;
	//	u32* scrtexbf;
	//	for (j = 0; j < dheight; j += 4)
	//	{
	//		v0 = (j >> 3) * (swidth >> 3);
	//		vx1 = (j * dwidth) >> 1;
	//		srcbuf0 = (u32*)WBuf + vx1;
	//		i = 0;
	//		__asm__  __volatile__(
	//			"mov r5, %[v0]\n"
	//			"mov r6, %[i]\n"
	//			"mov r7, %[j]\n"
	//			"mov r4, %[srcbuf0]\n"
	//			"loopW:\n"
	//			"add r0, %[v0], %[i]\n"
	//			"lsl r0, r0, #6\n"	//(v0+i)<<6

	//			"lsl r1,%[i],#3\n"	//
	//			"and r1, r1 ,#4\n"	//
	//			"lsl r1, r1, #2\n"	// ((i<<3) & 0x04) << 2
	//			"add r0,r0,r1\n"

	//			"and %[j], %[j], #4\n"
	//			"lsl %[j], %[j], #3\n"  //(j & 0x04) << 3
	//			"orr r0,r0,%[j]\n"

	//			"lsl  %[v0],  %[v0], #1\n"	//(vx0>>1)*4 equal vx0<<1
	//			"add %[scrtexbf], %[scrbuf], %[v0]\n"
	//			"mov r4, %[srcbuf0]\n"

	//			"ldm %[srcbuf0],{r0-r3} \n"
	//			"str r0,[%[scrtexbf],#0]\n"
	//			"str r1,[%[scrtexbf],#8]\n"
	//			"str r2,[%[scrtexbf],#32]\n"
	//			"str r3,[%[scrtexbf],#40]\n"
	//			"add r4, r4, #512\n"
	//			"add r4, r4, #512\n"
	//			"add r4, r4, #64\n"

	//			"ldm r4,{r0-r3} \n"
	//			"str r0,[%[scrtexbf],#4]\n"
	//			"str r1,[%[scrtexbf],#12]\n"
	//			"str r2,[%[scrtexbf],#36]\n"
	//			"str r3,[%[scrtexbf],#44]\n"
	//			"add r4, r4, #512\n"
	//			"add r4, r4, #512\n"
	//			"add r4, r4, #64\n"

	//			"ldm r4,{r0-r3} \n"
	//			"str r0,[%[scrtexbf],#16]\n"
	//			"str r1,[%[scrtexbf],#24]\n"
	//			"str r2,[%[scrtexbf],#48]\n"
	//			"str r3,[%[scrtexbf],#56]\n"
	//			"add r4, r4, #512\n"
	//			"add r4, r4, #512\n"
	//			"add r4, r4, #64\n"

	//			"ldm r4,{r0-r3} \n"
	//			"str r0,[%[scrtexbf],#20]\n"
	//			"str r1,[%[scrtexbf],#28]\n"
	//			"str r2,[%[scrtexbf],#52]\n"
	//			"str r3,[%[scrtexbf],#60]\n"

	//			"add %[srcbuf0], %[srcbuf0] ,#4\n"
	//			//"add r4, r4, #4\n"
	//			//"subs r1, r1, #1\n"
	//			"add %[i], %[i], #1\n"
	//			"cmp %[i],#60\n"
	//			"bne loopW\n"

	//			//: [v0]"+r"(v0), [i]"+r"(i), [j]"+r"(j)
	//			:
	//			: [srcbuf0] "r"(srcbuf0), [scrtexbf] "r"(scrtexbf), [scrbuf]"r"(scrbuf), [v0] "r"(v0), [i]"r"(i), [j]"r"(j)
	//			: "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7"
	//			);
	//}
#else
		const int dwidth = WIDTH * 2, dheight = HEIGHT;
		const int swidth = 1024, sheight = 256;
		int i, j, vx0, vx1, v0;
		u32* srcbuf0, * srcbuf1, * srcbuf2, * srcbuf3;
		u32* scrbuf = (u32*)WideTex.data;
		u32* scrtexbf;
		for (j = 0; j < dheight; j += 4)
		{
			v0 = (j >> 3) * (swidth >> 3);
			vx1 = (j * dwidth) >> 1;
			srcbuf0 = (u32*)WBuf + vx1;
			srcbuf1 = srcbuf0 + (dwidth >> 1);
			srcbuf2 = srcbuf1 + (dwidth >> 1);
			srcbuf3 = srcbuf2 + (dwidth >> 1);
			for (i = 0; i < (dwidth >> 3); ++i, srcbuf0 += 4, srcbuf1 += 4, srcbuf2 += 4, srcbuf3 += 4)
			{
				//vx0 = (((v0 + (i >> 3)) << 6) + ((i & 0x01) | ((j & 0x01) << 1)
				//	| ((i & 0x02) << 1) | ((j & 0x02) << 2) | ((i & 0x04) << 2) | ((j & 0x04) << 3)));

				vx0 = (((v0 + i) << 6) + (((i << 3) & 0x04) << 2) | (j & 0x04) << 3);

				scrtexbf = scrbuf + (vx0 >> 1);
				scrtexbf[0] = srcbuf0[0];
				scrtexbf[1] = srcbuf1[0];
				scrtexbf[2] = srcbuf0[1];
				scrtexbf[3] = srcbuf1[1];
				scrtexbf[4] = srcbuf2[0];
				scrtexbf[5] = srcbuf3[0];
				scrtexbf[6] = srcbuf2[1];
				scrtexbf[7] = srcbuf3[1];

				scrtexbf[8] = srcbuf0[2];
				scrtexbf[9] = srcbuf1[2];
				scrtexbf[10] = srcbuf0[3];
				scrtexbf[11] = srcbuf1[3];
				scrtexbf[12] = srcbuf2[2];
				scrtexbf[13] = srcbuf3[2];
				scrtexbf[14] = srcbuf2[3];
				scrtexbf[15] = srcbuf3[3];
				//srcbuf0 += 4, srcbuf1 += 4, srcbuf2 += 4, srcbuf3 += 4;
			}
		}
#endif // INLINE_ASM

		C3D_FrameBegin(C3D_FRAME_NONBLOCK);
		C2D_TargetClear(WideRenderTarget, C2D_Color32(0, 0, 0, 255));

		C2D_SceneBegin(WideRenderTarget);
		switch (ScreenRes)
		{
		case 0:		/* No Scale */
			//C2D_DrawImageAt(WideImage, 0.0f, 0.0f, 1.0f, NULL, 0.5f, 1.0f);
			C2D_DrawImageAt(WideImage, 65.0f, 10.0f, 1.0f, NULL, 0.5f, 1.0f);
			break;
		case 1:		/* Wide */
			C2D_DrawImageAt(WideImage, 37.0f, 0.0f, 0.5f, NULL, 0.6f, 1.06f);
			break;
		case 2:		/* Full Screen */
			C2D_DrawImageAt(WideImage, 0.0f, 0.0f, 0.5f, NULL, 0.73f, 1.06f);
			break;
		case 3:		/* Keep Aspect */
			//C2D_DrawImageAt(WideImage, 45.0f, 0.0f, 0.5f, NULL, 0.53f, 1.06f);
			C2D_DrawImageAt(WideImage, 55.0f, 0.0f, 0.5f, NULL, 0.53f, 1.06f);
			break;
		case 4:		/* ExtremelyLarge */
			C2D_DrawImageAt(WideImage, -12.0f, -21.0f, 0.5f, NULL, 0.78f, 1.24f);
			break;
		default:
			break;
		}
		if (IsShowFPS)
		{
			DrawText(std::to_string(fpsval).c_str(), 350, 0, 1.0f, 0.5f, 0.5f, C2D_Color32(255, 255, 255, 255));
			DrawText(("Skip" + std::to_string(4 - (UPeriod / 25))).c_str(), 310, 0, 1.0f, 0.5f, 0.5f, C2D_Color32(255, 255, 255, 255));
		}
#ifdef _MSX0
		if (UseMSX0)
		{
			if ((currJoyMode[0] >= HIDE_KEYBOARD) || (currJoyMode[1] >= HIDE_KEYBOARD))
			{
				C2D_TargetClear(BottomRenderTartget, C2D_Color32(0, 0, 0, 255));

				C2D_SceneBegin(BottomRenderTartget);
				C2D_DrawImageAt(WideImage, 0.0f, 0.0f, 1.0f, NULL, 0.5f, 1.0f);
				DrawHUD();
			}
		}
#endif // _MSX0
		C3D_FrameEnd(0);
	}
}

#ifdef USE_3D
void PutImage3D(void)
{
#ifdef VDP_V9990
	if (V9990Active)
	{
		if (V9KScrMode != OldV9KScrMode)
		{
			OldV9KScrMode = V9KScrMode;
			SetupWideScreen(false);
		}
	}
	else
#endif // VDP_V9990
	if (ScrMode != OldScrMode)
	{
		OldScrMode = ScrMode;
		SetupWideScreen(false);
	}
	if (Is3DNow != OldIs3DNow)
	{
		OldIs3DNow = Is3DNow;;
		SetupWideScreen(false);
		gfxSet3D(true);
		C3D_RenderTargetSetOutput(TopRenderTargetR, GFX_TOP, GFX_RIGHT, (GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
			GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
			GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO)));
	}
	if (OldStereo3DMode != Stereo3DMode)
	{
		OldStereo3DMode = Stereo3DMode;
		SetupWideScreen(false);
		gfxSet3D(true);
		C3D_RenderTargetSetOutput(TopRenderTargetR, GFX_TOP, GFX_RIGHT, (GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
			GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
			GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO)));
	}

	const int dwidth = WIDTH, dheight = HEIGHT;
	const int swidth = 512, sheight = 256;
	int i, j, vx0, vx1, v0;

	u32* srcbuf0, * srcbuf1, * srcbuf2, * srcbuf3;

	u32* scrbuf = (u32*)ScreenTex.data;
	u32* scrbufR = (u32*)ScreenTexR.data;
	u32* scrtexbf, * scrtexbfR;
#ifdef VDP_V9990
	u32* XXBuf = V9990Active ? (u32*)V9KXBuf : (u32*)XBuf;
#endif // VDP_V9990

	if (Stereo3DMode == 1)
	{
		unsigned char RVal = 0, RVal2 = 0, BVal = 0, BVal2 = 0;
		for (j = 0; j < dheight; j += 4)
		{
			v0 = (j >> 3) * (swidth >> 3);
			vx1 = (j * dwidth) >> 1;
#ifdef VDP_V9990
			srcbuf0 = XXBuf + vx1;
#else
			srcbuf0 = (u32*)XBuf + vx1;
#endif // VDP_V9990
			srcbuf1 = srcbuf0 + (dwidth >> 1);
			srcbuf2 = srcbuf1 + (dwidth >> 1);
			srcbuf3 = srcbuf2 + (dwidth >> 1);
			//for (i = 0; i < dwidth; i += 8)
			for (i = 0; i < (dwidth >> 3); ++i, srcbuf0 += 4, srcbuf1 += 4, srcbuf2 += 4, srcbuf3 += 4)
			{
				//vx0 = (((v0 + (i >> 3)) << 6) + ((i & 0x01) | ((j & 0x01) << 1)
				//	| ((i & 0x02) << 1) | ((j & 0x02) << 2) | ((i & 0x04) << 2) | ((j & 0x04) << 3)));
				vx0 = (((v0 + i) << 6) + (((i << 3) & 0x04) << 2) | ((j & 0x04) << 3));

				scrtexbf = scrbuf + (vx0 >> 1);
				RVal = (srcbuf0[0] >> 27) & 0x1F; RVal2 = (srcbuf0[0] >> 11) & 0x1F;
				scrtexbf[0] = (u32)RVal << 27 | (u32)RVal << 22 | (u32)RVal << 16 | (u32)RVal2 << 11 | (u32)RVal2 << 6 | (u32)RVal2;
				RVal = (srcbuf1[0] >> 27) & 0x1F; RVal2 = (srcbuf1[0] >> 11) & 0x1F;
				scrtexbf[1] = (u32)RVal << 27 | (u32)RVal << 22 | (u32)RVal << 16 | (u32)RVal2 << 11 | (u32)RVal2 << 6 | (u32)RVal2;
				RVal = (srcbuf0[1] >> 27) & 0x1F; RVal2 = (srcbuf0[1] >> 11) & 0x1F;
				scrtexbf[2] = (u32)RVal << 27 | (u32)RVal << 22 | (u32)RVal << 16 | (u32)RVal2 << 11 | (u32)RVal2 << 6 | (u32)RVal2;
				RVal = (srcbuf1[1] >> 27) & 0x1F; RVal2 = (srcbuf1[1] >> 11) & 0x1F;
				scrtexbf[3] = (u32)RVal << 27 | (u32)RVal << 22 | (u32)RVal << 16 | (u32)RVal2 << 11 | (u32)RVal2 << 6 | (u32)RVal2;
				RVal = (srcbuf2[0] >> 27) & 0x1F; RVal2 = (srcbuf2[0] >> 11) & 0x1F;
				scrtexbf[4] = (u32)RVal << 27 | (u32)RVal << 22 | (u32)RVal << 16 | (u32)RVal2 << 11 | (u32)RVal2 << 6 | (u32)RVal2;
				RVal = (srcbuf3[0] >> 27) & 0x1F; RVal2 = (srcbuf3[0] >> 11) & 0x1F;
				scrtexbf[5] = (u32)RVal << 27 | (u32)RVal << 22 | (u32)RVal << 16 | (u32)RVal2 << 11 | (u32)RVal2 << 6 | (u32)RVal2;
				RVal = (srcbuf2[1] >> 27) & 0x1F; RVal2 = (srcbuf2[1] >> 11) & 0x1F;
				scrtexbf[6] = (u32)RVal << 27 | (u32)RVal << 22 | (u32)RVal << 16 | (u32)RVal2 << 11 | (u32)RVal2 << 6 | (u32)RVal2;
				RVal = (srcbuf3[1] >> 27) & 0x1F; RVal2 = (srcbuf3[1] >> 11) & 0x1F;
				scrtexbf[7] = (u32)RVal << 27 | (u32)RVal << 22 | (u32)RVal << 16 | (u32)RVal2 << 11 | (u32)RVal2 << 6 | (u32)RVal2;

				RVal = (srcbuf0[2] >> 27) & 0x1F; RVal2 = (srcbuf0[2] >> 11) & 0x1F;
				scrtexbf[8] = (u32)RVal << 27 | (u32)RVal << 22 | (u32)RVal << 16 | (u32)RVal2 << 11 | (u32)RVal2 << 6 | (u32)RVal2;
				RVal = (srcbuf1[2] >> 27) & 0x1F; RVal2 = (srcbuf1[2] >> 11) & 0x1F;
				scrtexbf[9] = (u32)RVal << 27 | (u32)RVal << 22 | (u32)RVal << 16 | (u32)RVal2 << 11 | (u32)RVal2 << 6 | (u32)RVal2;
				RVal = (srcbuf0[3] >> 27) & 0x1F; RVal2 = (srcbuf0[3] >> 11) & 0x1F;
				scrtexbf[10] = (u32)RVal << 27 | (u32)RVal << 22 | (u32)RVal << 16 | (u32)RVal2 << 11 | (u32)RVal2 << 6 | (u32)RVal2;
				RVal = (srcbuf1[3] >> 27) & 0x1F; RVal2 = (srcbuf1[3] >> 11) & 0x1F;
				scrtexbf[11] = (u32)RVal << 27 | (u32)RVal << 22 | (u32)RVal << 16 | (u32)RVal2 << 11 | (u32)RVal2 << 6 | (u32)RVal2;
				RVal = (srcbuf2[2] >> 27) & 0x1F; RVal2 = (srcbuf2[2] >> 11) & 0x1F;
				scrtexbf[12] = (u32)RVal << 27 | (u32)RVal << 22 | (u32)RVal << 16 | (u32)RVal2 << 11 | (u32)RVal2 << 6 | (u32)RVal2;
				RVal = (srcbuf3[2] >> 27) & 0x1F; RVal2 = (srcbuf3[2] >> 11) & 0x1F;
				scrtexbf[13] = (u32)RVal << 27 | (u32)RVal << 22 | (u32)RVal << 16 | (u32)RVal2 << 11 | (u32)RVal2 << 6 | (u32)RVal2;
				RVal = (srcbuf2[3] >> 27) & 0x1F; RVal2 = (srcbuf2[3] >> 11) & 0x1F;
				scrtexbf[14] = (u32)RVal << 27 | (u32)RVal << 22 | (u32)RVal << 16 | (u32)RVal2 << 11 | (u32)RVal2 << 6 | (u32)RVal2;
				RVal = (srcbuf3[3] >> 27) & 0x1F; RVal2 = (srcbuf3[3] >> 11) & 0x1F;
				scrtexbf[15] = (u32)RVal << 27 | (u32)RVal << 22 | (u32)RVal << 16 | (u32)RVal2 << 11 | (u32)RVal2 << 6 | (u32)RVal2;

				scrtexbfR = scrbufR + (vx0 >> 1);
				RVal = (srcbuf0[0] >> 16) & 0x1F; RVal2 = (srcbuf0[0]) & 0x1F;
				scrtexbfR[0] = (u32)RVal << 27 | (u32)RVal << 22 | (u32)RVal << 16 | (u32)RVal2 << 11 | (u32)RVal2 << 6 | (u32)RVal2;
				RVal = (srcbuf1[0] >> 16) & 0x1F; RVal2 = (srcbuf1[0]) & 0x1F;
				scrtexbfR[1] = (u32)RVal << 27 | (u32)RVal << 22 | (u32)RVal << 16 | (u32)RVal2 << 11 | (u32)RVal2 << 6 | (u32)RVal2;
				RVal = (srcbuf0[1] >> 16) & 0x1F; RVal2 = (srcbuf0[1]) & 0x1F;
				scrtexbfR[2] = (u32)RVal << 27 | (u32)RVal << 22 | (u32)RVal << 16 | (u32)RVal2 << 11 | (u32)RVal2 << 6 | (u32)RVal2;
				RVal = (srcbuf1[1] >> 16) & 0x1F; RVal2 = (srcbuf1[1]) & 0x1F;
				scrtexbfR[3] = (u32)RVal << 27 | (u32)RVal << 22 | (u32)RVal << 16 | (u32)RVal2 << 11 | (u32)RVal2 << 6 | (u32)RVal2;
				RVal = (srcbuf2[0] >> 16) & 0x1F; RVal2 = (srcbuf2[0]) & 0x1F;
				scrtexbfR[4] = (u32)RVal << 27 | (u32)RVal << 22 | (u32)RVal << 16 | (u32)RVal2 << 11 | (u32)RVal2 << 6 | (u32)RVal2;
				RVal = (srcbuf3[0] >> 16) & 0x1F; RVal2 = (srcbuf3[0]) & 0x1F;
				scrtexbfR[5] = (u32)RVal << 27 | (u32)RVal << 22 | (u32)RVal << 16 | (u32)RVal2 << 11 | (u32)RVal2 << 6 | (u32)RVal2;
				RVal = (srcbuf2[1] >> 16) & 0x1F; RVal2 = (srcbuf2[1]) & 0x1F;
				scrtexbfR[6] = (u32)RVal << 27 | (u32)RVal << 22 | (u32)RVal << 16 | (u32)RVal2 << 11 | (u32)RVal2 << 6 | (u32)RVal2;
				RVal = (srcbuf3[1] >> 16) & 0x1F; RVal2 = (srcbuf3[1]) & 0x1F;
				scrtexbfR[7] = (u32)RVal << 27 | (u32)RVal << 22 | (u32)RVal << 16 | (u32)RVal2 << 11 | (u32)RVal2 << 6 | (u32)RVal2;

				RVal = (srcbuf0[2] >> 16) & 0x1F; RVal2 = (srcbuf0[2]) & 0x1F;
				scrtexbfR[8] = (u32)RVal << 27 | (u32)RVal << 22 | (u32)RVal << 16 | (u32)RVal2 << 11 | (u32)RVal2 << 6 | (u32)RVal2;
				RVal = (srcbuf1[2] >> 16) & 0x1F; RVal2 = (srcbuf1[2]) & 0x1F;
				scrtexbfR[9] = (u32)RVal << 27 | (u32)RVal << 22 | (u32)RVal << 16 | (u32)RVal2 << 11 | (u32)RVal2 << 6 | (u32)RVal2;
				RVal = (srcbuf0[3] >> 16) & 0x1F; RVal2 = (srcbuf0[3]) & 0x1F;
				scrtexbfR[10] = (u32)RVal << 27 | (u32)RVal << 22 | (u32)RVal << 16 | (u32)RVal2 << 11 | (u32)RVal2 << 6 | (u32)RVal2;
				RVal = (srcbuf1[3] >> 16) & 0x1F; RVal2 = (srcbuf1[3]) & 0x1F;
				scrtexbfR[11] = (u32)RVal << 27 | (u32)RVal << 22 | (u32)RVal << 16 | (u32)RVal2 << 11 | (u32)RVal2 << 6 | (u32)RVal2;
				RVal = (srcbuf2[2] >> 16) & 0x1F; RVal2 = (srcbuf2[2]) & 0x1F;
				scrtexbfR[12] = (u32)RVal << 27 | (u32)RVal << 22 | (u32)RVal << 16 | (u32)RVal2 << 11 | (u32)RVal2 << 6 | (u32)RVal2;
				RVal = (srcbuf3[2] >> 16) & 0x1F; RVal2 = (srcbuf3[2]) & 0x1F;
				scrtexbfR[13] = (u32)RVal << 27 | (u32)RVal << 22 | (u32)RVal << 16 | (u32)RVal2 << 11 | (u32)RVal2 << 6 | (u32)RVal2;
				RVal = (srcbuf2[3] >> 16) & 0x1F; RVal2 = (srcbuf2[3]) & 0x1F;
				scrtexbfR[14] = (u32)RVal << 27 | (u32)RVal << 22 | (u32)RVal << 16 | (u32)RVal2 << 11 | (u32)RVal2 << 6 | (u32)RVal2;
				RVal = (srcbuf3[3] >> 16) & 0x1F; RVal2 = (srcbuf3[3]) & 0x1F;
				scrtexbfR[15] = (u32)RVal << 27 | (u32)RVal << 22 | (u32)RVal << 16 | (u32)RVal2 << 11 | (u32)RVal2 << 6 | (u32)RVal2;
			}
		}
	}
	else if (Stereo3DMode == 2)
	{
		for (j = 0; j < dheight; j += 4)
		{
			v0 = (j >> 3) * (swidth >> 3);
			vx1 = (j * dwidth) >> 1;
#ifdef VDP_V9990
			srcbuf0 = XXBuf + vx1;
#else
			srcbuf0 = (u32*)XBuf + vx1;
#endif // VDP_V9990
			srcbuf1 = srcbuf0 + (dwidth >> 1);
			srcbuf2 = srcbuf1 + (dwidth >> 1);
			srcbuf3 = srcbuf2 + (dwidth >> 1);
			//for (i = 0; i < dwidth; i += 8)
			for (i = 0; i < (dwidth >> 3); ++i, srcbuf0 += 4, srcbuf1 += 4, srcbuf2 += 4, srcbuf3 += 4)
			{
				//vx0 = (((v0 + (i >> 3)) << 6) + ((i & 0x01) | ((j & 0x01) << 1)
				//	| ((i & 0x02) << 1) | ((j & 0x02) << 2) | ((i & 0x04) << 2) | ((j & 0x04) << 3)));
				vx0 = (((v0 + i) << 6) + (((i << 3) & 0x04) << 2) | ((j & 0x04) << 3));

				scrtexbf = scrbuf + (vx0 >> 1);
				scrtexbf[0] = srcbuf0[0] & 0xF800F800;
				scrtexbf[1] = srcbuf1[0] & 0xF800F800;
				scrtexbf[2] = srcbuf0[1] & 0xF800F800;
				scrtexbf[3] = srcbuf1[1] & 0xF800F800;
				scrtexbf[4] = srcbuf2[0] & 0xF800F800;
				scrtexbf[5] = srcbuf3[0] & 0xF800F800;
				scrtexbf[6] = srcbuf2[1] & 0xF800F800;
				scrtexbf[7] = srcbuf3[1] & 0xF800F800;

				scrtexbf[8] = srcbuf0[2] & 0xF800F800;
				scrtexbf[9] = srcbuf1[2] & 0xF800F800;
				scrtexbf[10] = srcbuf0[3] & 0xF800F800;
				scrtexbf[11] = srcbuf1[3] & 0xF800F800;
				scrtexbf[12] = srcbuf2[2] & 0xF800F800;
				scrtexbf[13] = srcbuf3[2] & 0xF800F800;
				scrtexbf[14] = srcbuf2[3] & 0xF800F800;
				scrtexbf[15] = srcbuf3[3] & 0xF800F800;

				scrtexbfR = scrbufR + (vx0 >> 1);
				scrtexbfR[0] = srcbuf0[0] & 0x07FF07FF;
				scrtexbfR[1] = srcbuf1[0] & 0x07FF07FF;
				scrtexbfR[2] = srcbuf0[1] & 0x07FF07FF;
				scrtexbfR[3] = srcbuf1[1] & 0x07FF07FF;
				scrtexbfR[4] = srcbuf2[0] & 0x07FF07FF;
				scrtexbfR[5] = srcbuf3[0] & 0x07FF07FF;
				scrtexbfR[6] = srcbuf2[1] & 0x07FF07FF;
				scrtexbfR[7] = srcbuf3[1] & 0x07FF07FF;

				scrtexbfR[8] = srcbuf0[2] & 0x07FF07FF;
				scrtexbfR[9] = srcbuf1[2] & 0x07FF07FF;
				scrtexbfR[10] = srcbuf0[3] & 0x07FF07FF;
				scrtexbfR[11] = srcbuf1[3] & 0x07FF07FF;
				scrtexbfR[12] = srcbuf2[2] & 0x07FF07FF;
				scrtexbfR[13] = srcbuf3[2] & 0x07FF07FF;
				scrtexbfR[14] = srcbuf2[3] & 0x07FF07FF;
				scrtexbfR[15] = srcbuf3[3] & 0x07FF07FF;
			}
		}
	}

	C3D_FrameBegin(C3D_FRAME_NONBLOCK);
	C2D_TargetClear(TopRenderTarget, C2D_Color32(0, 0, 0, 255));

	C2D_SceneBegin(TopRenderTarget);
	switch (ScreenRes)
	{
	case 0:		/* No Scale */
		//C2D_DrawImageAt(ScreenImage, 0.0f, 0.0f, 1.0f, NULL, 1.0f, 1.0f);
		C2D_DrawImageAt(ScreenImage, 65.0f, 10.0f, 1.0f, NULL, 1.0f, 1.0f);
		break;
	case 1:		/* Wide */
		C2D_DrawImageAt(ScreenImage, 37.0f, 0.0f, 0.5f, NULL, 1.2f, 1.06f);
		break;
	case 2:		/* Full Screen */
		C2D_DrawImageAt(ScreenImage, 0.0f, 0.0f, 0.5f, NULL, 1.46f, 1.06f);
		break;
	case 3:		/* Keep Aspect */
		//C2D_DrawImageAt(ScreenImage, 45.0f, 0.0f, 0.5f, NULL, 1.06f, 1.06f);
		C2D_DrawImageAt(ScreenImage, 55.0f, 0.0f, 0.5f, NULL, 1.06f, 1.06f);
		break;
	case 4:		/* ExtremelyLarge */
		C2D_DrawImageAt(ScreenImage, -12.0f, -21.0f, 0.5f, NULL, 1.56f, 1.24f);
		break;
	default:
		break;
	}
	if (IsShowFPS)
	{
		DrawText(std::to_string(fpsval).c_str(), 350, 0, 1.0f, 0.5f, 0.5f, C2D_Color32(255, 255, 255, 255));
		DrawText(("Skip" + std::to_string(4 - (UPeriod / 25))).c_str(), 310, 0, 1.0f, 0.5f, 0.5f, C2D_Color32(255, 255, 255, 255));
	}

	C2D_TargetClear(TopRenderTargetR, C2D_Color32(0, 0, 0, 255));
	C2D_SceneBegin(TopRenderTargetR);
	switch (ScreenRes)
	{
	case 0:		/* No Scale */
		C2D_DrawImageAt(ScreenImageR, 0.0f, 0.0f, 1.0f, NULL, 1.0f, 1.0f);
		break;
	case 1:		/* Wide */
		C2D_DrawImageAt(ScreenImageR, 37.0f, 0.0f, 0.5f, NULL, 1.2f, 1.06f);
		break;
	case 2:		/* Full Screen */
		C2D_DrawImageAt(ScreenImageR, 0.0f, 0.0f, 0.5f, NULL, 1.46f, 1.06f);
		break;
	case 3:		/* Keep Aspect */
		C2D_DrawImageAt(ScreenImageR, 45.0f, 0.0f, 0.5f, NULL, 1.06f, 1.06f);
		break;
	case 4:		/* ExtremelyLarge */
		C2D_DrawImageAt(ScreenImageR, -12.0f, -21.0f, 0.5f, NULL, 1.56f, 1.24f);
		break;
	default:
		break;
	}

	C3D_FrameEnd(0);
}
#endif // USE_3D

#ifdef VDP_V9990
void PutImageSuperImpose(void)
{
	if (V9990Active)
	{
		if (V9KScrMode != OldV9KScrMode)
		{
			OldV9KScrMode = V9KScrMode;
			SetupWideScreen(false);
		}
	}
	else
		if (ScrMode != OldScrMode)
		{
			OldScrMode = ScrMode;
			SetupWideScreen(false);
		}
	const int dwidth = WIDTH, dheight = HEIGHT;
	const int swidth = 512, sheight = 256;
	int i, j, vx0, vx1, v0;

	u32* srcbuf0, * srcbuf1, * srcbuf2, * srcbuf3, *srcbuf4, *srcbuf5, *srcbuf6, *srcbuf7;

	u32* scrbuf = (u32*)ScreenTex.data;
	u32* scrtexbf;

	for (j = 0; j < dheight; j += 4)
	{
		v0 = (j >> 3) * (swidth >> 3);
		vx1 = (j * dwidth) >> 1;
		srcbuf0 = (u32*)XBuf + vx1;
		srcbuf1 = srcbuf0 + (dwidth >> 1);
		srcbuf2 = srcbuf1 + (dwidth >> 1);
		srcbuf3 = srcbuf2 + (dwidth >> 1);

		srcbuf4 = (u32*)V9KXBuf + vx1;
		srcbuf5 = srcbuf4 + (dwidth >> 1);
		srcbuf6 = srcbuf5 + (dwidth >> 1);
		srcbuf7 = srcbuf6 + (dwidth >> 1);

		for (i = 0; i < (dwidth >> 3); ++i, srcbuf0 += 4, srcbuf1 += 4, srcbuf2 += 4, srcbuf3 += 4, srcbuf4 += 4, srcbuf5 +=4, srcbuf6 +=4, srcbuf7 +=4)
		{
			vx0 = (((v0 + i) << 6) + (((i << 3) & 0x04) << 2) | ((j & 0x04) << 3));

			scrtexbf = scrbuf + (vx0 >> 1);

			scrtexbf[0] = srcbuf4[0] ? srcbuf4[0] : srcbuf0[0];
			scrtexbf[1] = srcbuf5[0] ? srcbuf5[0] : srcbuf1[0];
			scrtexbf[2] = srcbuf4[1] ? srcbuf4[1] : srcbuf0[1];
			scrtexbf[3] = srcbuf5[1] ? srcbuf5[1] : srcbuf1[1];
			scrtexbf[4] = srcbuf6[0] ? srcbuf6[0] : srcbuf2[0];
			scrtexbf[5] = srcbuf7[0] ? srcbuf7[0] : srcbuf3[0];
			scrtexbf[6] = srcbuf6[1] ? srcbuf6[1] : srcbuf2[1];
			scrtexbf[7] = srcbuf7[1] ? srcbuf7[1] : srcbuf3[1];

			scrtexbf[8] = srcbuf4[2] ? srcbuf4[2] :srcbuf0[2];
			scrtexbf[9] = srcbuf5[2] ? srcbuf5[2] : srcbuf1[2];
			scrtexbf[10] = srcbuf4[3] ? srcbuf4[3] : srcbuf0[3];
			scrtexbf[11] = srcbuf5[3] ? srcbuf5[3] : srcbuf1[3];
			scrtexbf[12] = srcbuf6[2] ? srcbuf6[2] : srcbuf2[2];
			scrtexbf[13] = srcbuf7[2] ? srcbuf7[2] : srcbuf3[2];
			scrtexbf[14] = srcbuf6[3] ? srcbuf6[3] : srcbuf2[3];
			scrtexbf[15] = srcbuf7[3] ? srcbuf7[3] : srcbuf3[3];
		}
	}

	C3D_FrameBegin(C3D_FRAME_NONBLOCK);
	C2D_TargetClear(TopRenderTarget, C2D_Color32(0, 0, 0, 255));

	C2D_SceneBegin(TopRenderTarget);
	switch (ScreenRes)
	{
	case 0:		/* No Scale */
		C2D_DrawImageAt(ScreenImage, 65.0f, 10.0f, 1.0f, NULL, 1.0f, 1.0f);
		break;
	case 1:		/* Wide */
		C2D_DrawImageAt(ScreenImage, 37.0f, 0.0f, 0.5f, NULL, 1.2f, 1.06f);
		break;
	case 2:		/* Full Screen */
		C2D_DrawImageAt(ScreenImage, 0.0f, 0.0f, 0.5f, NULL, 1.46f, 1.06f);
		break;
	case 3:		/* Keep Aspect */
		C2D_DrawImageAt(ScreenImage, 55.0f, 0.0f, 0.5f, NULL, 1.06f, 1.06f);
		break;
	case 4:		/* ExtremelyLarge */
		C2D_DrawImageAt(ScreenImage, -12.0f, -21.0f, 0.5f, NULL, 1.56f, 1.24f);
		break;
	default:
		break;
	}
	if (IsShowFPS)
	{
		DrawText(std::to_string(fpsval).c_str(), 350, 0, 1.0f, 0.5f, 0.5f, C2D_Color32(255, 255, 255, 255));
		DrawText(("Skip" + std::to_string(4 - (UPeriod / 25))).c_str(), 310, 0, 1.0f, 0.5f, 0.5f, C2D_Color32(255, 255, 255, 255));
	}
#ifdef _MSX0
	if (UseMSX0)
	{
		if ((currJoyMode[0] >= HIDE_KEYBOARD) || (currJoyMode[1] >= HIDE_KEYBOARD))
		{
			C2D_TargetClear(BottomRenderTartget, C2D_Color32(0, 0, 0, 255));

			C2D_SceneBegin(BottomRenderTartget);
			C2D_DrawImageAt(ScreenImage, 0.0f, 0.0f, 1.0f, NULL, 1.0f, 1.0f);
			DrawHUD();
		}
	}
#endif // _MSX0

	C3D_FrameEnd(0);
}
#ifdef DUAL_SCREEN
void PutImageDualScreen(void)
{
	if (V9990Active)
	{
		if (V9KScrMode != OldV9KScrMode)
		{
			OldV9KScrMode = V9KScrMode;
			SetupWideScreen(false);
		}
	}
	else
		if (ScrMode != OldScrMode)
		{
			OldScrMode = ScrMode;
			SetupWideScreen(false);
		}
	const int dwidth = WIDTH, dheight = HEIGHT;
	const int swidth = 512, sheight = 256;
	int i, j, vx0, vx1, v0;

	u32* srcbuf0, * srcbuf1, * srcbuf2, * srcbuf3, * srcbuf4, * srcbuf5, * srcbuf6, * srcbuf7;

	u32* scrbuf = (u32*)ScreenTex.data;
	u32* scrtexbf;
	u32* scrbuf2 = (u32*)ScreenTexBottom.data;
	u32* scrtexbf2;

	for (j = 0; j < dheight; j += 4)
	{
		v0 = (j >> 3) * (swidth >> 3);
		vx1 = (j * dwidth) >> 1;
		srcbuf0 = (u32*)V9KXBuf + vx1;
		srcbuf1 = srcbuf0 + (dwidth >> 1);
		srcbuf2 = srcbuf1 + (dwidth >> 1);
		srcbuf3 = srcbuf2 + (dwidth >> 1);

		srcbuf4 = (u32*)XBuf + vx1;
		srcbuf5 = srcbuf4 + (dwidth >> 1);
		srcbuf6 = srcbuf5 + (dwidth >> 1);
		srcbuf7 = srcbuf6 + (dwidth >> 1);

		for (i = 0; i < (dwidth >> 3); ++i, srcbuf0 += 4, srcbuf1 += 4, srcbuf2 += 4, srcbuf3 += 4, srcbuf4 += 4, srcbuf5 += 4, srcbuf6 += 4, srcbuf7 += 4)
		{
			vx0 = (((v0 + i) << 6) + (((i << 3) & 0x04) << 2) | ((j & 0x04) << 3));

			scrtexbf = scrbuf + (vx0 >> 1);
			scrtexbf2 = scrbuf2 + (vx0 >> 1);

			scrtexbf[0] = srcbuf0[0];
			scrtexbf[1] = srcbuf1[0];
			scrtexbf[2] = srcbuf0[1];
			scrtexbf[3] = srcbuf1[1];
			scrtexbf[4] = srcbuf2[0];
			scrtexbf[5] = srcbuf3[0];
			scrtexbf[6] = srcbuf2[1];
			scrtexbf[7] = srcbuf3[1];

			scrtexbf[8] = srcbuf0[2];
			scrtexbf[9] = srcbuf1[2];
			scrtexbf[10] = srcbuf0[3];
			scrtexbf[11] = srcbuf1[3];
			scrtexbf[12] = srcbuf2[2];
			scrtexbf[13] = srcbuf3[2];
			scrtexbf[14] = srcbuf2[3];
			scrtexbf[15] = srcbuf3[3];

			scrtexbf2[0] = srcbuf4[0];
			scrtexbf2[1] = srcbuf5[0];
			scrtexbf2[2] = srcbuf4[1];
			scrtexbf2[3] = srcbuf5[1];
			scrtexbf2[4] = srcbuf6[0];
			scrtexbf2[5] = srcbuf7[0];
			scrtexbf2[6] = srcbuf6[1];
			scrtexbf2[7] = srcbuf7[1];

			scrtexbf2[8] = srcbuf4[2];
			scrtexbf2[9] = srcbuf5[2];
			scrtexbf2[10] = srcbuf4[3];
			scrtexbf2[11] = srcbuf5[3];
			scrtexbf2[12] = srcbuf6[2];
			scrtexbf2[13] = srcbuf7[2];
			scrtexbf2[14] = srcbuf6[3];
			scrtexbf2[15] = srcbuf7[3];
		}
	}
	C3D_FrameBegin(C3D_FRAME_NONBLOCK);
	C2D_TargetClear(TopRenderTarget, C2D_Color32(0, 0, 0, 255));

	C2D_SceneBegin(TopRenderTarget);
	switch (ScreenRes)
	{
	case 0:		/* No Scale */
		C2D_DrawImageAt(ScreenImage, 65.0f, 10.0f, 1.0f, NULL, 1.0f, 1.0f);
		break;
	case 1:		/* Wide */
		C2D_DrawImageAt(ScreenImage, 37.0f, 0.0f, 0.5f, NULL, 1.2f, 1.06f);
		break;
	case 2:		/* Full Screen */
		C2D_DrawImageAt(ScreenImage, 0.0f, 0.0f, 0.5f, NULL, 1.46f, 1.06f);
		break;
	case 3:		/* Keep Aspect */
		C2D_DrawImageAt(ScreenImage, 55.0f, 0.0f, 0.5f, NULL, 1.06f, 1.06f);
		break;
	case 4:		/* ExtremelyLarge */
		C2D_DrawImageAt(ScreenImage, -12.0f, -21.0f, 0.5f, NULL, 1.56f, 1.24f);
		break;
	default:
		break;
	}
	C2D_TargetClear(BottomRenderTartget, C2D_Color32(0, 0, 0, 255));
	C2D_SceneBegin(BottomRenderTartget);
	C2D_DrawImageAt(ScreenImageBottom, 0.0f, 0.0f, 1.0f, NULL, 1.18f, 1.06f);
	C3D_FrameEnd(0);
}
#endif // DUAL_SCREEN
#endif // VDP_V9990


void  SetupWideScreen(bool isWide)
{
	gfxSetWide(isWide);
	if (isWide)
	{
		C3D_RenderTargetSetOutput(WideRenderTarget, GFX_TOP, GFX_LEFT, (GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
			GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
			GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO)));
	}
	else
	{
		C3D_RenderTargetSetOutput(TopRenderTarget, GFX_TOP, GFX_LEFT, (GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
			GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
			GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO)));
	}
	IsWide = isWide;
}


void WideScreenOn()
{
	SetupWideScreen(true);
}


void WideScreenOff()
{
	SetupWideScreen(false);
}


void SetScreenFilter(void)
{
	//C3D_TexSetFilter(&WideTex, GPU_LINEAR, GPU_NEAREST);
	if (!ScreenFilter)
	{
		C3D_TexSetFilter(&ScreenTex, GPU_NEAREST, GPU_NEAREST);
		C3D_TexSetFilter(&WideTex, GPU_NEAREST, GPU_NEAREST);
	}
	else
	{
		C3D_TexSetFilter(&ScreenTex, GPU_LINEAR, GPU_LINEAR);
		C3D_TexSetFilter(&WideTex, GPU_LINEAR, GPU_LINEAR);
	}
}


/* Wrapper function */
void DoPutImage()
{
	PutImage();
}


/** PlayAllSound() *******************************************/
/** Render and play given number of microseconds of sound.  **/
/*************************************************************/
void PlayAllSound(int uSec)
{
  /* @@@ Twice the argument to avoid skipping */
  //RenderAndPlayAudio(2*uSec*UseSound/1000000);
}

/** Joystick() ***********************************************/
/** Query positions of two joystick connected to ports 0/1. **/
/** Returns 0.0.B2.A2.R2.L2.D2.U2.0.0.B1.A1.R1.L1.D1.U1.    **/
/*************************************************************/
unsigned int Joystick(void)
{
	return(JoyState);
	
}


void Keyboard(void)
{
	int ret;
	unsigned int buttons;

	hidScanInput();
	u32 kDown = hidKeysDown();
	u32 kUp = hidKeysUp();
	u32 kHeld = hidKeysHeld();

	if (TurboNow)
	{
		if (!kDown)return;
		TurboNow = 0;
		LoadOption(false);
		DrawKeyboard3DS();

		StartMenu();
		systemMenu();
		EndMenu();
	}

	hidTouchRead(&tp);
	int px = tp.px, py = tp.py;
	if (!(kDown & KEY_TOUCH) && !(kUp & KEY_TOUCH))
	{
		if (IsScreenShot)
		{
			if ((px != 0 && px != oldtp.px) || (py != 0 && py != oldtp.py))
			{
				if (IsSmallScrShot)
				{
					if (py > 150)
					{
						MoveScreenShot(px, py);
						oldtp.px = px;
						oldtp.py = py;
					}
				}
				else
				{
					MoveScreenShot(px, py);
					oldtp.px = px;
					oldtp.py = py;
				}
			}
		}
		else if ((currJoyMode[0] >= HIDE_KEYBOARD) || currJoyMode[1] >= HIDE_KEYBOARD)
		{
			if ((px != 0 && px != oldtp.px) || (py != 0 && py != oldtp.py))
			{
				oldtp.px = px;
				oldtp.py = py;
			}
		}
	}
	if (kDown & KEY_TOUCH)
	{
		kDown &= ~KEY_TOUCH;
#ifdef VDP_V9990
		if (V9990Dual)
		{
			StartMenu();
			if (BrowseOK("Do you want to end Dual Screen Mode?", "")==true)
			{
				V9990Dual &= 0xFD;
			}
			EndMenu();
			LoadOption(false);
			DrawKeyboard3DS();
			return;
		}
#endif // VDP_V9990
		SoftwareKeyboardCheck(true, px, py);
		oldtp.px = px;
		oldtp.py = py;
	}
	else if (kUp & KEY_TOUCH)
	{
		kUp &= ~KEY_TOUCH;
		SoftwareKeyboardCheck(false, oldtp.px, oldtp.py);
	}

	if ((kDown & KEY_UP) || (kHeld & KEY_UP))
	{
		if (KeyMaps3DS[0] == 0)JoyState |= JST_UP;
		else XKBD_SET(KeyMaps3DS[0]);
	}
	if ((kDown & KEY_DOWN) || (kHeld & KEY_DOWN))
	{
		if (KeyMaps3DS[1] == 0)JoyState |= JST_DOWN;
		else XKBD_SET(KeyMaps3DS[1]);
	}
	if ((kDown & KEY_LEFT) || (kHeld & KEY_LEFT))
	{
		if (KeyMaps3DS[2] == 0)JoyState |= JST_LEFT;
		else XKBD_SET(KeyMaps3DS[2]);
	}
	if ((kDown & KEY_RIGHT) || (kHeld & KEY_RIGHT))
	{
		if (KeyMaps3DS[3] == 0)JoyState |= JST_RIGHT;
		else XKBD_SET(KeyMaps3DS[3]);
	}
	if (kDown & KEY_A)
	{
		if (KeyMaps3DS[8] == 0)JoyState |= JST_FIREA;
		else XKBD_SET(KeyMaps3DS[8]);
	}
	if (kDown & KEY_B)
	{
		if (KeyMaps3DS[9] == 0)JoyState |= JST_FIREB;
		else XKBD_SET(KeyMaps3DS[9]);
	}
	if ((kDown & KEY_X) && KeyMaps3DS[10] != 0)XKBD_SET(KeyMaps3DS[10]);
	if ((kDown & KEY_Y) && KeyMaps3DS[11] != 0)XKBD_SET(KeyMaps3DS[11]);
	if ((kDown & KEY_L) && KeyMaps3DS[12] != 0)
	{
		if (currJoyMode[0] >= HIDE_KEYBOARD)JoyState |= JST_FIREA;
		else if (currJoyMode[1] >= HIDE_KEYBOARD)JoyState |= 0x1000;
		else XKBD_SET(KeyMaps3DS[12]);
	}
	if ((kDown & KEY_R) && KeyMaps3DS[13] != 0)
	{
		if ((CartSpecial[0] == CART_ARKANOID) || (CartSpecial[1] == CART_ARKANOID))JoyState |= JST_FIREA;
		else if (currJoyMode[0] >= HIDE_KEYBOARD)JoyState |= JST_FIREB;
		else if (currJoyMode[1] >= HIDE_KEYBOARD)JoyState |= 0x2000;
		else XKBD_SET(KeyMaps3DS[13]);
	}
	if ((kDown & KEY_SELECT) && KeyMaps3DS[15] != 0)XKBD_SET(KeyMaps3DS[15]);
	if ((kDown & KEY_ZL) && KeyMaps3DS[16] != 0)XKBD_SET(KeyMaps3DS[16]);
	if ((kDown & KEY_ZR) && KeyMaps3DS[17] != 0)XKBD_SET(KeyMaps3DS[17]);

	if (kUp & KEY_UP)
	{
		if (KeyMaps3DS[0] == 0)JoyState &= ~JST_UP;
		else XKBD_RES(KeyMaps3DS[0]);
	}
	if (kUp & KEY_DOWN)
	{
		if (KeyMaps3DS[1] == 0)JoyState &= ~JST_DOWN;
		else XKBD_RES(KeyMaps3DS[1]);
	}
	if (kUp & KEY_LEFT)
	{
		if (KeyMaps3DS[2] == 0)JoyState &= ~JST_LEFT;
		else XKBD_RES(KeyMaps3DS[2]);
	}
	if (kUp & KEY_RIGHT)
	{
		if (KeyMaps3DS[3] == 0)JoyState &= ~JST_RIGHT;
		else XKBD_RES(KeyMaps3DS[3]);
	}
	if (kUp & KEY_A)
	{
		if (KeyMaps3DS[8] == 0)JoyState &= ~JST_FIREA;
		else XKBD_RES(KeyMaps3DS[8]);
	}
	if (kUp & KEY_B)
	{
		if (KeyMaps3DS[9] == 0)JoyState &= ~JST_FIREB;
		else XKBD_RES(KeyMaps3DS[9]);
	}
	if ((kUp & KEY_X) && KeyMaps3DS[10] != 0)XKBD_RES(KeyMaps3DS[10]);
	if ((kUp & KEY_Y) && KeyMaps3DS[11] != 0)XKBD_RES(KeyMaps3DS[11]);
	if ((kUp & KEY_L) && KeyMaps3DS[12] != 0)
	{
		//if(CartSpecial==CART_ARKANOID)JoyState &= ~JST_FIREA;
		if (currJoyMode[0] >= HIDE_KEYBOARD)JoyState &= ~JST_FIREA;
		else if (currJoyMode[1] >= HIDE_KEYBOARD)JoyState &= ~0x1000;
		else XKBD_RES(KeyMaps3DS[12]);
	}
	if ((kUp & KEY_R) && KeyMaps3DS[13] != 0)
	{
		if ((CartSpecial[0] == CART_ARKANOID) || CartSpecial[1] == CART_ARKANOID)JoyState &= ~JST_FIREA;
		else if (currJoyMode[0] >= HIDE_KEYBOARD)JoyState &= ~JST_FIREB;
		else if (currJoyMode[1] >= HIDE_KEYBOARD)JoyState &= ~0x2000;
		else XKBD_RES(KeyMaps3DS[13]);
	}
	if ((kUp & KEY_SELECT) && KeyMaps3DS[15] != 0)XKBD_RES(KeyMaps3DS[15]);
	if ((kUp & KEY_ZL) && KeyMaps3DS[16] != 0)XKBD_RES(KeyMaps3DS[16]);
	if ((kUp & KEY_ZR) && KeyMaps3DS[17] != 0)XKBD_RES(KeyMaps3DS[17]);

	if ((kHeld & KEY_A) || rapidCounts[0] > 0)checkRapidFire(0);
	if ((kHeld & KEY_B) || rapidCounts[1] > 0)checkRapidFire(1);
	if ((kHeld & KEY_X) || rapidCounts[2] > 0)checkRapidFire(2);
	if ((kHeld & KEY_Y) || rapidCounts[3] > 0)checkRapidFire(3);
	if ((kHeld & KEY_L) || rapidCounts[4] > 0)checkRapidFire(4);
	if ((kHeld & KEY_R) || rapidCounts[5] > 0)checkRapidFire(5);
	if ((kHeld & KEY_START) || rapidCounts[6] > 0)checkRapidFire(6);
	if ((kHeld & KEY_SELECT) || rapidCounts[7] > 0)checkRapidFire(7);
	if ((kHeld & KEY_ZL) || rapidCounts[8] > 0)checkRapidFire(8);
	if ((kHeld & KEY_ZR) || rapidCounts[9] > 0)checkRapidFire(9);

	u8 isShell;
	PTMU_GetShellState(&isShell);
	if (isShellOpen == false && (isShell))
	{
		SetWait();
		isShellOpen = true;
	}
	if (!isShell)isShellOpen = false;
	if (isShellOpen==false)
	{
		StartMenu();
		systemMenu();
		EndMenu();
	}

	if (kDown & KEY_START)
	{
		StartMenu();
		systemMenu();
		EndMenu();
		//JoyState = 0;
		//DrawKeyboard3DS();
		//SetWait();
		//SDL_PauseAudio(0);
	}
	memcpy((void*)KeyState, (const void*)XKeyState, sizeof(KeyState));

	if (aptCheckHomePressRejected())
	{
		StartMenu();
		if (BrowseOK("You press the Home Button.", "Do you want to Quit to home menu?") == true)
		{
			ExitNow = 1;
			return;
		}
		EndMenu();
		//ReDrawKeyboard();
	}
}


void checkRapidFire(int idx)
{
	if (rapidInterVals[idx] == 0 && rapidCounts[idx] == 0)return;
	int rapidi = rapidCounts[idx];
	if (rapidi == 0)
	{
		if (KeyMaps3DS[8+idx] == 0)
		{
			switch (idx)
			{
			case 0:
				JoyState |= JST_FIREA;
			case 1:
				JoyState |= JST_FIREB;
			default:
				break;
			}
		}
		else XKBD_SET(KeyMaps3DS[idx]);
	}
	rapidi++;
	if (rapidi > rapidInterVals[idx])
	{
		rapidi = 0;
		if (KeyMaps3DS[8+idx] == 0)
		{
			switch (idx)
			{
			case 0:
				JoyState &= ~JST_FIREA;
				break;
			case 1:
				JoyState &= ~JST_FIREB;
				break;
			default:
				break;
			}
		}
		else XKBD_RES(KeyMaps3DS[8 + idx]);
	}
	rapidCounts[idx] = rapidi;
}


void SoftwareKeyboardCheck(bool flag0, int px, int py)
{
	if ((currJoyMode[0] >= HIDE_KEYBOARD) || (currJoyMode[1] >= HIDE_KEYBOARD) || IsScreenShot)
	{
		if (px > 285)
		{
			if (py > 198 && py < 240)
			{
				IsScreenShot = false;
				IsSmallScrShot = false;
				currJoyMode[0] = 1;
				SETJOYTYPE(0, 1);
				if (currJoyMode[1] != 1)
				{
					currJoyMode[1] = 0;
					SETJOYTYPE(1, 0);
				}
				DrawKeyboard3DS();
			}
			if (IsScreenShot)
			{
				if (py > 155 && py < 185)
				{
					IsScreenShot = false;
					IsSmallScrShot = false;
					if ((JoyMode[0] < HIDE_KEYBOARD) && (JoyMode[1] >= HIDE_KEYBOARD))
					{
						currJoyMode[1] = JoyMode[1];
						SETJOYTYPE(1, JoyMode[1]);
					}
					else if (JoyMode[0] >= HIDE_KEYBOARD)
					{
						currJoyMode[0] = JoyMode[0];
						SETJOYTYPE(0, JoyMode[0]);
					}
					else
					{
						currJoyMode[0] = 3;
						SETJOYTYPE(0, 3);
					}
					DrawMouseScr();
				}
			}
		}
	}
	if ((currJoyMode[0] < HIDE_KEYBOARD) && (currJoyMode[1] < HIDE_KEYBOARD) && (!IsScreenShot || IsSmallScrShot))
	{
		if (py < 20)
		{
			for (int i = 0; i < 30; i += 3)
			{
				if (px >= KeyLayout1Line[i] && px < KeyLayout1Line[i + 1])
				{
					if (flag0)XKBD_SET(KeyLayout1Line[i + 2]);
					else XKBD_RES(KeyLayout1Line[i + 2]);
					break;
				}
			}
		}
		else if (py >= 20 && py < 41)
		{
			for (int i = 0; i < 45; i += 3)
			{
				if (px >= KeyLayout2Line[i] && px < KeyLayout2Line[i + 1])
				{
					if (flag0)XKBD_SET(KeyLayout2Line[i + 2]);
					else XKBD_RES(KeyLayout2Line[i + 2]);
					break;
				}
			}
		}
		else if (py >= 41 && py < 62)
		{
			for (int i = 0; i < 42; i += 3)
			{
				if (px >= KeyLayout3Line[i] && px < KeyLayout3Line[i + 1])
				{
					if (flag0)XKBD_SET(KeyLayout3Line[i + 2]);
					else XKBD_RES(KeyLayout3Line[i + 2]);
					break;
				}
			}
		}
		else if (py >= 62 && py < 83)
		{
			for (int i = 0; i < 42; i += 3)
			{
				if (px >= KeyLayout4Line[i] && px < KeyLayout4Line[i + 1])
				{
					if (KeyLayout4Line[i + 2] == KBD_CONTROL)
					{
						if (flag0)
						{
							if (KeyModes & CON_CONTROL)
							{
								XKBD_RES(KBD_CONTROL);
								KeyModes &= ~CON_CONTROL;
							}
							else
							{
								XKBD_SET(KBD_CONTROL);
								KeyModes |= CON_CONTROL;
							}
						}
					}
					else if (flag0)XKBD_SET(KeyLayout4Line[i + 2]);
					else XKBD_RES(KeyLayout4Line[i + 2]);
					break;
				}
			}
		}
		else if (py >= 83 && py < 104)
		{
			for (int i = 0; i < 39; i += 3)
			{
				if (px >= KeyLayout5Line[i] && px < KeyLayout5Line[i + 1])
				{
					if (KeyLayout5Line[i + 2] == KBD_SHIFT)
					{
						if (flag0)
						{
							if (KeyModes & CON_SHIFT)
							{
								XKBD_RES(KBD_SHIFT);
								KeyModes &= ~CON_SHIFT;
							}
							else
							{
								XKBD_SET(KBD_SHIFT);
								KeyModes |= CON_SHIFT;
							}
						}
					}
					else if (flag0)XKBD_SET(KeyLayout5Line[i + 2]);
					else XKBD_RES(KeyLayout5Line[i + 2]);
					break;
				}
			}
		}
		else if (py >= 104 && py < 126 && px < 253)
		{
			for (int i = 0; i < 12; i += 3)
			{
				if (px >= KeyLayout6Line[i] && px < KeyLayout6Line[i + 1])
				{
					if (KeyLayout6Line[i + 2] == KBD_COUNTRY)
					{
						IsKanaReady = true;
						if (!RdZ80(0x2C))
						{
							if (flag0)XKBD_SET(KBD_COUNTRY);
							else XKBD_RES(KBD_COUNTRY);
						}
						else
						{
							if (flag0)
							{
								if (KeyModes & CON_ALT)
								{
									XKBD_RES(KBD_COUNTRY);
									KeyModes &= ~CON_ALT;
								}
								else
								{
									XKBD_SET(KBD_COUNTRY);
									KeyModes |= CON_ALT;
								}
							}
						}
					}
					else if (KeyLayout6Line[i + 2] == KBD_CAPSLOCK)
					{
						IsCapsReady = true;
						if (flag0)XKBD_SET(KBD_CAPSLOCK);
						else XKBD_RES(KBD_CAPSLOCK);
					}
					else if (KeyLayout6Line[i + 2] == KBD_GRAPH)
					{
						if (flag0)
						{
							if (KeyModes & CON_F11)
							{
								XKBD_RES(KBD_GRAPH);
								KeyModes &= ~CON_F11;
							}
							else
							{
								XKBD_SET(KBD_GRAPH);
								KeyModes |= CON_F11;
							}
						}
					}
					else if (flag0)XKBD_SET(KeyLayout6Line[i + 2]);
					else XKBD_RES(KeyLayout6Line[i + 2]);
					break;
				}
			}
		}
		else if (py >= 104 && py < 147 && px >= 253)
		{
			if (px >= 272 && px < 295)
			{
				if (py >= 126)
				{
					if (flag0)XKBD_SET(KBD_DOWN);
					else XKBD_RES(KBD_DOWN);
				}
				else
				{
					if (flag0)XKBD_SET(KBD_UP);
					else XKBD_RES(KBD_UP);
				}
			}
			else if (py >= 116 && py < 138)
			{
				if (px >= 253 && px < 274)
				{
					if (flag0)XKBD_SET(KBD_LEFT);
					else XKBD_RES(KBD_LEFT);
				}
				else if (px >= 294 && px < 315)
				{
					if (flag0)XKBD_SET(KBD_RIGHT);
					else XKBD_RES(KBD_RIGHT);
				}
			}
		}
		else if (py >= 150 && py < 180 && px >= 182 && px <= 265 && !IsSmallScrShot)
		{
			for (int i = 0; i < 12; i += 3)
			{
				if (px >= KeyLayout7Line[i] && px < KeyLayout7Line[i + 1])
				{
					if (flag0)XKBD_SET(KeyLayout7Line[i + 2]);
					else XKBD_RES(KeyLayout7Line[i + 2]);
					break;
				}
			}
		}
		else if (py >= 180 && py < 200 && px >= 182 && px <= 265 && !IsSmallScrShot)
		{
			for (int i = 0; i < 12; i += 3)
			{
				if (px >= KeyLayout8Line[i] && px < KeyLayout8Line[i + 1])
				{
					if (flag0)XKBD_SET(KeyLayout8Line[i + 2]);
					else XKBD_RES(KeyLayout8Line[i + 2]);
					break;
				}
			}
		}
		else if (py >= 200 && py < 220 && px >= 182 && px <= 265 && !IsSmallScrShot)
		{
			for (int i = 0; i < 12; i += 3)
			{
				if (px >= KeyLayout9Line[i] && px < KeyLayout9Line[i + 1])
				{
					if (flag0)XKBD_SET(KeyLayout9Line[i + 2]);
					else XKBD_RES(KeyLayout9Line[i + 2]);
					break;
				}
			}
		}
		else if (py >= 220 && py < 240 && px >= 182 && px <= 265 && !IsSmallScrShot)
		{
			for (int i = 0; i < 12; i += 3)
			{
				if (px >= KeyLayout10Line[i] && px < KeyLayout10Line[i + 1])
				{
					if (flag0)XKBD_SET(KeyLayout10Line[i + 2]);
					else XKBD_RES(KeyLayout10Line[i + 2]);
					break;
				}
			}
		}
		else if (flag0 && !IsSmallScrShot)
		{
			if (py >= 158 && py < 183)
			{
				if (px >= 4 && px < 37)
				{
					StartMenu();
					BrowseROM(0, BROWSE_ROM);
					EndMenu();
					//ReDrawKeyboard();
					return;
				}
				else if (px >= 45 && px < 78)
				{
					StartMenu();
					BrowseROM(1, BROWSE_ROM);
					EndMenu();
					//ReDrawKeyboard();
					return;
				}
				else if (px >= 92 && px < 125)
				{
					StartMenu();
					BrowseROM(0, BROWSE_DISK);
					EndMenu();
					//ReDrawKeyboard();
					return;
				}
				else if (px >= 130 && px < 163)
				{
					StartMenu();
					BrowseROM(1, BROWSE_DISK);
					EndMenu();
					//ReDrawKeyboard();
					return;
				}
			}
			else if (py >= 200)
			{
				if (px >= 4 && px < 37)
				{
					StartMenu();
					systemMenu();
					EndMenu();
					//JoyState = 0;
					//ReDrawKeyboard();
					return;
				}
				else if (px >= 45 && px < 78)
				{
					StartMenu();
					BrowseReset();
					EndMenu();
					//ReDrawKeyboard();
					return;
				}
				else if (px > 92 && px < 125)
				{
					StartMenu();
					if (BrowseOK("Do You want to Quit?", NULL) == true)
					{
						ExitNow = 1;
						return;
					}
					EndMenu();
					//ReDrawKeyboard();
				}
				else if (px >= 130 && px <= 163)
				{
					StartMenu();
					BrowseROM(0, BROWSE_TAPE);
					EndMenu();
					//ReDrawKeyboard();
					return;
				}
			}
		}
		if (px > 285 && py > 155 && py < 185)
		{
			IsScreenShot = false;
			IsSmallScrShot = false;
			if ((JoyMode[0] < HIDE_KEYBOARD) && (JoyMode[1] >= HIDE_KEYBOARD))
			{
				currJoyMode[1] = JoyMode[1];
				SETJOYTYPE(1, JoyMode[1]);
			}
			else if (JoyMode[0] >= HIDE_KEYBOARD)
			{
				currJoyMode[0] = JoyMode[0];
				SETJOYTYPE(0, JoyMode[0]);
			}
			else
			{
				currJoyMode[0] = 3;
				SETJOYTYPE(0, 3);
			}
			//currJoyMode[0] = 3;
			//SETJOYTYPE(0, 3);
			DrawMouseScr();
		}
	}
	if ((currJoyMode[0] < HIDE_KEYBOARD) && (currJoyMode[1] < HIDE_KEYBOARD))DrawKeyboard3DS();
}


//void ReDrawKeyboard()
//{
//	SetWait();
//	if ((currJoyMode[0] < HIDE_KEYBOARD) && (currJoyMode[1] < HIDE_KEYBOARD))
//	{
//		DrawKeyboard3DS();
//	}
//}


void DrawKeyboard3DS()
{
	if (TurboNow)return;
	if (V9990Dual & 0x02)return;
	if (IsScreenShot)
	{
		if(!IsSmallScrShot)DrawMouseScr();
		DrawScreenShotScr();
		return;
	}
	if ((currJoyMode[0] >= HIDE_KEYBOARD) || (currJoyMode[1] >= HIDE_KEYBOARD))
	{
		DrawMouseScr();
		return;
	}
	bool IsShift = KeyModes & CON_SHIFT ? true : false;
	bool IsGraph = KeyModes & CON_F11 ? true : false;
	bool IsCTRL = KeyModes & CON_CONTROL ? true : false;
	bool IsCaps = IsCapsReady ? RdZ80(0xFCAB) : false;
	bool IsKana = IsJpKey ? (IsKanaReady ? RdZ80(0xFCAC) ? true : RdZ80(0xFAFC) & 0x01 : false) : (KeyModes & CON_ALT ? true : false);
	DrawKeyboardScr(IsKana, IsCaps, IsShift, IsGraph, IsCTRL);
}


void StartMenu()
{
	SDL_PauseAudio(1);
}


void EndMenu()
{
	SetWait();
	DrawKeyboard3DS();
	SDL_PauseAudio(0);
	JoyState = 0;
}


/** Mouse() **************************************************/
/** Query coordinates of a mouse connected to port N.       **/
/** Returns F2.F1.Y.Y.Y.Y.Y.Y.Y.Y.X.X.X.X.X.X.X.X.          **/
/*************************************************************/

unsigned int Mouse(byte N)
{
	int px, py;

	hidTouchRead(&tp);
	if (!tp.px && !tp.py)
	{
		OldX = 0;
		OldY = 0;
		MouseDX3DS[N] = 0;
		MouseDY3DS[N] = 0;
		return(MousePos[0] | MousePos[1] << 8);
	}
	else if(!OldX && !OldY)
	{
		OldX = tp.px;
		OldY = tp.py;
	}
	px = tp.px-OldX;
	py = tp.py-OldY;
	OldX = tp.px;
	OldY = tp.py;

	MousePos[0] += px;
	MousePos[1] += py;
	MousePos[0] = MousePos[0] & 0xFF;
	MousePos[1] = MousePos[1] & 0xFF;
	px = -px;
	py = -py;
	MouseDX3DS[N] = px < -127 ? -127 : px>127 ? 127 : px;
	MouseDY3DS[N] = py < -127 ? -127 : py>127 ? 127 : py;

	return(MousePos[0] | (MousePos[1] << 8));
}


unsigned int ArkanoidPaddle(byte N)
{
	int px, py;
	hidTouchRead(&tp);
	if (!tp.px && !tp.py)
	{
		OldX = 0;
		OldY = 0;
		return(ArkanoidPos);
	}
	else if (!OldX && !OldY)
	{
		OldX = tp.px;
	}
	px = tp.px - OldX;
	OldX = tp.px;
	ArkanoidPos += px;
	ArkanoidPos = ArkanoidPos < 152 ? 152 : ArkanoidPos > 309 ? 309 : ArkanoidPos;
	return ArkanoidPos;
}


unsigned int GetTouchPad(byte N)
{
	if ((currJoyMode[0] >= HIDE_KEYBOARD) || (currJoyMode[1] >= HIDE_KEYBOARD))
	{
		hidTouchRead(&tp);
		if (N == 0)		/* N==0:x  N==1:y */
		{
			return (tp.px > 255 ? 255 : tp.px);
		}
		return (tp.py > 255 ? 255 : tp.py);
	}
	else return (0);
}


char GetPad(char Val)
{
	int X, Y;
	char result = -1;
	u32 kDown = 0;

	switch (Val) {
	case 0:
	case 4:
	case 8:
		hidTouchRead(&tp);
		hidScanInput();
		kDown = hidKeysDown();
		if (kDown & KEY_A)PadButton = 0xFF;
		else PadButton = 0;
		if (!tp.px && !tp.py)
		{
			OldX = 0;
			OldY = 0;
			PadDX = 450;
			PadDY = 450;
		}
		else
		{
		if (OldX == 0 && OldY == 0)
		{
			OldX = tp.px -245;
			OldY = tp.py -245;
		}
		else
		{
			PadDX = tp.px - OldX + 245;
			PadDY = tp.py - OldY + 245;
			OldX = tp.px - 245;
			OldY = tp.py - 245;
		}
		}
		result = 0xFF;
		break;
	case 12:
	case 16:
		hidTouchRead(&tp);
		if (!tp.px && !tp.py)
		{
			OldX = 0;
			OldY = 0;
			PadDX = -256;
			PadDY = -256;
		}
		else
		{
			if (OldX == 0 && OldY == 0)
			{
				OldX = tp.px+128;
				OldY = tp.py+128;
			}
			else
			{
				PadDX = tp.px - OldX - 128;
				PadDY = tp.py - OldY - 128;
				OldX = tp.px+128;
				OldY = tp.py+128;
			}
		}
		result = 0xFF;
		break;
	case 3:
	case 7:
	case 11:
		result = PadButton;
		break;
	case 1:
	case 5:
	case 9:
	case 13:
	case 17:
		result = PadDX;
		break;
	case 2:
	case 6:
	case 10:
	case 14:
	case 18:
		result = PadDY;
		break;
	}
	return(result);
}


char GetPaddle(char Val)
{
	hidTouchRead(&tp);
	if (tp.px || tp.py)
	{
		PaddleVal = tp.px;
	}
	return PaddleVal;
}

/** SetColor() ***********************************************/
/** Set color N to (R,G,B).                                 **/
/*************************************************************/
void SetColor(byte N,byte R,byte G,byte B)
{
	unsigned int J;
	//J = (((int)(R / 8) << 11) | ((int)(G / 8) << 6) | (int)(B / 8));
	J = ((31 * (unsigned int)R / 255) << 11) | ((63 * (unsigned int)G / 255) << 5) | (31 * (unsigned int)B / 255);
	if (N) XPal[N] = J; else XPal0 = J;
}

/** HandleKeys() *********************************************/
/** Keyboard handler.                                       **/
/*************************************************************/
void HandleKeys(unsigned int Key)
{
  /* When in MenuMSX() or ConDebug(), ignore keypresses */
  if(InMenu) return;
  
  Key&=CON_KEYCODE;
  if(Key<128) XKBD_SET(Key);

  
}


void Reset3DS()
{
	IsCapsReady = false;
	IsKanaReady = false;
	ScreenShotOffx = 0;
	ScreenShotOffy = 0;
	IsScreenShot = false;
	IsImposeScreenShot = false;
	IsSmallScrShot = false;
	playYM2413 = 0;
	playY8950 = 0;
	DA1bit = 0;
	DA8bit = 0;
	ArkanoidPos = 236;
	SetWait();
}


void SetWait(void)
{
	//dt = (long)(1000000.0 / 60);
	float refrate = !regionid ? (PALVideo ? 50.158974 : 59.922743)
		: (VIDEO(MSX_PAL) ? 50.158974 : 59.922743);
	dt = (long)(1000000.0 / refrate);
	TimePerFrame = (u64)((u64)dt * 268123480 / 1000) / 1000;
	LineTimePerFrame = TimePerFrame / (PALVideo ? 312 : 261);
	AudioTimePerFrame = TimePerFrame / (refrate * 8);
	//AudioTimePerFrame = TimePerFrame / (refrate * 14) ;
	//AudioTimePerFrame = TimePerFrame / (refrate*4);
	LineStepTimePerFrame = LineTimePerFrame << waitStep;
	//TimePerFrame = (long)(1000000.0 / SyncFreq);
	NextTime = svcGetSystemTick() + TimePerFrame;
	NextLineTime = svcGetSystemTick() + LineTimePerFrame;
	NextAudioTime = svcGetSystemTick() + AudioTimePerFrame;
	NextLineStepTime = svcGetSystemTick() + LineStepTimePerFrame;
	//ScanTimePerFrame = TimePerFrame / 262;
	//NextScanLine = svcGetSystemTick() + ScanTimePerFrame;
}

//void ScanLineUpdate(void)
//{
//	u32 DiffScan = (NextScanLine - svcGetSystemTick());
//	if (DiffScan > 0)
//	{
//		ScanLine = ScanLine < (PALVideo ? 312 : 261) ? ScanLine + 1 : 0;
//	}
//	NextScanLine += ScanTimePerFrame;
//}


int WaitSync(void)
{
	if (OldPALVidedo != PALVideo)
	{
		OldPALVidedo = PALVideo;
		WaitCount = 0;
		SetWait();
	}
	if (IsFrameLimit == false)return 0;
	bool IsSkip = true;
	if(svcGetSystemTick() <=  NextTime)
	{
		IsSkip = false;
		while (aptMainLoop())
		{
			if (svcGetSystemTick() > NextTime)break;
			//if ((((currScanLine + VScroll) & 0xFF) - VDP[19]) & 0xFF == 2)break;

			//SyncAudio();
			VDPSync();
		}

	}
	NextTime += TimePerFrame;
	if (IsSkip == false)
	{
		WaitCount = 0;
		FPSNoDelay++;
	}
	else
	{
		WaitCount++;
		FPSDelay++;
		//if (WaitCount > 100)
		if (WaitCount > 0x400)
		{
			WaitCount = 0;
			SetWait();
		}
	}
	if (!IsSkip) return 0;
	else return 1;
}


int WaitSyncLine()
{
	//if (IsFrameLimit == false)return 0;
	if (IsFrameLimit == false)
	{
		//Move to LoopZ80() to improve speed.
		//svcSleepThread(1000);
		return 0;
	}
	bool IsSkip = true;
	//u64 DiffTime = (NextLineTime - svcGetSystemTick());
	//if (DiffTime > 0)
	if (svcGetSystemTick() <= NextLineTime)
	{
		IsSkip = false;
		while (aptMainLoop())
		{
			//svcSleepThread(1000);
			if (svcGetSystemTick() > NextLineTime)break;

			//SyncAudio();
			VDPSync();
		}
	}
	NextLineTime += LineTimePerFrame;
	if (IsSkip == false)
	{
		WaitCount = 0;
		FPSNoDelay++;
	}
	else
	{
		WaitCount++;
		FPSDelay++;
		if (WaitCount > 1000)
		{
			WaitCount = 0;
			SetWait();
		}
	}
	if (!IsSkip) return 0;
	else return 1;
}


int WaitSyncLineStep()
{
	if (IsFrameLimit == false)
	{
		return 0;
	}
	WaitStepCnt++;
	if (waitStep && (WaitStepCnt & ((1 << waitStep) - 1)) != 0)return 0;
	WaitStepCnt = 0;
	int audioloop = 0;
	bool IsSkip = true;
	if (svcGetSystemTick() <= NextLineStepTime)
	{
		IsSkip = false;
		//CalcAudio();
		//audioloop++;
		while (aptMainLoop())
		{
			if (svcGetSystemTick() > NextLineStepTime)break;
			////SyncAudio();
			//if (audioloop < 2)
			//{
			//	CalcAudio();
			//	audioloop++;
			//}
			VDPSync();
		}
	}
	NextLineStepTime += LineStepTimePerFrame;
	if (IsSkip == false)
	{
		WaitCount = 0;
		FPSNoDelay++;
	}
	else
	{
		WaitCount++;
		FPSDelay++;
		//if (WaitCount > 1000)
		if (WaitCount > 0x1000)
		{
			WaitCount = 0;
			SetWait();
		}
	}
	if (IsSkip)return 0;
	else return audioloop;
	//if (!IsSkip) return 0;
	//else return 1;
}


void CheckPALVideo()
{
	if (OldPALVidedo != PALVideo)
	{
		OldPALVidedo = PALVideo;
		WaitCount = 0;
		SetWait();
	}
}


void SetFirstLineTime()
{
	FirstLineTime = svcGetSystemTick();
}


void CalcFPS()
{
	fpscount++;
	if (fpscount < 10)return;
	fpscount = 0;

	u32 ctick = SDL_GetTicks();
	u32 oldtick = ticksarr[5];
	u32 oldtick2 = ticksarr[4];
	fpsval = (60000 / (ctick - oldtick) + 50000 / (ctick - oldtick2)) / 2;
	ticksarr[5] = ticksarr[4];
	ticksarr[4] = ticksarr[3];
	ticksarr[3] = ticksarr[2];
	ticksarr[2] = ticksarr[1];
	ticksarr[1] = ticksarr[0];
	ticksarr[0] = ctick;
	//if ((PALVideo && (fpsval < 48)) || (!PALVideo && (fpsval < 58)))UPeriod = UPeriod <= 25 ? 25 : UPeriod-25;
	//if ((PALVideo && (fpsval >= 50)) || (!PALVideo && (fpsval >= 59)))UPeriod = UPeriod >= 100 ? 100 : UPeriod + 25;
	if (oldtick < 1 || oldtick2 < 1)fpsval = 0;
}


void checkAutoFrameSkip()
{
	if (AutoFrameSkip == false)return;
	if (FrameSkipChangeCnt <=5)
	{
		FrameSkipChangeCnt++;
		return;
	}
	if (FPSDelay < FPSNoDelay)
	{
		UPeriod = UPeriod >= 100 ? 100 : UPeriod + 25;
		FrameSkipChangeCnt = 0;
	}
	if (FPSNoDelay == 0)
	{
		//UPeriod = UPeriod <= 25 ? 25 : UPeriod - 25;
		if ((PALVideo && (fpsval < 49)) || (!PALVideo && (fpsval < 59)))UPeriod = UPeriod <= 25 ? 25 : UPeriod - 25;
		FrameSkipChangeCnt = 0;
	}
	//if(FPSDelay/16 > FPSNoDelay)UPeriod = UPeriod <= 25 ? 25 : UPeriod - 25;
	FPSDelay = 0;
	FPSNoDelay = 0;
}


int GetCurrScanLine()
{
	u64 LineTime = FirstLineTime + LineTimePerFrame * currScanLine;
	u64 currTime = svcGetSystemTick();
	if (LineTime > currTime)return currScanLine;
	return ((int)((LineTime-currTime)/LineTimePerFrame));
}

#ifdef LOG_ERROR
void ErrorLogUpdate()
{
	ErrorVec.push_back(ErrorChar);
	ErrorChar = NULL;
}
#endif // LOG_ERROR


//void SyncAudio()
//{
//	//if (AudioCalcCnt < 1)return;
//	//for (int i = 0; i < AudioCalcCnt; i++)
//	//{
//	//	CalcAudio();
//	//}
//	//AudioCalcCnt = 0;
//
//	//if (AudioCalcCnt>1)return;
//	//if (svcGetSystemTick() > NextAudioTime)
//	//{
//	//	CalcAudio();
//	//	NextAudioTime += AudioTimePerFrame;
//	//	//NextAudioTime = svcGetSystemTick() + AudioTimePerFrame;
//	//}
//}


void SaveScrrenShot(const char* pathchr)
{
	DrawMessage("Saving screenshot.Please Wait.",NULL, 50, 100, 1, false);
#ifdef VDP_V9990
	SDL_Surface* topsurf = IsWide ? SDL_CreateRGBSurfaceFrom(WBuf, WIDTH * 2, HEIGHT, 16, 4 * WIDTH, 0xF800, 0x07E0, 0x001F, 0)
		: (V9990Active ? SDL_CreateRGBSurfaceFrom(V9KXBuf, WIDTH, HEIGHT, 16, 2 * WIDTH, 0xF800, 0x07E0, 0x001F, 0)
			: SDL_CreateRGBSurfaceFrom(XBuf, WIDTH, HEIGHT, 16, 2 * WIDTH, 0xF800, 0x07E0, 0x001F, 0));
#else
	SDL_Surface* topsurf = IsWide ? SDL_CreateRGBSurfaceFrom(WBuf, WIDTH*2, HEIGHT, 16, 4 * WIDTH, 0xF800, 0x07E0, 0x001F, 0)
		: SDL_CreateRGBSurfaceFrom(XBuf, WIDTH, HEIGHT, 16, 2 * WIDTH, 0xF800, 0x07E0, 0x001F, 0);
#endif // VDP_V9990
	SDL_SaveBMP(topsurf, pathchr);
}


FILE* zipfopen(const char* _name, const char* _mode)
{
	FILE* F;
	if (IPSPatchSize > 0)
	{
		F = fmemopen(IPSPatchBuf, IPSPatchSize, _mode);
		return F;
	}
	std::string namestr = (std::string)_name;
#ifdef LOG_ERROR
	if (namestr.size() < 4)
	{
		ErrorVec.push_back(_name);
		ErrorVec.push_back("zipname too short");
		return NULL;
	}
#else
	if (namestr.size() < 4)return NULL;
#endif // LOG_ERROR
	if (namestr.find_first_of("/") != 0)namestr = Get3DSPath(_name);
	const char* fname = namestr.c_str();

	zipMessage = 0;

	const char* extname = &fname[strlen(fname) - 4];
	if (strcasecmp(extname, ".ZIP") != 0)
	{
		const char* extsname = &fname[strlen(fname) - 3];
		if (strcasecmp(extsname, ".GZ") != 0)
		{
			F = fopen(fname, _mode);
			return F;
		}
		gzFile GZF = gzopen(fname, "rb");
		if (!GZF)return NULL;
		int gsize = 0;
		int readSize;
		char* tempbuf;
		tempbuf = (char*)malloc(0x4000);
		for (;;)
		{
			readSize = gzread(GZF, tempbuf, 0x4000);
			gsize += readSize;
			if (readSize != 0x4000)break;
		}
		free(tempbuf);
		zipBuf = ResizeMemory(zipBuf, gsize);
		if (!zipBuf)
		{
			if (Verbose)printf("No memory for read the GZIP file.\n");
			zipMessage = 0x02;	/* 0x02: GZIP Out of memory. */
			if (gsize >= HDD_DETECT_SIZE)zipMessage |= 0x04;	/* 0x04:Is Hard Disk Size */
			gzclose(GZF);
			return NULL;
		}
		gzrewind(GZF);
		gzread(GZF, zipBuf, gsize);
		gzclose(GZF);
		/* If too big size, treat as hard disk image. */
		if (gsize >= HDD_DETECT_SIZE)
		{
			if (HDDStream)
			{
				fclose(HDDStream);
				HDDStream = 0;
			}
#ifdef _3DS_RESIZE_LINEAR
			if (HDD[0].Data)
			{
				linearFree(HDD[0].Data);
				HDD[0].Data = 0;
			}
#else
			if (HDD[0].Data)
			{
				free(HDD[0].Data);
				HDD[0].Data = 0;
			}
#endif // _3DS_RESIZE_LINEAR
			HDD[0].Data = zipBuf;
			HDD[0].DataSize = gsize;
			IsHardDisk = 1;
			HDDSize = gsize;
			return NULL;
		}
		F = fmemopen(zipBuf, gsize, _mode);
		return F;
	}
	unzFile zipFile = unzOpen(fname);
#ifdef LOG_ERROR
	if (!zipFile)
	{
		ErrorVec.push_back(_name);
		ErrorVec.push_back("ZipOpenError");
		return NULL;
	}
#else
	if (!zipFile)return NULL;
#endif // LOG_ERROR

	unz_global_info globalInfo;
	if (unzGetGlobalInfo(zipFile, &globalInfo) != UNZ_OK)
	{
		unzClose(zipFile);
		return NULL;
	}
	for (int i = 0; i < globalInfo.number_entry; i++)
	{
		char fileName[1024];
		unz_file_info fileInfo;
		if (unzGetCurrentFileInfo(zipFile, &fileInfo, fileName, sizeof(fileName), NULL, 0, NULL, 0) == UNZ_OK)
		{
			const char* ext = GetFileExtension(fileName);
			/* Maybe mistaken. but i test it many times and works in all of the case, so i leave it. */
			/* Use if(std::string(ext) == ".ROM") etc. to fix. */
			if (strcasecmp(ext, ".ROM") == 0 || strcasecmp(ext, ".DSK") == 0 || strcasecmp(ext, ".CAS") == 0 || strcasecmp(ext, ".MX1") == 0
#ifdef HFE_DISK
				|| strcasecmp(ext, ".HFE") == 0
#endif // HFE_DISK

				|| strcasecmp(ext, ".MX2") == 0 || strcasecmp(ext, ".IPS") == 0)
			{
				if (i == ZipIndex || ZipIndex < 0)
				{
					int unzipsize = fileInfo.uncompressed_size;
					/* Check const char* ext here because if malloc() get error it overwritten. */
					int isDisk = 0;
					/* If the file has '.DSK' extension and has too big size, treat as hard disk image. */
					if (std::string(ext) == ".dsk")isDisk = 1;
					if (std::string(ext) == ".DSK")isDisk = 1;
					if (unzipsize < HDD_DETECT_SIZE)isDisk = 0;

					if (isDisk)
					{
						if (HDDStream)
						{
							fclose(HDDStream);
							HDDStream = 0;
						}
#ifdef _3DS_RESIZE_LINEAR
						if (HDD[0].Data)
						{
							linearFree(HDD[0].Data);
							HDD[0].Data = 0;
						}
#else
						if (HDD[0].Data)
						{
							free(HDD[0].Data);
							HDD[0].Data = 0;
						}
#endif // _3DS_RESIZE_LINEAR
					}
					zipBuf = ResizeMemory(zipBuf, unzipsize);
					if (!zipBuf)
					{
						if (Verbose)printf("No memory for read the ZIP file.\n");
						zipMessage = 0x01;		/* 0x01: ZIP Out of memory. */
						if (unzipsize >= HDD_DETECT_SIZE)zipMessage |= 0x04;	/* 0x04:Is Hard Disk Size */
						unzClose(zipFile);
						return NULL;
					}

					unzOpenCurrentFile(zipFile);
					int err = unzReadCurrentFile(zipFile, zipBuf, unzipsize);
					unzCloseCurrentFile(zipFile);
					unzClose(zipFile);
					if (isDisk)
					{
						HDD[0].Data = zipBuf;
						HDD[0].DataSize = unzipsize;
						IsHardDisk = 1;
						HDDSize = unzipsize;
						return NULL;
					}
					F = fmemopen(zipBuf, unzipsize, _mode);
					if (!F)
					{
#ifdef _3DS_RESIZE_LINEAR
						linearFree(zipBuf);
#else
						free(zipBuf);
#endif // _3DS_RESIZE_LINEAR
						if (Verbose)printf("Open ZIP error.Size:%d %s\n",unzipsize , _name);
					}
					return F;
				}
			}
		}
		if (i + 1 < globalInfo.number_entry)
		{
			if (unzGoToNextFile(zipFile) != UNZ_OK)
			{
				//unzClose(zipFile);
				break;
			}
		}
	}
	//return NULL;

	unzClose(zipFile);
	return NULL;
}


FILE* zipfopenExtract(const char* _name, const char* _savepath ,const char* _mode)
{
	FILE* F;
	std::string namestr = (std::string)_name;
#ifdef LOG_ERROR
	if (namestr.size() < 4)
	{
		ErrorVec.push_back(_name);
		ErrorVec.push_back("zipname too short");
		return NULL;
	}
#else
	if (namestr.size() < 4)return NULL;
#endif // LOG_ERROR
	if (namestr.find_first_of("/") != 0)namestr = Get3DSPath(_name);
	const char* fname = namestr.c_str();

	const char* extname = &fname[strlen(fname) - 4];
	if (strcasecmp(extname, ".ZIP") != 0)return NULL;
	unzFile zipFile = unzOpen(fname);
	zipMessage = 0;
#ifdef LOG_ERROR
	if (!zipFile)
	{
		ErrorVec.push_back(_name);
		ErrorVec.push_back("ZipOpenError");
		return NULL;
	}
#else
	if (!zipFile)return NULL;
#endif // LOG_ERROR

	unz_global_info globalInfo;
	if (unzGetGlobalInfo(zipFile, &globalInfo) != UNZ_OK)
	{
		unzClose(zipFile);
		return NULL;
	}
	for (int i = 0; i < globalInfo.number_entry; i++)
	{
		char fileName[1024];
		unz_file_info fileInfo;
		if (unzGetCurrentFileInfo(zipFile, &fileInfo, fileName, sizeof(fileName), NULL, 0, NULL, 0) == UNZ_OK)
		{
			const char* ext = GetFileExtension(fileName);
			if (strcasecmp(ext, ".ROM") == 0 || strcasecmp(ext, ".DSK") == 0 || strcasecmp(ext, ".CAS") == 0 || strcasecmp(ext, ".MX1") == 0
				|| strcasecmp(ext, ".MX2") == 0 || strcasecmp(ext, ".IPS") == 0)
			{
				if (i == ZipIndex || ZipIndex < 0)
				{
					int isDisk = 0;
					if (std::string(ext) == ".dsk")isDisk = 1;
					if (std::string(ext) == ".DSK")isDisk = 1;

					unzOpenCurrentFile(zipFile);

					int unzipsize = fileInfo.uncompressed_size;
					int err = UNZ_OK;

					int zipbufsize = unzipsize > 0x200000 ? 0x200000 : unzipsize;
					zipBuf = ResizeMemory(zipBuf, zipbufsize);
					if (!zipBuf)
					{
						unzClose(zipFile);
						zipMessage = 0x01;	/* ZIP Out of memory. */
						return NULL;
					}
					F = fopen(_savepath, "wb");
					do
					{
						err = unzReadCurrentFile(zipFile, zipBuf, zipbufsize);
						if (err < 0)
						{
							unzCloseCurrentFile(zipFile);
							unzClose(zipFile);
							fclose(F);
							linearFree(zipBuf);
							return NULL;
						}
						if (err > 0)
						{
							fwrite(zipBuf, err, 1, F);
						}
					} while (err > 0);
					linearFree(zipBuf);

					//fclose(F);
					unzCloseCurrentFile(zipFile);
					unzClose(zipFile);
					F = freopen(_savepath, _mode, F);

					if ((isDisk) && (unzipsize >= HDD_DETECT_SIZE))
					{
						if (HDDStream)fclose(HDDStream);
						HDDStream = F;
						IsHardDisk = 1;
						HDDSize = unzipsize;
					}
					return F;
				}
			}
		}
		if (i + 1 < globalInfo.number_entry)
		{
			if (unzGoToNextFile(zipFile) != UNZ_OK)
			{
				//unzClose(zipFile);
				break;
			}
		}
	}
	//return NULL;

	unzClose(zipFile);
	return NULL;
}


FILE* gzipfopenExtract(const char* _name, const char* _savepath ,const char* _mode)
{
	FILE* F;
	std::string namestr = (std::string)_name;
	const char* fname = namestr.c_str();
	const char* extsname = &fname[strlen(fname) - 3];
	if (strcasecmp(extsname, ".GZ") != 0)
	{
		F = fopen(fname, _mode);
		return F;
	}
	gzFile GZF = gzopen(fname, "rb");
	int gsize = 0;
	int readSize;
	char* tempbuf;
	tempbuf = (char*)malloc(0x4000);
	for (;;)
	{
		readSize = gzread(GZF, tempbuf, 0x4000);
		gsize += readSize;
		if (readSize != 0x4000)break;
	}
	free(tempbuf);
	int zipbufsize = gsize > 0x200000 ? 0x200000 : gsize;
	zipBuf = ResizeMemory(zipBuf, zipbufsize);
	if (!zipBuf)
	{
		gzclose(GZF);
		zipMessage = 0x02;	/* GZIP Out of memory. */
		return NULL;
	}
	F = fopen(_savepath, "wb");
	int err = 0;
	gzrewind(GZF);
	do
	{
		err = gzread(GZF, zipBuf, zipbufsize);
		if (err < 0)
		{
			gzclose(GZF);
			fclose(F);
			linearFree(zipBuf);
		}
		if (err > 0)
		{
			fwrite(zipBuf, err, 1, F);
		}
	} while (err > 0);
	linearFree(zipBuf);
	gzclose(GZF);
	F = freopen(_savepath, _mode, F);
	if (gsize >= HDD_DETECT_SIZE)
	{
		if (HDDStream)fclose(HDDStream);
		HDDStream = F;
		IsHardDisk = 1;
		HDDSize = gsize;
	}
	return F;
}


FILE* sramfopen(const char* _name, const char* _mode)
{
	FILE* F;
	std::string filestr = std::string(_name);
	std::string currstr = std::string(_name);
	filestr.erase(filestr.begin(), filestr.begin() + ((int)filestr.find_last_of("/") + 1));
	currstr = "/FMSX3DS/SRAM/" + filestr;
	F = fopen(currstr.c_str(), _mode);
	return F;
}

/* Read "CARTS.CRC" file from msxDS. ( http://msxds.msxblue.com/download-jp.html )*/
int CalcCRC32(void* Buf, const char* filePath, int Size)
{
	FILE* F;
	int I, J, K, L, Retval = -1;
	char S[256], MapName[32], SramName[32];
	std::string filestr = filePath;
	bool IsValid = filestr == "/FMSX3DS/CARTS.CRC" ? false : true;
	std::string filename = &filestr[filestr.find_last_of("/") + 1];
	filestr.erase(filestr.begin(), filestr.begin() + ((int)filestr.find_last_of("/") + 1));
	if ((F = fopen(StringToChar(Get3DSPath(filename.c_str())), "rb")))
	{
		uLong crc = crc32(0, Z_NULL, 0);
		crc = crc32(crc, (byte*)Buf, Size);
		K = crc;
		while (fgets(S, sizeof(S) - 4, F))
		{
			std::string sstr = S;
			/*
			 Find "msxDS" text to detect the "CARTS.CRC" from msxDS. 
			*/
			if (!IsValid)
			{
				if (sstr.find("msxDS") != std::string::npos)IsValid = true;
			}
			/* Comment out after ";" character */
			L = sstr.find_first_of(";");
			if (L >= 0)sstr.erase(L);
			if (sscanf(sstr.c_str(), "%08X,%[^,],%s", &J, MapName, SramName) == 3)
			{
				if (K == J)
				{
					I = -1;
					std::string mapstr = MapName;
					std::string sramstr = SramName;
					if (mapstr == "Generic8")I = MAP_GEN8;
					else if (mapstr == "Generic16")I = MAP_GEN16;
					else if (mapstr == "Konami5")
					{
						if (sramstr == "mb2flh")I = MAP_MANBOW2;
						else I = MAP_KONAMI5;
					}
					else if (mapstr == "Konami4")I = MAP_KONAMI4;
					else if (mapstr == "ASC8")
					{
						if (sramstr == "SRAM8")I = MAP_ASCII8SRM32;
						else I = MAP_ASCII8;
					}
					else if (mapstr == "ASC16")I = MAP_ASCII16;
					else if (mapstr == "Konami3")I = MAP_GMASTER2;
					else if (mapstr == "Rtype")I = MAP_RType;
					else if (mapstr == "HarryFox16")I = MAP_HarryFox16;
					else if (mapstr == "CrossBlaim")I = MAP_CrossBlaim;
					else if (mapstr == "SLR")I = MAP_LodeRunner;
					else if (mapstr == "SuperPierrot")I = MAP_Pierrot;
					else if (mapstr == "Zemina")I = MAP_Zemina;
					else if (mapstr == "ZeminaDS2")I = MAP_ZeminaDS2;
					else if (mapstr == "SWANGI")I = MAP_SWANGI;
					else if (mapstr == "DOOLY")I = MAP_DOOLY;
					else if (mapstr == "XXin1")I = MAP_XXin1;
					else if (mapstr == "126in1")I = MAP_126in1;
					else if (mapstr == "MSX90")I = MAP_MSX90;
					else if (mapstr == "FMPAC")I = MAP_FMPAC;
					else if (mapstr == "ASC16_2")I = MAP_ASCII16_2;
					else if (mapstr == "WIZ")I = MAP_Wizardry;
					else if (mapstr == "msxdos2")I = MAP_GEN16;
					else if (mapstr == "MAJUTSUSHI")I = MAP_MAJUTSUSHI;
					else if (mapstr == "SCCPLUS")I = MAP_SCCPLUS;
					else if (mapstr == "SCCPLUS_2")I = MAP_SCCPLUS_2;
					else if (mapstr == "PLAIN")I = MAP_PLAIN;
					else if (mapstr == "WingWarr")I = MAP_WingWarr;
					else if (mapstr == "SYNTHE")I = MAP_SYNTHE;
					else if (mapstr == "QURAN")I = MAP_QURAN;
					Retval = I;
					break;
				}
			}
		}
		fclose(F);
	}
	if (!IsValid)return -1;
	return Retval;
}


/* Check Special Cartridge. Cartidges that use special device (i.e. Arkanoid Paddle), or cartridge that use special I/O port, and so on.  */
void CheckSpecialCart(void* Buf, int Size, int Type, int Slot)
{
	int I;
	uLong crc = crc32(0, Z_NULL, 0);
	crc = crc32(crc, (byte*)Buf, Size);
	I = crc;
	int OldCartSpecial = CartSpecial[Slot];
	switch (I)
	{
	case 0xC9A22DFC:	/* Arkanoid  */
	case 0x2E131532:	/* Arkanoid II - Revenge of Doh */
	case 0x00D36AF6:	/* Arkanoid II - Revenge of Doh */
		CartSpecial[Slot] = CART_ARKANOID;
		break;
	case 0xFF974EE3:	/* Yaksa */
		CartSpecial[Slot] = CART_YAKSA;
		break;
	case 0x6A2E3726:	/* Rambo */
	case 0xDBA4377B:	/* Rambo */
	case 0x0859F662:	/* Rambo */
	case 0x2236DDF6:	/* Rambo */
	case 0xA8B1AB2B:	/* Rambo */
		CartSpecial[Slot] = CART_NEED_CBIOS;
		break;
	case 0x71A1B1EC:	/* Baby Dinosaur Dooly */
	case 0xB9B0999A:	/* Konami's Synthesizer */
	case 0x8C2A49B2:	/* Konami's Synthesizer */
		CartSpecial[Slot] = CART_SMALL;
		break;
	case 0x688E2E8F:	/* Zombie Hunter */
	case 0x80A5DBB8:	/* Uchu Yohei(MSXdev2023) */
		CartSpecial[Slot] = CART_SOFT_RESET;
		break;
	case 0xEC34F9F2:	/* R-Type (RomMapper changed) */
		CartSpecial[Slot] = CART_FMBUG;
		break;
	case 0xF115257A:	/* Aquapolis SOS */
	case 0xE223FFD1:	/* Galaxian */
	case 0x0AD3707D:	/* Bee & Flower */
	case 0x004976D3:	/* GameLand (CASIO) */
	case 0xA3A56524:	/* Champion Boulder Dash */
	case 0xCF744D2E:	/* Megalopolis SOS */
	case 0x4A45CBC0:	/* Space Maze Attack */
	case 0x99518A12:	/* Hustle Chumy */
		CartSpecial[Slot] = CART_SLOTBUG;
		break;
	default:
		switch (Type)
		{
		case MAP_MANBOW2:
			CartSpecial[Slot] = CART_MEGASCC;
			break;
		case MAP_RType:
			CartSpecial[Slot] = CART_FMBUG;
			break;
		case MAP_LodeRunner:
			CartSpecial[Slot] = CART_LODERUNNER;
			break;
		case MAP_ZeminaDS2:
			CartSpecial[Slot] = CART_ZEMINADS2;
			break;
		case MAP_MSX90:
			CartSpecial[Slot] = CART_MSX90;
			break;
		case MAP_WingWarr:
			CartSpecial[Slot] = CART_READSCC;
			break;
		case MAP_MEGASCSI:
			CartSpecial[Slot] = CART_MEGASCSI;
			break;
		case MAP_QURAN:
			CartSpecial[Slot] = CART_ARABIC;
			break;
		default:
			CartSpecial[Slot] = 0;
			break;
		}
		break;
	}

	HasSpecialCart = ((!CartSpecial[0]) && (!CartSpecial[1])) ? 0 : 1;

	if (CartSpecial[Slot] == CART_NEED_CBIOS)ReloadBIOS = 1;
	if (OldCartSpecial == CART_NEED_CBIOS && CartSpecial[Slot] != CART_NEED_CBIOS)ReloadBIOS = 1;

	if (OldCartSpecial == CART_ARKANOID && CartSpecial[Slot] != CART_ARKANOID)
	{
		if (JoyMode[0] != currJoyMode[0])currJoyMode[0] = JoyMode[0];
	}
	else if (CartSpecial[Slot] == CART_ARKANOID)
	{
		if (currJoyMode[0] != JOY_ARKANOID)currJoyMode[0] = JOY_ARKANOID;
	}

	if (OldCartSpecial == CART_ARABIC && CartSpecial[Slot] != CART_ARABIC)
	{
#ifdef HDD_NEXTOR
		if ((HDD->Data) || (HDDStream))return;
#endif // HDD_NEXTOR
		 LoadCart(0, 7, 0);
	}
	else if(CartSpecial[Slot] == CART_ARABIC)
	{
		LoadCart("ARABIC.rom", 7, MAP_GUESS);
	}

	if (!Slot && OldCartSpecial == CART_FMBUG && CartSpecial[Slot] != CART_FMBUG)LoadCart(0, 1, 0);
}

void LoadCartAtStart()
{
	if (IsStart)
	{
		if (IsStartLoadFile)
		{
			SDL_PauseAudio(1);
			BrowseROM(0, BROWSE_START | BROWSE_ALL);
			//DrawKeyboard3DS();
			SetWait();
			SDL_PauseAudio(0);
		}
		DrawKeyboard3DS();
		IsStart = false;
	}
}


#ifdef SUPERIMPOSE
void InitScreenShotTexture(SDL_Surface* ssurface)
{
	C3D_TexInit(&ScreenTexImpose, TopTexWidth, TopTexHeight, GPU_RGBA8);
	ScreenSubTexImpose.width = TopTexWidth;
	ScreenSubTexImpose.height = TopTexHeight;
	ScreenSubTexImpose.left = 0.0f;
	ScreenSubTexImpose.right = 1.0f;
	ScreenSubTexImpose.top = 1.0f;
	ScreenSubTexImpose.bottom = 0.0f;

	ScreenImageImpose.tex = &ScreenTexImpose;
	ScreenImageImpose.subtex = &ScreenSubTexImpose;
	SDLSurfaceToC3DTexData(ssurface, ScreenTexImpose.data, ScreenSubTexImpose.width, ScreenSubTexImpose.height, 50);
}


void ChangeScreenImposeTransparent(int alpha)
{
	C3DTextureChangeAlpha(ScreenTexImpose, alpha);
}
#endif // SUPERIMPOSE


#ifdef HFE_DISK
unsigned short CRC16Table[256] = {
	0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
	0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
	0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
	0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
	0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
	0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
	0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
	0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,

	0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
	0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
	0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
	0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
	0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
	0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
	0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
	0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,

	0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
	0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
	0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
	0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
	0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
	0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
	0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
	0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,

	0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
	0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
	0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
	0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
	0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
	0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
	0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
	0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0,
};

/* Add support for HFE HxC Floppy Emulator file format. */
/* https://hxc2001.com/floppy_drive_emulator/HFE-file-format.html */
/* Based on infomation from */
/* https://middleriver.chagasi.com/electronics/fddemu.html */
/* and infomation at MSX techenical guide book disk edition. */
/* http://www.ascat.jp/tg/tgdindex.html */
byte loadHFE_File(int slotid, const char* filename)
{
	FILE* hfeFile;
	byte* hfeBuf, * P;
	byte buf0, buf1, buf2, buf3, buf4, buf5, bufp0, bufp1, bufp2, bufp3, cdata, cdata0, cdata1;
	word wdata;
	int hfesize;
	hfeFile = zipfopen(filename, "rb");
	if (!hfeFile)return(0);
	fseek(hfeFile, 0, SEEK_END);
	hfesize = ftell(hfeFile);
	hfeBuf = ResizeMemory(hfeBuf, hfesize);
	rewind(hfeFile);
	fread(hfeBuf, 1, hfesize, hfeFile);
	byte* currBuf = hfeBuf + 0x400;
	byte* startBuf = hfeBuf + 0x400;
	byte* dataBuf = (byte*)malloc(hfesize / 2);
	byte* currDataBuf = dataBuf + 0;
	int dataRecCount = 0;
	int idxRecCount = 0;
	uint16_t offsets[85];
	uint16_t track_lens[85];
	byte* pictrackBuf = hfeBuf + 0x200;
	word checkWord = 0;
	int currShifted = 0;
	int sectorSkipped = 0;
	bool trackHasData = false;
	bool IsSectorHasData = false;
	int recordCount = 0;
	int headerCount = 0;
	int trackCount = 0;
	int sectorSum = 0;
	int oldC;
	byte sectorInfos[8];	/* [0:C]  [1:H]  [2:R]  [3:N]  [4:old C]  [5:old H]  [6:old R]  [7:old N] */
	uint16_t calcCRC = 0;
	uint16_t idxcrc = 0;
	uint16_t dataCRC = 0;
	byte isCopyProtect = 0;
	std::unordered_set<int> sectorSet;
	if (derBuf == NULL)derBuf = (unsigned char*)malloc(600);
	else derBuf = (unsigned char*)realloc(derBuf, 600);
	memset(derBuf, 0, 600);
	for (int i = 0; i < hfeBuf[9]; i++)
	{
		offsets[i] = ((uint16_t)pictrackBuf[(i << 2) + 1] << 8) | (uint16_t)pictrackBuf[i << 2];
		track_lens[i] = ((uint16_t)pictrackBuf[(i << 2) + 3] << 8) | (uint16_t)pictrackBuf[(i << 2) + 2];
	}

	for (int g = 0; g < hfeBuf[9]; g++)
	{
		if ((offsets[g] == 0) || (track_lens[g] == 0))break;
		//if (Verbose&0x04)printf("offset[%02Xh]  len[%02Xh]\n", offsets[g], track_lens[g]);
		/* h==0->Side 0,  h==1->Side 1 */
		for (int h = 0; h < 2; h++)
		{
			//if (Verbose & 0x04)printf("Track[%d]  Side[%d]\n", g, h);
			currBuf = startBuf = hfeBuf + (((int)offsets[g] * 0x200) + (h * 0x100) + 0x200);
			currShifted = 0;
			//if (!dataRecCount)trackHasData = false;
			//if (Verbose & 0x04)printf("Addr: %d\n", (((int)offsets[g] * 0x200) + (h * 0x100) + 0x200));
			while ((int)(currBuf - startBuf) < track_lens[g])
			{
				for (int i = 0; i < 256; i += 2, currBuf += 2)
				{
					if ((int)(currBuf - hfeBuf) >= hfesize - 260)break;
					/* Convert RAW MFM encoded value to data. */
					buf0 = currBuf[0];
					buf1 = i >= 255 ? currBuf[257] : currBuf[1];
					buf2 = i >= 254 ? currBuf[258] : currBuf[2];
					buf3 = i >= 253 ? currBuf[259] : currBuf[3];
					wdata = GetHfeShiftedVal(buf0, buf1, buf2, buf3, currShifted);
					cdata0 = wdata >> 8;
					cdata1 = wdata & 0xFF;
					/* Convert to LSB data. */
					/* Ignore Odd bit(clock bit). */
					cdata = ((cdata0 & 0x01) << 7) | ((cdata0 & 0x04) << 4) | ((cdata0 & 0x10) << 1) | ((cdata0 & 0x40) >> 2)
						| ((cdata1 & 0x01) << 3) | (cdata1 & 0x04) | ((cdata1 & 0x10) >> 3) | ((cdata1 & 0x40) >> 6);

					if (dataRecCount)
					{
						dataRecCount--;
						switch (dataRecCount)
						{
						case 0:
							dataCRC |= cdata;
							if (calcCRC != dataCRC)
							{
								isCopyProtect |= 1;
								sectorSum = (sectorInfos[2] - 1 + (recordCount * (sectorInfos[0] * (headerCount + 1) + sectorInfos[1])));
								if ((sectorSum >> 3) <= 200)
								{
									derBuf[sectorSum >> 3] |= (0x80 >> (sectorSum & 0x07));
									if (Verbose & 0x04)printf("Data CRC Error in Sector[%d] : CalcCRC[%04Xh]  DataCRC[%04Xh]\n"
										, sectorSum, calcCRC, dataCRC);
								}
							}
							else sectorSum++;
							break;
						case 1:
							dataCRC  = cdata << 8;
							break;
						default:
							currDataBuf[0] = cdata;
							currDataBuf++;
							calcCRC = (Uint16)((calcCRC << 8) ^ CRC16Table[(byte)(calcCRC >> 8) ^ cdata]);
							break;
						}
						continue;
					}
					if (idxRecCount)
					{
						switch (6 - idxRecCount)
						{
						case 0:	/* C */
							sectorInfos[0] = cdata;
							if ((cdata >= trackCount) && (cdata <= 82))trackCount = cdata;
							calcCRC = (Uint16)((calcCRC << 8) ^ CRC16Table[(byte)(calcCRC >> 8) ^ cdata]);
							//if ((cdata != oldC + 1) && (cdata != 0) && (cdata!=1) && (cdata != oldC))
							//{
							//	if (Verbose & 0x04)printf("Wrong Cylinder Num[%d] oldC[%d] in Sector[%d]\n", cdata, oldC, sectorSum);
							//}
							oldC = cdata;
							if ((Verbose & 0x04) && (cdata>=83))printf("Wrong Cylinder Num[%d] in Sector[%d]\n", cdata, sectorSum);
							//if (Verbose & 0x04)printf("Cylinder Num[%d]", cdata);
							break;
						case 1:	/* H */
							sectorInfos[1] = cdata;
							calcCRC = (Uint16)((calcCRC << 8) ^ CRC16Table[(byte)(calcCRC >> 8) ^ cdata]);
							if (cdata == 0x01)headerCount = 1;
							if ((Verbose & 0x04) && (cdata>=2))printf("Wrong Header Num[%d] in Sector[%d]\n", cdata, sectorSum);
							//if (Verbose & 0x04)printf(" Header Num[%d]", cdata);
							break;
						case 2:	/* R */
							sectorInfos[2] = cdata;
							if (cdata <= 9)
							{
								if (cdata >= recordCount)recordCount = cdata;
								if ((sectorInfos[0] <= 82) && (sectorInfos[1] <= 1))
								{
									sectorSum = (sectorInfos[2] - 1 + (recordCount * (sectorInfos[0] * (headerCount + 1) + sectorInfos[1])));
								}
							}
							else
							{
								if(Verbose & 0x04)printf("Wrong Record Num[%d] in Sector[%d]\n", cdata, sectorSum);
							}
							calcCRC = (Uint16)((calcCRC << 8) ^ CRC16Table[(byte)(calcCRC >> 8) ^ cdata]);
							//if (Verbose & 0x04)printf(" Record Num[%d]\n", cdata);
							break;
						case 3:	/* N */
							sectorInfos[3] = cdata;
							calcCRC = (Uint16)((calcCRC << 8) ^ CRC16Table[(byte)(calcCRC >> 8) ^ cdata]);
							if ((Verbose & 0x04) && (cdata!=2))printf("Wrong Sector Size[%d] in Sector[%d]\n", cdata, sectorSum);
							break;
						case 4:	/* ID Mark CRC */
							idxcrc = cdata<<8;
							break;
						case 5:	/* ID Mark CRC */
							idxcrc |= cdata;
							if (calcCRC != idxcrc)
							{
								//isErrorCRC |= 1;
								//sectorSum = (sectorInfos[2] - 1  + (recordCount * (sectorInfos[0] * (headerCount + 1) + sectorInfos[1])));
								//if ((sectorSum >> 3) <= 200)
								//{
								//	derBuf[sectorSum >> 3] |= (0x80 >> (sectorSum & 0x07));
									if (Verbose & 0x04)printf("ID Mark CRC Error in Sector[%d] : CalcCRC[%04Xh]  IdMarkCRC[%04Xh]\n"
										, sectorSum, calcCRC, idxcrc);
								//}
							}
							if ((!IsSectorHasData) && (sectorSum>=2))
							{
								if (Verbose & 0x04)printf("SkipData in sector[%d]\n", sectorSum);
							}
							IsSectorHasData = false;
							break;
						default:
							break;
						}
						idxRecCount--;
						continue;
					}

					/* 0xA1 with missing clock bit to detect Address Mark(ID Address Mark or Data Address Mark) */
					/* Detect data shift, and fix that. */
					if ((int)(currBuf - hfeBuf) < hfesize - 261)
					{
						buf4 = i >= 252 ? currBuf[260] : currBuf[4];
						for (int j = 0; j < 16; j++)
						{
							checkWord = GetHfeShiftedVal(buf0, buf1, buf2, buf3, j);
							/* data "0xA1" with missing clock */	/* 0x9148(LSB) == 0x4489(MSB) */
							if (checkWord == 0x9148)
							{
								buf5 = i >= 251 ? currBuf[261] : currBuf[5];
								checkWord = GetHfeShiftedVal(buf2, buf3, buf4, buf5, j);
								if (checkWord == 0x9148)
								{
									//if (Verbose & 0x08)
									//{
									//	if (currShifted != j)printf("currShift[%d] ", j);
									//}
									currShifted = j;
									break;
								}
							}
						}
					}
					if ((cdata == 0xF8) || (cdata == 0xFB) || (cdata == 0xFE))
					{
						//buf1 = i >= 255 ? currBuf[257] : currBuf[1];
						bufp0 = i >= 4 ? currBuf[-4] : currBuf[-260];
						bufp1 = i >= 3 ? currBuf[-3] : currBuf[-259];
						bufp2 = i >= 2 ? currBuf[-2] : currBuf[-258];
						bufp3 = i >= 1 ? currBuf[-1] : currBuf[-257];
						if (cdata == 0xF8)	/* Start Deleted data */
						{
							/* data "0xA1" with missing clock */	/* 0x9148(LSB) == 0x4489(MSB) */
							//if (GetHfeShiftedVal(currBuf[-4], currBuf[-3], currBuf[-2], currBuf[-1], currShifted) == 0x9148)
							if (GetHfeShiftedVal(bufp0, bufp1, bufp2, bufp3, currShifted) == 0x9148)
							{
								if (Verbose & 0x04)printf("Deleted Data\n");
							}
						}
						if (cdata == 0xFB)	/* Start Data */
						{
							/* data "0xA1" with missing clock */	/* 0x9148(LSB) == 0x4489(MSB) */
							//if (GetHfeShiftedVal(currBuf[-4], currBuf[-3], currBuf[-2], currBuf[-1], currShifted) == 0x9148)
							if (GetHfeShiftedVal(bufp0, bufp1, bufp2, bufp3, currShifted) == 0x9148)
							{
								dataRecCount = 512 + 2;		/* Data:512byte,  CRC of data:2byte */
								trackHasData = true;
								/* Start Calculate CRC for data here */
								calcCRC = 0xFFFF;
								calcCRC = 0xFFFF;
								for (int j = 0; j < 3; j++)
								{
									calcCRC = (Uint16)((calcCRC << 8) ^ CRC16Table[(byte)(calcCRC >> 8) ^ 0xA1]);
								}
								calcCRC = (Uint16)((calcCRC << 8) ^ CRC16Table[(byte)(calcCRC >> 8) ^ 0xFB]);
								//if (sectorInfos[2] > 9)sectorInfos[2] = oldR >= 9 ? 1 : oldR + 1;
								if ((sectorInfos[0] <= 82) && (sectorInfos[1] <= 1) && (sectorInfos[2] <= 9))
								{
									sectorSum = (sectorInfos[2] - 1 + (recordCount * (sectorInfos[0] * (headerCount + 1) + sectorInfos[1])));
									//if ((sectorSum != oldSecorSum) && (sectorSum*512 + 512 <hfesize/2))
									if (sectorSum < 0 && !sectorSet.count(sectorSum))
									{
										sectorSum = 0;
										//sectorSet.insert(sectorSum);
									}
									//if ((sectorSum != oldSecorSum) && (sectorSum * 512 + 512 < hfesize / 2) && (sectorSum>=0))
									if ((sectorSum * 512 + 512 < hfesize / 2) && (sectorSum >= 0))
									{
										if (sectorSet.count(sectorSum))
										{
											if (Verbose & 0x04)printf("Multi sector in Sector[%d]\n", sectorSum);
											if ((sectorSum >> 3) <= 200)
											{
												derBuf[(sectorSum >> 3) + 400] |= (0x80 >> (sectorSum & 0x07));
												byte MultiBuf[512];
												memset(MultiBuf, 0, 512);
												MultiSecMap[sectorSum] = MultiBuf;
												currDataBuf = MultiBuf + 0;
												isCopyProtect |= 0x04;
											}
										}
										else
										{
											sectorSet.insert(sectorSum);
											currDataBuf = dataBuf + (sectorSum * 512);
										}
										IsSectorHasData = true;
									}
									else
									{
										if ((Verbose & 0x04) && (sectorSum!=0))printf("Illegal sector[%d] in Sector[%d]\n", sectorInfos[2], sectorSum);
									}

									//currDataBuf = dataBuf + ((sectorInfos[2] - 1 + sectorSkipped
									//	+ (recordCount * (sectorInfos[0] * (headerCount + 1) + sectorInfos[1]))) * 512);

								// //currDataBuf = dataBuf + ((sectorInfos[2] - 1 - sectorSkipped +
								// //	(recordCount * (sectorInfos[0] * (headerCount + 1) + sectorInfos[1]))) * 512);
								}
								else
								{
									byte IllBuf[512];
									memset(IllBuf, 0, 512);
									IllegalSecMap[(Uint32)sectorInfos[0] << 16 | (Uint32)sectorInfos[1] << 8 | (Uint32)sectorInfos[2]] = IllBuf;
									currDataBuf = IllBuf + 0;
									isCopyProtect |= 0x08;
								}
								// //int J = D->SectorNumber - 1 + D->Disk[D->Drive]->Sectors * (D->CurrTrack * D->Disk[D->Drive]->Sides + D->Side);
							}
						}

						if (cdata == 0xFE) /* Start ID Address Mark */
						{
							//if (GetHfeShiftedVal(currBuf[-4], currBuf[-3], currBuf[-2], currBuf[-1], currShifted) == 0x9148)
							if (GetHfeShiftedVal(bufp0, bufp1, bufp2, bufp3, currShifted) == 0x9148)
							{
								idxRecCount = 6;
								/* Calculate CRC for header here. */
								calcCRC = 0xFFFF;
								for (int j = 0; j < 3; j++)
								{
									calcCRC = (Uint16)((calcCRC << 8) ^ CRC16Table[(byte)(calcCRC >> 8) ^ 0xA1]);
								}
								calcCRC = (Uint16)((calcCRC << 8) ^ CRC16Table[(byte)(calcCRC >> 8) ^ 0xFE]);
							}
						}
					}
				}
				currBuf += 256;
			}
			//if (trackHasData)trackCount++;
			//else
			//{
			//	if (Verbose & 0x04)printf("Record not found[%d]\n",sectorSum);
			//}
		}
	}
	//trackCount /= (headerCount + 1);
	trackCount += 1;
	if (Verbose & 0x04)
	{
		printf("HFE Disk Record[%d] Header[%d] Track[%d] Length[%08Xh]\n", recordCount, headerCount, trackCount,
			recordCount * (headerCount + 1) * trackCount * 512);
	}
	sectorSum = recordCount * (headerCount + 1) * trackCount;
	for (int i = 0; i < sectorSum; i++)
	{
		if (!sectorSet.count(i))
		{
			if (Verbose & 0x04)printf("Record not found in Sector[%d]\n", i);
			//if (i == 0)
			//{
			//	memcpy(dataBuf + 0, dataBuf + 8*512, 512);
			//	continue;
			//}
			if ((i >> 3) <= 200)
			{
				/* Because ".der" file doesn't support "Record not found" error, so i use the expanded one.
				 Sector number of "Record not found" error stores after the normal data of ".der" file format.
				 It stores the data with the same algorithm to the address 200-380(0xC8-0x17C)
				 Sector number of multiple sector stores after the "Record not found" error infomation data.
				 */

				///* Next sector for multi sector. */
				//if (i > 0)
				//{
				//	if (derBuf[((i - 1) >> 3) + 400] & (0x80 >> ((i - 1) & 0x07)))
				//	{
				//		continue;
				//	}
				//}

				derBuf[(i >> 3) + 200] |= (0x80 >> (i & 0x07));
				isCopyProtect |= 2;
			}
		}
	}

	if (!isCopyProtect)
	{
		free(derBuf);
		derBuf = 0;
		isLoadDer = 0;
	}
	else
	{
		if (isCopyProtect == 8)
		{
			free(derBuf);
			derBuf = 0;
		}
		isLoadDer = isCopyProtect;
	}

	P = NewFDI(&FDD[slotid], headerCount+1, trackCount, recordCount, 512);
	if (!P) { fclose(hfeFile); return(0); }
	memcpy(P, dataBuf+0, recordCount * (headerCount + 1) * trackCount * 512 * sizeof(byte));
	P = FDD[slotid].Data;
	FDD[slotid].Data = P;
	FDD[slotid].Format = FMT_MSXDSK;
	fclose(hfeFile);

	//FILE* writeFile;
	//if (writeFile = fopen("/FMSX3DS/TEST.TXT", "wb"))
	//{
	//	fwrite(dataBuf, 1, hfesize / 2, writeFile);
	//}
	//fclose(writeFile);

	linearFree(hfeBuf);
	free(dataBuf);
	return 1;
}


word GetHfeShiftedVal(byte val0, byte val1, byte val2, byte val3, int shiftv)
{
	uint checkWord;
	checkWord = (((uint)val0 | ((uint)val1 << 8) | ((uint)val2 << 16) | ((uint)val3 << 24)) >> shiftv) & 0xFFFF;
	return (checkWord >> 8 | ((checkWord & 0xFF) << 8));
}


/* Wrapper function to use std::map to C */
byte* GetMultiSector(int SectorNum)
{
	return MultiSecMap[SectorNum];
}

/* Wrapper function to use std::map to C */
byte* GetIllegalSector(byte C, byte H, byte R)
{
	return IllegalSecMap[(Uint32)C << 16 | (Uint32)H << 8 | (Uint32)R];
}

#endif // HFE_DISK


void InitXbuf()
{
	if (XBuf == NULL)XBuf = (unsigned short*)malloc(WIDTH * HEIGHT * sizeof(unsigned short) * 2);
	int i;
	int XSize = WIDTH * HEIGHT * 2;
	for (i = 0; i < XSize; i++)
	{
		XBuf[i] = 0x00;
	}
}


void InitWbuf()
{
	if (WBuf == NULL)WBuf = (unsigned short*)malloc(512 * HEIGHT * sizeof(unsigned short) * 2);
	int i;
	int WSize = 512 * HEIGHT * 2;
	for (i = 0; i < WSize; i++)
	{
		WBuf[i] = 0x00;
	}
}


void ShowMessage3DS(char* msg, char* msg2)
{
	DrawMessage(msg, msg2, 10, 50, 1000, true);
}


/* Wrapper function of LoadFMPAC() in 3DSMenu.cpp */
void DoReloadFMPAC()
{
	LoadFMPAC(2,1);
}


int Debug_CalcBPalVlue(int R, int G, int B)
{
	int bpalint = 0;
	for (int i = 0; i < 256; i++)
	{
		unsigned long LongR = ((unsigned long)R&0x07)*255/7 ;
		unsigned long LongG = ((unsigned long)G&0x07)*255/7;
		unsigned long LongB = ((unsigned long)B&0x03)*255/3;
		unsigned int TempBPal = ((LongR / 8) << 11 | (LongG / 8) << 6 | (LongB / 8));
		if (TempBPal == BPal[i])
		{
			bpalint = i;
			break;
		}
	}
	return bpalint;
}


void Show_3DS_BreakPoint(const char* message)
{
	if (!ShowDebugMsg3DS)return;
	Debug_BreakPoint3DS(message);
}


void Show_3DS_BreakPointVal(int val)
{
	if (!ShowDebugMsg3DS)return;
	char msg[1024];
	sprintf(msg, "%d", val);
	Debug_BreakPoint3DS(msg);
}

void Show_3DS_BreakPointArg(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	char msg[1024];
	sprintf(msg, format,args);
	Debug_BreakPoint3DS(msg);
	va_end(args);
}

#ifdef VDP_V9990
void InitV9KXbuf()
{
	if (V9KXBuf == NULL)V9KXBuf = (unsigned short*)malloc(WIDTH * HEIGHT * sizeof(unsigned short) * 2);
	int i;
	int XSize = WIDTH * HEIGHT * 2;
	for (i = 0; i < XSize; i++)
	{
		//XBuf[i] = 0x00;
		V9KXBuf[i] = 0x00;
	}
}

void V9990SetColor(byte N, byte R, byte G, byte B)
{
	unsigned int J;
	J = ((unsigned int)R << 11) | ((unsigned int)G << 6) | (unsigned int)B;
	V9kXPal[N >> 2] = J;
}
#endif // VDP_V9990
