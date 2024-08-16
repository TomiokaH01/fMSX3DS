#include <stdio.h>
#include <string.h>
#include <iostream>
#include <algorithm>
#include <unordered_set>
#include <3ds.h>

#include <vector>
#include <map>
#include <citro2d.h>
#include <sstream>
#include <sys/stat.h>
#include <dirent.h>

#include "MSX.h"
#include "3DSMenu.h"
#include "3DSLib.h"
#include "MenuJapanese.h"

extern "C"
{
	#include "unzip.h"
}

static std::string DiskStr[2] = {"",""};
static std::string DiskRawStr[2] = { "","" };
static std::string ROMStr[2] = {"",""};
static std::string CASStr = "";

bool isScrollText = true;

C2D_TextBuf textbuf;
//C3D_RenderTarget* top;
//C3D_RenderTarget* bottom;
C2D_Text dynText;

SDL_Surface* KeySurface;
SDL_Surface* KeySurfaceEUR;
SDL_Surface* KeySurfaceGra;
SDL_Surface* KeySurfaceGraEUR;
SDL_Surface* KetySurfaceGraShift;

SDL_Surface* ScreenShotSurface;

C2D_Image HUDImage;
C3D_Tex HUDTex;
Tex3DS_SubTexture HUDSubTex;

u32 Color_Screen = C2D_Color32(104, 120, 255, 255);
u32 Color_White = C2D_Color32(255, 255, 255, 255);
u32 Color_Green = C2D_Color32(0, 255, 0, 255);
u32 Color_Black = C2D_Color32(0, 0, 0, 255);
u32 Color_Select = C2D_Color32(70, 80, 170, 255);
u32 Color_Folder0 = C2D_Color32(240, 234, 79, 255);
u32 Color_Folder1 = C2D_Color32(223, 186, 74, 255);
u32 Color_Transp = C2D_Color32(255, 255, 255, 30);

static int sysmenuIdx = 0;
static int menucnt = 0;
static std::string currdir = "/";
std::vector<std::string> dirstring;
std::vector<std::string> ftypestring;
std::vector<u8> ftypevec;
std::vector<std::string> recentlylist;
std::vector<std::string> recentftypelist;
std::string SystemLoadString = "";

u32 scrollTime = 0;
bool isMouseDown = false;
u8 rapidCounts[10] = { 0,0,0,0,0,0,0,0,0,0 };
u8 rapidInterVals[10] = { 0,0,0,0,0,0,0,0,0,0 };

const int textSize = 20;
const int textStartPos = 20;
const int textEndPos = 300;
const float fontSize = 0.5f;
const int OKButtonSize = 20;
const int scrollButtonSize = 20;
const int textColumn = 240 / textSize;

static int systemLanguage = 1;
static int IsNew3DS = 1;
static int menuLanguage = -1;//-1:Auto 0:Japanese 1:English
static std::map<std::string, int> optionMap;
static std::map<int, int> keyMenuMap;
static std::vector<char*> KeyNameVec;
static std::string latestPath;

touchPosition oldtp;
touchPosition tp;
int ScreenShotOffx = 0;
int ScreenShotOffy = 0;
bool IsFrameLimit = true;
bool IsDoubleBuffer = true;
unsigned char IsDrawDiskLamp[2] = { 0,0 };
int Use2413 = 1;
int Use8950 = 1;
bool IsPSGHQ = false;
bool IsSCCHQ = false;
bool Is2413HQ = true;
unsigned char PSGType = 0;
unsigned char SoundChannels = 0;
unsigned char UseDCFilter = 0;
unsigned char UseRCFilter = 0;
unsigned char UseFIRFilter = 0;
unsigned char SoundSampRate = 0;
unsigned char Use8950TurboR = 0;
unsigned char ReadSCCPlus = 0;
char IsDebug = 0;
int CartSpecial[2] = { 0,0 };
unsigned char IsStartLoadFile = 1;
int IPSPatchSize = 0;
unsigned char* IPSPatchBuf;
unsigned char ScreenFilter;
unsigned char TurboNow = 0;
#ifdef MEMORY_CURSOR_POS
unsigned char MemorySysMenupos = 0;
#endif // MEMORY_CURSOR_POS
int TextDelay = 100;
int fpsval;
unsigned char NewPSGType = 0;
unsigned char NewSoundChs = 0;
unsigned char NewMSX_Mode = 2;
unsigned char NewRAMSize = 3;
//unsigned char NewMSXDOS = 0;
#ifdef USE_3D
unsigned char OldStereo3DMode = 0;
unsigned char Stereo3DMode = 0;
unsigned char OldIs3DNow = 0;
unsigned char Is3DNow = 0;
#endif // USE_3D

#ifdef DEBUG_LOG
#define MAXDEBUG	1024
int debugCnt = 0;
#endif // DEBUG_LOG

#ifdef LOG_ERROR
std::vector<const char*> ErrorVec;
#endif // LOG_ERROR

struct OptionItem
{
	std::string name;
	const int defaultIdx;
	const int old3DSIdx;
	int currentIdx;
	const std::vector<char*> menuChar;
};

std::vector<char*> OptionNull = { "" };
std::vector<char*> OptionOffOn = { "Off","On" };
std::vector<char*> OptionOnOff = { "On","Off" };
//std::vector<char*> OptionOffOnUnsafe = { "Off", "On(Unsafe)" };
std::vector<char*> OptionNum = { "0","1","2","3","4","5","6","7","8","9","10" };
std::vector<char*> OptionNum3 = { "0","1","2","3" };
std::vector<char*> OptionNum4 = { "0","1","2","3","4" };
std::vector<char*> OptionNum15 = { "0(Silent)","1","2","3","4","5","6","7","8","9","10(Normal)" ,"11", "12", "13", "14", "15(Maximum)"};
std::vector<char*> OptionRAMSize = { "64K", "128K", "256K", "512K", "1MB", "2MB" };
#ifdef USE_OVERCLOCK
std::vector<char*> OptionOverClock = { "0(None)","x2(Unsafe)", "x4(Unsafe)" };
#endif // USE_OVERCLOCK
std::vector<char*> OptionMSXType = {"MSX","MSX2","MSX2Plus","MSXTurboR"};
std::vector<char*> OptinJoyPort = { "None", "JoyStick","Mouse as Joystick","Mouse","Arkanoid","TouchPad" };
std::vector<char*> OptionPrinter = {"None", "PrintToFile", "VoiceBox", "+PCM", "COVOX"};
std::vector<char*> OprionStereo3D = { "None", "Anaglyph", "Anaglyph(Color)" };
std::vector<char*> OptionLanguage = { "Auto","Japanese","English" };
std::vector<char*> OptionHQ = { "Normal", "High Quality" };
std::vector<char*> OptionPSG = {"YM2419", "AY-3-8910"};
std::vector<char*> OptionStereo = {"Mono","Stereo"};
std::vector<char*> OptionRegion = { "Auto","NTSC(60Mhz)","PAL(50Mhz)"};
std::vector<char*> OptionCBIOSReg = { "Brazil","European","Japan","US" };
std::vector<char*> OptionUperiod = { "3","2","1","0 (No Skip)" };
std::vector<char*> OptionScreenRes = { "No Scale", "Wide", "Full Screen", "Keep Aspect", "ExtremelyLarge"};
std::vector<char*> OptionVsyncQuality = {"Highest(No Skip)","Higher","High","Normal","Low"};
std::vector<char*> OptionRapidFire = {"OFF", "30/sec", "15/sec", "10/sec", "8/sec", "6/sec", "5/sec", "4/sec", "3/sec","2/sec","1/sec"};
std::vector<char*> OptionMSX0 = {"None", "Encoder(3Dslider)"};

static std::vector<char*> FileCharVec =
{
	"0[Empty]","1[Empty]","2[Empty]","3[Empty]","4[Empty]","5[Empty]","6[Empty]","7[Empty]","8[Empty]","9[Empty]"
};

static std::vector<OptionItem> optionItem =
{
	{"[Back]", 0,0,0,OptionNull},
	{"<Emulation Option>",0,0,0,OptionNull},
	{"MSX Model",2,2,2,OptionMSXType},
#ifdef VDP_V9990
	{"Use V9990",0,0,0,OptionOffOn},
#endif // VDP_V9990
	{"Machine Region",0,0,0,OptionRegion},
	{"Force Japanese BIOS",0,0,0,OptionOffOn},
	{"C-BIOS Region",2,2,2,OptionCBIOSReg},
	{"Force C-BIOS",0,0,0,OptionOffOn},
	{"Skip MSX2 Plus boot screen",0,0,0,OptionOffOn},
	{"Keyboad Region",0,0,0,OptionLanguage},
	{"RAM Size",3,3,3,OptionRAMSize},
	{"Frame Skip",3,2,3,OptionUperiod},
	{"Auto Frame Skip",0,1,0,OptionOffOn},
	{"Accurate AudioSync",1,1,1,OptionOffOn},
	{"AudioSync Quality",3,3,3,OptionVsyncQuality},
	{"Frame Limit",1,1,1,OptionOffOn},
	//{"Load MSXDOS2",0,0,0,OptionOffOn},
	//{"Patch Disk ROM",1,1,1,OptionOffOn},
	{"",0,0,0,OptionNull},
	{"<Graphic Option>",0,0,0,OptionNull},
#ifdef USE_3D
	{"3D Stereoscopic mode",0,0,0,OprionStereo3D },
#endif // USE_3D
	{"800px wide mode",1,1,1,OptionOffOn},
	{"Screen Strech",1,1,1,OptionScreenRes},
	{"Screen Filter",1,1,1,OptionOffOn},
	{"Use Interlace",1,1,1,OptionOffOn},
	{"Use Double Buffer",1,1,1,OptionOffOn},
	{"",0,0,0,OptionNull},
	{"<3DS Option>",0,0,0,OptionNull},
	{"Load  a file when Startup",1,1,1,OptionOffOn},
	{"Menu Language",2,2,2,OptionLanguage},
	{"Scroll Text",1,1,1,OptionOffOn},
	{"Menu select speed",5,5,5,OptionNum},
#ifdef MEMORY_CURSOR_POS
	{"Memory systemmenu cursor",0,0,0,OptionOffOn },
#endif // MEMORY_CURSOR_POS
	{"ShowFPS",0,0,0,OptionOffOn},
	{"New3DS Boost",0,1,0,OptionOnOff},
	{"",0,0,0,OptionNull},
	{"<Sound Option>",0,0,0,OptionNull},
	{"Use FM Sound(Ym2413)",1,0,1,OptionOffOn},
	{"Use FM Sound(Y8950)",1,0,1,OptionOffOn},
	{"Use Y8950 on MSXTurboR",0,0,0,OptionOffOn},
	{"PSG Chip Type",0,0,0,OptionPSG},
	{"Sound Volume",10,10,10,OptionNum15},
	{"Read SCC Plus Wave",0,0,0,OptionOffOn},
	{"",0,0,0,OptionNull},
	{"<Input/Output Hardware Option>",0,0,0,OptionNull},
	{"JoyPort A",1,1,1,OptinJoyPort},
	{"JoyPort B",0,0,0,OptinJoyPort},
	{"Printer Port",0,0,0,OptionPrinter},
	{"",0,0,0,OptionNull},
#ifdef _MSX0
	{"<MSX0 Option>",0,0,0,OptionNull},
	{"Use MSX0",0,0,0,OptionOffOn},
	{"Load X-BASIC2",1,1,1,OptionOffOn},
	{"MSX0 Device A(i2c_a)",0,0,0,OptionMSX0},
	{"",0,0,0,OptionNull},
#endif // _MSX0

	{"<Advanced Sound Option>",0,0,0,OptionNull},
	{"PSG Volume",10,10,10,OptionNum},
	{"SCC Volume",10,10,10,OptionNum},
	{"YM2413 Volume",10,10,10,OptionNum},
	{"Y8950 Volume",10,10,10,OptionNum},
	{"PCM Volume",10,10,10,OptionNum},
	{"Force SCC Plus",0,0,0,OptionOffOn},
	{"Sound Type",0,0,0, OptionStereo},
	{"PSG Quality",0,0,0,OptionHQ},
	{"SCC Qualty",0,0,0,OptionHQ},
	{"YM2413 Quality",1,1,1,OptionHQ},
	{"Use DC Filter",1,0,1,OptionOffOn},
	{"Use RC Filter",0,0,0,OptionOffOn},
	{"Use FIR Filter",1,1,1,OptionOffOn},
	{"",0,0,0,OptionNull},
	{"[Reset Default]",0,0,0,OptionNull}
};

struct KeyConfItem
{
	std::string name;
	const int defaultIdx;
	int currentIdx;
};


static std::vector<KeyConfItem> keyconfItem =
{
	{"[Back]",0,0},
	{"UP(D-PAD)",0,0},
	{"DOWN(D-PAD)",0,0},
	{"LEFT(D-PAD)",0,0},
	{"RIGHT(D-PAD)",0,0},
	{"UP(Analog)",0,0},
	{"DOWN(Analog)",0,0},
	{"LEFT(Analog)",0,0},
	{"RIGHT(Analog)",0,0},
	{"A Button",0,0},
	{"B Button",0,0},
	{"X Button",'x','x'},
	{"Y Button",'m','m'},
	{"L Button",KBD_F1,KBD_F1},
	{"R Button",KBD_F2,KBD_F2},
	{"Start Button",0,0},
	{"Select Button",KBD_F5,KBD_F5},
	{"ZL Button",KBD_CONTROL,KBD_CONTROL},
	{"ZR Button",KBD_GRAPH,KBD_GRAPH},
	{"[Rapidfire Setting]",0,0},
	{"[Reset Default]",0,0}
};

struct KeyMenuItem
{
	std::string name;
	int keyidx;
};

static std::vector<KeyMenuItem> keyValueItem =
{
	{"[Back]",0},{"KEYPAD 0",KBD_NUMPAD0},{"KEYPAD 1",KBD_NUMPAD1},{"KEYPAD 2",KBD_NUMPAD2},{"KEYPAD 3",KBD_NUMPAD3},{"KEYPAD 4",KBD_NUMPAD4}
	,{"KEYPAD 5",KBD_NUMPAD5},{"KEYPAD 6",KBD_NUMPAD6},{"KEYPAD 7",KBD_NUMPAD7},{"KEYPAD 8",KBD_NUMPAD8},{"KEYPAD 9",KBD_NUMPAD9}
	,{"UP",KBD_UP},{"RIGHT",KBD_RIGHT},{"DOWN",KBD_DOWN},{"LEFT",KBD_LEFT},{"Space",KBD_SPACE},{"Shift",KBD_SHIFT},{"Return",KBD_ENTER}
	,{"Home",KBD_HOME},{"Ins/Del",KBD_INSERT},{"Graph",KBD_GRAPH},{"Kana",KBD_COUNTRY},{"CTRL",KBD_CONTROL},{"Stop",KBD_STOP},{"Esc",KBD_ESCAPE}
	,{"Tab",KBD_TAB},{"BackSpace",KBD_BS},{"Copy",KBD_DELETE},{"Caps",KBD_CAPSLOCK}
	,{"F1",KBD_F1},{"F2",KBD_F2},{"F3",KBD_F3},{"F4",KBD_F4},{"F5",KBD_F5}
	,{"1",'1'},{"2",'2'},{"3",'3'},{"4",'4'},{"5",'5'},{"6",'6'},{"7",'7'},{"8",'8'},{"9",'9'},{"0",'0'}
	,{"a",'a'},{"b",'b'},{"c",'c'},{"d",'d'},{"e",'e'},{"f",'f'},{"g",'g'},{"h",'h'},{"i",'i'},{"j",'j'}
	,{"k",'k'},{"l",'l'},{"m",'m'},{"n",'n'},{"o",'o'},{"p",'p'},{"q",'q'},{"r",'r'},{"s",'s'},{"t",'t'}
	,{"u",'u'},{"v",'v'},{"w",'w'},{"x",'x'},{"y",'y'},{"z",'z'},{"-",'-'},{"^",'^'},{"YEN",'\\'},{"@",'@'},{"[",'['}
		,{";",';'},{":",'"'},{"]",']'},{",",','},{".",'.'},{"/",'/'},{"_",'_'},{"[Default Value]",-1}
//	,{";",';'},{":",':'},{"]",']'},{",",','},{".",'.'},{"/",'/'},{"_",'_'},{"[Default Value]",-1}
};

static std::vector<std::string> RapidMenuVec =
{
	"[Back]",
	"A Button",
	"B Button",
	"X Button",
	"Y Button",
	"L Button",
	"R Button",
	"Start Button",
	"Select Button",
	"ZL Button",
	"ZR Button",
	"[Reset Default]"
};

static std::vector<std::string> menuItem =
{
	"[Back]",
	"[Reset]",
	"",
	"[File Open]",
	"",
	"[KeyConfig]",
	"[Option]",
	"",
	"[Quit fMSX]",
	"",
	"<ROM Files>",
	"[LoadROM(Slot1)]",
	"[LoadROM(Slot2)]",
	"[Load Konami SCC+ ROM]",
	"[Load Ese SCC 512k ROM]",
	"[Change ROM Mapper]",
	"[Apply IPS Patch]",
	"[Eject ROM]",
	"",
	"<Disk Files>",
	"[LoadDisk(DriveA)]",
	"[LoadDisk(DriveB)]",
	"[SaveDisk(DriveA)]",
	"[SaveDisk(DriveB)]",
	"[Create New Disk]",
	"[Apply IPS Patch]",
	"[Eject Disk]",
#ifdef HDD_NEXTOR
	"[Load HardDisk]",
#endif // HDD_NEXTOR
	"",
	"<Cassette Tape Files>",
	"[Load Cassette Tape]",
	"[Load SaveDATA Cassette Tape]",
	"[Create new SaveDATA Cassette Tape]",
	"[Rewind Cassette Tape]",
	"",
	"[State Load]",
	"[State Save]",
	"[Load Screen Shot]",
	"[Load Screen Shot](Show Keyboard)",
	"[Save Screen Shot]",
	"[Load Reference Image]",
	"[Fast Forward]",
	"[V9990 Dual Screen]",
	"[OverClockR800(Unsafe)]",
	"[Cheat]",
#ifdef DEBUG_ENABLE
		"[Debug]",
#endif // DEBUG_ENABLE
	""
};

u8* fbbottom;
bool AllowWide = false;
bool IsWide = false;
bool IsOld2DS = false;
bool AutoFrameSkip = false;
bool IsScreenShot = false;
bool ShowDebugMsg3DS = false;
bool IsSmallScrShot = false;
unsigned char ScreenRes = 0;
int ZipIndex = -1;
std::string zipfileStr = "";
C3D_RenderTarget* TopRenderTarget;
C3D_RenderTarget* WideRenderTarget;
C3D_RenderTarget* BottomRenderTartget;
#ifdef USE_3D
C3D_RenderTarget* TopRenderTargetR;
#endif // USE_3D


//byte ShowTenkey = 0;

byte KeyMaps3DS[20] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

void InitCitro()
{
	gfxInitDefault();
	gfxSetDoubleBuffering(GFX_BOTTOM, false);
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
	C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
	C2D_Prepare();

	//top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
	//bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

	gfxSetWide(true);
	WideRenderTarget = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);

	gfxSetWide(false);
	TopRenderTarget = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);

#ifdef USE_3D
	TopRenderTargetR = C2D_CreateScreenTarget(GFX_TOP, GFX_RIGHT);
	//gfxSet3D(true);
#endif // USE_3D

	BottomRenderTartget = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

	//C3D_TexInit(&HUDTex, 512, 512, GPU_RGBA8);
	//HUDSubTex.width = 512;
	//HUDSubTex.height = 512;
	//HUDSubTex.left = 0.0f;
	//HUDSubTex.right = 1.0f;
	//HUDSubTex.top = 1.0f;
	//HUDSubTex.bottom = 0.0f;
	//HUDImage.tex = &HUDTex;
	//HUDImage.subtex = &HUDSubTex;

	C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
	C2D_TargetClear(TopRenderTarget, Color_Black);
	C2D_SceneBegin(TopRenderTarget);
	//C2D_TargetClear(top, Color_Black);
	//C2D_SceneBegin(top);
	C3D_FrameEnd(0);

	C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
	C2D_TargetClear(BottomRenderTartget, Color_Black);
	C2D_SceneBegin(BottomRenderTartget);

	C3D_FrameEnd(0);
}


void Init3DS()
{
	for (int i = 0; i < optionItem.size(); i++)
	{
		optionMap[optionItem[i].name] = i;
	}
	for (int i = 0; i < keyValueItem.size(); i++)
	{
		keyMenuMap[keyValueItem[i].keyidx] = i;
		KeyNameVec.push_back((char*)keyValueItem[i].name.c_str());
	}
	recentlylist.clear();
	recentftypelist.clear();

	mkdir("/FMSX3DS", 0777);
	mkdir("/FMSX3DS/CHEAT", 0777);
	mkdir("/FMSX3DS/SAVEDISK", 0777);
	mkdir("/FMSX3DS/SNAP", 0777);
	mkdir("/FMSX3DS/SRAM", 0777);
	mkdir("/FMSX3DS/STATE", 0777);
	mkdir("/FMSX3DS/TAPE", 0777);

	ReadOptionCFG();
	LoadOption(true);
	for (int i = 0; i < 20; i++)
	{
		KeyMaps3DS[i] = keyconfItem[i + 1].currentIdx;
	}

	consoleInit(GFX_BOTTOM, 0);
	InitCitro();

	C3D_TexInit(&HUDTex, 512, 512, GPU_RGBA8);
	HUDSubTex.width = 512;
	HUDSubTex.height = 512;
	HUDSubTex.left = 0.0f;
	HUDSubTex.right = 1.0f;
	HUDSubTex.top = 1.0f;
	HUDSubTex.bottom = 0.0f;
	HUDImage.tex = &HUDTex;
	HUDImage.subtex = &HUDSubTex;

	cfguInit();
	u8 language = 0;
	Result res;
	res = CFGU_GetSystemLanguage(&language);
	systemLanguage = (int)language>1 ? 1 :(int)language;
	u8 model3ds = 0;
	res = CFGU_GetSystemModel(&model3ds);
	IsNew3DS = (model3ds>1 && model3ds !=3) ? 1 : 0;
	IsOld2DS = (model3ds == 3) ? 1 : 0;
	aptSetHomeAllowed(false);
	//aptSetSleepAllowed(false);
	//if (IsNew3DS)aptSetHomeAllowed(true);
	//else aptSetHomeAllowed(false);
	cfguExit();

	KeySurface = SDL_LoadBMP(Get3DSPath("MSXKeyboard.bmp").c_str());
	KeySurfaceEUR = SDL_LoadBMP(Get3DSPath("MSXKeyboardEUR.bmp").c_str());
	KeySurfaceGra = SDL_LoadBMP(Get3DSPath("MSXKeyGra.bmp").c_str());
	KeySurfaceGraEUR = SDL_LoadBMP(Get3DSPath("MSXKeyGraEUR.bmp").c_str());
	KetySurfaceGraShift = SDL_LoadBMP(Get3DSPath("MSXKeyGraShift.bmp").c_str());

	SDL_Surface* HUDSurface = SDLSurfaceExtract(KeySurface, 280, 150, KeySurface->w-280, KeySurface->h-150);
	SDLSurfaceToC3DTexData(HUDSurface, HUDTex.data, HUDTex.width, HUDTex.height);

	if (currJoyMode[0] < HIDE_KEYBOARD)DrawSurfaceRectBottom(KeySurface, 0, 320, 0, 240);
	else if (currJoyMode[1] < HIDE_KEYBOARD)DrawSurfaceRectBottom(KeySurface, 0, 320, 0, 240);
	else DrawMouseScr();
	gfxSetScreenFormat(GFX_TOP, GSP_BGR8_OES);
	gfxSetDoubleBuffering(GFX_TOP, IsDoubleBuffer);

#ifdef DEBUG_LOG
	//char* tempbuf;
	//tempbuf = (char*)malloc(unzipsize);
	//int err = unzReadCurrentFile(zipFile, tempbuf, unzipsize);
	//F = fmemopen(tempbuf, unzipsize, _mode);

	//debugFile = fopen("/FMSX3DS/DebugLog.txt", "w");
	//debugFile = open_memstream(&debugBuf, &debugBufSize);
	debugFile = fmemopen(debugBuf, 0x10000, "r+");
	//Verbose = 0x2C;	/*  0x02:VDP Command,  0x04:Disk IO,  0x8:MAP ROM,  0x20:IO Port,  0x40:MSXTurboR */
	//Verbose = 0x20;
	Verbose = 0xA0;
	//Verbose = 0x44;
	//Verbose = 9;
#endif // DEBUG_LOG
}


void Quit3DS()
{
	//C3D_RenderTargetDelete(top);
	C3D_RenderTargetDelete(TopRenderTarget);
	C3D_RenderTargetDelete(WideRenderTarget);
	C3D_RenderTargetDelete(BottomRenderTartget);
#ifdef USE_3D
	C3D_RenderTargetDelete(TopRenderTargetR);
#endif // USE_3D
	C2D_TextBufDelete(textbuf);

	dirstring.clear();
	dirstring.shrink_to_fit();
	ftypevec.clear();
	ftypevec.shrink_to_fit();
	ftypestring.clear();
	ftypestring.shrink_to_fit();
	recentlylist.clear();
	recentlylist.shrink_to_fit();
	recentftypelist.clear();
	recentftypelist.shrink_to_fit();

	TrashAudio();

#ifdef DEBUG_LOG
	fclose(debugFile);
#endif // DEBUG_LOG
}


