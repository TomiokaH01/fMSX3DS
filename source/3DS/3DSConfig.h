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
#define UPD_FDC		/* Emulate MSXTurboR FDC. */
#define _MSX0
//#define MSX0_OLED
#define VDP_V9990
#define DUAL_SCREEN
#define SUPERIMPOSE
#define USE_OVERCLOCK
//#define AUDIO_SYNC
#define HDD_NEXTOR
//#define	HDD_IDE
//#define MEGASCSI_HD			/* Mega-SCSI cartridge for SCSI devices. http://www.hat.hi-ho.ne.jp/tujikawa/ese/megascsi.html */
//#define MEMORY_CURSOR_POS		/* Memory position of the cursor in system menu. */

//#define DEBUG_ENABLE
#define LOG_ERROR
#define DEBUG_LOG
//#define OLDSND
//#define DEBUG_PCM
#define DEBUGGER_3DS
#define ALTPCM
//#define INLINE_ASM	/* Currently, only very very little speed up. */

#ifdef DEBUG_LOG
#define printf printfToFile
#endif // DEBUG_LOG