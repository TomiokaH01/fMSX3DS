#include <stdio.h>
#include <string.h>
//#include <string>
#include <iostream>
#include <vector>

extern "C"
{
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
}

#define BROWSE_ROM		1
#define BROWSE_DISK		2
#define BROWSE_TAPE		4
#define BROWSE_CHEAT	8
#define BROWSE_PATCH	0x10
#define BROWSE_START	0x20
#define BROWSE_IMG		0x40
#define BROWSE_DER		0x100
#if defined(HDD_NEXTOR) || defined(HDD_IDE) || defined(MEGASCSI_HD)
#define BROWSE_HDD		0x80
#define BROWSE_ALL		(BROWSE_ROM | BROWSE_DISK | BROWSE_TAPE | BROWSE_CHEAT | BROWSE_PATCH | BROWSE_IMG | BROWSE_DER | BROWSE_HDD)
#else
#define BROWSE_ALL		(BROWSE_ROM | BROWSE_DISK | BROWSE_TAPE | BROWSE_CHEAT | BROWSE_PATCH | BROWSE_IMG | BROWSE_DER)
#endif //HDD_NEXTOR    HDD_IDE     MEGASCSI_HD

#define FTYPE_NONE		0
#define FTYPE_DIR		1
#define FTYPE_FILE		2

void InitCitro();
void Init3DS();
void Quit3DS();
void BrowseROM(int slotid, int browsetype);
std::vector<byte> IPSPatchVector(const char* IPSName, const char* SourceName);
void BrowseMCF();
void loadDerFile(const char* derName);
int CreateBlankDisk(int slotid);
const char* BrowseZip(const char* path, const char* extchar);
bool CheckOKButton(int px, int py);
bool CheckCancelButton(int px, int py);
bool CheckScrollButtonL(int px, int py);
bool CheckScrollButtonR(int px, int py);
void BrowseLoadRecently(int slotid, int browsetype);
std::string BrowseFrequentlyUsedFolder(int slotid, int browsetype);
void DrawMessage(const char* message, const char* message2, int x, int y, int waittime, bool isStop);
std::string getZipSaveDiskPath(std::string pathstr, const char* extname);
void AutoSaveDisk(int slotid);
bool BrowseOK(char* msgtxt, char* msgtext2);
//int BrowseInt(char* msgtxt, std::vector<char*> CharVec, int idx, int defid);
int BrowseInt(char* msgtxt, std::vector<char*> CharVec, int idx, int defid, bool cancelZero);
void BrowseOptions();
void BrowseKeyconfig();
void BrowseRapidFire();
std::vector<char*> MenuAddBackDeafult(std::vector<char*> charvec);
std::string BrowseFile(const char* extchr, const char* folderchar, bool IsSave, const char* msgchr);
//void OpenDirectory(std::string dirstr, int browsetype);
int OpenDirectoryDir(std::string dirstr, int browsetype);
void dirstringRebuid(int browsetype);
void PrintMenu(const char* msg, int startid);
void systemMenu();
void BrowseReset();
void BrowseHomeButton();
void DebugMenu();
void ExitConsole();
void DrawTextTranslate(const char* text, float x, float y, float z, float scaleX, float scaleY, u32 color);
bool DrawTextScroll(const char* text, float x, float y, float z, float scaleX, float scaleY, u32 color);
void DrawText(const char* text, float x, float y, float z, float scaleX, float scaleY, u32 color);
void DrawFolderIcon(float x, float y, float width, float height);
void DrawOKButton(const char* okchar);
void DrawCancelButton(const char* canchar);
void DrawScrollButtonL();
void DrawScrollButtonR();
void GetMovedMenuIndex(int& selectidx, int& startidx, int val, int maxcount);
std::string GetUniqueName(std::string pathname, std::string extname);
std::string Get3DSPath(const char* path);
//std::string GetSystemPath(const char* path);
//std::string GetValidSysPath(std::string syspath, std::string pathstr, bool IsMainROM);
void DrawSoldBottom(int xmin, int xmax, int ymin, int ymax, u8 r, u8 g, u8 b);
void DrawSurfaceRectBottom(SDL_Surface* surface, int xmin, int xmax, int ymin, int ymax);
void DrawSurfaceLockBottom(SDL_Surface* surface, int xmin, int xmax, int ymin, int ymax);
void DrawKeyboardScr(bool kana, bool caps, bool shift, bool graph, bool ctrl);
void DrawScreenShotScr();
void MoveScreenShot(int px, int py);
void DrawkeyboardLock(int xmin, int xmax, int ymin, int ymax);
void DrawMouseScr();
void DrawSurfaceBlitzBottom(SDL_Surface* surface, bool IsDefalt, int xmin, int ymin, int width, int height, int dxmin, int dymin);
void SDLSurfaceToC3DTexData(SDL_Surface* surface, void* data, int swidth, int sheight, byte Alpha);
SDL_Surface* SDLSurfaceExtract(SDL_Surface* surface, int xmin, int ymin, int width, int height);
void DrawHUD(void);
void C3DTextureChangeAlpha(C3D_Tex tex, uint alpha);
void AdjustReferenceImageImpose(void);
void AddRecentlyList(std::string str, const char* ftype);
void RemoveRecentlyList(std::string str);
void DoPatchGetPAD(int oldPort, int newPort, byte PortNum);
const char* GetFileExtension(std::string filestr);
char* StringToChar(std::string str);
void Debug_BreakPoint3DS(const char* message);
int GetSoftKeyInput(char* text);
#ifdef DEBUGGER_3DS
void DebuggerBottomScreen();
bool BrowseDebuggerMenu();
void DisplayMemory(int startmemory);
#endif // DEBUGGER_3DS