//Browse ROM, Disk, Cassette Tape, Cheat etc.
void BrowseROM(int slotid, int browsetype)
{
	textbuf = C2D_TextBufNew(4096);
	scrollTime = SDL_GetTicks();
	std::string msgstr = "";
	switch (browsetype)
	{
		case BROWSE_ROM :
			msgstr = "[Select a ROM file]";
			break;
		case BROWSE_DISK:
			msgstr = "[Select a Disk Image]";
			break;
		case BROWSE_TAPE:
			msgstr = "[Select a cassette tape image]";
			break;
		case BROWSE_CHEAT:
			msgstr = "[Select a cheat file]";
			break;
		case BROWSE_PATCH:
			msgstr = "[Select a IPS patch file]";
			break;
		case BROWSE_IMG:
			msgstr = "[Select a reference image file]";
			break;
#ifdef HDD_NEXTOR
		case BROWSE_HDD:
			msgstr = "[Select a HDD image]";
			break;
#endif // HDD_NEXTOR

		case BROWSE_START | BROWSE_ALL:
		case BROWSE_ALL:
			msgstr = "[Select a file]";
			break;
		default:
			break;
	}
	int startid = menucnt-2;
	GetMovedMenuIndex(menucnt, startid, 0, dirstring.size() - 1);
	if (dirstring.size() < 1)OpenDirectoryDir(currdir, browsetype);
	else
	{
		dirstringRebuid(browsetype);
		PrintMenu(msgstr.c_str(), startid);
	}
	while (aptMainLoop())
	{
		SDL_Delay(10);
		hidScanInput();
		u32 kDown = hidKeysDown();
		u32 kHeld = hidKeysHeld();
		bool IsAPress = false, IsBPress = false, IsLPress = false, IsRPress = false;
		if (kDown & KEY_START)
			return;
		if (kDown & KEY_A)IsAPress = true;
		else if (kDown & KEY_B)IsBPress = true;
		else if (kDown & KEY_Y)
		{
			OpenDirectoryDir("/",browsetype);
			currdir = "/";
			scrollTime = SDL_GetTicks();
		}
		else if (kHeld & KEY_UP)
		{
			GetMovedMenuIndex(menucnt, startid, -1, NULL);
			scrollTime = SDL_GetTicks();
			SDL_Delay(TextDelay);
		}
		else if (kHeld & KEY_DOWN)
		{
			GetMovedMenuIndex(menucnt, startid, 1, dirstring.size() - 1);
			scrollTime = SDL_GetTicks();
			SDL_Delay(TextDelay);
		}
		else if (kHeld & KEY_LEFT) IsLPress = true;
		else if (kHeld & KEY_RIGHT)IsRPress = true;
		hidTouchRead(&tp);
		int px = tp.px;
		int py = tp.py;
		int oldpx = oldtp.px;
		int oldpy = oldtp.py;
		if (px != 0 && py != 0)
		{
			if (py > textSize && py < 240 - textSize && px>textStartPos && px < textEndPos)
			{
				menucnt = py / textSize + startid - 1;
				SDL_Delay(TextDelay);
			}
			else
			{
				if (CheckScrollButtonL(px, py))IsLPress = true;
				if (CheckScrollButtonR(px, py))IsRPress = true;
			}
		}
		if (kDown & KEY_TOUCH)
		{
			kDown &= ~KEY_TOUCH;
			if (CheckOKButton(px, py))IsAPress = true;
			if (CheckCancelButton(px, py))IsBPress = true;
		}
		oldtp.px = px;
		oldtp.py = py;
		if (IsAPress && menucnt >= 0 && menucnt < dirstring.size())
		{
			if (dirstring[menucnt] == "")
			{
				continue;
			}
			if (dirstring[menucnt] == "[Back]")
			{
				return;
			}
			else if (dirstring[menucnt]=="[Start with no cartridge.]")
			{
				return;
			}
			else if (dirstring[menucnt] == "[Parent Directory]")
			{
				size_t dirint = currdir.find_last_of("/");
				if (dirint > 1 && currdir.size() > 2)currdir.erase(dirint);
				else currdir = "/";
				startid = 0;
				OpenDirectoryDir(currdir,browsetype);
				continue;
			}
			else if (dirstring[menucnt] == "[Load Recently Used]")
			{
				BrowseLoadRecently(slotid, browsetype);
				return;
			}
			else if(dirstring[menucnt] == "[Frequently Used Folder]")
			{
				currdir = BrowseFrequentlyUsedFolder(slotid, browsetype);
				if (currdir.size()>0)
				{
					startid = 0;
					OpenDirectoryDir(currdir, browsetype);
				}
				continue;

				//dirstringRebuid(browsetype);
				//PrintMenu(msgstr.c_str(), startid);
			}
			else if(dirstring[menucnt]=="[Create Blank Disk]")
			{
				CreateBlankDisk(slotid);
			}
			size_t flen = strlen(dirstring[menucnt].c_str());
			if (flen >= 4)
			{
				const char* extname = &dirstring[menucnt].c_str()[flen - 4];
				const char* extsname = &dirstring[menucnt].c_str()[flen-3];
				std::string cfstring = currdir + "/" + dirstring[menucnt];
				zipfileStr = "";
				if(strcasecmp(extname,".ZIP")==0)
				{
					extname = BrowseZip(cfstring.c_str(), NULL);
				}
				else if(strcasecmp(extsname,".GZ")==0)
				{
					std::string gzname = dirstring[menucnt];
					gzname.pop_back(); gzname.pop_back(); gzname.pop_back();
					extname = GetFileExtension(gzname);
				}
				if (strcasecmp(extname,".ROM")==0)
				{
					if (ROMStr[slotid] != cfstring || zipfileStr.length() > 0)
					{
						if (LoadCart(cfstring.c_str(), slotid, MAP_GUESS))
						{
							ROMStr[slotid] = cfstring;
							AddRecentlyList(cfstring, ".ROM");
							ResetMSX(Mode, RAMPages, VRAMPages);
							//ResetSound();
						}
					}
					return;
				}
				else if (strcasecmp(extname, ".MX1") == 0)
				{
					if (ROMStr[slotid] != cfstring || zipfileStr.length() > 0)
					{
						if (LoadCart(cfstring.c_str(), slotid, MAP_GUESS))
						{
							ROMStr[slotid] = cfstring;
							AddRecentlyList(cfstring, ".MX1");
							ResetMSX(Mode, RAMPages, VRAMPages);
							//ResetSound();
						}
					}
					return;
				}
				else if (strcasecmp(extname, ".MX2") == 0)
				{
					if (ROMStr[slotid] != cfstring || zipfileStr.length() > 0)
					{
						if (LoadCart(cfstring.c_str(), slotid, MAP_GUESS))
						{
							ROMStr[slotid] = cfstring;
							AddRecentlyList(cfstring, ".MX2");
							ResetMSX(Mode, RAMPages, VRAMPages);
							//ResetSound();
						}
					}
					return;
				}
				else if(strcasecmp(extname, ".DSK")==0)
				{
#ifdef HDD_NEXTOR
					if (browsetype == BROWSE_HDD)
					{
						if (ChangeHDDWithFormat(0, cfstring.c_str(), FMT_MSXDSK))
						{
							LoadPatchedNEXTOR();
							return;
						}
					}
#endif // HDD_NEXTOR

					AutoSaveDisk(slotid);
					std::string currstr = getZipSaveDiskPath(cfstring, extname);
					std::string savestr;
					savestr = cfstring;
					struct stat buf;
					if (stat(currstr.c_str(), &buf) == 0)cfstring = currstr;

					if (DiskRawStr[slotid] != cfstring || zipfileStr.length() > 0)
					{
						//if (ChangeDisk(slotid, cfstring.c_str()))
						if(ChangeDiskWithFormat(slotid,cfstring.c_str(),FMT_MSXDSK))
						{
							DiskStr[slotid] = currstr;
							DiskRawStr[slotid] = cfstring;
							AddRecentlyList(savestr, ".DSK");
							//AddRecentlyList(cfstring, ".DSK");
						}
					}
					//else
					//{
					//	DiskWrited[slotid] = 0;
					//}
					return;
				}
				else if (strcasecmp(extname, ".CAS")==0)
				{
					if (CASStr != cfstring || zipfileStr.length() > 0)
					{
#ifdef TURBO_R
						if(MODEL(MSX_MSXTR))ShowMessage3DS("MSXTurboR doesn't support cassette tapes.", "Chnage MSX Hardware Model and try again.");
#endif // TURBO_R
						if (ChangeTape(cfstring.c_str()))
						{
							RewindTape();
							CASStr = cfstring;
							AddRecentlyList(cfstring, ".CAS");
						}
					}
					return;
				}
				else if (strcasecmp(extname, ".PNG") == 0 || strcasecmp(extname, ".JPG") == 0 || strcasecmp(extname, ".JPEG") == 0 || strcasecmp(extname,".BMP")==0)
				{
					ScreenShotSurface = IMG_Load(cfstring.c_str());
					ScreenShotOffx = ScreenShotOffy = 0;
					IsScreenShot = true;
					return;
				}
				else if (strcasecmp(extname, ".MCF") == 0)
				{
					if (LoadMCF(cfstring.c_str()) != 0)
					{
						BrowseMCF();
					}
					return;
				}
				else if (strcasecmp(extname, ".CHT") == 0)
				{
					if (LoadCHT(cfstring.c_str()) != 0)Cheats(CHTS_ON);
					return;
				}
				else if (strcasecmp(extname, ".IPS") == 0)
				{
					std::vector<std::string> ipsvec;
					std::vector<char*> ipsmenu;
					ipsvec.clear();
					ipsmenu.clear();
					ipsvec.push_back("Back");
					ipsmenu.push_back("[Back]");
					if (ROMStr[0].size() > 0)
					{
						ipsvec.push_back(ROMStr[0]);
						ipsmenu.push_back(StringToChar((std::string)"Slot0: " + ROMStr[0]));
					}
					if (ROMStr[1].size() > 0)
					{
						ipsvec.push_back(ROMStr[1]);
						ipsmenu.push_back(StringToChar((std::string)"Slot1: " + ROMStr[1]));
					}
					if (DiskRawStr[0].size() > 0)
					{
						ipsvec.push_back(DiskRawStr[0]);
						ipsmenu.push_back(StringToChar((std::string)"DiskA: " + DiskRawStr[0]));
					}
					if (DiskStr[1].size() > 0)
					{
						ipsvec.push_back(DiskRawStr[1]);
						ipsmenu.push_back(StringToChar((std::string)"DiskB: " + DiskRawStr[1]));
					}
					if (CASStr.size() > 0)
					{
						ipsvec.push_back(CASStr);
						ipsmenu.push_back(StringToChar((std::string)"Cassette: " + CASStr));
					}
					if (ipsvec.size() < 2)
					{
						DrawMessage("Load the ROM to patched, first.", NULL, 20, 100, 1000, true);
						return;
					}
					//ipsvec.push_back("NewFile");
					//ipsmenu.push_back("Open new file for patch");
					int ipsidx = BrowseInt("Select a file to patch.", ipsmenu, 0, 0, false);

					if (ipsvec[ipsidx] == "Back")return;
					else if (ipsvec[ipsidx] == ROMStr[0])
					{
						std::vector<byte> patchvec = IPSPatchVector(cfstring.c_str(), ROMStr[0].c_str());
						IPSPatchSize = patchvec.size();
						IPSPatchBuf = patchvec.data();
						//IPSPatchBuf = PatchBuf;
						LoadCart("hogehoge", 0, MAP_GUESS);
						IPSPatchSize = 0;
					}
					else if (ipsvec[ipsidx] == ROMStr[1])
					{
						std::vector<byte> patchvec = IPSPatchVector(cfstring.c_str(), ROMStr[1].c_str());
						IPSPatchSize = patchvec.size();
						IPSPatchBuf = patchvec.data();
						//IPSPatchBuf = PatchBuf;
						LoadCart("hogehoge", 1, MAP_GUESS);
						IPSPatchSize = 0;
					}
					else if (ipsvec[ipsidx] == DiskRawStr[0])
					{
						std::vector<byte> patchvec = IPSPatchVector(cfstring.c_str(), DiskRawStr[0].c_str());
						IPSPatchSize = patchvec.size();
						IPSPatchBuf = patchvec.data();
						//ChangeDisk(0, DiskRawStr[0].c_str());
						ChangeDiskWithFormat(0, DiskRawStr[0].c_str(),FMT_MSXDSK);
						IPSPatchSize = 0;
						ResetMSX(Mode, RAMPages, VRAMPages);
					}
					else if (ipsvec[ipsidx] == DiskRawStr[1])
					{
						std::vector<byte> patchvec = IPSPatchVector(cfstring.c_str(), DiskRawStr[1].c_str());
						IPSPatchSize = patchvec.size();
						IPSPatchBuf = patchvec.data();
						//ChangeDisk(0, DiskRawStr[1].c_str());
						ChangeDiskWithFormat(0, DiskRawStr[1].c_str(),FMT_MSXDSK);
						IPSPatchSize = 0;
						ResetMSX(Mode, RAMPages, VRAMPages);
					}
					else if(ipsvec[ipsidx]==CASStr)
					{
						fclose(CasStream); CasStream = 0;
						std::vector<byte> patchvec = IPSPatchVector(cfstring.c_str(), CASStr.c_str());
						IPSPatchSize = patchvec.size();
						IPSPatchBuf = patchvec.data();
						ChangeTape(CASStr.c_str());
						rewind(CasStream);
						IPSPatchSize = 0;
						ResetMSX(Mode, RAMPages, VRAMPages);
					}
					return;
				}
			}
			if (currdir == "" || currdir == "/")
			{
				currdir = "/" + dirstring[menucnt];
			}
			else
			{
				currdir += "/" + dirstring[menucnt];
			}
			startid = 0;
			OpenDirectoryDir(currdir, browsetype);
			//menucnt = 0;
		}
		if (IsBPress)
		{
			if (currdir == "" || currdir == "/")return;
			scrollTime = SDL_GetTicks();
			size_t dirint = currdir.find_last_of("/");
			if (dirint > 1 && currdir.size() > 2)currdir.erase(dirint);
			else currdir = "/";
			startid = 0;
			OpenDirectoryDir(currdir, browsetype);
		}
		if (IsLPress)
		{
			GetMovedMenuIndex(menucnt, startid, -(textColumn - 2), NULL);
			scrollTime = SDL_GetTicks();
			SDL_Delay(TextDelay);
		}
		if (IsRPress)
		{
			GetMovedMenuIndex(menucnt, startid, (textColumn - 2), dirstring.size());
			scrollTime = SDL_GetTicks();
			SDL_Delay(TextDelay);
		}
		BrowseHomeButton();
		if (ExitNow == 1)return;
		PrintMenu(msgstr.c_str(),startid);
	}
}


std::vector<byte> IPSPatchVector(const char* IPSName, const char* SourceName)
{
	FILE* ipsFile, * sourceF;
	byte* ipsBuf, * SourceBuf;
	unsigned int offseti, sizei, ipsize = 0;
	int sourceSize;
	byte Buf[16];

	ipsFile = zipfopen(IPSName, "rb");
	const char* extname = &SourceName[strlen(SourceName) - 4];
	if (strcasecmp(extname, ".ZIP") == 0)
	{
		BrowseZip(SourceName, NULL);
	}
	sourceF = zipfopen(SourceName, "rb");
	std::vector<byte> patchvec, nullvec;
	fseek(sourceF, 0, SEEK_END);
	sourceSize = ftell(sourceF);
	SourceBuf = (byte*)malloc(sourceSize);
	rewind(sourceF);
	sourceSize = fread(SourceBuf, 1, sourceSize, sourceF);
	for (int i = 0; i < sourceSize; i++)
	{
		patchvec.push_back(SourceBuf[0]);
		SourceBuf++;
	}
	if (fread(Buf, 1, 5, ipsFile) != 5)
	{
		fclose(ipsFile);
		fclose(sourceF);
		return nullvec;
	}
	for (;;)
	{
		if (fread(Buf, 1, 5, ipsFile) != 5)break;
		if (Buf[0] == 'E' && Buf[1] == 'O' && Buf[2] == 'F')break;
		offseti = (((unsigned int)Buf[0] << 16) & 0x00FF0000) | (((unsigned int)Buf[1] << 8) & 0x0000FF00)
			| ((unsigned int)Buf[2] & 0x000000FF);
		if(offseti == 0xFFFFFF)break;
		sizei = (((unsigned int)Buf[3] << 8) & 0xFF00) | ((unsigned int)Buf[4] & 0xFF);
		if (sizei == 0)
		{
			if (fread(Buf, 1, 3, ipsFile) != 3)break;
			sizei = (((unsigned int)Buf[0] << 8) & 0xFF00) | ((unsigned int)Buf[1] & 0xFF);
			if (ipsize < offseti + sizei) ipsize = offseti + sizei;
		}
		else
		{
			if (ipsize < offseti + sizei) ipsize = offseti + sizei;
			if (fseek(ipsFile, sizei, SEEK_CUR) < 0) break;
		}
	}
	rewind(ipsFile);
	ipsBuf = (byte*)malloc(ipsize);
	int readsize = fread(ipsBuf, 1, ipsize, ipsFile);
	*ipsBuf = 0;
	ipsBuf += 5;
	for (;;)
	{
		if (ipsBuf[0] == 'E' && ipsBuf[1] == 'O' && ipsBuf[2] == 'F')
		{
			ipsBuf += 3;
			break;
		}
		else
		{
			unsigned int offseti = (((unsigned int)ipsBuf[0] << 16) & 0x00FF0000) | (((unsigned int)ipsBuf[1] << 8) & 0x0000FF00)
				| ((unsigned int)ipsBuf[2] & 0x000000FF);
			ipsBuf += 3;
			unsigned int sizei = (((unsigned int)ipsBuf[0] << 8) & 0xFF00) | ((unsigned int)ipsBuf[1] & 0xFF);
			ipsBuf += 2;
			if (sizei == 0)
			{
				unsigned int rlei = (((unsigned int)ipsBuf[0] << 8) & 0xFF00) | ((unsigned int)ipsBuf[1] & 0xFF);
				unsigned int dataSize = offseti + rlei;
				if (dataSize > sourceSize)
				{
					patchvec.resize(dataSize);
					sourceSize = dataSize;
				}
				ipsBuf += 2;
				for (int i = 0; i < rlei; i++)
				{
					patchvec[i + offseti] = ipsBuf[0];
				}
				ipsBuf++;
			}
			else
			{
				unsigned int dataSize = offseti + sizei;
				if (dataSize > sourceSize)
				{
					patchvec.resize(dataSize);
					sourceSize = dataSize;
				}
				for (int i = 0; i < sizei; i++)
				{
					patchvec[i + offseti] = ipsBuf[i];
				}
				ipsBuf += sizei;
			}
		}
	}

	fclose(ipsFile);
	fclose(sourceF);
	return patchvec;
}


void BrowseMCF()
{
	std::vector<char*> mcfchrvec;
	mcfchrvec.clear();
	mcfchrvec.push_back("[Back]");
	for (int i = 0; i < MCFCount; i++)
	{
		mcfchrvec.push_back(MCFEntries[i].Note);
	}
	int mcfidx = BrowseInt("Select Cheat",mcfchrvec ,0,0,false);
	if (mcfidx)
	{
		std::stringstream ss0, ss1;
		ss0 << std::hex << (MCFEntries[mcfidx - 1].Addr & 0xFFFF);
		ss1 << std::hex << (MCFEntries[mcfidx - 1].Data & 0xFF);
		std::string mcfstr = ss0.str() + "-" + ss1.str();
		if (AddCheat(mcfstr.c_str()) != 0)Cheats(CHTS_ON);
	}
}


int CreateBlankDisk(int slotid)
{
	std::string newdskstring = latestPath.size() < 2 ? "/FMSX3DS/fMSX.dsk" : latestPath;
	std::string diskname(newdskstring);
	diskname.erase(diskname.begin(), diskname.begin() + ((int)diskname.find_last_of("/") + 1));
	diskname.erase(diskname.find_last_of("."));

	diskname = "User_" + diskname;
	static SwkbdState swkbd;
	SwkbdButton button = SWKBD_BUTTON_NONE;
	static char kbdchar[256];
	swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 2, -1);
	swkbdSetInitialText(&swkbd, diskname.c_str());
	swkbdSetHintText(&swkbd, "Enter the name of new disk.");
	button = swkbdInputText(&swkbd, kbdchar, sizeof(kbdchar));
	if (std::string(kbdchar).size() < 1)
	{
		DrawMessage("You cancel creating the new disk.",NULL, 10, 100, 1, true);
		return 0;
	}
	diskname = std::string(kbdchar) + ".DSK";

	newdskstring = newdskstring.erase(newdskstring.find_last_of("/")) + "/" + diskname;

	std::string filestr = std::string(newdskstring);
	filestr.erase(filestr.begin(), filestr.begin() + ((int)filestr.find_last_of("/") + 1));
	newdskstring = "/FMSX3DS/SAVEDISK/" + filestr;
	//newdskstring = "/FMSX3DS/" + filestr;
	newdskstring = newdskstring.erase(newdskstring.find_last_of(".")) + ".DSK";
	newdskstring = GetUniqueName(newdskstring.erase(newdskstring.find_last_of(".")), ".DSK");

	DrawMessage("Creating disk", filestr.c_str(), 10, 100, 1, false);
	if (DiskWrited[slotid] == 1)
	{
		if (DiskStr[slotid].size() > 2)
		{
			if (SaveFDI(&FDD[slotid], DiskStr[slotid].c_str(), FMT_MSXDSK))DiskWrited[slotid] = 0; ;
		}
	}
	FormatFDI(&FDD[slotid], FMT_MSXDSK);
	if(SaveFDI(&FDD[slotid], newdskstring.c_str(), FMT_MSXDSK)==0)return 0;
	if (BrowseOK("You Created a New Disk.","Do you want to Insert that?") == true)
	{
		if (ChangeDisk(slotid, newdskstring.c_str()))
		{
			DiskWrited[slotid] = 0;
			DiskStr[slotid] = newdskstring;
			//DiskStr[slotid] = "/FMSX3DS/SAVEDISK/" + filestr;
			DiskRawStr[slotid] = newdskstring;
			//AddRecentlyList("/FMSX3DS/SAVEDISK/" + filestr, ".DSK");
			AddRecentlyList(newdskstring,".DSK");
		}
	}
	return 1;
}


