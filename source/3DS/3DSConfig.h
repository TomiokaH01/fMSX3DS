#define LSB_FIRST
#define _3DS
#define FMSX
#define ALTSOUND
#define NO_AUDIO_PLAYBACK
#define USE_CBIOS
//#define DEBUG_ENABLE
#define LOG_ERROR
//#define DEBUG_LOG
//#define OLDSND
//#define DEBUG_PCM

#ifdef DEBUG_LOG
#define printf printfToFile
#endif // DEBUG_LOG