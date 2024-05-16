#define LSB_FIRST
#define _3DS
#define FMSX
#define ALTSOUND
#define NO_AUDIO_PLAYBACK
#define USE_CBIOS
#define USE_3D	/* Emulate stereoscopic 3D glass with nintendo 3DS's Stereoscopic 3D */
#define TURBO_R	/* MSXturboR */
/* Currently i get confused "_3DS" and "TURBO_R". You need define both of them.*/
/* TODO: "_3DS" only covers Nintendo 3DS(libctru) related code only. */
#define _MSX0
//#define VDP_V9990
//#define HDD_NEXTOR
//#define	HDD_IDE

//#define DEBUG_ENABLE
#define LOG_ERROR
//#define DEBUG_LOG
//#define OLDSND
//#define DEBUG_PCM
#define ALTPCM
//#define INLINE_ASM	/* Currently, only very very little speed up. */

#ifdef DEBUG_LOG
#define printf printfToFile
#endif // DEBUG_LOG