const char* BrowseZip(const char* path, const char* extchar)
{
	ZipIndex = -1;
	const char* retchar = "";
	unzFile zipFile = unzOpen(path);
	if (!zipFile)return retchar;
	unz_global_info globalInfo;
	if (unzGetGlobalInfo(zipFile, &globalInfo) != UNZ_OK)
	{
		unzClose(zipFile);
		return retchar;
	}
	std::vector<std::string> zipstrvec;
	zipstrvec.clear();
	std::vector<int> zipidvec;
	zipidvec.clear();
	for (int i = 0; i < globalInfo.number_entry; i++)
	{
		char fileName[1024];
		unz_file_info fileInfo;
		if (unzGetCurrentFileInfo(zipFile, &fileInfo, fileName, sizeof(fileName), NULL, 0, NULL, 0) == UNZ_OK)
		{
			std::string filestr = fileName;
			const char* ext = &fileName[filestr.find_last_of(".")];
			if (extchar == NULL)
			{
				if (strcasecmp(ext, ".ROM") == 0 || strcasecmp(ext, ".DSK") == 0 || strcasecmp(ext, ".CAS") == 0 || strcasecmp(ext, ".MX1") == 0
					|| strcasecmp(ext, ".MX2") == 0 || strcasecmp(ext, ".IPS") == 0)
				{
					zipstrvec.push_back(filestr);
					zipidvec.push_back(i);
				}
			}
			else
			{
				if (strcasecmp(ext, extchar))
				{
					zipstrvec.push_back(filestr);
					zipidvec.push_back(i);
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
	unzClose(zipFile);
	if (zipstrvec.size() == 1)
	{
		ZipIndex = -1;
		return GetFileExtension(zipstrvec[0]);
	}
	std::vector<char*> zipnamevec;
	zipnamevec.clear();
	for (int i = 0; i < zipstrvec.size(); i++)
	{
		zipnamevec.push_back(StringToChar(zipstrvec[i]));
	}
	int zipid = BrowseInt("      [Select a file in th zip archive.]", zipnamevec, 0, 0, false);
	if (zipid >= 0)
	{
		ZipIndex = zipidvec[zipid];
		zipfileStr = zipnamevec[zipid];
		return GetFileExtension(zipnamevec[zipid]);
	}
	return retchar;
}


bool CheckOKButton(int px, int py)
{
	if (py <= 240 - OKButtonSize || py > 240)return false;
	if (px > 220 && px < 320)return true;
	if (px > 220 - OKButtonSize && px <= 220)
	{
		int fpx = px - 220;
		int fpy = py - 240;
		float dist = sqrtf(fpx * fpx + fpy * fpy);
		if (dist < OKButtonSize)return true;
	}
	return false;
}


bool CheckCancelButton(int px, int py)
{
	if (py <= 240 - OKButtonSize || py > 240)return false;
	if (px > 0 && px < 100)return true;
	if (px > 100 && px <= 100 + OKButtonSize)
	{
		int fpx = px - 100;
		int fpy = py - 240;
		float dist = sqrtf(fpx * fpx + fpy * fpy);
		if (dist < OKButtonSize)return true;
	}
	return false;
}


bool CheckScrollButtonL(int px, int py)
{
	if (px > textStartPos)return false;
	if (py >= 120 - scrollButtonSize * 2 && py <= 120 + scrollButtonSize * 2)
	{
		int fpx = px + scrollButtonSize;
		int fpy = py - 120;
		float dist = sqrtf(fpx * fpx + fpy * fpy);
		if (dist < scrollButtonSize * 2)return true;
	}
	return false;
}


bool CheckScrollButtonR(int px, int py)
{
	if (px < textEndPos)return false;
	if (py >= 120 - scrollButtonSize * 2 && py <= 120 + scrollButtonSize * 2)
	{
		int fpx = px - 320 + scrollButtonSize;
		int fpy = py - 120;
		float dist = sqrtf(fpx * fpx + fpy * fpy);
		if (dist < scrollButtonSize * 2)return true;
	}
	return false;
}


void BrowseLoadRecently(int slotid, int browsetype)
{
	if (recentlylist.size() < 1)return;
	if (recentlylist.size() != recentftypelist.size())return;
	std::vector<std::string> recentmenulist;
	recentmenulist.clear();
	recentmenulist.push_back("[Back]");
	for (int i = recentlylist.size() - 1; i >= 0; i--)
	{	
		const char* extchr = recentftypelist[i].c_str();
		if ((strcasecmp(extchr, ".ROM") == 0 && (browsetype & BROWSE_ROM)) || (strcasecmp(extchr,".MX1")==0 && (browsetype & BROWSE_ROM))
			|| (strcasecmp(extchr,".MX2")==0 && (browsetype & BROWSE_ROM)) || (strcasecmp(extchr, ".DSK") == 0 && (browsetype & BROWSE_DISK))
			|| (strcasecmp(extchr, ".CAS") == 0 && (browsetype & BROWSE_TAPE)))
		{
			recentmenulist.push_back(recentlylist[i]);
			//if(recentlylist[i]!=ROMStr[slotid] && recentlylist[i]!=DiskRawStr[slotid]
			//	&& recentlylist[i]!=CASStr)recentmenulist.push_back(recentlylist[i]);
		}
	}
	recentmenulist.push_back("[Clear Recently Used File List]");
	int menuItemCount = recentmenulist.size();
	int selectIndex = 0;
	int startid = 0;
	bool needRedraw = true;
	scrollTime = SDL_GetTicks();
	while (aptMainLoop())
	{
		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
		C2D_TargetClear(BottomRenderTartget, Color_Screen);
		C2D_SceneBegin(BottomRenderTartget);
		DrawTextTranslate("[Load Recently Used]", 80, 0, 1.0f, fontSize, fontSize, Color_White);
		int cnt0 = 0;
		for (int i = startid; i < startid + textColumn - 2; i++)
		{
			if (i >= menuItemCount)break;
			int idx = std::max(0, i - startid);
			std::string filename = recentmenulist[i];
			if (filename.length() < 1)continue;
			filename = &filename[filename.find_last_of("/") + 1];
			if (i == 0 || i == menuItemCount - 1)
			{
				if (i == selectIndex)
				{
					C2D_DrawRectSolid(textSize, textSize * (float)(idx + 1), 1.0f, 320 - textSize * 2, textSize, Color_White);
					DrawTextScroll(filename.c_str(), textStartPos, textSize * (float)(idx + 1), 1.0f, fontSize, fontSize, Color_Select);
				}
				else
				{
					DrawTextScroll(filename.c_str(), textStartPos, textSize * (float)(idx + 1), 1.0f, fontSize, fontSize, Color_White);
				}
			}
			else if (i == selectIndex)
			{
				C2D_DrawRectSolid(textSize, textSize * (float)(idx + 1), 1.0f, 320 - textSize * 2, textSize, Color_White);
				DrawTextScroll(filename.c_str(), textStartPos, textSize * (float)(idx + 1), 1.0f, fontSize, fontSize, Color_Select);
			}
			else
			{
				DrawText(filename.c_str(), textStartPos, textSize * (float)(idx + 1), 1.0f, fontSize, fontSize, Color_White);
			}
		}
		if (selectIndex == 0)DrawOKButton("Select");
		if (selectIndex > 0 && selectIndex < menuItemCount)DrawOKButton("Open");

		DrawCancelButton("Back");
		C2D_DrawRectSolid(0, 0, 1, textSize, 240 - textSize, Color_Screen);
		C2D_DrawRectSolid(320 - textSize, 0, 1, textSize, 240 - textSize, Color_Screen);
		if (menuItemCount > textColumn - 2)
		{
			int textNum = textColumn - 2;
			if (menuItemCount > textNum)
			{
				if (startid > 0)DrawScrollButtonL();
				if ((startid / textNum + 1) * textNum < menuItemCount) DrawScrollButtonR();
			}
		}
		needRedraw = false;
		C3D_FrameEnd(0);
		//}

		SDL_Delay(10);
		hidScanInput();
		u32 kDown = hidKeysDown();
		u32 kHeld = hidKeysHeld();
		bool IsAPress = false, IsLPress = false, IsRPress = false;
		if (kDown & KEY_B)
		{
			return;
		}
		else if (kDown & KEY_A)
		{
			IsAPress = true;
		}
		if (kHeld & KEY_UP)
		{
			GetMovedMenuIndex(selectIndex, startid, -1, NULL);
			scrollTime = SDL_GetTicks();
			SDL_Delay(TextDelay);
			needRedraw = true;
		}
		else if (kHeld & KEY_DOWN)
		{
			GetMovedMenuIndex(selectIndex, startid, 1, menuItemCount - 1);
			scrollTime = SDL_GetTicks();
			SDL_Delay(TextDelay);
			needRedraw = true;
		}
		else if (kHeld & KEY_LEFT)IsLPress = true;
		else if (kHeld & KEY_RIGHT)IsRPress = true;
		hidTouchRead(&tp);
		int px = tp.px;
		int py = tp.py;
		int oldpx = oldtp.px;
		int oldpy = oldtp.py;
		//if (px != oldpx || py != oldpy)
		if (px != 0 && py != 0)
		{
			if (py > textSize && py < 240 - textSize && px < 280)
			{
				selectIndex = py / textSize + startid - 1;
				needRedraw = true;
				SDL_Delay(TextDelay);
			}
			if (CheckScrollButtonL(px, py))IsLPress = true;
			if (CheckScrollButtonR(px, py))IsRPress = true;
		}
		if (kDown & KEY_TOUCH)
		{
			kDown &= ~KEY_TOUCH;
			if (CheckCancelButton(px, py))return;
			if (CheckOKButton(px, py))IsAPress = true;
		}
		oldtp.px = px;
		oldtp.py = py;
		if (IsAPress && selectIndex >= 0 && selectIndex < menuItemCount)
		{
			if (selectIndex == 0)return;
			else if(selectIndex==menuItemCount-1)
			{
				if (BrowseOK("Clear Recently Used List", "Are you sure?"))
				{
					recentlylist.clear();
					recentlylist.shrink_to_fit();
					recentftypelist.clear();
					recentftypelist.shrink_to_fit();
				}
				return;
			}
			size_t flen = strlen(recentmenulist[selectIndex].c_str());
			if (flen >= 4)
			{
				std::string filestr = recentmenulist[selectIndex];
				const char* extname = &recentmenulist[selectIndex].c_str()[flen - 4];
				const char* extsname = &recentmenulist[selectIndex].c_str()[flen - 3];
				std::string cfstring = recentmenulist[selectIndex];
				bool NotFound = false;
				zipfileStr = "";
				if (strcasecmp(extname, ".ZIP") == 0)
				{
					extname = BrowseZip(cfstring.c_str(), NULL);
#ifdef LOG_ERROR
					if (strlen(extname) < 4)
					{
						ErrorVec.push_back(cfstring.c_str());
						ErrorVec.push_back("BrowseZip Failed");
						NotFound = true;
					}
#else
					if (strlen(extname) < 4)NotFound = true;
#endif // LOG_ERROR
				}
				else if (strcasecmp(extsname, ".GZ") == 0)
				{
					std::string gzname = recentmenulist[selectIndex];
					gzname = &gzname[gzname.find_last_of("/") + 1];
					gzname.pop_back(); gzname.pop_back(); gzname.pop_back();
					extname = GetFileExtension(gzname);
				}
				if (!NotFound)
				{
					if (strcasecmp(extname, ".ROM") == 0)
					{
						if (ROMStr[slotid] != cfstring || zipfileStr.length() > 0)
						{
							if (LoadCart(cfstring.c_str(), slotid, MAP_GUESS))
							{
								ROMStr[slotid] = cfstring;
								AddRecentlyList(cfstring, ".ROM");
								ResetMSX(Mode, RAMPages, VRAMPages);
								//ResetSound();
							}
							else NotFound = true;
						}
					}
					else if (strcasecmp(extname, ".MX1") == 0)
					{
						if (ROMStr[slotid] != cfstring || zipfileStr.length() > 0)
						{
							if (LoadCart(cfstring.c_str(), slotid, MAP_GUESS))
							{
								ROMStr[slotid] = cfstring;
								AddRecentlyList(cfstring, ".MX1");
								ResetMSX(Mode, RAMPages, VRAMPages);
								//ResetSound();
							}
							else NotFound = true;
						}
					}
					else if (strcasecmp(extname, ".MX2") == 0)
					{
						if (ROMStr[slotid] != cfstring || zipfileStr.length() > 0)
						{
							if (LoadCart(cfstring.c_str(), slotid, MAP_GUESS))
							{
								ROMStr[slotid] = cfstring;
								AddRecentlyList(cfstring, ".MX2");
								ResetMSX(Mode, RAMPages, VRAMPages);
								//ResetSound();
							}
							else NotFound = true;
						}
					}
					else if (strcasecmp(extname, ".DSK") == 0)
					{
						AutoSaveDisk(slotid);
						std::string currstr = getZipSaveDiskPath(cfstring, extname);
						std::string savestr;
						savestr = cfstring;
						struct stat buf;
						if (stat(currstr.c_str(), &buf) == 0)cfstring = currstr;

						if (cfstring != DiskRawStr[slotid] || zipfileStr.length() > 0)
						{
							//if (ChangeDisk(slotid, cfstring.c_str()))
							if(ChangeDiskWithFormat(slotid,cfstring.c_str(),FMT_MSXDSK))
							{
								DiskStr[slotid] = currstr;
								DiskRawStr[slotid] = cfstring;
								//AddRecentlyList(cfstring, ".DSK");
								AddRecentlyList(savestr, ".DSK");
							}
#ifdef LOG_ERROR
					        else
							{
								NotFound = true;
								ErrorVec.push_back(cfstring.c_str());
								ErrorVec.push_back("Change Disk Failed");
							}
#else
							else NotFound = true;
#endif // LOG_ERROR
						}
					}
					else if (strcasecmp(extname, ".CAS") == 0)
					{
						if (CASStr != cfstring || zipfileStr.length() > 0)
						{
#ifdef TURBO_R
							if (MODEL(MSX_MSXTR))ShowMessage3DS("MSXTurboR doesn't support cassette tapes.", "Chnage MSX Hardware Model and try again.");
#endif // TURBO_R
							if (ChangeTape(cfstring.c_str()))
							{
								CASStr = cfstring;
								RewindTape();
								AddRecentlyList(cfstring, ".CAS");
							}
							else NotFound = true;
						}
					}
				}
				if (NotFound)
				{
					if (BrowseOK("File no found.", "Do you want to delete this from the list?") == true)
					{
						RemoveRecentlyList(cfstring);
						latestPath = "";
					}
				}
				return;
			}
		}
		if (IsLPress)
		{
			GetMovedMenuIndex(selectIndex, startid, -(textColumn - 2), NULL);
			scrollTime = SDL_GetTicks();
			SDL_Delay(TextDelay);
			needRedraw = true;
		}
		if (IsRPress)
		{
			GetMovedMenuIndex(selectIndex, startid, (textColumn - 2), menuItemCount - 1);
			scrollTime = SDL_GetTicks();
			SDL_Delay(TextDelay);
			needRedraw = true;
		}
		BrowseHomeButton();
		if (ExitNow == 1)return;
	}
}


std::string BrowseFrequentlyUsedFolder(int slotid, int browsetype)
{
	if (recentlylist.size() < 1)return "";
	if (recentlylist.size() != recentftypelist.size())return "";
	std::unordered_set<std::string> recentFolderSet;
	recentFolderSet.clear();
	for (int i = 0; i <recentlylist.size(); i++)
	{
		std::string str = recentlylist[i];
		str.erase(str.begin() + (str.find_last_of('/')), str.end());
		recentFolderSet.emplace(str);
	}
	std::vector<std::string> recentmenulist;
	recentmenulist.clear();
	recentmenulist.push_back("[Back]");
	for(std::unordered_set<std::string>::iterator itr =recentFolderSet.begin();itr!=recentFolderSet.end();itr++)
	{
		recentmenulist.push_back(*itr);
	}
	int menuItemCount = recentmenulist.size();
	int selectIndex = 0;
	int startid = 0;
	bool needRedraw = true;
	scrollTime = SDL_GetTicks();
	while (aptMainLoop())
	{
		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
		C2D_TargetClear(BottomRenderTartget, Color_Screen);
		C2D_SceneBegin(BottomRenderTartget);
		DrawTextTranslate("[Frequently Used Folder]", 80, 0, 1.0f, fontSize, fontSize, Color_White);
		int cnt0 = 0;
		for (int i = startid; i < startid + textColumn - 2; i++)
		{
			if (i >= menuItemCount)break;
			int idx = std::max(0, i - startid);
			std::string foldername = recentmenulist[i];
			if (foldername.length() < 1)continue;
			//filename = &filename[filename.find_last_of("/") + 1];
			if (i == 0 || i == menuItemCount - 1)
			{
				if (i == selectIndex)
				{
					C2D_DrawRectSolid(textSize, textSize * (float)(idx + 1), 1.0f, 320 - textSize * 2, textSize, Color_White);
					DrawTextScroll(foldername.c_str(), textStartPos, textSize * (float)(idx + 1), 1.0f, fontSize, fontSize, Color_Select);
				}
				else
				{
					DrawTextScroll(foldername.c_str(), textStartPos, textSize * (float)(idx + 1), 1.0f, fontSize, fontSize, Color_White);
				}
			}
			else if (i == selectIndex)
			{
				C2D_DrawRectSolid(textSize, textSize * (float)(idx + 1), 1.0f, 320 - textSize * 2, textSize, Color_White);
				DrawTextScroll(foldername.c_str(), textStartPos, textSize * (float)(idx + 1), 1.0f, fontSize, fontSize, Color_Select);
			}
			else
			{
				DrawText(foldername.c_str(), textStartPos, textSize * (float)(idx + 1), 1.0f, fontSize, fontSize, Color_White);
			}
		}
		if (selectIndex == 0)DrawOKButton("Select");
		if (selectIndex > 0 && selectIndex < menuItemCount)DrawOKButton("Open");

		DrawCancelButton("Back");
		C2D_DrawRectSolid(0, 0, 1, textSize, 240 - textSize, Color_Screen);
		C2D_DrawRectSolid(320 - textSize, 0, 1, textSize, 240 - textSize, Color_Screen);
		if (menuItemCount > textColumn - 2)
		{
			int textNum = textColumn - 2;
			if (menuItemCount > textNum)
			{
				if (startid > 0)DrawScrollButtonL();
				if ((startid / textNum + 1) * textNum < menuItemCount) DrawScrollButtonR();
			}
		}
		needRedraw = false;
		C3D_FrameEnd(0);
		//}

		SDL_Delay(10);
		hidScanInput();
		u32 kDown = hidKeysDown();
		u32 kHeld = hidKeysHeld();
		bool IsAPress = false, IsLPress = false, IsRPress = false;
		if (kDown & KEY_B)
		{
			return "";
		}
		else if (kDown & KEY_A)
		{
			IsAPress = true;
		}
		if (kHeld & KEY_UP)
		{
			GetMovedMenuIndex(selectIndex, startid, -1, NULL);
			scrollTime = SDL_GetTicks();
			SDL_Delay(TextDelay);
			needRedraw = true;
		}
		else if (kHeld & KEY_DOWN)
		{
			GetMovedMenuIndex(selectIndex, startid, 1, menuItemCount - 1);
			scrollTime = SDL_GetTicks();
			SDL_Delay(TextDelay);
			needRedraw = true;
		}
		else if (kHeld & KEY_LEFT)IsLPress = true;
		else if (kHeld & KEY_RIGHT)IsRPress = true;
		hidTouchRead(&tp);
		int px = tp.px;
		int py = tp.py;
		int oldpx = oldtp.px;
		int oldpy = oldtp.py;
		//if (px != oldpx || py != oldpy)
		if (px != 0 && py != 0)
		{
			if (py > textSize && py < 240 - textSize && px < 280)
			{
				selectIndex = py / textSize + startid - 1;
				needRedraw = true;
				SDL_Delay(TextDelay);
			}
			if (CheckScrollButtonL(px, py))IsLPress = true;
			if (CheckScrollButtonR(px, py))IsRPress = true;
		}
		if (kDown & KEY_TOUCH)
		{
			kDown &= ~KEY_TOUCH;
			if (CheckCancelButton(px, py))return "";
			if (CheckOKButton(px, py))IsAPress = true;
		}
		oldtp.px = px;
		oldtp.py = py;
		if (IsAPress && selectIndex >= 0 && selectIndex < menuItemCount)
		{
			if (selectIndex == 0)return "";
			return recentmenulist[selectIndex];
		}
		if (IsLPress)
		{
			GetMovedMenuIndex(selectIndex, startid, -(textColumn - 2), NULL);
			scrollTime = SDL_GetTicks();
			SDL_Delay(TextDelay);
			needRedraw = true;
		}
		if (IsRPress)
		{
			GetMovedMenuIndex(selectIndex, startid, (textColumn - 2), menuItemCount - 1);
			scrollTime = SDL_GetTicks();
			SDL_Delay(TextDelay);
			needRedraw = true;
		}
		BrowseHomeButton();
		if (ExitNow == 1)return "";
	}
}


std::string getZipSaveDiskPath(std::string pathstr, const char* extname)
{
	std::string filestr = std::string(pathstr);
	std::string currstr = std::string(pathstr);
	if (currstr.find("/FMSX3DS/SAVEDISK/") == std::string::npos)
	{
		filestr.erase(filestr.begin(), filestr.begin() + ((int)filestr.find_last_of("/") + 1));
		if (zipfileStr.length() >= 4)
		{
			filestr.erase(filestr.find_last_of("."));
			filestr += "/";
		}
		currstr = "/FMSX3DS/SAVEDISK/" + filestr;
	}
	if (zipfileStr.length() >= 4)
	{
		currstr = currstr + zipfileStr;
	}
	else currstr = currstr.erase(currstr.find_last_of(".")) + extname;

	return currstr;
}


void AutoSaveDisk(int slotid)
{
	if (!DiskWrited[slotid])return;
	if (DiskStr[slotid].size() < 3)return;
	std::string tempstr = std::string(DiskStr[slotid]);
	tempstr.erase(tempstr.begin(), tempstr.begin()+((int)tempstr.find("/SAVEDISK/")+10));
	if (tempstr.find("/") != std::string::npos)
	{
		tempstr = std::string(DiskStr[slotid]);
		tempstr.erase(tempstr.find_last_of("/"));
		mkdir(tempstr.c_str(), 0777);
	}

	if(SaveFDI(&FDD[slotid], DiskStr[slotid].c_str(), FMT_MSXDSK))DiskWrited[slotid] = 0;
}


void DrawMessage(const char* message, const char* message2, int x, int y,int waittime,bool isStop)
{
	C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
	C2D_TargetClear(BottomRenderTartget, Color_Screen);
	C2D_SceneBegin(BottomRenderTartget);
	DrawTextTranslate(message, x, y, 1.0f, fontSize, fontSize, Color_White);
	if (message2)DrawTextTranslate(message2, x, y + textSize * 2, 1.0f, fontSize, fontSize, Color_White);
	if (isStop)
	{
		DrawTextTranslate("Press any key to continue.", x, y + textSize * (message2 ? 4 : 2), 1.0f, fontSize, fontSize, Color_White);
		C3D_FrameEnd(0);
		SDL_Delay(waittime);
		while (aptMainLoop())
		{
			hidScanInput();
			u32 kDown = hidKeysDown();
			if (kDown)break;
		}
	}
	else
	{
		C3D_FrameEnd(0);
		SDL_Delay(waittime);
	}
}


bool BrowseOK(char* msgtxt, char* msgtext2)
{
	bool needRedraw = true;
	while (aptMainLoop())
	{
		if (needRedraw == true)
		{
			C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
			C2D_TargetClear(BottomRenderTartget, Color_Screen);
			C2D_SceneBegin(BottomRenderTartget);
			DrawTextTranslate(msgtxt, textSize, 0, 1, fontSize, fontSize, Color_White);
			if (msgtext2)DrawTextTranslate(msgtext2, textSize, textSize * 2, 1, fontSize, fontSize, Color_White);
			DrawOKButton("OK(A)");
			DrawCancelButton("Cancel(B)");
			needRedraw = false;
			C3D_FrameEnd(0);
		}
		SDL_Delay(10);
		hidScanInput();
		u32 kDown = hidKeysDown();
		u32 kHeld = hidKeysHeld();
		if (kDown & KEY_A)
		{
			SDL_Delay(100);
			return true;
		}
		if (kDown & KEY_B)
		{
			SDL_Delay(100);
			return false;
		}
		hidTouchRead(&tp);
		int px = tp.px;
		int py = tp.py;
		int oldpx = oldtp.px;
		int oldpy = oldtp.py;
		if (kDown & KEY_TOUCH)
		{
			kDown &= ~KEY_TOUCH;
			if (CheckCancelButton(px, py))return false;
			if (CheckOKButton(px, py))return true;
		}
		oldtp.px = px;
		oldtp.py = py;
	}
	return false;
}


//int BrowseInt(char* msgtxt, std::vector<char*> CharVec, int idx, int defid)
int BrowseInt(char* msgtxt, std::vector<char*> CharVec, int idx, int defid, bool cancelZero)
{
	bool needRedraw = true;
	//int selectIndex = idx - 2;
	int selectIndex = idx;
	//int startid = selectIndex;
	int startid = std::max(0, selectIndex-(240/textSize-2)/2);
	GetMovedMenuIndex(selectIndex, startid, 0, CharVec.size() - 1);
	while (aptMainLoop())
	{
		if (needRedraw == true)
		{
			C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
			C2D_TargetClear(BottomRenderTartget, Color_Screen);
			C2D_SceneBegin(BottomRenderTartget);
			DrawTextTranslate(msgtxt, textSize, 0, 1.0f, fontSize, fontSize, Color_White);
			for (int i = startid; i < startid + (240 / textSize) - 2; i++)
			{
				if (i >= CharVec.size())break;
				int idx0 = std::max(0, i - startid) + 1;
				if (i == selectIndex)
				{
					C2D_DrawRectSolid(textSize, textSize * (float)idx0, 1.0f, 320 - textSize * 2, textSize, Color_White);
					DrawTextTranslate(CharVec[i], textStartPos, textSize * (float)idx0, 1.0f, fontSize, fontSize, Color_Select);
				}
				else
				{
					DrawTextTranslate(CharVec[i], textStartPos, textSize * (float)idx0, 1.0f, fontSize, fontSize, Color_White);
				}
			}
			if (selectIndex >= 0 && selectIndex < CharVec.size())
			{
				DrawOKButton("Select");
			}
			DrawCancelButton("Back");
			int textNum = 240 / textSize - 2;
			if (CharVec.size() > textNum)
			{
				if (startid > 0)DrawScrollButtonL();
				if ((startid / textNum + 1) * textNum < CharVec.size()) DrawScrollButtonR();
			}
			needRedraw = false;
			C3D_FrameEnd(0);
		}
		bool IsAPress = false, IsLPress = false, IsRPress = false;
		SDL_Delay(10);
		hidScanInput();
		u32 kDown = hidKeysDown();
		u32 kHeld = hidKeysHeld();
		if (kDown & KEY_A)
		{
			IsAPress = true;
		}
		else if (kDown & KEY_B)
		{
			return cancelZero ? 0 : idx;
			//return idx;
		}
		else if (kHeld & KEY_UP)
		{
			GetMovedMenuIndex(selectIndex, startid, -1, NULL);
			SDL_Delay(TextDelay);
			needRedraw = true;
		}
		else if (kHeld & KEY_DOWN)
		{
			GetMovedMenuIndex(selectIndex, startid, 1, CharVec.size() - 1);
			SDL_Delay(TextDelay);
			needRedraw = true;
		}
		else if (kHeld & KEY_LEFT)IsLPress = true;
		else if (kHeld & KEY_RIGHT)IsRPress = true;
		hidTouchRead(&tp);
		int px = tp.px;
		int py = tp.py;
		int oldpx = oldtp.px;
		int oldpy = oldtp.py;
		if (px != 0 && py != 0)
		{
			if (py > textSize && py < 240 - textSize && px >textStartPos && px < textEndPos)
			{
				selectIndex = py / textSize + startid - 1;
				SDL_Delay(TextDelay);
				needRedraw = true;
			}
			else if (CheckScrollButtonL(px, py))IsLPress = true;
			else if (CheckScrollButtonR(px, py))IsRPress = true;
		}
		if (kDown & KEY_TOUCH)
		{
			kDown &= ~KEY_TOUCH;
			//if (CheckCancelButton(px, py))return idx;
			if (CheckCancelButton(px, py))return cancelZero ? 0 : idx;
			if (CheckOKButton(px, py))IsAPress = true;
		}
		oldtp.px = px;
		oldtp.py = py;
		if (IsAPress && selectIndex >= 0 && selectIndex < CharVec.size())
		{
			if (CharVec[selectIndex] == "[Back]")
			{
				//return idx;
				return cancelZero ? 0 : idx;
			}
			if (CharVec[selectIndex] == "[Default Value]")
			{
				return defid;
			}
			return selectIndex;
		}
		if (IsLPress)
		{
			GetMovedMenuIndex(selectIndex, startid, -(textColumn - 2), NULL);
			SDL_Delay(TextDelay);
			needRedraw = true;
		}
		if (IsRPress)
		{
			GetMovedMenuIndex(selectIndex, startid, (textColumn - 2), CharVec.size() - 1);
			SDL_Delay(TextDelay);
			needRedraw = true;
		}
		BrowseHomeButton();
		if (ExitNow == 1)return selectIndex;
	}
	return selectIndex;
}


void BrowseOptions()
{
	int menuItemCount = optionItem.size();
	int selectIndex = 0;
	int startid = 0;
	bool needRedraw = true;
	while (aptMainLoop())
	{
		if (needRedraw == true)
		{
			C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
			C2D_TargetClear(BottomRenderTartget, Color_Screen);
			C2D_SceneBegin(BottomRenderTartget);
			DrawTextTranslate("[Option]", 120, 0, 1.0f, fontSize, fontSize, Color_White);
			for (int i = startid; i < startid + textColumn - 2; i++)
			{
				if (i >= menuItemCount)break;
				int idx = std::max(0, i - startid);
				int i0 = optionItem[i].currentIdx;
				char* ic0 = optionItem[i].menuChar[i0];
				if(i==selectIndex)
				{
					C2D_DrawRectSolid(textSize, textSize * (float)(idx + 1), 1.0f, 320 - textSize * 2, textSize, Color_White);
					DrawTextTranslate(optionItem[i].name.c_str(), textStartPos, textSize * (float)(idx + 1), 1.0f, fontSize, fontSize, Color_Select);
					DrawTextTranslate(ic0, 200, textSize * (float)(idx + 1), 1.0f, fontSize, fontSize, Color_Select);
				}
				else
				{
					DrawTextTranslate(optionItem[i].name.c_str(), textStartPos, textSize * (float)(idx + 1), 1.0f, fontSize, fontSize, Color_White);
					DrawTextTranslate(ic0, 200, textSize * (float)(idx + 1), 1.0f, fontSize, fontSize, Color_White);
				}
			}
			if (selectIndex >= 0 && selectIndex < menuItemCount)
			{
				std::string tempstr = "";
				std::string smenustr = optionItem[selectIndex].name;
				if (smenustr == "[Back]" || smenustr == "[Reset Default]")
				{
					tempstr = "Select";
				}
				else
				{
					tempstr = "Config";
				}
				DrawOKButton(tempstr.c_str());
			}
			DrawCancelButton("Back");
			int textNum = 240 / textSize - 2;
			if (menuItemCount > textNum)
			{
				if (startid > 0)DrawScrollButtonL();
				if ((startid / textNum + 1) * textNum < menuItemCount) DrawScrollButtonR();
			}
			needRedraw = false;
			C3D_FrameEnd(0);
		}
		SDL_Delay(10);
		bool IsAPress = false, IsLPress = false, IsRPress = false;
		hidScanInput();
		u32 kDown = hidKeysDown();
		u32 kHeld = hidKeysHeld();
		if (kDown & KEY_B)
		{
			return;
		}
		if (kDown & KEY_A)IsAPress = true;
		else if (kDown & KEY_START)
		{
			optionItem[selectIndex].currentIdx = IsNew3DS ? optionItem[selectIndex].defaultIdx : optionItem[selectIndex].old3DSIdx;
			needRedraw = true;
			IsAPress = true;
			SDL_Delay(100);
		}
		if (kHeld & KEY_UP)
		{
			GetMovedMenuIndex(selectIndex, startid, -1, NULL);
			SDL_Delay(TextDelay);
			needRedraw = true;
		}
		else if (kHeld & KEY_DOWN)
		{
			GetMovedMenuIndex(selectIndex, startid, 1, menuItemCount - 1);
			SDL_Delay(TextDelay);
			needRedraw = true;
		}
		else if (kHeld & KEY_LEFT)IsLPress = true;
		else if (kHeld & KEY_RIGHT)IsRPress = true;
		hidTouchRead(&tp);
		int px = tp.px;
		int py = tp.py;
		int oldpx = oldtp.px;
		int oldpy = oldtp.py;
		if (px != 0 && py != 0)
		{
			if (py > textSize && py < 240 - textSize && px >textStartPos && px < textEndPos)
			{
				selectIndex = py / textSize + startid - 1;
				SDL_Delay(TextDelay);
				needRedraw = true;
			}
			else if (CheckScrollButtonL(px, py))IsLPress = true;
			else if (CheckScrollButtonR(px, py))IsRPress = true;
		}
		if (kDown & KEY_TOUCH)
		{
			kDown &= ~KEY_TOUCH;
			if (CheckCancelButton(px, py))return;
			if (CheckOKButton(px, py))IsAPress = true;
		}
		oldtp.px = px;
		oldtp.py = py;
		if (IsAPress && selectIndex >= 0 && selectIndex < menuItemCount)
		{
			IsAPress = false;
			if (optionItem[selectIndex].name == "[Reset Default]")
			{
				if (BrowseOK("Dou you want to Reset?",NULL) == true)
				{
					for (int i = 0; i < optionItem.size(); i++)
					{
						optionItem[i].currentIdx = IsNew3DS ? optionItem[i].defaultIdx : optionItem[i].old3DSIdx;
					}
					return;
				}
				needRedraw = true;
			}
			else if (optionItem[selectIndex].name == "[Back]")
			{
				return;
			}
			else if (selectIndex >= 0 && selectIndex < menuItemCount)
			{
				if (strcmp(optionItem[selectIndex].menuChar[0], "") == 0)continue;
				std::vector<char*> charvec;
				charvec.clear();
				charvec.push_back("[Back]");
				for (int i = 0; i < optionItem[selectIndex].menuChar.size(); i++)
				{
					charvec.push_back(optionItem[selectIndex].menuChar[i]);
				}
				charvec.push_back("[Default Value]");
				optionItem[selectIndex].currentIdx = BrowseInt((char*)optionItem[selectIndex].name.c_str(), charvec,
					optionItem[selectIndex].currentIdx + 1, (IsNew3DS ? optionItem[selectIndex].defaultIdx : optionItem[selectIndex].old3DSIdx) + 1, false) - 1;
				needRedraw = true;
			}
		}
		if (IsLPress)
		{
			GetMovedMenuIndex(selectIndex, startid, -(textColumn - 2), NULL);
			SDL_Delay(TextDelay);
			needRedraw = true;
		}
		if (IsRPress)
		{
			GetMovedMenuIndex(selectIndex, startid, (textColumn - 2), menuItemCount - 1);
			SDL_Delay(TextDelay);
			needRedraw = true;
		}
		BrowseHomeButton();
		if (ExitNow == 1)return;
	}
}


void BrowseKeyconfig()
{
	int menuItemCount = keyconfItem.size();
	int startid = 0;
	int selectIndex = 0;
	bool needRedraw = true;
	while (aptMainLoop())
	{
		if (needRedraw == true)
		{
			C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
			C2D_TargetClear(BottomRenderTartget, Color_Screen);
			C2D_SceneBegin(BottomRenderTartget);
			DrawTextTranslate("[Key Config]", 120, 0, 1.0f, fontSize, fontSize, Color_White);
			int cnt0 = 0;
			for (int i = startid; i < startid + (240 / textSize) - 2; i++)
			{
				if (i >= menuItemCount)break;
				int idx = std::max(0, i - startid);
				int i0 = keyMenuMap[keyconfItem[i].currentIdx];
				if (i == selectIndex)
				{
					C2D_DrawRectSolid(textSize, textSize * (float)(idx + 1), 1.0f, 320 - textSize * 2, textSize, Color_White);
					DrawTextTranslate(keyconfItem[i].name.c_str(), textStartPos, textSize * (float)(idx + 1), 1.0f, fontSize, fontSize, Color_Select);
					if (keyValueItem[i0].name != "[Back]" && i0 > 0)
						DrawTextTranslate(keyValueItem[i0].name.c_str(), 200, textSize * (float)(idx + 1), 1.0f, fontSize, fontSize, Color_Select);
				}
				else
				{
					DrawTextTranslate(keyconfItem[i].name.c_str(), textStartPos, textSize * (float)(idx + 1), 1.0f, fontSize, fontSize, Color_White);
					if (keyValueItem[i0].name != "[Back]" && i0 > 0)
						DrawTextTranslate(keyValueItem[i0].name.c_str(), 200, textSize * (float)(idx + 1), 1.0f, fontSize, fontSize, Color_White);
				}
			}
			if (selectIndex >= 0 && selectIndex < menuItemCount)
			{
				DrawOKButton("Select");
			}
			DrawCancelButton("Back");
			int textNum = 240 / textSize - 2;
			if (menuItemCount > textNum)
			{
				if (startid > 0)DrawScrollButtonL();
				if ((startid / textNum + 1) * textNum < menuItemCount) DrawScrollButtonR();
			}
			needRedraw = false;
			C3D_FrameEnd(0);
		}
		bool IsAPress = false, IsLPress = false, IsRPress = false;
		SDL_Delay(10);
		hidScanInput();
		u32 kDown = hidKeysDown();
		u32 kHeld = hidKeysHeld();
		if (kDown & KEY_B)return;
		if (kDown & KEY_A)IsAPress = true;
		else if (kDown & KEY_START)
		{
			keyconfItem[selectIndex].currentIdx = keyconfItem[selectIndex].defaultIdx;
			SDL_Delay(100);
			needRedraw = true;
		}
		if (kHeld & KEY_UP)
		{
			GetMovedMenuIndex(selectIndex, startid, -1, NULL);
			SDL_Delay(TextDelay);
			needRedraw = true;
		}
		else if (kHeld & KEY_DOWN)
		{
			GetMovedMenuIndex(selectIndex, startid, 1, menuItemCount - 1);
			SDL_Delay(TextDelay);
			needRedraw = true;
		}
		else if (kHeld & KEY_LEFT)IsLPress = true;
		else if (kHeld & KEY_RIGHT)IsRPress = true;

		hidTouchRead(&tp);
		int px = tp.px;
		int py = tp.py;
		int oldpx = oldtp.px;
		int oldpy = oldtp.py;
		if (px != 0 && py != 0)
		{
			if (py > textSize && py < 240 - textSize && px < 280)
			{
				selectIndex = py / textSize + startid - 1;
				SDL_Delay(TextDelay);
				needRedraw = true;
			}
			if (CheckScrollButtonL(px, py))IsLPress = true;
			if (CheckScrollButtonR(px, py))IsRPress = true;
		}
		if (kDown & KEY_TOUCH)
		{
			kDown &= ~KEY_TOUCH;
			if (CheckCancelButton(px, py))return;
			if (CheckOKButton(px, py))IsAPress = true;
		}
		oldtp.px = px;
		oldtp.py = py;

		if (IsAPress && selectIndex >= 0 && selectIndex < menuItemCount)
		{
			IsAPress = false;
			if (keyconfItem[selectIndex].name == "[Back]")
			{
				return;
			}
			else if (keyconfItem[selectIndex].name == "[Rapidfire Setting]")
			{
				BrowseRapidFire();
				needRedraw = true;
				continue;
				//return;
			}
			else if (keyconfItem[selectIndex].name == "[Reset Default]")
			{
				if (BrowseOK("Dou you want to Reset?",NULL) == true)
				{
					for (int i = 0; i < keyconfItem.size(); i++)
					{
						keyconfItem[i].currentIdx = keyconfItem[i].defaultIdx;
					}
					for (int i = 0; i < sizeof(rapidInterVals); i++)rapidInterVals[i] = 0;
					needRedraw = true;
				}
				selectIndex = 0;
			}
			else
			{
				keyconfItem[selectIndex].currentIdx = keyValueItem[BrowseInt((char*)keyconfItem[selectIndex].name.c_str(), KeyNameVec
					, keyMenuMap[keyconfItem[selectIndex].currentIdx], keyMenuMap[keyconfItem[selectIndex].defaultIdx], false)].keyidx;
				if (keyconfItem[selectIndex].currentIdx < 0)keyconfItem[selectIndex].currentIdx = keyconfItem[selectIndex].defaultIdx;
				needRedraw = true;
			}
		}
		if (IsLPress)
		{
			GetMovedMenuIndex(selectIndex, startid, -(textColumn - 2), NULL);
			SDL_Delay(TextDelay);
			needRedraw = true;
		}
		if (IsRPress)
		{
			GetMovedMenuIndex(selectIndex, startid, (textColumn - 2), menuItemCount - 1);
			SDL_Delay(TextDelay);
			needRedraw = true;
		}
		BrowseHomeButton();
		if (ExitNow == 1)return;
	}
}


void BrowseRapidFire()
{
	bool needRedraw = true;
	int menuVecCount = RapidMenuVec.size();
	int selectIndex = 0;
	int startid = 0;
	bool IsAPress = false;
	while (aptMainLoop())
	{
		if (needRedraw == true)
		{
			C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
			C2D_TargetClear(BottomRenderTartget, Color_Screen);
			C2D_SceneBegin(BottomRenderTartget);
			DrawTextTranslate("[Rapidfire Setting]", 120, 0, 1.0f, fontSize, fontSize, Color_White);
			for (int i = startid; i < startid + (240 / textSize) - 2; i++)
			{
				if (i >= menuVecCount)break;
				int idx = std::max(0, i - startid);
				if (i == selectIndex)
				{
					C2D_DrawRectSolid(textSize, textSize * (float)(idx + 1), 1.0f, 320 - textSize * 2, textSize, Color_White);
					DrawTextTranslate(RapidMenuVec[i].c_str(), textStartPos, textSize * (float)(idx + 1), 1.0f, fontSize, fontSize, Color_Select);
					if (RapidMenuVec[i] != "[Back]" && RapidMenuVec[i] != "[Reset Default]")
					{
						int rapididx = rapidInterVals[i - 1];
						const char* rapidchar = OptionRapidFire[rapididx];
						DrawText(rapidchar, 200, textSize * (float)(idx + 1), 1.0f, fontSize, fontSize, Color_Select);
					}
				}
				else
				{
					DrawTextTranslate(RapidMenuVec[i].c_str(), textStartPos, textSize * (float)(idx + 1), 1.0f, fontSize, fontSize, Color_White);
					if (RapidMenuVec[i] != "[Back]" && RapidMenuVec[i] != "[Reset Default]")
					{
						int rapididx = rapidInterVals[i - 1];
						const char* rapidchar = OptionRapidFire[rapididx];
						DrawText(rapidchar, 200, textSize * (float)(idx + 1), 1.0f, fontSize, fontSize, Color_White);
					}
				}
			}

			if (selectIndex >= 0 && selectIndex <= menuVecCount)
			{
				DrawOKButton("Select");
			}
			DrawCancelButton("Back");
			int textNum = 240 / textSize - 2;
			if (menuVecCount > textNum)
			{
				if (startid > 0)DrawScrollButtonL();
				if ((startid / textNum + 1) * textNum < menuVecCount) DrawScrollButtonR();
			}
			needRedraw = false;
			C3D_FrameEnd(0);
		}
		SDL_Delay(10);

		bool IsLPress = false, IsRPress = false;
		hidScanInput();
		u32 kDown = hidKeysDown();
		u32 kHeld = hidKeysHeld();
		if (kHeld & KEY_UP)
		{
			GetMovedMenuIndex(selectIndex, startid, -1, NULL);
			SDL_Delay(TextDelay);
			needRedraw = true;
		}
		else if (kHeld & KEY_DOWN)
		{
			GetMovedMenuIndex(selectIndex, startid, 1, menuVecCount - 1);
			SDL_Delay(TextDelay);
			needRedraw = true;
		}
		else if (kHeld & KEY_LEFT)IsLPress = true;
		else if (kHeld & KEY_RIGHT)IsRPress = true;
		if (kDown & KEY_A)
		{
			IsAPress = true;
		}
		if (kDown & KEY_B) { return; }
		hidTouchRead(&tp);
		int px = tp.px;
		int py = tp.py;
		int oldpx = oldtp.px;
		int oldpy = oldtp.py;
		if (px != 0 && py != 0)
		{
			if (py > textSize && py < 240 - textSize && px>textStartPos && px < textEndPos)
			{
				selectIndex = (py / textSize) + startid - 1;
				SDL_Delay(TextDelay);
				needRedraw = true;
			}
			else if (CheckScrollButtonL(px, py))IsLPress = true;
			else if (CheckScrollButtonR(px, py))IsRPress = true;
		}
		if (kDown & KEY_TOUCH)
		{
			kDown &= ~KEY_TOUCH;
			if (CheckCancelButton(px, py))return;
			if (CheckOKButton(px, py))IsAPress = true;
		}
		oldtp.px = px;
		oldtp.py = py;

		if (IsAPress && selectIndex >= 0 && selectIndex < menuVecCount)
		{
			if (selectIndex == 0)
			{
				SDL_Delay(100);
				return;
			}
			std::string basicMode = "";
			std::string selectmenu = RapidMenuVec[selectIndex];
			if (selectmenu == "[Back]")
			{
				SDL_Delay(100);
				return;
			}
			if (selectmenu == "A Button")
			{
				rapidInterVals[0] = BrowseInt("A Button Rapid Interval", MenuAddBackDeafult(OptionRapidFire), rapidInterVals[0] + 1, 1, false) - 1;
			}
			else if (selectmenu == "B Button")
			{
				rapidInterVals[1] = BrowseInt("B Button Rapid Interval", MenuAddBackDeafult(OptionRapidFire), rapidInterVals[1] + 1, 1, false) - 1;
			}
			else if (selectmenu == "X Button")
			{
				rapidInterVals[2] = BrowseInt("X Button Rapid Interval", MenuAddBackDeafult(OptionRapidFire), rapidInterVals[2] + 1, 1, false) - 1;
			}
			else if (selectmenu == "Y Button")
			{
				rapidInterVals[3] = BrowseInt("Y Button Rapid Interval", MenuAddBackDeafult(OptionRapidFire), rapidInterVals[3] + 1, 1, false) - 1;
			}
			else if (selectmenu == "L Button")
			{
				rapidInterVals[4] = BrowseInt("L Button Rapid Interval", MenuAddBackDeafult(OptionRapidFire), rapidInterVals[4] + 1, 1, false) - 1;
			}
			else if (selectmenu == "R Button")
			{
				rapidInterVals[5] = BrowseInt("R Button Rapid Interval", MenuAddBackDeafult(OptionRapidFire), rapidInterVals[5] + 1, 1, false) - 1;
			}
			else if (selectmenu == "Start Button")
			{
				rapidInterVals[6] = BrowseInt("Start Button Rapid Interval", MenuAddBackDeafult(OptionRapidFire), rapidInterVals[6] + 1, 1, false) - 1;
			}
			else if (selectmenu == "Select Button")
			{
				rapidInterVals[7] = BrowseInt("Select Button Rapid Interval", MenuAddBackDeafult(OptionRapidFire), rapidInterVals[7] + 1, 1, false) - 1;
			}
			else if (selectmenu == "ZL Button")
			{
				rapidInterVals[8] = BrowseInt("ZL Button Rapid Interval", MenuAddBackDeafult(OptionRapidFire), rapidInterVals[8] + 1, 1, false) - 1;
			}
			else if (selectmenu == "ZR Button")
			{
				rapidInterVals[9] = BrowseInt("ZR Button Rapid Interval", MenuAddBackDeafult(OptionRapidFire), rapidInterVals[9] + 1, 1, false) - 1;
			}
			else if (selectmenu == "[Reset Default]")
			{
				if (BrowseOK("Dou you want to Reset?",NULL) == true)
				{
					for (int i = 0; i < sizeof(rapidInterVals); i++)rapidInterVals[i] = 0;
				}
				return;
			}
			return;
		}
		if (IsLPress)
		{
			GetMovedMenuIndex(selectIndex, startid, -(textColumn - 2), NULL);
			SDL_Delay(TextDelay);
			needRedraw = true;
		}
		if (IsRPress)
		{
			GetMovedMenuIndex(selectIndex, startid, (textColumn - 2), menuVecCount - 1);
			SDL_Delay(TextDelay);
			needRedraw = true;
		}
		BrowseHomeButton();
		if (ExitNow == 1)return;
	}
}


std::vector<char*> MenuAddBackDeafult(std::vector<char*> charvec)
{
	std::vector<char*> newvec;
	newvec.push_back("[Back]");
	for (int i = 0; i < charvec.size(); i++)
	{
		newvec.push_back(charvec[i]);
	}
	newvec.push_back("[Default Value]");
	return newvec;
}


//Browse SaveState, ScreenShot, Save Cassette Tape etc
std::string BrowseFile(const char* extchr, const char* folderchar, bool IsSave, const char* msgchr)
{
	std::string cfstring = latestPath.size() < 2 ? "/FMSX3DS/Default.ROM" : latestPath;
	std::vector<char*> FileNumVec;
	FileNumVec.clear();
	std::vector<std::string> pathStringVec;
	std::vector<bool> isOverrideVec;
	pathStringVec.clear();
	FileNumVec.push_back("[Back]");
	pathStringVec.push_back("");
	isOverrideVec.push_back(false);
	for (int i = 0; i < 10; i++)
	{
		std::string filestr = std::string(cfstring);
		std::string currstr = std::string(cfstring);
		filestr.erase(filestr.begin(), filestr.begin() + ((int)filestr.find_last_of("/") + 1));
		currstr = "/FMSX3DS" + (std::string)folderchar + filestr;
		currstr = currstr.erase(currstr.find_last_of(".")) + std::to_string(i) + extchr;
		struct stat buf;
		std::string statestr;
		if (stat(currstr.c_str(), &buf) < 0)
		{
			FileNumVec.push_back(FileCharVec[i]);
			if (!IsSave)pathStringVec.push_back("");
			else
			{
				isOverrideVec.push_back(false);
				pathStringVec.push_back(currstr);
			}
			continue;
		}
		isOverrideVec.push_back(true);

		u64 mtime = 0;
		archive_getmtime(currstr.c_str(), &mtime);
		time_t mtimet = (time_t)mtime;
		struct tm* timeStruct = gmtime(&mtimet);
		std::string tempstr = std::to_string(i) + "[Saved:" + std::to_string(timeStruct->tm_year + 1900) + "/" + std::to_string(timeStruct->tm_mon+1)
			+ "/" + std::to_string(timeStruct->tm_mday) + " " + std::to_string(timeStruct->tm_hour)
			+ ":" + std::to_string(timeStruct->tm_min) + "]";
		FileNumVec.push_back(StringToChar(tempstr));
		pathStringVec.push_back(currstr);
	}
	int idx = BrowseInt((char*)msgchr, FileNumVec, 0, 0, false);
	if (idx < pathStringVec.size())
	{
		if (IsSave && (idx< isOverrideVec.size()))
		{
			if (isOverrideVec[idx])
			{
				if (BrowseOK("Delete old files and override?", NULL))return pathStringVec[idx];
				else return "";
			}
		}
		return pathStringVec[idx];
	}
	return "";
}


void DoAutoSave()
{
	for (int i = 0; i < MAXDRIVES; i++)
	{
		if (DiskWrited[i] == 1)
		{
			if (DiskStr[i].size() > 2)SaveFDI(&FDD[i], DiskStr[i].c_str(), FMT_MSXDSK);
		}
	}
}


void GetMovedMenuIndex(int &selectidx, int &startidx, int val,int maxcount)
{
	selectidx += val;
	if (val >= 0)
	{
		if (selectidx > startidx + textColumn - 5)startidx += val;
		selectidx = selectidx > maxcount ? maxcount : selectidx;
		startidx = startidx > maxcount ? maxcount : startidx;
		//startidx = startidx > maxcount - textColumn ? maxcount - textColumn : startidx;
	}
	if (val <= 0)
	{
		if (selectidx < startidx + 2)startidx +=val;
		selectidx = selectidx < 0 ? 0 : selectidx;
		startidx = startidx < 0 ? 0 : startidx;
	}
}


std::string GetUniqueName(std::string pathname, std::string extname)
{
	struct stat buf;
	if (stat((pathname + extname).c_str(), &buf) < 0)
	{
		return(pathname + extname);
	}
	for (int i = 0; i < 4096; i++)
	{
		std::ostringstream oss;
		oss << i;
		if (stat((pathname + oss.str() + extname).c_str(), &buf) < 0)
		{
			return (pathname + oss.str() + extname).c_str();
		}
	}
	return "";
}


std::string Get3DSPath(const char* path)
{
	struct stat buf;
	std::string pathstr = (std::string)path;
	std::string fMSX3DSpath = (std::string)"/FMSX3DS/" + pathstr;
	if (stat(fMSX3DSpath.c_str(), &buf) == 0)return fMSX3DSpath.c_str();
	std::string romfspath = (std::string)"romfs:/FMSX3DS/" + pathstr;
	if (stat(romfspath.c_str(), &buf) == 0)return romfspath.c_str();
	std::string retroPath0 = (std::string)"/retroarch/cores/system/" + pathstr;
	if (stat(retroPath0.c_str(), &buf) == 0)return retroPath0.c_str();
	std::string retroPath1 = (std::string)"/retroarch/cores/system/Machines/Shared Roms/" + pathstr;
	if (stat(retroPath1.c_str(), &buf) == 0)return retroPath1.c_str();
	std::string MSXDSPath = (std::string)"/msxDS/" + pathstr;
	if (stat(MSXDSPath.c_str(), &buf) == 0)return MSXDSPath.c_str();
	std::string RootPath = (std::string)"/" + pathstr;
	if (stat(RootPath.c_str(), &buf) == 0)return RootPath.c_str();
	return "";
}


/* Safely load BIOS.*/
/* Use main-ROM and sub-ROM in same folder as mush as possible.*/
/* No need for use it for many case? */
//std::string GetSystemPath(const char* path)
//{
//	struct stat buf;
//	std::string pathstr = (std::string)path;
//	bool IsMainROM = (pathstr == "MSX2.ROM" || pathstr == "MSX2P.ROM") ? true : false;
//	if (SystemLoadString.size()>1)
//	{
//		std::string sysPath = GetValidSysPath(SystemLoadString, pathstr, IsMainROM);
//		if (sysPath.size() > 1)return sysPath;
//	}
//
//	std::string fMSX3DSpath = GetValidSysPath("/FMSX3DS/", pathstr, IsMainROM);
//	if (fMSX3DSpath.size() > 1)return fMSX3DSpath.c_str();
//	std::string romfspath = GetValidSysPath("romfs:/FMSX3DS/", pathstr, IsMainROM);
//	if (romfspath.size() > 1)return romfspath.c_str();
//	std::string retroPath0 = GetValidSysPath("/retroarch/cores/system/", pathstr, IsMainROM);
//	if (retroPath0.size() > 1)return retroPath0.c_str();
//	std::string retroPath1 = GetValidSysPath("/retroarch/cores/system/Machines/Shared Roms/", pathstr, IsMainROM);
//	if (retroPath1.size() > 1)return retroPath1;
//	std::string MSXDSPath = GetValidSysPath("/msxDS/", pathstr, IsMainROM);
//	if (MSXDSPath.size() > 1)return MSXDSPath;
//	std::string RootPath = GetValidSysPath("/", pathstr, IsMainROM);
//	if (RootPath.size() > 1)return RootPath;
//	return "";
//}


//std::string GetValidSysPath(std::string syspath, std::string pathstr, bool IsMainROM)
//{
//	struct stat buf;
//	std::string fMSX3DSpath = syspath + pathstr;
//	if (stat(fMSX3DSpath.c_str(), &buf) == 0)
//	{
//		if (IsMainROM)
//		{
//			std::string fMSX3DSSubPath = syspath;
//			if (pathstr == "MSX2.ROM")fMSX3DSSubPath += "MSX2EXT.ROM";
//			else if (pathstr == "MSX2P.ROM")fMSX3DSSubPath += "MSX2PEXT.ROM";
//			if (stat(fMSX3DSSubPath.c_str(), &buf) == 0)
//			{
//				SystemLoadString = syspath;
//				return fMSX3DSpath.c_str();
//			}
//			else return "";
//		}
//		return fMSX3DSpath.c_str();
//	}
//	return "";
//}


//void OpenDirectory(std::string dirstr, int browsetype)
//{
//	const char* path = dirstr.c_str();
//	Handle dirHandle;
//	static FS_DirectoryEntry entry;
//	memset(&entry, 0, sizeof(FS_DirectoryEntry));
//
//	u16* utf16 = (u16*)calloc(strlen(path) + 1, sizeof(u16));
//	ssize_t utf16Len = utf8_to_utf16(utf16, (const uint8_t*)path, strlen(path));
//	FS_Path* dirPath = (FS_Path*)calloc(1, sizeof(FS_Path));
//	dirPath->type = PATH_UTF16;
//	dirPath->size = (utf16Len + 1) * sizeof(u16);
//	dirPath->data = utf16;
//
//	FS_Archive sdmcArchive;
//	FSUSER_OpenArchive(&sdmcArchive, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""));
//	FSUSER_OpenDirectory(&dirHandle, sdmcArchive, *dirPath);
//	u32 entriesRead;
//	static char* name;
//	dirstring.clear();
//	dirstring.shrink_to_fit();
//	ftypevec.clear();
//	ftypevec.shrink_to_fit();
//	ftypestring.clear();
//	ftypestring.shrink_to_fit();
//	menucnt = 0;
//	bool isrroot = false;
//	if (dirstr == "/")isrroot = true;
//	std::vector<std::string> foldervec;
//	std::vector<std::string> filevec;
//	foldervec.clear();
//	filevec.clear();
//	for (;;)
//	{
//		entriesRead = 0;
//		FSDIR_Read(dirHandle, &entriesRead, 1, (FS_DirectoryEntry*)&entry);
//		if (entriesRead)
//		{
//			name = (char*)calloc(sizeof(entry.name) + 1, sizeof(char));
//			size_t namelen = utf16_to_utf8((uint8_t*)name, entry.name, sizeof(entry.name));
//
//			const char* extname = &name[namelen - 4];
//			const char* extsname = &name[namelen - 3];
//			if (entry.attributes == FS_ATTRIBUTE_DIRECTORY)
//			{
//				foldervec.push_back((std::string)name);
//			}
//			else if (
//				((strcasecmp(extname, ".ROM") == 0 || strcasecmp(extname, ".MX1") == 0 || strcasecmp(extname, ".MX2") == 0) && (browsetype & BROWSE_ROM)) ||
//				(strcasecmp(extname, ".DSK") == 0 && (browsetype & BROWSE_DISK)) ||
//				(strcasecmp(extname, ".CAS") == 0 && (browsetype & BROWSE_TAPE)) ||
//				(strcasecmp(extname, ".IPS") == 0 && (browsetype & BROWSE_PATCH)) ||
//				((strcasecmp(extname, ".MCF") == 0 || strcasecmp(extname, ".CHT") == 0) && (browsetype & BROWSE_CHEAT)) ||
//				((strcasecmp(extname, ".JPG") == 0 || strcasecmp(extname, ".PNG") == 0 || strcasecmp(extname, ".BMP") == 0) && (browsetype & BROWSE_IMG)) ||
//				strcasecmp(extname, ".ZIP") == 0 || strcasecmp(extsname, ".GZ") == 0)
//			{
//				filevec.push_back((std::string)name);
//			}
//		}
//		else
//		{
//			break;
//		}
//	}
//	std::sort(foldervec.begin(), foldervec.end(), [](const std::string& a, const std::string& b)
//		{
//			for (int i = 0; i < std::min(a.size(), b.size()); i++)
//			{
//				const int a_char = tolower(a[i]);
//				const int b_char = tolower(b[i]);
//				if (a_char != b_char)return a_char < b_char;
//			}
//			return a.size() < b.size();
//		});
//	std::sort(filevec.begin(), filevec.end(), [](const std::string& a, const std::string& b)
//		{
//			for (int i = 0; i < std::min(a.size(), b.size()); i++)
//			{
//				const int a_char = tolower(a[i]);
//				const int b_char = tolower(b[i]);
//				if (a_char != b_char)return a_char < b_char;
//			}
//			return a.size() < b.size();
//		});
//	for (int i = 0; i < foldervec.size(); i++)
//	{
//		dirstring.push_back(foldervec[i]);
//		ftypevec.push_back(FTYPE_DIR);
//	}
//	for (int i = 0; i < filevec.size(); i++)
//	{
//		dirstring.push_back(filevec[i]);
//		ftypevec.push_back(FTYPE_FILE);
//	}
//	if (isrroot)
//	{
//		if (browsetype & BROWSE_DISK)
//		{
//			dirstring.insert(dirstring.end(), "[Create Blank Disk]");
//			ftypevec.insert(ftypevec.end(), FTYPE_NONE);
//		}
//		dirstring.insert(dirstring.begin(), "[Load Recently Used]");
//		dirstring.insert(dirstring.begin(), "[Back]");
//		ftypevec.insert(ftypevec.begin(), FTYPE_NONE);
//		ftypevec.insert(ftypevec.begin(), FTYPE_NONE);
//		if (browsetype & BROWSE_START)
//		{
//			dirstring.insert(dirstring.begin(), "[Start with no cartridge.]");
//			ftypevec.insert(ftypevec.begin(), FTYPE_NONE);
//		}
//	}
//	else
//	{
//		if (browsetype & BROWSE_DISK)
//		{
//			dirstring.insert(dirstring.end(), "[Create Blank Disk]");
//			ftypevec.insert(ftypevec.end(), FTYPE_NONE);
//		}
//		dirstring.insert(dirstring.begin(), "[Load Recently Used]");
//		dirstring.insert(dirstring.begin(), "[Parent Directory]");
//		ftypevec.insert(ftypevec.begin(), FTYPE_NONE);
//		ftypevec.insert(ftypevec.begin(), FTYPE_NONE);
//		if (browsetype & BROWSE_START)
//		{
//			dirstring.insert(dirstring.begin(), "[Start with no cartridge.]");
//			ftypevec.insert(ftypevec.begin(), FTYPE_NONE);
//		}
//	}
//	PrintMenu("[Open]", 0);
//	FSDIR_Close(dirHandle);
//	svcCloseHandle(dirHandle);
//	FSUSER_CloseArchive(sdmcArchive);
//
//	free(utf16);
//	free(dirPath);
//	free(name);
//}


int OpenDirectoryDir(std::string dirstr, int browsetype)
{
	SDL_Delay(1000);
	DIR* pathdir = opendir(dirstr.c_str());
	if (pathdir == NULL)return -1;
	struct dirent* pathent = NULL;
	dirstring.clear();
	dirstring.shrink_to_fit();
	ftypevec.clear();
	ftypevec.shrink_to_fit();
	menucnt = 0;
	bool isrroot = false;
	if (dirstr == "/")isrroot = true;
	std::vector<std::string> foldervec;
	std::vector<std::string> filevec;
	foldervec.clear();
	filevec.clear();
	while ((pathent = readdir(pathdir))!=NULL)
	{
		if (pathent->d_type & DT_DIR)
		{
			foldervec.push_back((std::string)pathent->d_name);
		}
		else
		{
			if (strlen(pathent->d_name) > 4)
			{
				const char* extname = &pathent->d_name[strlen(pathent->d_name) - 4];
				const char* extsname = &pathent->d_name[strlen(pathent->d_name) - 3];
				if (
					((strcasecmp(extname, ".ROM") == 0 || strcasecmp(extname, ".MX1") == 0 || strcasecmp(extname, ".MX2") == 0) && (browsetype & BROWSE_ROM)) ||
					(strcasecmp(extname, ".DSK") == 0 && (browsetype & BROWSE_DISK)) ||
#if defined(HDD_NEXTOR) || defined(HDD_IDE)
					(strcasecmp(extname, ".DSK") == 0 && (browsetype & BROWSE_HDD)) ||
#endif // HDD_NEXTOR	HDD_IDE
					(strcasecmp(extname, ".CAS") == 0 && (browsetype & BROWSE_TAPE)) ||
					(strcasecmp(extname, ".IPS") == 0 && (browsetype & BROWSE_PATCH)) ||
					((strcasecmp(extname, ".MCF") == 0 || strcasecmp(extname, ".CHT") == 0) && (browsetype & BROWSE_CHEAT)) ||
					((strcasecmp(extname, ".JPG") == 0 || strcasecmp(extname, ".PNG") == 0 || strcasecmp(extname, ".BMP") == 0) && (browsetype & BROWSE_IMG)) ||
					strcasecmp(extname, ".ZIP") == 0 || strcasecmp(extsname, ".GZ") == 0)
				{
					filevec.push_back((std::string)pathent->d_name);
				}
			}
		}
	}
	closedir(pathdir);

	std::sort(foldervec.begin(), foldervec.end(), [](const std::string& a, const std::string& b)
		{
			for (int i = 0; i < std::min(a.size(), b.size()); i++)
			{
				const int a_char = tolower(a[i]);
				const int b_char = tolower(b[i]);
				if (a_char != b_char)return a_char < b_char;
			}
			return a.size() < b.size();
		});
	std::sort(filevec.begin(), filevec.end(), [](const std::string& a, const std::string& b)
		{
			for (int i = 0; i < std::min(a.size(), b.size()); i++)
			{
				const int a_char = tolower(a[i]);
				const int b_char = tolower(b[i]);
				if (a_char != b_char)return a_char < b_char;
			}
			return a.size() < b.size();
		});

	for (int i = 0; i < foldervec.size(); i++)
	{
		dirstring.push_back(foldervec[i]);
		ftypevec.push_back(FTYPE_DIR);
	}
	for (int i = 0; i < filevec.size(); i++)
	{
		dirstring.push_back(filevec[i]);
		ftypevec.push_back(FTYPE_FILE);
	}
	if (isrroot)
	{
		dirstring.insert(dirstring.begin(), "[Frequently Used Folder]");
		dirstring.insert(dirstring.begin(), "[Load Recently Used]");
		//dirstring.insert(dirstring.begin(), "[Frequently Used Folder]");
		dirstring.insert(dirstring.begin(), "[Back]");
		ftypevec.insert(ftypevec.begin(), FTYPE_NONE);
		ftypevec.insert(ftypevec.begin(), FTYPE_NONE);
		ftypevec.insert(ftypevec.begin(), FTYPE_NONE);
		if (browsetype & BROWSE_START)
		{
			dirstring.insert(dirstring.begin(), "[Start with no cartridge.]");
			ftypevec.insert(ftypevec.begin(), FTYPE_NONE);
		}
	}
	else
	{
		dirstring.insert(dirstring.begin(), "[Frequently Used Folder]");
		dirstring.insert(dirstring.begin(), "[Load Recently Used]");
		//dirstring.insert(dirstring.begin(), "[Frequently Used Folder]");
		dirstring.insert(dirstring.begin(), "[Parent Directory]");
		ftypevec.insert(ftypevec.begin(), FTYPE_NONE);
		ftypevec.insert(ftypevec.begin(), FTYPE_NONE);
		ftypevec.insert(ftypevec.begin(), FTYPE_NONE);
		if (browsetype & BROWSE_START)
		{
			dirstring.insert(dirstring.begin(), "[Start with no cartridge.]");
			ftypevec.insert(ftypevec.begin(), FTYPE_NONE);
		}
	}

	PrintMenu("[Open]", 0);
	return 1;
}


void dirstringRebuid(int browsetype)
{
	std::vector<std::string>::iterator itr = dirstring.begin();
	//std::vector<std::string>::iterator itr2 = ftypestring.begin();
	std::vector<u8>::iterator itr3 = ftypevec.begin();
	for (int i = 0; i < 5; i++, itr++, itr3++)
	{
		std::string curstr = *itr;
		if (!(browsetype & BROWSE_START))
		{
			if (curstr == "[Start with no cartridge.]")
			{
				dirstring.erase(itr);
				//ftypestring.erase(itr2);
				ftypevec.erase(itr3);
			}
		}
	}
}


void PrintMenu(const char* msg ,int startid)
{
	C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
	C2D_TargetClear(BottomRenderTartget, Color_Screen);
	C2D_SceneBegin(BottomRenderTartget);
	DrawTextTranslate(msg, 80, 0, 1.0f, fontSize, fontSize, Color_White);
	int menulen = (int)dirstring.size();
	int cnt0 = 0;
	for (int i = startid; i < startid + (240 / textSize) - 2; i++)
	{
		if (i >= menulen)break;
		int idx = std::max(0, i - startid) + 1;
		if (i == menucnt)
		{
			if(ftypevec[i]==FTYPE_DIR)
			{
				C2D_DrawRectSolid(textSize, textSize * (float)idx, 1.0f, 320 - textSize * 2, textSize, Color_White);
				DrawFolderIcon(textStartPos, textSize * (float)idx, (float)textSize, (float)textSize);
				DrawTextScroll(dirstring[i].c_str(), (float)textSize + textStartPos, textSize * (float)idx, 1.0f, fontSize, fontSize, Color_Select);
			}
			//else if (dirstring[i].find_first_of("[") == 0)		/* Slow and unsafe on Old 3DS? */
			else if(ftypevec[i]==FTYPE_NONE)
			{
				C2D_DrawRectSolid(textSize, textSize * (float)idx, 1.0f, 320 - textSize * 2, textSize, Color_White);
				DrawTextTranslate(dirstring[i].c_str(), textStartPos, textSize * (float)idx, 1.0f, fontSize, fontSize, Color_Select);
			}
			else
			{
				C2D_DrawRectSolid(textSize, textSize * (float)idx, 1.0f, 320 - textSize * 2, textSize, Color_White);
				//DrawText(dirstring[i].c_str(), textStartPos, textSize * (float)idx, 1.0f, fontSize, fontSize, Color_Select);
				DrawTextScroll(dirstring[i].c_str(), textStartPos, textSize * (float)idx, 1.0f, fontSize, fontSize, Color_Select);
			}
		}
		else
		{
			if(ftypevec[i]==FTYPE_DIR)
			{
				DrawFolderIcon(textStartPos, textSize * (float)idx, (float)textSize, textSize);
				DrawText(dirstring[i].c_str(), (float)textSize + textStartPos, textSize * (float)idx, 1.0f, fontSize, fontSize, Color_White);
				//DrawTextTranslate(dirstring[i].c_str(), (float)textSize + textStartPos, textSize * (float)idx, 1.0f, fontSize, fontSize, Color_White);
			}
			//else if (dirstring[i].find_first_of("[") == 0)		/* Slow and unsafe on Old 3DS? */
			else if(ftypevec[i]==FTYPE_NONE)
			{
				DrawTextTranslate(dirstring[i].c_str(), textStartPos, textSize * (float)idx, 1.0f, fontSize, fontSize, Color_White);
			}
			else
			{
				DrawText(dirstring[i].c_str(), textStartPos, textSize * (float)idx, 1.0f, fontSize, fontSize, Color_White);
				//DrawTextTranslate(dirstring[i].c_str(), textStartPos, textSize * (float)idx, 1.0f, fontSize, fontSize, Color_White);
			}
		}
	}
	if (menucnt >= 0 && menucnt < menulen)
	{
		std::string tempstr = "";
		if (menucnt < ftypevec.size())
		{
			if(ftypevec[menucnt]==FTYPE_DIR)
			{
				tempstr = "Open";
			}
			else if(ftypevec[menucnt]==FTYPE_FILE)
			{
				tempstr = "Open";
			}
			else
			{
				tempstr = "Select";
			}
		}
		DrawOKButton(tempstr.c_str());
	}
	DrawCancelButton("Back");
	C2D_DrawRectSolid(0, 0, 1, textSize, 240 - textSize, Color_Screen);
	C2D_DrawRectSolid(320 - textSize, 0, 1, textSize, 240 - textSize, Color_Screen);
	int textNum = 240 / textSize - 2;
	if (menulen > textNum)
	{
		if (startid > 0)DrawScrollButtonL();
		if ((startid / textNum + 1) * textNum < menulen) DrawScrollButtonR();
	}
	C3D_FrameEnd(0);
}


void systemMenu()
{
	if (IsDebug)
	{
		for (;;)
		{
			hidScanInput();
			u32 kDown = hidKeysDown();
			if (kDown & KEY_SELECT)return;
			if (kDown & KEY_START)break;
		}
		IsDebug = false;
		C2D_Fini();
		C3D_Fini();
		gfxInitDefault();
		consoleInit(GFX_BOTTOM, 0);
		gfxSetDoubleBuffering(GFX_TOP, false);
		InitCitro();
		gfxSetScreenFormat(GFX_TOP, GSP_RGB565_OES);
		DrawSoldBottom(0, 320, 0, 240, 71, 71, 71);
	}
	textbuf = C2D_TextBufNew(4096);
	bool needRedraw = true;
	int menuItemCount = menuItem.size();
#ifdef MEMORY_CURSOR_POS
	if (!MemorySysMenupos)sysmenuIdx = 0;
#else
	sysmenuIdx = 0;
#endif // MEMORY_CURSOR_POS
	int startid = 0;
	while (aptMainLoop())
	{
		SDL_Delay(10);
		if (needRedraw == true)
		{
			C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
			C2D_TargetClear(BottomRenderTartget, Color_Screen);
			C2D_SceneBegin(BottomRenderTartget);
			DrawTextTranslate("[System Menu]", 120, 0, 1.0f, fontSize, fontSize, Color_White);
			for (int i = startid; i < startid + textColumn- 2; i++)
			{
				if (i >= menuItemCount)break;
				int idx = std::max(0, i - startid);
				if (i == sysmenuIdx)
				{
					C2D_DrawRectSolid(textSize, textSize * (float)(idx + 1), 1.0f, 320 - textSize * 2, textSize, Color_White);
					DrawTextTranslate(menuItem[i].c_str(), textStartPos, textSize * (float)(idx + 1), 1.0f, fontSize, fontSize, Color_Select);
				}
				else
				{
					DrawTextTranslate(menuItem[i].c_str(), textStartPos, textSize * (float)(idx + 1), 1.0f, fontSize, fontSize, Color_White);
				}
			}
			if (sysmenuIdx >= 0 && sysmenuIdx < menuItemCount)
			{
				DrawOKButton("Select");
			}
			DrawCancelButton("Back");
			int textNum = textColumn - 2;
			if (menuItemCount > textNum)
			{
				if (startid > 0)DrawScrollButtonL();
				if ((startid / textNum + 1) * textNum < menuItemCount) DrawScrollButtonR();
			}
			needRedraw = false;
			C3D_FrameEnd(0);
		}
		SDL_Delay(10);
		hidScanInput();
		u32 kDown = hidKeysDown();
		u32 kHeld = hidKeysHeld();
		bool IsAPress = false, IsLPress = false, IsRPress = false;
		if (kHeld & KEY_UP)
		{
			GetMovedMenuIndex(sysmenuIdx, startid, -1, NULL);
			SDL_Delay(TextDelay);
			needRedraw = true;
		}
		if (kHeld & KEY_DOWN)
		{
			GetMovedMenuIndex(sysmenuIdx, startid, 1, menuItemCount - 1);
			SDL_Delay(TextDelay);
			needRedraw = true;
		}
		if (kDown & KEY_A)
		{
			IsAPress = true;
		}
		else if (kDown & KEY_B)
		{
			return;
		}
		else if (kHeld & KEY_LEFT)IsLPress = true;
		else if (kHeld & KEY_RIGHT)IsRPress = true;
		hidTouchRead(&tp);
		int px = tp.px;
		int py = tp.py;
		int oldpx = oldtp.px;
		int oldpy = oldtp.py;
		if (px != 0 && py != 0)
		{
			if (py > textSize && py < 240 - textSize && px>textStartPos && px < textEndPos)
			{
				sysmenuIdx = (py / textSize) + startid - 1;
				SDL_Delay(TextDelay);
				needRedraw = true;
			}
			if (CheckScrollButtonL(px, py))IsLPress = true;
			if (CheckScrollButtonR(px, py))IsRPress = true;
		}
		if (kDown & KEY_TOUCH)
		{
			kDown &= ~KEY_TOUCH;
			if (CheckCancelButton(px, py))return;
			if (CheckOKButton(px, py))IsAPress = true;
		}
		oldtp.px = px;
		oldtp.py = py;
		if (IsAPress && sysmenuIdx >= 0 && sysmenuIdx < menuItemCount)
		{
			if (sysmenuIdx == 0)
			{
				SDL_Delay(100);
				return;
			}
			std::string selectmenu = menuItem[sysmenuIdx];
			if (selectmenu == "[Back]")
			{
				SDL_Delay(100);
				return;
			}
			if (selectmenu == "[Reset]")
			{
				BrowseReset();
				return;
			}
			else if(selectmenu == "[File Open]")
			{
				BrowseROM(0, BROWSE_ALL);
				return;
			}
			else if (selectmenu == "[LoadDisk(DriveA)]")
			{
				BrowseROM(0, BROWSE_DISK);
				return;
			}
			else if (selectmenu == "[LoadDisk(DriveB)]")
			{
				BrowseROM(1, BROWSE_DISK);
				return;
			}
			else if (selectmenu == "[LoadROM(Slot1)]")
			{
				BrowseROM(0, BROWSE_ROM);
				return;
			}
			else if (selectmenu == "[LoadROM(Slot2)]")
			{
				BrowseROM(1, BROWSE_ROM);
				return;
			}
			else if (selectmenu == "[Eject ROM]")
			{
				std::vector<char*> menuvec;
				menuvec.clear();
				menuvec.push_back("[Back]");
				menuvec.push_back("[Eject ROM(Slot 1)]");
				menuvec.push_back("[Eject ROM(Slot 2)]");
				menuvec.push_back("");
				int menuid = BrowseInt("[Select a Slot]", menuvec, 0, 0, false);
				std::string menustr = std::string(menuvec[menuid]);
				if (menustr == "[Back]")
				{
					return;
				}
				else if (menustr.substr(0, 11) == "[Eject ROM(")
				{
					int slotid = menustr == "[Eject ROM(Slot 1)]" ? 0 : 1;
					LoadCart(0, slotid, 0);
					ROMStr[slotid] = "";
					return;
				}
			}
			else if (selectmenu == "[Apply IPS Patch]")
			{
				BrowseROM(0, BROWSE_PATCH);
				return;
			}
			else if(selectmenu== "[SaveDisk(DriveA)]")
			{
				if (DiskStr[0].size() > 2)
				{
					if(SaveFDI(&FDD[0], DiskStr[0].c_str(), FMT_MSXDSK))DiskWrited[0] = 0;
				}
			}
			else if(selectmenu== "[SaveDisk(DriveB)]")
			{
				if (DiskStr[1].size() > 2)
				{
					if(SaveFDI(&FDD[1], DiskStr[1].c_str(), FMT_MSXDSK))DiskWrited[1] = 0;
				}
			}
			else if(selectmenu=="[Create New Disk]")
			{
				CreateBlankDisk(0);
			}
			else if (selectmenu == "[Eject Disk]")
			{
				std::vector<char*> menuvec;
				menuvec.clear();
				menuvec.push_back("[Back]");
				menuvec.push_back("[Eject Disk(Drive A)]");
				menuvec.push_back("[Eject Disk(Drive B)]");
				menuvec.push_back("");
				int menuid = BrowseInt("[Select a Disk Drive]", menuvec, 0, 0, false);
				std::string menustr = std::string(menuvec[menuid]);
				if (menustr == "[Back]")
				{
					return;
				}
				else if (menustr.substr(0, 12) == "[Eject Disk(")
				{
					int slotid = menustr == "[Eject Disk(Drive A)]" ? 0 : 1;
					if (DiskWrited[slotid] == 1)
					{
						if (DiskStr[slotid].size() > 2)
						{
							if(SaveFDI(&FDD[slotid], DiskStr[slotid].c_str(), FMT_MSXDSK))DiskWrited[slotid] = 0;
						}
					}
					ChangeDisk(slotid, 0);
					DiskStr[slotid] = "";
					DiskRawStr[slotid] = "";
					return;
				}
			}
#ifdef HDD_NEXTOR
			else if (selectmenu == "[Load HardDisk]")
			{
				BrowseROM(0, BROWSE_HDD);
				return;
				//LoadPatchedNEXTOR();
			}
#endif // HDD_NEXTOR
			else if (selectmenu == "[Load Cassette Tape]")
			{
				BrowseROM(0, BROWSE_TAPE);
				return;
			}
			else if(selectmenu == "[Load SaveDATA Cassette Tape]")
			{
				std::string tapepath = BrowseFile(".CAS", "/TAPE/", false, "Load Data(Cassette Tape)");
				if (tapepath.size() > 2)
				{
					ChangeTape(tapepath.c_str());
					RewindTape();
					return;
				}
			}
			else if(selectmenu== "[Create new SaveDATA Cassette Tape]")
			{
				std::string tapepath = BrowseFile(".CAS", "/TAPE/", true, "Create new Cassette Tape");
				if (tapepath.size() > 2)
				{
					FILE* fp = fopen(tapepath.c_str(), "w");
					fclose(fp);
					ChangeTape(tapepath.c_str());
					return;
				}
			}
			else if(selectmenu=="[Rewind Cassette Tape]")
			{
				RewindTape();
				return;
			}
			else if(selectmenu=="[State Load]")
			{
				std::string statePath = BrowseFile(".STA","/STATE/",false,"Load State");
				if(statePath.size() > 2)LoadSTA(statePath.c_str());
				//DrawMessage("You load a save state.", "Press any key or touch screen to resume.", 10, 50, 1000, false);
			}
			else if(selectmenu== "[State Save]")
			{
				std::string statePath = BrowseFile(".STA", "/STATE/",true, "Save State");
				if (statePath.size() > 2)SaveSTA(statePath.c_str());
				//DrawMessage("You save a save state.", "Press any key or touch screen to resume.", 10, 50, 1000, false);
			}
			else if(selectmenu== "[KeyConfig]")
			{
				std::vector<char*> keyconfvec;
				keyconfvec.clear();
				keyconfvec.push_back("[Back]");
				keyconfvec.push_back("[Assign 3DS Pad as Joystick]");
				keyconfvec.push_back("[Assign 3DS Pad as Cursor keys]");
				keyconfvec.push_back("[Assign 3DS Pad as Numpads]");
				keyconfvec.push_back("[Custom Keyconfig]");
				keyconfvec.push_back("[Rapidfire Setting]");
				keyconfvec.push_back("[Reset Default]");
				keyconfvec.push_back("");
				int menuid = BrowseInt("        [Browse Keyconfig]", keyconfvec, 0, 0, false);

				std::string confstr = std::string(keyconfvec[menuid]);
				if (confstr == "[Back]")
				{
					return;
				}
				else if(confstr == "[Assign 3DS Pad as Joystick]")
				{
					if (BrowseOK("Change the all key bindings for 3DS Pad.", "Do you want to continue?"))
					{
						keyconfItem[1].currentIdx = 0;
						keyconfItem[2].currentIdx = 0;
						keyconfItem[3].currentIdx = 0;
						keyconfItem[4].currentIdx = 0;
						keyconfItem[9].currentIdx = 0;
						keyconfItem[10].currentIdx = 0;
					}
				}
				else if(confstr == "[Assign 3DS Pad as Cursor keys]")
				{
					if (BrowseOK("Change the all key bindings for 3DS Pad.", "Do you want to continue?"))
					{
						keyconfItem[1].currentIdx = KBD_UP;
						keyconfItem[2].currentIdx = KBD_DOWN;
						keyconfItem[3].currentIdx = KBD_LEFT;
						keyconfItem[4].currentIdx = KBD_RIGHT;
						keyconfItem[9].currentIdx = KBD_SPACE;
						keyconfItem[10].currentIdx = KBD_SHIFT;
					}
				}
				else if(confstr == "[Assign 3DS Pad as Numpads]")
				{
					if (BrowseOK("Change the all key bindings for 3DS Pad.", "Do you want to continue?"))
					{
						keyconfItem[1].currentIdx = KBD_NUMPAD8;
						keyconfItem[2].currentIdx = KBD_NUMPAD2;
						keyconfItem[3].currentIdx = KBD_NUMPAD4;
						keyconfItem[4].currentIdx = KBD_NUMPAD6;
						keyconfItem[9].currentIdx = KBD_SPACE;
						keyconfItem[10].currentIdx = KBD_SHIFT;
					}
				}
				else if(confstr == "[Custom Keyconfig]")
				{
					BrowseKeyconfig();
				}
				else if(confstr== "[Rapidfire Setting]")
				{
					BrowseRapidFire();
				}
				else if(confstr == "[Reset Default]")
				{
					if (BrowseOK("Dou you want to Reset?", NULL) == true)
					{
						for (int i = 0; i < keyconfItem.size(); i++)
						{
							keyconfItem[i].currentIdx = keyconfItem[i].defaultIdx;
						}
						for (int i = 0; i < sizeof(rapidInterVals); i++)rapidInterVals[i] = 0;
					}
				}
				for (int i = 0; i < 20; i++)
				{
					KeyMaps3DS[i] = keyconfItem[i + 1].currentIdx;
				}
				return;
			}
			else if (selectmenu == "[Option]")
			{
				BrowseOptions();
				LoadOption(false);
				return;
			}
			else if(selectmenu == "[Load Screen Shot]" || selectmenu == "[Load Screen Shot](Show Keyboard)")
			{
				std::string scrpath = BrowseFile(".bmp", "/SNAP/", false, "Load Screen Shot");
				if (scrpath.size() > 2)
				{
					ScreenShotSurface = SDL_LoadBMP(scrpath.c_str());
					ScreenShotOffx = ScreenShotOffy = 0;
					IsScreenShot = true;
					if (selectmenu == "[Load Screen Shot](Show Keyboard)")IsSmallScrShot = true;
					else IsSmallScrShot = false;
				}
			}
			else if(selectmenu== "[Save Screen Shot]")
			{
				std::string screenpath = BrowseFile(".bmp","/SNAP/", true, "Save Screen Shot");
				if (screenpath.size() > 2)SaveScrrenShot(screenpath.c_str());
			}
			else if(selectmenu == "[Load Reference Image]")
			{
				BrowseROM(0, BROWSE_IMG);
			}
			else if (selectmenu == "[Change ROM Mapper]")
			{
				std::vector<char*> menuvec;
				menuvec.clear();
				menuvec.push_back("[Back]");
				menuvec.push_back("[Change Mapper(Slot 1)]");
				menuvec.push_back("[Change Mapper(Slot 2)]");
				menuvec.push_back("");
				int menuid = BrowseInt("[Select a Slot]", menuvec, 0, 0, false);
				std::string menustr = std::string(menuvec[menuid]);
				if (menustr == "[Back]")
				{
					return;
				}
				else if(menustr.substr(0,15) == "[Change Mapper(")
				{
					int slotid = menustr == "[Change Mapper(Slot 1)]" ? 0 : 1;
					if (ROMStr[slotid].size() < 2)
					{
						BrowseOK("Load ROM before selecting the ROM Mapper.", "");
						return;
					}
					std::vector<char*> mappervec;
					mappervec.clear();
					mappervec.push_back("[Cancel Select]");
					mappervec.push_back("GEN8");
					mappervec.push_back("GEN16");
					mappervec.push_back("KONAMI5");
					mappervec.push_back("KONAMI4");
					mappervec.push_back("ASCII8");
					mappervec.push_back("ASCII16");
					mappervec.push_back("GMASTER2");
					mappervec.push_back("FMPAC");
					mappervec.push_back("ASCII8SRM32");
					mappervec.push_back("ASCII16_2");
					mappervec.push_back("CrossBlaim");
					mappervec.push_back("HarryFox16");
					mappervec.push_back("Zemina");
					mappervec.push_back("ZeminaDS2");
					mappervec.push_back("XXin1");
					mappervec.push_back("126in1");
					mappervec.push_back("MSX90");
					mappervec.push_back("Swangi");
					mappervec.push_back("Dooly");
					mappervec.push_back("LodeRunner");
					mappervec.push_back("Pierrot");
					mappervec.push_back("RType");
					mappervec.push_back("Wizardry");
					mappervec.push_back("MANBOW2");
					mappervec.push_back("MAJUTSUSHI ");
					mappervec.push_back("SCCPLUS");
					mappervec.push_back("SCCPLUS_2");
					mappervec.push_back("PLAIN");
					mappervec.push_back("WingWarr");
					mappervec.push_back("ESESCC");
					mappervec.push_back("Synthesizer");
					mappervec.push_back("[Reset Mapper]");
					mappervec.push_back("");
					int mapperid = BrowseInt("Choose a Mapper", mappervec, 0, 0, false);
					if (mapperid > 0)
					{
						NeedRest = 0;
						if ((mapperid-1) == MAP_GUESS)LoadCart(ROMStr[slotid].c_str(), slotid, MAP_GUESS);
						else
						{
							SetROMType(slotid, mapperid-1);
							LoadCart(ROMStr[slotid].c_str(), slotid, GetROMType(slotid));
						}
						NeedRest = 1;
						if (BrowseOK("Need Reset for change the mapper.", "Do you want to Reset now?"))
						{
							ResetMSX(Mode, RAMPages, VRAMPages);
							return;
						}
					}
				}
			}
			else if(selectmenu == "[Load Konami SCC+ ROM]")
			{
				//LoadSCCPLUS(0);
				LoadCart("SCCPLUS", 0, MAP_SCCPLUS_2);
				if (ReadSCCPlus)CartSpecial[0] = CART_READSCC;
				ResetMSX(Mode, RAMPages, VRAMPages);
				return;
			}
			else if(selectmenu == "[Load Ese SCC 512k ROM]")
			{
				LoadCart("esescc512A", 0, MAP_ESESCC);
				ResetMSX(Mode, RAMPages, VRAMPages);
				return;
			}
#ifdef VDP_V9990
#ifdef DUAL_SCREEN
			else if(selectmenu == "[V9990 Dual Screen]")
			{
				V9990DualScreen = BrowseInt("[V9990 Dual Screen]", OptionOffOn, V9990DualScreen, 0, false);
				if (!V9990DualScreen)V9990Dual &= 0xFD;
				else
				{
					DrawMessage("fMSX3DS running on dual screen mode now.", "Touch screen to end.", 10, 50, 1000, true);
					if (V9990Active)V9990Dual |= 2;
					return;
				}
			}
#endif // DUAL_SCREEN
#endif // VDP_V9990
#ifdef USE_OVERCLOCK
			else if (selectmenu == "[OverClockR800(Unsafe)]")
			{
				overClockRatio = BrowseInt("OverClock Rate", OptionOverClock, overClockRatio, 0, false);
			}
#endif // USE_OVERCLOCK
			else if (selectmenu == "[Fast Forward]")
			{
			if (BrowseOK("Do you want to enter fast forward mode?", NULL))
			{
				TurboNow = 1;
				IsFrameLimit = false;
				UPeriod = 10;
				DrawMessage("fMSX3DS running on fast forward mode now.", "Press any key or touch screen to end.", 10, 50, 1000, false);
				return;
			}
			}
			else if (selectmenu == "[Cheat]")
			{
				std::vector<char*> cheatvec;
				cheatvec.clear();
				cheatvec.push_back("[Back]");
				cheatvec.push_back("[Open Cheat File]");
				cheatvec.push_back("[Load IPS patch]");
				cheatvec.push_back("[Enter Cheat Code]");
				cheatvec.push_back("[Start Cheat]");
				cheatvec.push_back("[Stop Cheat]");
				cheatvec.push_back("[Load Cheat]");
				cheatvec.push_back("[Save Cheats]");
				cheatvec.push_back("");
				int menuid = BrowseInt("[Browse Cheat]", cheatvec, 0, 0, false);

				std::string cheatstr = std::string(cheatvec[menuid]);
				if (cheatstr == "[Back]")
				{
					return;
				}
				else if (cheatstr == "[Open Cheat File]")
				{
					BrowseROM(0, BROWSE_CHEAT);
					return;
				}
				else if(cheatstr == "[Enter Cheat Code]")
				{
					static SwkbdState swkbd;
					SwkbdButton button = SWKBD_BUTTON_NONE;
					static char kbdchar0[5], kbdchar1[3];
					static std::string swkbdstr0,swkbdstr1,swkbdstr;
					swkbdInit(&swkbd, SWKBD_TYPE_QWERTY, 1, 4);
					swkbdSetHintText(&swkbd, "Enter cheat code adress (First 4 character of the cheat code.)");
					button = swkbdInputText(&swkbd, kbdchar0, sizeof(kbdchar0));
					swkbdstr0 = std::string(kbdchar0);
					if (swkbdstr0.size() < 1)return;
					swkbdInit(&swkbd, SWKBD_TYPE_QWERTY, 1, 2);
					swkbdSetHintText(&swkbd, "Enter cheat code value (Last 2 character of the cheat code.)");
					button = swkbdInputText(&swkbd, kbdchar1, sizeof(kbdchar1));
					swkbdstr1 = std::string(kbdchar1);
					if (swkbdstr1.size() < 1)return;
					swkbdstr = swkbdstr0 + "-" + swkbdstr1;
					if (AddCheat(swkbdstr.c_str()) != 0)Cheats(1);
					return;
				}
				else if(cheatstr == "[Start Cheat]")
				{
					Cheats(CHTS_ON);
					return;
				}
				else if(cheatstr == "[Stop Cheat]")
				{
					Cheats(CHTS_OFF);
					return;
				}
				else if(cheatstr== "[Load Cheat]")
				{
					std::string loadpath = BrowseFile(".CHT", "/CHEAT/", false, "LoadCheat");
					if (loadpath.size() > 2)if (LoadCHT(loadpath.c_str()) != 0)Cheats(CHTS_ON);
				}
				else if (cheatstr == "[Save Cheats]")
				{
					std::string savepath = BrowseFile(".CHT", "/CHEAT/", true, "SaveCheat");
					if (savepath.size() > 2)SaveCHT(savepath.c_str());
				}
			}
			else if (selectmenu == "[Quit fMSX]")
			{
				if (BrowseOK("Do You want to Quit?",NULL)==true)
				{
					ExitNow = 1;
					return;
				}
			}
#ifdef DEBUG_ENABLE
			else if(selectmenu== "[Debug]")
			{
				DebugMenu();
				if (IsDebug)return;
			}
#endif // DEBUG_ENABLE
			else
			{
				needRedraw = true;
				continue;
			}
			SDL_Delay(1000);
			return;
		}
		if (IsLPress)
		{
			GetMovedMenuIndex(sysmenuIdx, startid, -(textColumn - 2), NULL);
			SDL_Delay(TextDelay);
			needRedraw = true;
		}
		if (IsRPress)
		{
			GetMovedMenuIndex(sysmenuIdx, startid, (textColumn - 2), menuItemCount - 1);
			SDL_Delay(TextDelay);
			needRedraw = true;
		}
		BrowseHomeButton();
	}
	//ExitNow = 1;
}


void BrowseReset()
{
	std::vector<char*> resetvec;
	resetvec.clear();
	resetvec.push_back("[Back]");
#ifdef TURBO_R
	resetvec.push_back("[Hardware Rest(MSXTR)]");
	resetvec.push_back("[Software Reset(MSXTR)]");
#endif // TURBO_R
	resetvec.push_back("[Hardware Rest(MSX2+)]");
	resetvec.push_back("[Software Reset(MSX2+)]");
	resetvec.push_back("[Hardware Rest(MSX2)]");
	resetvec.push_back("[Software Reset(MSX2)]");
	resetvec.push_back("[Hardware Rest(MSX1)]");
	resetvec.push_back("[Software Reset(MSX1)]");


	//int resetid = BrowseInt("        [Select Reset Hardware]", resetvec, 0, 0);
	int resetid = BrowseInt("        [Select Reset Hardware]", resetvec, (3 - (Mode & MSX_MODEL)) * 2 + 1, 0, true);
#ifdef TURBO_R
	switch (resetid)
	{
	case 0:
		return;
	case 1:
		IsHardReset = 1;
		ResetMSX((Mode & ~MSX_MODEL) | MSX_MSXTR, 4<<NewRAMSize, VRAMPages);
		return;
	case 2:
		IsHardReset = 0;
		ResetMSX((Mode & ~MSX_MODEL) | MSX_MSXTR, 4 << NewRAMSize, VRAMPages);
		return;
	case 3:
		IsHardReset = 1;
		ResetMSX((Mode & ~MSX_MODEL) | MSX_MSX2P, 4 << NewRAMSize, VRAMPages);
		//ResetSound();
		return;
	case 4:
		IsHardReset = 0;
		ResetMSX((Mode & ~MSX_MODEL) | MSX_MSX2P, 4 << NewRAMSize, VRAMPages);
		//ResetSound();
		return;
	case 5:
		IsHardReset = 1;
		ResetMSX((Mode & ~MSX_MODEL) | MSX_MSX2, 4 << NewRAMSize, VRAMPages);
		//ResetSound();
		return;
	case 6:
		IsHardReset = 0;
		ResetMSX((Mode & ~MSX_MODEL) | MSX_MSX2, 4 << NewRAMSize, VRAMPages);
		//ResetSound();
		return;
	case 7:
		IsHardReset = 1;
		ResetMSX((Mode & ~MSX_MODEL) | MSX_MSX1, 4 << NewRAMSize, VRAMPages);
		//ResetSound();
		return;
	case 8:
		IsHardReset = 0;
		ResetMSX((Mode & ~MSX_MODEL) | MSX_MSX1, 4 << NewRAMSize, VRAMPages);
		//ResetSound();
		return;

	default:
		break;
	}
#else
	switch (resetid)
	{
	case 0:
		return;
	case 1:
		IsHardReset = 1;
		ResetMSX((Mode & ~MSX_MODEL) | MSX_MSX2P, RAMPages, VRAMPages);
		//ResetSound();
		return;
	case 2:
		IsHardReset = 0;
		ResetMSX((Mode & ~MSX_MODEL) | MSX_MSX2P, RAMPages, VRAMPages);
		//ResetSound();
		return;
	case 3:
		IsHardReset = 1;
		ResetMSX((Mode & ~MSX_MODEL) | MSX_MSX2, RAMPages, VRAMPages);
		//ResetSound();
		return;
	case 4:
		IsHardReset = 0;
		ResetMSX((Mode & ~MSX_MODEL) | MSX_MSX2, RAMPages, VRAMPages);
		//ResetSound();
		return;
	case 5:
		IsHardReset = 1;
		ResetMSX((Mode & ~MSX_MODEL) | MSX_MSX1, RAMPages, VRAMPages);
		//ResetSound();
		return;
	case 6:
		IsHardReset = 0;
		ResetMSX((Mode & ~MSX_MODEL) | MSX_MSX1, RAMPages, VRAMPages);
		//ResetSound();
		return;

	default:
		break;
	}
#endif // TURBO_R
	Mode = (Mode & ~MSX_MODEL) | MSX_MSX2;
	ResetMSX(Mode, RAMPages, VRAMPages);
	//ResetSound();
}


void BrowseHomeButton()
{
	if (aptCheckHomePressRejected())
	{
		if (BrowseOK("You press the Home Button.", "Do you want to Quit to home menu?") == true)
		{
			ExitNow = 1;
			return;
		}
	}
}


#ifdef DEBUG_ENABLE
void DebugMenu()
{
	std::vector<std::string> debugmenuItem =
	{
		"[Back]",
		"[Show VDP Register]",
#ifdef VDP_V9990
		"[Show V9990 VDP Rgister]",
		"[Show V9990 VDP Port]",
		"[V9990 Status Reset]",
#endif // VDP_V9990
		"[Show Disk Info]",
		"[Set VDP Status Value]",
		"[Set VDP value]",
		"[Show Slot Info]",
		"[Set Slot Value]",
		"[Reset PPI]",
		"[Show Sprite Adress]",
		"[Show VRAM]",
		"[Show RAM]",
		"[Clear VDP Register #15]",
		"[ToggleVkey]",
		"[Show PSG Register]",
		"[Calc Screen8 Table]",
		"[Clear Debug Log]",
		"[Show Debug Message]",
		"[Enter Debug mode.]",
		""
	};
	textbuf = C2D_TextBufNew(4096);
	bool needRedraw = true;
	int menuItemCount = debugmenuItem.size();
	int selectIndex = 0;
	int startid = 0;
	bool IsInConsole = false;
	while (aptMainLoop())
	{
		SDL_Delay(10);
		if (needRedraw == true)
		{
			C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
			C2D_TargetClear(BottomRenderTartget, Color_Screen);
			C2D_SceneBegin(BottomRenderTartget);
			DrawTextTranslate("[Debug Menu]", 120, 0, 1.0f, fontSize, fontSize, Color_White);
			for (int i = startid; i < startid + (240 / textSize) - 2; i++)
			{
				if (i >= menuItemCount)break;
				int idx = std::max(0, i - startid);
				if (i == selectIndex)
				{
					C2D_DrawRectSolid(textSize, textSize * (float)(idx + 1), 1.0f, 320 - textSize * 2, textSize, Color_White);
					DrawTextTranslate(debugmenuItem[i].c_str(), textStartPos, textSize * (float)(idx + 1), 1.0f, fontSize, fontSize, Color_Select);
				}
				else
				{
					DrawTextTranslate(debugmenuItem[i].c_str(), textStartPos, textSize * (float)(idx + 1), 1.0f, fontSize, fontSize, Color_White);
				}
			}
			if (selectIndex >= 0 && selectIndex < menuItemCount)
			{
				DrawOKButton("Select");
			}
			DrawCancelButton("Back");
			int textNum = 240 / textSize - 2;
			if (menuItemCount > textNum)
			{
				if (startid > 0)DrawScrollButtonL();
				if ((startid / textNum + 1) * textNum < menuItemCount) DrawScrollButtonR();
			}
			needRedraw = false;
			C3D_FrameEnd(0);
		}
		SDL_Delay(10);
		hidScanInput();
		u32 kDown = hidKeysDown();
		u32 kHeld = hidKeysHeld();
		bool IsAPress = false, IsLPress = false, IsRPress = false;
		if (kHeld & KEY_UP)
		{
			GetMovedMenuIndex(selectIndex, startid, -1, NULL);
			SDL_Delay(TextDelay);
			needRedraw = true;
		}
		if (kHeld & KEY_DOWN)
		{
			GetMovedMenuIndex(selectIndex, startid, 1, menuItemCount - 1);
			SDL_Delay(TextDelay);
			needRedraw = true;
		}
		if (kDown & KEY_A)
		{
			IsAPress = true;
		}
		else if (kDown & KEY_B)
		{
			break;
		}
		else if (kHeld & KEY_LEFT)IsLPress = true;
		else if (kHeld & KEY_RIGHT)IsRPress = true;
		hidTouchRead(&tp);
		int px = tp.px;
		int py = tp.py;
		int oldpx = oldtp.px;
		int oldpy = oldtp.py;
		if (px != 0 && py != 0)
		{
			if (py > textSize && py < 240 - textSize && px>textStartPos && px < textEndPos)
			{
				selectIndex = (py / textSize) + startid - 1;
				SDL_Delay(TextDelay);
				needRedraw = true;
			}
			if (CheckScrollButtonL(px, py))IsLPress = true;
			if (CheckScrollButtonR(px, py))IsRPress = true;
		}
		if (kDown & KEY_TOUCH)
		{
			kDown &= ~KEY_TOUCH;
			if (CheckCancelButton(px, py))return;
			if (CheckOKButton(px, py))IsAPress = true;
		}
		oldtp.px = px;
		oldtp.py = py;
		if (IsAPress && selectIndex >= 0 && selectIndex < menuItemCount)
		{
			std::string selectmenu = debugmenuItem[selectIndex];
			if (selectmenu == "[Back]")
			{
				SDL_Delay(TextDelay);
				break;
			}
			else if (selectmenu == "[Show Disk Info]")
			{
				gfxInitDefault();
				consoleInit(GFX_BOTTOM, _NULL);
				IsInConsole = true;
				for (int i = 0; i < 6; i++)
				{
					printf("Header["); printf(std::to_string(i).c_str()); printf("]: ");
					printf(std::to_string(FDD->Header[i]).c_str()); printf("\n");
				}
				printf("Data Size: "); printf(std::to_string(FDD->DataSize).c_str()); printf("\n");
				printf("Disk Format: "); printf(std::to_string(FDD->Format).c_str()); printf("\n");
				printf("Sector Size: "); printf(std::to_string(FDD->SecSize).c_str()); printf("\n");
				printf("Sector Num: "); printf(std::to_string(FDD->Sectors).c_str()); printf("\n");
				printf("Sides: "); printf(std::to_string(FDD->Sides).c_str()); printf("\n");
				printf("Tracks: "); printf(std::to_string(FDD->Tracks).c_str()); printf("\n");
				ExitConsole();
				break;
			}
			else if(selectmenu== "[Show VDP Register]")
			{
				gfxInitDefault();
				consoleInit(GFX_BOTTOM, _NULL);
				IsInConsole = true;
				for (int i = 0; i < 28; i++)
				{
					printf("VDP["); printf(std::to_string(i).c_str()); printf("]: ");
					printf(std::to_string(VDP[i]).c_str());
					if (i & 1)printf("\n");
					else printf("  ");
				}
				for (int i = 32; i < 47; i++)
				{
					printf("VDP["); printf(std::to_string(i).c_str()); printf("]: ");
					printf(std::to_string(VDP[i]).c_str());
					if (i & 1)printf("\n");
					else printf("  ");
				}
				printf("Screen Mode:");
				if (ScrMode != 8 && ScrMode!=7)printf(std::to_string(ScrMode).c_str());
				else if (ModeYAE && !MODEL(MSX_MSX2))printf("10"); else if (ModeYJK && !MODEL(MSX_MSX2))printf("12");
				else printf(std::to_string(ScrMode).c_str());  printf("\n");
				printf("HScroll:"); printf(std::to_string(HScroll).c_str()); printf("\n");
				int Hscrollfix = (VDP[27] & 0x07) - ((VDP[26] & 0x03F) << 3);
				printf("Hscroll fix:"); printf(std::to_string(Hscrollfix).c_str()); printf("\n");

				for (int i = 0; i < 9; i++)
				{
					printf("VDPStatus["); printf(std::to_string(i).c_str()); printf("]: ");
					printf(std::to_string(VDPStatus[i]).c_str());
					if (i & 1)printf("\n");
					else printf("  ");
				}
				ExitConsole();
				break;
			}
#ifdef VDP_V9990
			else if(selectmenu == "[Show V9990 VDP Rgister]")
			{
				gfxInitDefault();
				consoleInit(GFX_BOTTOM, _NULL);
				IsInConsole = true;
				for (int i = 0; i < 28; i++)
				{
					printf("V9990VDP["); printf(std::to_string(i).c_str()); printf("]: ");
					printf(std::to_string(V9990VDP[i]).c_str());
					if (i & 1)printf("\n");
					else printf("  ");
				}
				for (int i = 32; i < 55; i++)
				{
					printf("V9990VDP["); printf(std::to_string(i).c_str()); printf("]: ");
					printf(std::to_string(V9990VDP[i]).c_str());
					if (i & 1)printf("\n");
					else printf("  ");
				}
				printf("Screen Mode:");
				printf(std::to_string(V9KScrMode).c_str()); printf("\n");
				ExitConsole();
				break;
			}
			else if(selectmenu == "[Show V9990 VDP Port]")
			{
				gfxInitDefault();
				consoleInit(GFX_BOTTOM, _NULL);
				IsInConsole = true;
				for (int i = 0; i < 16; i++)
				{
					printf("V9990Port["); printf(std::to_string(i).c_str()); printf("]: ");
					printf(std::to_string(V9990Port[i]).c_str());
					if (i & 1)printf("\n");
					else printf("  ");
				}
				ExitConsole();
				break;
			}
			else if (selectmenu== "[V9990 Status Reset]")
			{
				ResetV9990VDPRegister();
				return;
			}
#endif // VDP_V9990
			else if(selectmenu== "[Set VDP Status Value]")
			{
				std::vector<char*> vdpstvec;
				vdpstvec.clear();
				for (int i = 0; i < 9; i++)
				{
					std::string str = "VDPStatus[" + std::to_string(i) + "]";
					vdpstvec.push_back(StringToChar(str));
				}
				int vdpstid = BrowseInt("      [Select a VDP Status Register for edit.]", vdpstvec, 0, 0, false);
				static SwkbdState swkbd;
				SwkbdButton button = SWKBD_BUTTON_NONE;
				static char kbdchar[4];
				static int val;
				swkbdInit(&swkbd, SWKBD_TYPE_NUMPAD, 1, 3);
				swkbdSetHintText(&swkbd, "Enter VDP Status value");
				button = swkbdInputText(&swkbd, kbdchar, sizeof(kbdchar));
				val = std::atoi(kbdchar);
				VDPStatus[vdpstid] = val;
				break;
			}
			else if(selectmenu== "[Set VDP value]")
			{
				std::vector<char*> vdpstvec;
				vdpstvec.clear();
				int i = 0;
				for (; i < 28; i++)
				{
					std::string str = "VDP[" + std::to_string(i) + "]";
					vdpstvec.push_back(StringToChar(str));
				}
				for (i=32; i < 47; i++)
				{
					std::string str = "VDP[" + std::to_string(i) + "]";
					vdpstvec.push_back(StringToChar(str));
				}
				int vdpstid = BrowseInt("      [Select a VDP Register for edit.]", vdpstvec, 0, 0, false);
				static SwkbdState swkbd;
				SwkbdButton button = SWKBD_BUTTON_NONE;
				static char kbdchar[4];
				static int val;
				swkbdInit(&swkbd, SWKBD_TYPE_NUMPAD, 1, 3);
				swkbdSetHintText(&swkbd, "Enter VDP value");
				button = swkbdInputText(&swkbd, kbdchar, sizeof(kbdchar));
				val = std::atoi(kbdchar);
				VDP[vdpstid] = val;
				break;
			}
			else if (selectmenu == "[Show Slot Info]")
			{
				gfxInitDefault();
				consoleInit(GFX_BOTTOM, _NULL);
				IsInConsole = true;
				printf("PSLReg:"); printf(std::to_string(PSLReg).c_str()); printf("\n");
				for (int i = 0; i < 4; i++)
				{
					printf("SSLReg["); printf(std::to_string(i).c_str()); printf("]: ");
					printf(std::to_string(SSLReg[i]).c_str());printf("\n");
				}
				ExitConsole();
				break;
			}
			else if (selectmenu == "[Set Slot Value]")
			{
			std::vector<char*> slotstvec;
			slotstvec.clear();
			slotstvec.push_back("PSLReg");
			for (int i = 0; i < 4; i++)
			{
				std::string str = "SSLReg[" + std::to_string(i) + "]";
				slotstvec.push_back(StringToChar(str));
			}
			int slotstid = BrowseInt("      [Select a Slot Register for edit.]", slotstvec, 0, 0, false);
			static SwkbdState swkbd;
			SwkbdButton button = SWKBD_BUTTON_NONE;
			static char kbdchar[4];
			static int val;
			swkbdInit(&swkbd, SWKBD_TYPE_NUMPAD, 1, 3);
			swkbdSetHintText(&swkbd, "Enter Slot Register value");
			button = swkbdInputText(&swkbd, kbdchar, sizeof(kbdchar));
			val = std::atoi(kbdchar);
			if (slotstid == 0)PSLReg = val;
			else SSLReg[slotstid - 1] = val;
			break;
			}
			else if (selectmenu == "[Show VRAM]")
			{
				static SwkbdState swkbd;
				SwkbdButton button = SWKBD_BUTTON_NONE;
				static char kbdchar[5];
				static int val;
				swkbdInit(&swkbd, SWKBD_TYPE_QWERTY, 1, 4);
				swkbdSetHintText(&swkbd, "Enter VRAM Address");
				button = swkbdInputText(&swkbd, kbdchar, sizeof(kbdchar));
				val = strtol(kbdchar, NULL, 16);
				std::string str = std::to_string(VRAM[val]);
				BrowseOK(StringToChar(str),NULL);
				break;
			}
			else if(selectmenu == "[Reset PPI]")
			{
			ResetPPI();
			}
			else if (selectmenu == "[Show RAM]")
			{
				static SwkbdState swkbd;
				SwkbdButton button = SWKBD_BUTTON_NONE;
				static char kbdchar[5];
				static int val;
				swkbdInit(&swkbd, SWKBD_TYPE_QWERTY, 1, 4);
				swkbdSetHintText(&swkbd, "Enter RAM Address");
				button = swkbdInputText(&swkbd, kbdchar, sizeof(kbdchar));
				val = strtol(kbdchar, NULL, 16);
				std::string str = std::to_string(RdZ80(val));
				BrowseOK(StringToChar(str), NULL);
				break;
			}
			else if(selectmenu== "[Show PSG Register]")
			{
				gfxInitDefault();
				consoleInit(GFX_BOTTOM, _NULL);
				IsInConsole = true;
				for (int i = 0; i < 16; i++)
				{
					printf("PSG["); printf(std::to_string(i).c_str()); printf("]: ");
					printf(std::to_string(PeekPSGReg(i)).c_str());
					if (i & 1)printf("\n");
					else printf("  ");
				}
				ExitConsole();
				break;
			}
			else if(selectmenu== "[ToggleVkey]")
			{
				ToggleVKey();
				break;
			}
			else if(selectmenu=="[Calc Screen8 Table]")
			{
				static SwkbdState swkbd;
				SwkbdButton button = SWKBD_BUTTON_NONE;
				static char kbdchar_g[2], kbdchar_r[2], kbdchar_b[2];
				static int ir, ig, ib;
				swkbdInit(&swkbd, SWKBD_TYPE_NUMPAD, 1, 1);
				swkbdSetHintText(&swkbd, "Enter R value of color");
				button = swkbdInputText(&swkbd, kbdchar_r, sizeof(kbdchar_r));
				ir = std::atoi(kbdchar_r);
				swkbdInit(&swkbd, SWKBD_TYPE_NUMPAD, 1, 1);
				swkbdSetHintText(&swkbd, "Enter G value of color");
				button = swkbdInputText(&swkbd, kbdchar_g, sizeof(kbdchar_g));
				ig = std::atoi(kbdchar_g);
				swkbdInit(&swkbd, SWKBD_TYPE_NUMPAD, 1, 1);
				swkbdSetHintText(&swkbd, "Enter B value of color");
				button = swkbdInputText(&swkbd, kbdchar_b, sizeof(kbdchar_b));
				ib = std::atoi(kbdchar_b);
				int dbgint = Debug_CalcBPalVlue(ir, ig, ib);
				std::string dbgstr = "Screen Table8 Table[15]: " + std::to_string(dbgint) + "\n";
				BrowseOK((char*)dbgstr.c_str(),NULL);
				return;
			}
#ifdef DEBUG_LOG
			else if (selectmenu=="[Clear Debug Log]")
			{
				fclose(debugFile);
				debugFile = fmemopen(debugBuf, 0x10000, "r+");
				debugCnt = 0;
				return;
			}
#endif	// DEBUG_LOG
			else if(selectmenu=="[Enter Debug mode.]")
			{
				for (;;)
				{
					hidScanInput();
					u32 kDown = hidKeysDown();
					if (kDown & KEY_SELECT)
					{
						IsDebug = IsDebug ? 0 : 1;
						if (IsDebug)
						{
							consoleInit(GFX_BOTTOM, _NULL);
							gfxSetDoubleBuffering(GFX_BOTTOM, false);
							Verbose = 0xFF;
							FDD[0].Verbose = 0xFF;
							verboseFDC();
							//Verbose = 0x08;
							//Verbose = 0x02;
							//Verbose = 31;
							//FDD[0].Verbose = 31;
							//Verbose = 5;
							//FDD[0].Verbose = 31;
							printf("Press start to stop Debug mode.");
							return;
						}
						break;
					}
					if (kDown & KEY_START)break;
				}
				ExitConsole();
				break;
			}
			else if (selectmenu == "[Show Debug Message]")
			{
				std::vector<char*> onoffvec;
				onoffvec.clear();
				onoffvec.push_back("OFF");
				onoffvec.push_back("ON");
				ShowDebugMsg3DS = BrowseInt("      [Show Debug Message]", onoffvec, 0, 0, false);
				return;
			}
			else if (selectmenu == "[Show Sprite Adress]")
			{
				gfxInitDefault();
				consoleInit(GFX_BOTTOM, _NULL);
				IsInConsole = true;
				printf("Sprite Gen:");printf(std::to_string((int)(SprGen - VRAM)).c_str());printf("\n");
				printf("Sprite Tab:"); printf(std::to_string((int)(SprTab - VRAM)).c_str()); printf("\n");
				printf("Sprite TabM:"); printf(std::to_string(SprTabM).c_str()); printf("\n");
				ExitConsole();
				break;
			}
		}
		if (IsLPress)
		{
			GetMovedMenuIndex(selectIndex, startid, -(textColumn - 2), NULL);
			SDL_Delay(TextDelay);
			needRedraw = true;
		}
		if (IsRPress)
		{
			GetMovedMenuIndex(selectIndex, startid, (textColumn - 2), menuItemCount - 1);
			SDL_Delay(TextDelay);
			needRedraw = true;
		}
	}
}
#endif // DEBUG_ENABLE


void ExitConsole()
{
	for (;;)
	{
		hidScanInput();
		u32 kDown = hidKeysDown();
		if (kDown)break;
	}
	C2D_Fini();
	C3D_Fini();
	gfxInitDefault();
	consoleInit(GFX_BOTTOM, 0);
	gfxSetDoubleBuffering(GFX_TOP, false);
	InitCitro();
}


void DrawTextTranslate(const char* text, float x, float y, float z, float scaleX, float scaleY, u32 color)
{
	int langidx = menuLanguage < 0 ? systemLanguage : menuLanguage;
	if (langidx == 0)
	{
		std::string engtext = (std::string)text;
		std::string jptext = TextJPMap[engtext];
		
		if (jptext.size() > 1)
		{
			C2D_TextBufClear(textbuf);
			C2D_TextParse(&dynText, textbuf, jptext.c_str());
			C2D_TextOptimize(&dynText);
			C2D_DrawText(&dynText, C2D_WithColor, x, y, z, scaleX, scaleY, color);
			return;
		}
	}
	C2D_TextBufClear(textbuf);
	C2D_TextParse(&dynText, textbuf, text);
	C2D_TextOptimize(&dynText);
	C2D_DrawText(&dynText, C2D_WithColor, x, y, z, scaleX, scaleY, color);
}

bool DrawTextScroll(const char* text, float x, float y, float z, float scaleX, float scaleY, u32 color)
{
	if (!isScrollText)
	{
		DrawText(text, x, y, z, scaleX, scaleY, color);
		return false;
	}
	C2D_TextBufClear(textbuf);
	C2D_TextParse(&dynText, textbuf, text);
	C2D_TextOptimize(&dynText);
	float owidth, oheight;
	C2D_TextGetDimensions(&dynText, scaleX, scaleY, &owidth, &oheight);
	int waittime = 160;
	if (owidth > 320)
	{
		u32 currTime = SDL_GetTicks() - scrollTime;
		int spos = (int)(std::max((int)currTime - 3000, 0) / 20.0f);
		if (x - spos < -owidth + (320 - waittime))
		{
			scrollTime = SDL_GetTicks();
		}
		C2D_DrawText(&dynText, C2D_WithColor, std::max(320 - owidth - x, x - spos), y, z, scaleX, scaleY, color);
		return true;
	}
	else
	{
		C2D_DrawText(&dynText, C2D_WithColor, x, y, z, scaleX, scaleY, color);
	}
	return false;
}


void DrawText(const char* text, float x, float y, float z, float scaleX, float scaleY, u32 color)
{
	C2D_TextBufClear(textbuf);
	C2D_TextParse(&dynText, textbuf, text);
	C2D_TextOptimize(&dynText);
	C2D_DrawText(&dynText, C2D_WithColor, x, y, z, scaleX, scaleY, color);
}


void DrawFolderIcon(float x, float y, float width, float height)
{
	float xmax = x + width * 0.9f;
	float ymax = y + height * 0.9f;

	C2D_DrawRectSolid(x + width * 0.1f, y + height * 0.1f, 1.0f, width * 0.8f, height * 0.7f, Color_Folder0);
	C2D_DrawRectSolid(x + width * 0.15f, y, 1.0f, width * 0.3f, height * 0.2f, Color_Folder1);
}

void DrawOKButton(const char* okchar)
{
	C2D_DrawRectSolid(220, 240 - OKButtonSize, 1, 100, OKButtonSize, Color_White);
	C2D_DrawEllipseSolid(220 - OKButtonSize, 240 - OKButtonSize, 1, OKButtonSize * 2, OKButtonSize * 2, Color_White);
	DrawTextTranslate(okchar, 250, 240 - OKButtonSize, 1, fontSize, fontSize, Color_Screen);
}


void DrawCancelButton(const char* canchar)
{
	C2D_DrawRectSolid(0, 240 - OKButtonSize, 1, 100, OKButtonSize, Color_White);
	C2D_DrawEllipseSolid(100 - OKButtonSize, 240 - OKButtonSize, 1, OKButtonSize * 2, OKButtonSize * 2, Color_White);
	DrawTextTranslate(canchar, 30, 240 - OKButtonSize, 1, fontSize, fontSize, Color_Screen);
}

void DrawScrollButtonL()
{
	C2D_DrawEllipseSolid(-scrollButtonSize * 3, 120 - scrollButtonSize * 2, 1, scrollButtonSize * 4, scrollButtonSize * 4, Color_White);
	C2D_DrawRectSolid(10, 118, 1, 10, 5, Color_Screen);
	C2D_DrawTriangle(0, 120, Color_Screen, 10, 115, Color_Screen, 10, 125, Color_Screen,1);
}


void DrawScrollButtonR()
{
	C2D_DrawEllipseSolid(320 - scrollButtonSize, 120 - scrollButtonSize * 2, 1, scrollButtonSize * 4, scrollButtonSize * 4, Color_White);
	C2D_DrawRectSolid(300, 118, 1, 10, 5, Color_Screen);
	C2D_DrawTriangle(320, 120, Color_Screen, 310, 115, Color_Screen, 310, 125, Color_Screen,1);
}


void DrawSoldBottom(int xmin, int xmax, int ymin, int ymax, u8 r, u8 g, u8 b)
{
	fbbottom = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL);
	int swidth = 320, sheight = 240;
	for (int i = xmin; i < xmax; i++)
	{
		for (int j = ymin; j < ymax; j++)
		{
			u32 v0 = ((sheight - 1) - j) * 3 + (i * (sheight * 3));
			fbbottom[v0] = r;
			fbbottom[v0 + 1] = g;
			fbbottom[v0 + 2] = b;
		}
	}
}


void DrawSurfaceRectBottom(SDL_Surface* surface, int xmin, int xmax, int ymin, int ymax)
{
	if (surface == NULL)return;
	if (IsDebug)return;
	fbbottom = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL);
	unsigned char* buffer = (unsigned char*)surface->pixels;
	int width = 320, height = 240;
	int r, g, b;
	int bpp = surface->format->BytesPerPixel;
	int swidth = 320, sheight = 240;
	for (int i = xmin; i < xmax; i++)
	{
		if (i > surface->w)break;
		for (int j = ymin; j < ymax; j++)
		{
			if (j > surface->h)break;
			int di = (i * width) / swidth;
			int dj = (j * height) / sheight;
			u32 v0 = ((sheight - 1) - j) * 3 + (i * (sheight * 3));
			int v1 = ((dj * width) + di) * bpp;
			r = buffer[v1];
			g = buffer[v1 + 1];
			b = buffer[v1 + 2];
			fbbottom[v0] = r;
			fbbottom[v0 + 1] = g;
			fbbottom[v0 + 2] = b;
		}
	}
}


void DrawSurfaceLockBottom(SDL_Surface* surface, int xmin, int xmax, int ymin, int ymax)
{
	if (surface == NULL)return;
	if (IsDebug)return;
	fbbottom = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL);
	unsigned char* buffer = (unsigned char*)surface->pixels;
	int height = surface->h, width = surface->w;
	int r, g, b;
	int bpp = surface->format->BytesPerPixel;
	int swidth = 320, sheight = 240;
	for (int i = xmin; i < xmax; i++)
	{
		for (int j = ymin; j < ymax; j++)
		{
			int di = (i * width) / swidth;
			int dj = (j * height) / sheight;
			u32 v0 = ((sheight - 1) - j) * 3 + (i * (sheight * 3));
			int v1 = ((dj * width) + di) * bpp;
			b = buffer[v1];
			g = buffer[v1 + 1];
			r = buffer[v1 + 2];
			fbbottom[v0] = b;
			fbbottom[v0 + 1] = g / 2;
			fbbottom[v0 + 2] = r / 2;
		}
	}
}


void DrawKeyboardScr(bool kana, bool caps, bool shift, bool graph, bool ctrl)
{
	if (graph == true)
	{
		//if (jp)
		if(IsJpKey)
		{
			DrawSurfaceRectBottom(KeySurfaceGra, 0, 320, 0, 106);
			DrawSurfaceRectBottom(KeySurface, 0, 320, 106, 240);
		}
		else
		{
			if (shift)DrawSurfaceRectBottom(KetySurfaceGraShift, 0, 320, 0, 106);
			else DrawSurfaceRectBottom(KeySurfaceGraEUR, 0, 320, 0, 106);
			DrawSurfaceRectBottom(KeySurface, 0, 320, 106, 240);
		}
	}
	else
	{
		if (IsJpKey)DrawSurfaceRectBottom(KeySurface, 0, 320, 0, 240);
		else DrawSurfaceRectBottom(KeySurfaceEUR, 0, 320, 0, 240);
	}
	if (kana == true)DrawkeyboardLock(231, 252, 104, 125);
	if (caps == true)DrawkeyboardLock(2, 45, 104, 126);
	if (shift == true)
	{
		DrawkeyboardLock(2, 45, 83, 104);
		DrawkeyboardLock(280, 320, 83, 104);
	}
	if (graph == true)DrawkeyboardLock(45, 67, 104, 126);
	if (ctrl == true)DrawkeyboardLock(2, 38, 62, 83);
}


void DrawScreenShotScr()
{
	//DrawSoldBottom(0, 320, 0, 240, 71, 71, 71);
	if (IsSmallScrShot)
	{
		DrawSurfaceBlitzBottom(ScreenShotSurface, false, 0, 120, ScreenShotSurface->w, ScreenShotSurface->h, ScreenShotOffx, ScreenShotOffy+50);
		DrawSurfaceRectBottom(KeySurface, 0, 320, 0, 120);
		DrawSurfaceRectBottom(KeySurface, 270, 320, 120, 240);
	}
	else
	{
		DrawSurfaceBlitzBottom(ScreenShotSurface, false, 0, 0, ScreenShotSurface->w, ScreenShotSurface->h, ScreenShotOffx, ScreenShotOffy);
		DrawSurfaceRectBottom(KeySurface, 280, 320, 151, 240);
	}
}


void MoveScreenShot(int px, int py)
{
	if (ScreenShotSurface->w > 320)
	{
		ScreenShotOffx += (oldtp.px - px);
		ScreenShotOffx = ScreenShotOffx < 0 ? 0 : ScreenShotOffx>ScreenShotSurface->w - 320 ? ScreenShotSurface->w - 320 : ScreenShotOffx;
	}
	if ((ScreenShotSurface->h > 240) || IsSmallScrShot)
	{
		ScreenShotOffy += (oldtp.py - py);

		ScreenShotOffy = ScreenShotOffy < (IsSmallScrShot ? -60 : 0) ? (IsSmallScrShot ? -60 : 0)
			: ScreenShotOffy>ScreenShotSurface->h - (IsSmallScrShot ? 180 : 240) ? ScreenShotSurface->h - (IsSmallScrShot ? 180 : 240) : ScreenShotOffy;
	}
}


void DrawkeyboardLock(int xmin, int xmax, int ymin, int ymax)
{
	if(IsJpKey)DrawSurfaceLockBottom(KeySurface, xmin, xmax, ymin, ymax);
	else DrawSurfaceLockBottom(KeySurfaceEUR, xmin, xmax, ymin, ymax);
}


void DrawMouseScr()
{
	DrawSoldBottom(0, 320, 0, 240, 71, 71, 71);
	DrawSurfaceRectBottom(KeySurface, 280, 320, 151, 240);
}


void DrawSurfaceBlitzBottom(SDL_Surface* surface, bool IsDefault, int xmin, int ymin, int width, int height, int dxmin, int dymin)
{
	if (surface == NULL)return;
	fbbottom = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL);
	unsigned char* buffer = (unsigned char*)surface->pixels;
	int dwidth = IsDefault ? 320 : surface->w, dheight = IsDefault ? 240 : surface->h;
	int r, g, b;
	int bpp = surface->format->BytesPerPixel;
	int swidth = 320, sheight = 240;
	//int xmax = xmin + width, ymax = ymin + height;
	int xmax = std::min(xmin + width, 320), ymax = std::min(ymin + height,240);
	int offsetx = dxmin - xmin, offsety = dymin - ymin;
	for (int i = xmin; i < xmax; i++)
	{
		for (int j = ymin; j < ymax; j++)
		{
			u32 v0 = ((sheight - 1) - j) * 3 + (i * (sheight * 3));
			if (j + offsety > dheight)continue;
			if (i + offsetx > dwidth)continue;
			int v1 = (((j + offsety) * dwidth) + (i + offsetx)) * bpp;
			r = buffer[v1];
			g = buffer[v1 + 1];
			b = buffer[v1 + 2];
			fbbottom[v0] = r;
			fbbottom[v0 + 1] = g;
			fbbottom[v0 + 2] = b;
		}
	}
}


void DrawDiskLamp()
{
	if (IsScreenShot)return;
	if (TurboNow)return;
	if ((currJoyMode[0] >= HIDE_KEYBOARD) || (currJoyMode[1]>=HIDE_KEYBOARD))return;
	if (DiskAccess[0]>0)
	{
		DiskAccess[0] --;
		DrawSoldBottom(95, 120, 152, 156, 0, 255, 0);
		IsDrawDiskLamp[0] = 1;
	}
	else if(DiskAccess[0]==0 && IsDrawDiskLamp[0]==1)
	{
		DrawSurfaceRectBottom(KeySurface, 95, 120, 152, 156);
		IsDrawDiskLamp[0] = 0;
	}
	if (DiskAccess[1]>0)
	{
		DiskAccess[1] --;
		DrawSoldBottom(132, 160, 152, 156, 0, 255, 0);
		IsDrawDiskLamp[1] = 1;
	}
	else if (DiskAccess[1]==0 && IsDrawDiskLamp[1]==1)
	{
		DrawSurfaceRectBottom(KeySurface, 132, 160, 152, 156);
		IsDrawDiskLamp[1] = 0;
	}
}


SDL_Surface* SDLSurfaceExtract(SDL_Surface* surface, int xmin, int ymin, int width, int height)
{
	SDL_Surface* newSurface = SDL_CreateRGBSurface(SDL_HWSURFACE, width, height, 24, 0xff000000, 0x00ff0000, 0x0000ff00, 0x00000000);
	SDL_Rect sdlrect;
	sdlrect.x = xmin;
	sdlrect.y = ymin;
	sdlrect.w = width;
	sdlrect.h = height;
	SDL_Rect newrect;
	newrect.x = 0;
	newrect.y = 0;
	newrect.w = width;
	newrect.h = height;
	SDL_BlitSurface(surface, &sdlrect, newSurface, &newrect);
	return newSurface;
}


/* Based on blagSNES.(CopyBitmapToTexture() in main.c). */
void SDLSurfaceToC3DTexData(SDL_Surface* surface, void* data, int swidth, int sheight)
{
	int dwidth = surface->w, dheight = surface->h;
	int i, j, v0;
	byte c0, c1, c2, c3;
	u32* scrbuf = (u32*)data;
	u8* srcbuf = (u8*)surface->pixels;

	for (i = 0; i <dheight; i++)
	{
		for (j = 0; j < dwidth; j++)
		{
			u8 b = *srcbuf++;
			u8 g = *srcbuf++;
			u8 r = *srcbuf++;

			v0 = ((j&0x1F8)<<3) + ((i&0xF8)<<3)*64 + (j & 0x01) + ((i & 0x01) << 1) + ((j & 0x02) << 1) + ((i & 0x02) << 2) + ((j & 0x04) << 2)
			 + ((i&0x04)<<3);

			u32 px = (r << 24) | (g << 16) | (b << 8) | 0xFF;
			scrbuf[v0] = px;
		}
	}
}


void DrawHUD()
{
	C2D_DrawImageAt(HUDImage, 280.0f, 150.0f, 1.0f, NULL, 1.0f, 1.0f);
}


void AddRecentlyList(std::string str, const char* ftype)
{
	RemoveRecentlyList(str);
	recentlylist.push_back(str);
	recentftypelist.push_back(ftype);
	if (recentlylist.size() > 30)
	{
		recentlylist.erase(recentlylist.begin());
		recentftypelist.erase(recentftypelist.begin());
	}
	latestPath = str;
}


void RemoveRecentlyList(std::string str)
{
	std::vector<std::string> templist;
	templist.clear();
	std::string str1 = str;
	str1.erase(str1.find_last_of("."));
	std::vector<std::string> tempftypelist;
	tempftypelist.clear();
	for (int i = 0; i < recentlylist.size(); i++)
	{
		std::string str0 = recentlylist[i];
		str0.erase(str0.find_last_of("."));
		if (str0 != str1)
		{
			templist.push_back(recentlylist[i]);
			tempftypelist.push_back(recentftypelist[i]);
		}
	}
	recentlylist = templist;
	recentftypelist = tempftypelist;
}


void WriteOptionCFG()
{
	std::string pathstr = "/FMSX3DS";
	pathstr += "/fMSX3DS.cfg";
	FILE* fp = fopen(pathstr.c_str(), "w");
	if (!fp)return;
	fprintf(fp, "#StartOption");
	fprintf(fp, "\n");
	for (int i = 1; i < optionItem.size() - 1; i++)
	{
		if (optionItem[i].name.find_first_of("<") == 0)continue;
		if (optionItem[i].name.size() < 2)continue;
		fprintf(fp, optionItem[i].name.c_str());
		fprintf(fp, "=");
		fprintf(fp, std::to_string(optionItem[i].currentIdx).c_str());
		fprintf(fp, "\n");
	}
	fprintf(fp, "#End");
	fprintf(fp, "\n");
	fprintf(fp, "#StartKeyConfig");
	fprintf(fp, "\n");
	for (int i = 1; i < keyconfItem.size() - 1; i++)
	{
		fprintf(fp, keyconfItem[i].name.c_str());
		fprintf(fp, "=");
		fprintf(fp, std::to_string(keyconfItem[i].currentIdx).c_str());
		fprintf(fp, "\n");
	}
	fprintf(fp, "#End");
	fprintf(fp, "\n");
	fprintf(fp, "#StartRapidFire");
	fprintf(fp, "\n");
	for (int i = 0; i < 10; i++)
	{
		fprintf(fp, RapidMenuVec[i + 1].c_str());
		fprintf(fp, "=");
		fprintf(fp, std::to_string(rapidInterVals[i]).c_str());
		fprintf(fp, "\n");
	}
	fprintf(fp, "#End");
	fprintf(fp, "\n");
	if (recentlylist.size() != recentftypelist.size())
	{
		fclose(fp);
		return;
	}
	fprintf(fp, "#StartMostRecentlyUsed");
	fprintf(fp, "\n");
	for (int i = 0; i < recentlylist.size(); i++)
	{
		fprintf(fp, recentlylist[i].c_str());
		fprintf(fp, ",");
		fprintf(fp, recentftypelist[i].c_str());
		fprintf(fp, "\n");
	}
	fprintf(fp, "#End");
	fprintf(fp, "\n");
	fclose(fp);
}


void ReadOptionCFG()
{
	std::string pathstr = "/FMSX3DS";
	pathstr += "/fMSX3DS.cfg";
	FILE* fp;
	fp = fopen(pathstr.c_str(), "r");
	if (fp == NULL)return;
	char buf[256] = "";
	bool IsReadrecently = false, IsReadOption = false, IsReadKeys = false, IsReadRapid = false;
	while (fgets(buf, 256, fp) != NULL)
	{
		std::string bufstr = (std::string)buf;
		if (bufstr.size() < 2)continue;
		bufstr[strlen(bufstr.c_str()) - 1] = '\0';
		if (bufstr[0] == '#')
		{
			if (bufstr.find("#StartMostRecentlyUsed") != std::string::npos)IsReadrecently = true;
			else if (bufstr.find("#StartOption") != std::string::npos)IsReadOption = true;
			else if (bufstr.find("#StartKeyConfig") != std::string::npos)IsReadKeys = true;
			else if (bufstr.find("StartRapidFire") != std::string::npos)IsReadRapid = true;
			else if (bufstr.find("#End") != std::string::npos)
			{
				IsReadrecently = false;
				IsReadOption = false;
				IsReadKeys = false;
				IsReadRapid = false;
			}
			continue;
		}
		if (IsReadrecently && bufstr.size() > 1)
		{
			std::string filestr = std::string(bufstr);
			int strsc = filestr.find_last_of(",");
			if (strsc != std::string::npos)
			{
				filestr.erase(filestr.begin() + strsc, filestr.end());
				std::string typestr = std::string(bufstr);
				typestr.erase(typestr.begin(), typestr.begin() + (strsc + 1));

				recentlylist.push_back(filestr);
				recentftypelist.push_back(typestr);
			}
		}
		else if (IsReadOption && bufstr.size() > 1)
		{
			int flast = bufstr.find_last_of("=");
			if (flast < 1)continue;
			std::string tempstr = bufstr;
			int idx0 = optionMap[tempstr.erase(flast)];
			std::string valstr = &bufstr[flast + 1];
			int valint = atoi(valstr.c_str());
			if (valint >= 0 && valint < optionItem[idx0].menuChar.size())optionItem[idx0].currentIdx = valint;
		}
		else if (IsReadKeys && bufstr.size() > 1)
		{
			for (int i = 0; i < keyconfItem.size(); i++)
			{
				if (bufstr.find(keyconfItem[i].name) == 0)
				{
					std::string valstr = &bufstr[bufstr.find_last_of("=") + 1];
					int valint = atoi(valstr.c_str());
					if (valint >= 0)keyconfItem[i].currentIdx = valint;
				}
			}
		}
		else if (IsReadRapid && bufstr.size() > 1)
		{
			for (int i = 0; i < 10; i++)
			{
				if (bufstr.find(RapidMenuVec[i + 1]) == 0)
				{
					std::string valstr = &bufstr[bufstr.find_last_of("=") + 1];
					int valint = atoi(valstr.c_str());
					if (valint >= 0)rapidInterVals[i] = valint;
				}
			}
		}
	}
}

#ifdef LOG_ERROR
void WriteErrorLog()
{
	if (ErrorVec.size() < 1)return;
	std::string pathstr = "/FMSX3DS";
	pathstr += "/Errorlog.txt";
	FILE* fp = fopen(pathstr.c_str(), "w");
	if (!fp)return;
	fprintf(fp, "#StartErrorMessage");
	fprintf(fp, "\n");
	for (int i = 0; i < ErrorVec.size(); i++)
	{
		fprintf(fp, ErrorVec[i]);
		fprintf(fp, "\n");
	}
	fprintf(fp, "#End");
	fprintf(fp, "\n");
	fclose(fp);
}
#endif // LOG_ERROR


#ifdef DEBUG_LOG
void printfToFile(const char* format, ...)
{
	if (debugCnt > MAXDEBUG)return;
	debugCnt++;
	va_list args;
	va_start(args, format);
	vfprintf(debugFile, format, args);
	va_end(args);
}


void writeDebugLog()
{
	char buf[256] = "";
	FILE* fp = fopen("/FMSX3DS/DebugLog.txt", "w");
	if (!fp)return;
	rewind(debugFile);
	while (fgets(buf, 256, debugFile) != NULL)
	{
		std::string bufstr = (std::string)buf;
		if (bufstr.size() < 2)continue;
		bufstr[strlen(bufstr.c_str()) - 1] = '\0';
		fprintf(fp, bufstr.c_str());
		fprintf(fp, "\n");
	}
	fclose(fp);
	free(debugBuf);
}
#endif // DEBUG_LOG



void LoadOption(bool IsInit)
{
	//bool resetMsg = false;
	unsigned char messageType = 0;
	if (IsInit)
	{
		NewMSX_Mode = Mode & MSX_MODEL;
		NewPSGType = PSGType;
		NewSoundChs = SoundChannels;
		NewRAMSize = 0;
		int ramsz = 4;
		while (RAMPages>ramsz)
		{
			ramsz <<= 1;
			NewRAMSize++;
		}
	}
	IsShowFPS = optionItem[optionMap["ShowFPS"]].currentIdx;
	if (NewMSX_Mode != optionItem[optionMap["MSX Model"]].currentIdx)
	{
		NewMSX_Mode = optionItem[optionMap["MSX Model"]].currentIdx;
		messageType = 2;
	}
	if (NewRAMSize != optionItem[optionMap["RAM Size"]].currentIdx)
	{
		NewRAMSize = optionItem[optionMap["RAM Size"]].currentIdx;
		messageType = 1;
	}
#ifdef VDP_V9990
	UseV9990 = optionItem[optionMap["Use V9990"]].currentIdx;
#endif // VDP_V9990

	if (IsWide != optionItem[optionMap["800px wide mode"]].currentIdx)
	{
		if (optionItem[optionMap["800px wide mode"]].currentIdx == 1)
		{
			AllowWide = true;
#ifdef VDP_V9990
			if (V9990Active)
			{
				if ((V9KScrMode == 1) || (V9KScrMode > 3))SetupWideScreen(true);
				else SetupWideScreen(false);
			}
			else
#endif // VDP_V9990
			if ((ScrMode == 6) || ((ScrMode == 7) && !ModeYJK) || (ScrMode == MAXSCREEN + 1))
			{
				SetupWideScreen(true);
			}
			else
			{
				SetupWideScreen(false);
			}
		}
		else
		{
			AllowWide = false;
			SetupWideScreen(false);
		}
	}
#ifdef USE_3D
	if (Stereo3DMode != optionItem[optionMap["3D Stereoscopic mode"]].currentIdx)
	{
		Stereo3DMode = optionItem[optionMap["3D Stereoscopic mode"]].currentIdx;
		if (!Stereo3DMode)OldIs3DNow = 0;
	}
#endif // USE_3D
#ifdef _MSX0
	if (UseMSX0 != optionItem[optionMap["Use MSX0"]].currentIdx)
	{
		if (!UseMSX0)
		{
			PatchGETPAD();
			NeedRest = 0;
			LoadCart("fakeiot.rom", 3, MAP_GUESS);
			if (!LoadCart("MSX0_XBASIC.ROM", 1, MAP_GUESS))LoadCart("XBASIC2.rom", 1, MAP_GUESS);
			NeedRest = 1;
			messageType = 1;
		}
		else
		{
			if ((JoyMode[0] != JOY_TOUCHPAD) && (JoyMode[1] != JOY_TOUCHPAD))UnPatchGETPAD();
			NeedRest = 0;
			LoadCart(0, 3, 0);
			if (LoadXBASIC)LoadCart(0, 1, 0);
			NeedRest = 1;
			messageType = 1;
		}
		UseMSX0 = optionItem[optionMap["Use MSX0"]].currentIdx;
	}
	if (LoadXBASIC!=optionItem[optionMap["Load X-BASIC2"]].currentIdx)
	{
		if (UseMSX0)
		{
			messageType = 1;
			if (!LoadXBASIC)
			{
				NeedRest = 0;
				if (!LoadCart("MSX0_XBASIC.ROM", 1, MAP_GUESS))LoadCart("XBASIC2.rom", 1, MAP_GUESS);
				NeedRest = 1;
			}
			else
			{
				NeedRest = 0;
				LoadCart(0, 1, 0);
				NeedRest = 1;
			}
		}
		LoadXBASIC = optionItem[optionMap["Load X-BASIC2"]].currentIdx;
	}
	if (MSX0_I2CA != optionItem[optionMap["MSX0 Device A(i2c_a)"]].currentIdx)
	{
		MSX0_I2CA = optionItem[optionMap["MSX0 Device A(i2c_a)"]].currentIdx;
	}
#endif // _MSX0
	AutoFrameSkip = optionItem[optionMap["Auto Frame Skip"]].currentIdx;
	ScreenRes = optionItem[optionMap["Screen Strech"]].currentIdx;
	if (ScreenFilter != optionItem[optionMap["Screen Filter"]].currentIdx)
	{
		ScreenFilter = optionItem[optionMap["Screen Filter"]].currentIdx;
		SetScreenFilter();
	}
	AccurateAudioSync = optionItem[optionMap["Accurate AudioSync"]].currentIdx;
	waitStep = optionItem[optionMap["AudioSync Quality"]].currentIdx;
	if (optionItem[optionMap["New3DS Boost"]].currentIdx == 1)osSetSpeedupEnable(0);
	else osSetSpeedupEnable(1);
	if(IsDoubleBuffer != optionItem[optionMap["Use Double Buffer"]].currentIdx)
	{
		IsDoubleBuffer = optionItem[optionMap["Use Double Buffer"]].currentIdx;
		gfxSetDoubleBuffering(GFX_TOP, IsDoubleBuffer);
	}
	UseInterlace = optionItem[optionMap["Use Interlace"]].currentIdx;
	IsStartLoadFile = optionItem[optionMap["Load  a file when Startup"]].currentIdx;
	menuLanguage = optionItem[optionMap["Menu Language"]].currentIdx-1;
	isScrollText = optionItem[optionMap["Scroll Text"]].currentIdx;
#ifdef MEMORY_CURSOR_POS
	MemorySysMenupos = optionItem[optionMap["Memory systemmenu cursor"]].currentIdx;
#endif // MEMORY_CURSOR_POS
	TextDelay = 600/(optionItem[optionMap["Menu select speed"]].currentIdx + 1);
	IsFrameLimit = optionItem[optionMap["Frame Limit"]].currentIdx;
	if (Use2413 != optionItem[optionMap["Use FM Sound(Ym2413)"]].currentIdx)
	{
		Use2413 = optionItem[optionMap["Use FM Sound(Ym2413)"]].currentIdx;
		OPLLChangeOption(Use2413);
		PatchFMPAC(0,2);
		DoReloadFMPAC();
		messageType = 2;
	}
	if (Use8950 != optionItem[optionMap["Use FM Sound(Y8950)"]].currentIdx)
	{
		Use8950 = optionItem[optionMap["Use FM Sound(Y8950)"]].currentIdx;
		OPLChangeOption(Use8950);
		messageType = messageType > 1 ? messageType : 1;
	}
	if (Use8950TurboR != optionItem[optionMap["Use Y8950 on MSXTurboR"]].currentIdx)
	{
		Use8950TurboR = optionItem[optionMap["Use Y8950 on MSXTurboR"]].currentIdx;
		messageType = messageType > 1 ? messageType : 1;
	}
	if (ReadSCCPlus != optionItem[optionMap["Read SCC Plus Wave"]].currentIdx)
	{
		ReadSCCPlus = optionItem[optionMap["Read SCC Plus Wave"]].currentIdx;
		if (GetROMType(0) == MAP_SCCPLUS_2)
		{
			if (ReadSCCPlus)CartSpecial[0] = CART_READSCC;
			else if (CartSpecial[0] == CART_READSCC)CartSpecial[0] = 0;
			messageType = messageType > 1 ? messageType : 1;
		}
	}
	if (NewPSGType != optionItem[optionMap["PSG Chip Type"]].currentIdx)
	{
		NewPSGType = optionItem[optionMap["PSG Chip Type"]].currentIdx;
		messageType = 2;
	}
	if (NewSoundChs != optionItem[optionMap["Sound Type"]].currentIdx)
	{
		NewSoundChs = optionItem[optionMap["Sound Type"]].currentIdx;
		messageType = 2;
	}
	MasterVol = optionItem[optionMap["Sound Volume"]].currentIdx*0.1f;
	FactorPSG = optionItem[optionMap["PSG Volume"]].currentIdx * MasterVol *0.15f;
	FactorSCC = optionItem[optionMap["SCC Volume"]].currentIdx * MasterVol*0.15f;
	/* YM2413 and Y8950 is low because it increase in AudioCallback() in Snd3DS.cpp. This reduce noise in loud volume. */
	Factor2413 = optionItem[optionMap["YM2413 Volume"]].currentIdx * MasterVol*0.135f;
	Factor8950 = optionItem[optionMap["Y8950 Volume"]].currentIdx * MasterVol * 0.11f;
	//Factor8950 = optionItem[optionMap["Y8950 Volume"]].currentIdx * MasterVol*0.135f;
	FactorPCM = optionItem[optionMap["PCM Volume"]].currentIdx * MasterVol*0.2f;
	//FactorPSG = optionItem[optionMap["PSG Volume"]].currentIdx * 0.15;
	//FactorSCC = optionItem[optionMap["SCC Volume"]].currentIdx * 0.15;
	//Factor2413 = optionItem[optionMap["YM2413 Volume"]].currentIdx * 0.2;
	//Factor8950 = optionItem[optionMap["Y8950 Volume"]].currentIdx * 0.2;
	//FactorPCM = optionItem[optionMap["PCM Volume"]].currentIdx * 0.2;
	ForceSCCP = optionItem[optionMap["Force SCC Plus"]].currentIdx;
	if (IsPSGHQ != optionItem[optionMap["PSG Quality"]].currentIdx)
	{
		IsPSGHQ = optionItem[optionMap["PSG Quality"]].currentIdx;
		messageType = 2;
	}
	if (IsSCCHQ != optionItem[optionMap["SCC Qualty"]].currentIdx)
	{
		IsSCCHQ = optionItem[optionMap["SCC Qualty"]].currentIdx;
		messageType = 2;
	}
	if (Is2413HQ != optionItem[optionMap["YM2413 Quality"]].currentIdx)
	{
		Is2413HQ = optionItem[optionMap["YM2413 Quality"]].currentIdx;
		messageType = 2;
	}
	if (UseDCFilter != optionItem[optionMap["Use DC Filter"]].currentIdx)
	{
		UseDCFilter = optionItem[optionMap["Use DC Filter"]].currentIdx;
		DCFilterChangeOption(UseDCFilter);
	}
	if (UseRCFilter != optionItem[optionMap["Use RC Filter"]].currentIdx)
	{
		UseRCFilter = optionItem[optionMap["Use RC Filter"]].currentIdx;
		RCFilterChangeOption(UseRCFilter);
	}
	if (UseFIRFilter != optionItem[optionMap["Use FIR Filter"]].currentIdx)
	{
		UseFIRFilter = optionItem[optionMap["Use FIR Filter"]].currentIdx;
		FIRFilterChangeOption(UseFIRFilter);
	}
	if (ForceJPBIOS != optionItem[optionMap["Force Japanese BIOS"]].currentIdx)
	{
		ForceJPBIOS = optionItem[optionMap["Force Japanese BIOS"]].currentIdx;
		ReloadBIOS = 1;
		messageType = messageType > 1 ? messageType : 1;
	}
	if (cbiosReg != optionItem[optionMap["C-BIOS Region"]].currentIdx)
	{
		cbiosReg = optionItem[optionMap["C-BIOS Region"]].currentIdx;
		ReloadBIOS = 1;
		messageType = messageType>1 ? messageType : 1;
	}
	if (ForceCBIOS != optionItem[optionMap["Force C-BIOS"]].currentIdx)
	{
		ForceCBIOS = optionItem[optionMap["Force C-BIOS"]].currentIdx;
		ReloadBIOS = 1;
	}
	if (regionid != optionItem[optionMap["Machine Region"]].currentIdx)
	{
		regionid = optionItem[optionMap["Machine Region"]].currentIdx;
		if(regionid==1)Mode = (Mode & ~MSX_VIDEO) & ~MSX_PAL;
		else if(regionid==2)Mode = (Mode & ~MSX_VIDEO) | MSX_PAL;
		messageType = messageType > 1 ? messageType : 1;
	}
	if (KeyRegion != optionItem[optionMap["Keyboad Region"]].currentIdx)
	{
		KeyRegion = optionItem[optionMap["Keyboad Region"]].currentIdx;
		IsJpKey = KeyRegion ? 1 - (KeyRegion - 1) : IsJPKeyBIOS;
	}
	if (SkipBIOS != optionItem[optionMap["Skip MSX2 Plus boot screen"]].currentIdx)
	{
		SkipBIOS = optionItem[optionMap["Skip MSX2 Plus boot screen"]].currentIdx;
		IsHardReset = 1;
		messageType = messageType > 1 ? messageType : 1;
	}
	UPeriod = (optionItem[optionMap["Frame Skip"]].currentIdx + 1) *25;
	//if (NewMSXDOS != optionItem[optionMap["Load MSXDOS2"]].currentIdx)
	//{
	//	NewMSXDOS = optionItem[optionMap["Load MSXDOS2"]].currentIdx;
	//	messageType = 2;
	//}
	if (JoyMode[0] != optionItem[optionMap["JoyPort A"]].currentIdx)
	{
		DoPatchGetPAD(JoyMode[0], optionItem[optionMap["JoyPort A"]].currentIdx,0);
		JoyMode[0] =currJoyMode[0] = optionItem[optionMap["JoyPort A"]].currentIdx;
#ifdef _MSX0
		SETJOYTYPE(0, JoyMode[0] > HIDE_KEYBOARD + 1 ? HIDE_KEYBOARD : JoyMode[0]);
#else
		SETJOYTYPE(0, JoyMode[0]>HIDE_KEYBOARD+1 ? HIDE_KEYBOARD+1 :JoyMode[0]);
#endif // _MSX0
	}
	if (JoyMode[1] != optionItem[optionMap["JoyPort B"]].currentIdx)
	{
		DoPatchGetPAD(JoyMode[1], optionItem[optionMap["JoyPort B"]].currentIdx,1);
		JoyMode[1] = currJoyMode[1] = optionItem[optionMap["JoyPort B"]].currentIdx;
#ifdef _MSX0
		SETJOYTYPE(1, JoyMode[1] > HIDE_KEYBOARD + 1 ? HIDE_KEYBOARD : JoyMode[1]);
#else
		SETJOYTYPE(1, JoyMode[1] >HIDE_KEYBOARD+1 ? HIDE_KEYBOARD+1 :JoyMode[1]);
#endif // _MSX0
	}
	if (PrinterMode != optionItem[optionMap["Printer Port"]].currentIdx)
	{
		PrinterMode = optionItem[optionMap["Printer Port"]].currentIdx;
		if (PrinterMode == PRINTER_PRINT2FILE)ChangePrinter("/FMSX3DS/printer.txt");
		messageType = messageType > 1 ? messageType : 1;
	}
	if (IsInit)
	{
		Mode = (Mode & ~MSX_MODEL) | NewMSX_Mode;
		//if(NewMSXDOS)Mode |= MSX_MSXDOS2;
		//else Mode &= ~MSX_MSXDOS2;
		RAMPages = 4 << NewRAMSize;
		PSGType = NewPSGType;
		SoundChannels = NewSoundChs;
	}
	else
	{
		switch (messageType)
		{
		case 1:
			DrawMessage("This option requires reset the MSX.", "Select [Reset] in the menu to reset.", 10, 50, 1000, true);
			break;
		case 2:
			DrawMessage("This option requires Restarting the emulator.", "Quit fMSX3DS and restart to enable that.", 10, 50, 1000, true);
			break;
		default:
			break;
		}
	}
}


void ChangeOptionMSXModel(const char* optionName, int val)
{
	optionItem[optionMap["MSX Model"]].currentIdx = val;
}


void DoPatchGetPAD(int oldPort, int newPort, byte PortNum)
{
	bool isOldGetPAD = false;
	if (oldPort == JOY_TOUCHPAD)isOldGetPAD = true;
	bool isNewGetPAD = false;
	if (newPort == JOY_TOUCHPAD)isNewGetPAD = true;
	if ((isOldGetPAD==false) && (isNewGetPAD==true))PatchGETPAD();
	if ((isOldGetPAD == true) && (isNewGetPAD == false) && (!UseMSX0))UnPatchGETPAD();
}


const char* GetFileExtension(std::string filestr)
{
	size_t retsize = filestr.find_last_of(".");
	std::string retstr;
	const char char0 = filestr[retsize];
	const char char1 = filestr[retsize + 1];
	const char char2 = filestr[retsize + 2];
	const char char3 = filestr[retsize + 3];
	retstr = retstr + char0 + char1 + char2 + char3;
	return retstr.c_str();
}

char* StringToChar(std::string str)
{
	char* tempchar = new char[str.size() + 1];
	str.copy(tempchar, str.size() + 1);
	tempchar[str.size()] = '\0';
	return tempchar;
}


void Debug_BreakPoint3DS(const char* message)
{
	textbuf = C2D_TextBufNew(4096);
	C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
	C2D_TargetClear(BottomRenderTartget, Color_Screen);
	C2D_SceneBegin(BottomRenderTartget);
	DrawTextTranslate(message, 120, 0, 1.0f, fontSize, fontSize, Color_White);
	C3D_FrameEnd(0);
	//for (;;)
	while (aptMainLoop())
	{
		SDL_Delay(10);
		hidScanInput();
		u32 kDown = hidKeysDown();
		if (kDown & KEY_START)break;
	}
	if ((currJoyMode[0] >= HIDE_KEYBOARD) || currJoyMode[1] >= HIDE_KEYBOARD)
	{
		if(IsJpKey)DrawSurfaceRectBottom(KeySurface, 0, 320, 0, 240);
		else DrawSurfaceRectBottom(KeySurfaceEUR, 0, 320, 0, 240);
	}
	else DrawMouseScr();
}