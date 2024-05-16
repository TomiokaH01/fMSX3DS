/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                         SDLsnd.c                        **/
/**                                                         **/
/** This file contains standard sound generation routines   **/
/** for the SDL library.                                    **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1996-2000                 **/
/**               Vincent van Dam 2002                      **/
/**    	                                                    **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/

/** NINTENDO 3DS Version modified by h.tomioka 2023         **/

#include <stdio.h>
#include <stdlib.h>

#include <string.h>

#include "3DSConfig.h"

#ifndef OSD_CPU_H
#define OSD_CPU_H
typedef unsigned char	UINT8;   /* unsigned  8bit */
typedef unsigned short	UINT16;  /* unsigned 16bit */
typedef unsigned int	UINT32;  /* unsigned 32bit */
typedef signed char		INT8;    /* signed  8bit   */
typedef signed short	INT16;   /* signed 16bit   */
typedef signed int		INT32;   /* signed 32bit   */
#endif

#if defined(FMSX) && defined(ALTSOUND)
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int  uint32;

typedef signed char int8;
typedef signed short int16;
typedef signed int int32;

#ifdef OLDSND
#define EMU2413_COMPACTION
#include "snddrv/OLD/emu2149.h"
#include "snddrv/OLD/emu2212.h"
#include "snddrv/OLD/emu2413.h"
#include "snddrv/OLD/emu8950.h"
#include "snddrv/filter.h"
#include "snddrv/dc_filter.h"
#include "snddrv/rc_filter.h"
#else
#include "snddrv/emu2149.h"
#include "snddrv/emu2212.h"
#include "snddrv/emu2413.h"
#include "snddrv/emu8950.h"
#include "snddrv/filter.h"
#include "snddrv/dc_filter.h"
#include "snddrv/rc_filter.h"
#endif // OLDSND
#endif

extern "C"
{
#include <SDL/SDL.h>
}
#include <3ds.h>
#include "Sound.h"
#include "3DSLib.h"
static void MixAudio(short* buffer, unsigned int length);
static void AudioCallback(void* buf, Uint8* stream, int len);

#if defined(FMSX) && defined(ALTSOUND)
#define MSX_CLK 3579545

static OPLL* opll;
static OPL* opl;
static PSG* psg;
static SCC* scc;

static DCF *dcfL, *dcfR;
static RCF *rcfL, *rcfR;
static FIR *firL, *firR, *firR1;

//static Uint8* AudioBuf;
static INT16* Audio16Buf;
static INT16* AudioFMBuf;

volatile int audiopos;
unsigned char IsSndRegUpd = 0;
volatile unsigned char AudioCalcCnt = 0;
unsigned char IsVoice = 0;

//#define SND_BUFF_SIZE 384
#define SND_BUFF_SIZE 1024
//#define SND_BUFF_SIZE 2048

//#define CUT_OFF 1024
//#define CUT_OFF 2048
#define CUT_OFF 4096

float FactorPSG = 1.00;   /* Volume percentage of PSG        */
float FactorSCC = 1.00;   /* Volume percentage of SCC        */
float Factor2413 = 1.00;  /* Volume percentage of MSX-MUSIC  */
float Factor8950 = 1.00;  /* Volume percentage of MSX-AUDIO  */
float FactorPCM = 1.00;   /* Volume percentage of PCM        */

float MasterVol = 1.00f;
int playYM2413 = 0;
int playY8950 = 0;
int DAVal = 0;
int DA1bit = 0;
int DA8bit = 0;
unsigned char playSCC = 0;

#else
static sample SndData[SND_BUFSIZE];
static int MixBuffer[SND_BUFSIZE];
//static int WPtr = 0;
static Uint8 *WPtr = 0;
#endif

static int SndRate     = 0;  /* Audio sampling rate          */

/** StopSound() **********************************************/
/** Temporarily suspend sound.                              **/
/*************************************************************/
void StopSound(void) { SDL_PauseAudio(1); }

/** ResumeSound() ********************************************/
/** Resume sound after StopSound().                         **/
/*************************************************************/
void ResumeSound(void) { SDL_PauseAudio(0); }

/** InitAudio() **********************************************/
/** Initialize sound. Returns rate (Hz) on success, else 0. **/
/** Rate=0 to skip initialization (will be silent).         **/
/*************************************************************/
unsigned int InitAudio(unsigned int Rate, unsigned int Latency)
{
	if (!SDL_WasInit(SDL_INIT_AUDIO)) SDL_InitSubSystem(SDL_INIT_AUDIO);

	SDL_AudioSpec* audioSpec;
	audioSpec = (SDL_AudioSpec*)calloc(1, sizeof(SDL_AudioSpec));
	audioSpec->freq = Rate;
	audioSpec->format = AUDIO_S16LSB;
	//audioSpec->channels = 2;
	audioSpec->channels = SoundChannels + 1;
	audioSpec->samples = SND_BUFF_SIZE;
	audioSpec->callback = &AudioCallback;
	audioSpec->userdata = NULL;

	if (SDL_OpenAudio(audioSpec, NULL) < 0)
	{
		return(SndRate = 0);
	}

	TrashAudio();

#if defined(FMSX) && defined(ALTSOUND)
	/* MSX-MUSIC emulation */
	if (Use2413)
	{
		opll = OPLL_new(MSX_CLK, Rate);
		OPLL_reset(opll);
	}

	/* MSX-AUDIO emulation */
	if (Use8950)
	{
		opl = OPL_new(MSX_CLK, Rate);
		// //OPL_setQuality(opl, 49716);
		OPL_reset(opl);
		//OPL_setRate(opl, 49716);
	}

	dcfL = DCF_new();
	dcfL->enable = 1;
	DCF_reset(dcfL, Rate);
	dcfR = DCF_new();
	dcfR->enable = 1;
	DCF_reset(dcfR, Rate);

	rcfL = RCF_new();
	rcfL->enable = 1;
	RCF_reset(rcfL, Rate, 4.7e3, 0.010e-6);
	//RCF_reset(rcfL, Rate, 3870, 15);
	rcfR = RCF_new();
	rcfR->enable = 1;
	RCF_reset(rcfR, Rate, 4.7e3, 0.010e-6);
	//RCF_reset(rcfR, Rate, 3870, 15);

	firL = FIR_new();
    //FIR_reset(firL, Rate, CUT_OFF, 31);
	FIR_reset(firL, Rate, (CUT_OFF) << 1, 31);
    //FIR_reset(firL, Rate, (CUT_OFF) << 1, 8);

	firR = FIR_new();
	FIR_reset(firR, Rate, CUT_OFF, 31);

	firR1 = FIR_new();
	FIR_reset(firR1, Rate, CUT_OFF, 31);

	/* PSG/SCC emulation */
	psg = !PSGType ? PSG_new(MSX_CLK, Rate) : PSG_new(MSX_CLK / 2, Rate);
#ifndef OLDSND
	if (!PSGType)PSG_setClockDivider(psg, 1);
#endif // !OLDSND
	PSG_setVolumeMode(psg, PSGType + 1);
	PSG_reset(psg);

	scc = SCC_new(MSX_CLK, Rate);
	SCC_reset(scc);
	//Write2212(0xBFFE, 0x20);
	//Write2212(0xB000, 0x80);
	SCC_set_type(scc, SCC_STANDARD);
#ifdef OLDSND
    if (Use2413 && !Is2413HQ) opll->quality = 0;
#else
	PSG_set_quality(psg, IsPSGHQ);
	SCC_set_quality(scc, IsSCCHQ);
	if (Use2413 && !Is2413HQ) opll->conv = 0;
#endif // OLDSND
	//if (!Is2413HQ) OPLL_setRate(opll, 49716);

	playYM2413 = 0;
	playY8950 = 0;
	DAVal = 0;
	DA1bit = 0;
	DA8bit = 0;
	//AudioBuf = (Uint8*)malloc(SND_BUFF_SIZE*256);
	Audio16Buf = (INT16*)malloc(SND_BUFF_SIZE * 128);
	AudioFMBuf = (INT16*)malloc(SND_BUFF_SIZE * 128);

	SDL_PauseAudio(0);
	SDL_Delay(3000);
#else
	WPtr = 0;
	memset(SndData, 0, sizeof(SndData));
	memset(MixBuffer, 0, sizeof(MixBuffer));
#endif

	/* Done */
	return(SndRate = Rate);
}

/** ResetSound() *********************************************/
/** Reset sound chips.                                      **/
/*************************************************************/
void ResetSound()
{
#if defined(FMSX) && defined(ALTSOUND)
  PSG_reset(psg);
  SCC_reset(scc);
  if (SCCEnhanced || ForceSCCP)
  {
      SCC_set_type(scc, SCC_ENHANCED);
      Write2212(0xBFFE, 0x20);
      Write2212(0xB000, 0x80);
  }
  else SCC_set_type(scc, SCC_STANDARD);
  //Write2212(0xBFFE, 0x20);
  //Write2212(0xB000, 0x80);

  if (Use2413) OPLL_reset(opll);
  if (Use8950)OPL_reset(opl);
  if (UseDCFilter)
  {
      DCF_reset(dcfL, SndRate);
      DCF_reset(dcfR, SndRate);
  }
  if (UseRCFilter)
  {
      //RCF_reset(rcfL, SndRate, 3870, 15);
      //RCF_reset(rcfR, SndRate, 3870, 15);
      RCF_reset(rcfL, SndRate, 4.7e3, 0.010e-6);
      RCF_reset(rcfR, SndRate, 4.7e3, 0.010e-6);
  }
  if (UseFIRFilter)
  {
      FIR_reset(firL, SndRate, (CUT_OFF) << 1, 31);
      FIR_reset(firR, SndRate, CUT_OFF, 31);
      FIR_reset(firR1, SndRate, (CUT_OFF) << 1, 31);
      //FIR_reset(firR, SndRate, CUT_OFF, 31);
  }
#ifdef OLDSND
  if (Use2413 && !Is2413HQ) opll->quality = 0;
#else
  PSG_set_quality(psg, IsPSGHQ);
  SCC_set_quality(scc, IsSCCHQ);
  if (Use2413 && !Is2413HQ) opll->conv = 0;
#endif // OLDSND
  playSCC = 0;
  //if(!Is2413HQ) OPLL_setRate(opll, 49716);
#endif
}

/** TrashAudio() *********************************************/
/** Free resources allocated by InitAudio().                **/
/*************************************************************/
void TrashAudio(void)
{
  if (!SndRate) return;
  SndRate = 0;

  /* Shutdown wave audio */
  StopSound();

#if defined(FMSX) && defined(ALTSOUND)
  /* clean up MSXMUSIC */
  //OPLL_delete(opll);
  if (opll)OPLL_delete(opll);

  /* clean up MSXAUDIO */
  //if (Use8950) OPL_delete(opl);
  if (opl)OPL_delete(opl);

  /* clean up Filters */
  if (dcfL)DCF_delete(dcfL);
  if (dcfR)DCF_delete(dcfR);
  if (rcfL)RCF_delete(rcfL);
  if (rcfR)RCF_delete(rcfR);
  if (firL)FIR_delete(firL);
  if (firR)FIR_delete(firR);
  if (firR1)FIR_delete(firR1);

  /* clean up PSG/SCC */
  PSG_delete(psg);
  //psg.~PSG();
  SCC_delete(scc);
#endif
}

/** AudioCallback() ******************************************/
/** Called by the system to render sound                    **/
/*************************************************************/
static void AudioCallback(void* buf, Uint8* stream, int len)
{
    if (audiopos < len)return;

	int J, R, R1, P , O, A, S, D;

    //Sint16* OutBuf = (Sint16*)stream + 0;
    Uint8* Out = stream + 0;
    INT16* LBuf = Audio16Buf + 0;

    INT16* FMBuf = AudioFMBuf + 0;

	if (!SoundChannels)
	{
		for (J = 0; J < len; J += 2)
		{
			R = LBuf[J >> 1];
			//if (UseFIRFilter)R = FIR_calc(firL, R);
			if (playYM2413 || playY8950)
			{
				R1 = FMBuf[J >> 1];
                /* YM2413 and Y8950 calculate low volume in CalcAudio() and Increase here. This reduce noise in loud volume. */
                R1 = R1 < -21844 ? R1 : (R1 > 21844 ? R1 : R1 * 1.5f);
				if (UseFIRFilter)R1 = FIR_calc(firR, R1);
				R += R1;
			}
            //R = FIR_calc(firR, R);
			if (UseDCFilter)R = DCF_calc(dcfL, R);
			if (UseRCFilter)R = RCF_calc(rcfL, R);
			R = R < -32768 ? -32768 : (R > 32767 ? 32767 : R);

			//R = compressBuf(R);
			Out[J] = R & 0xFF;
			Out[J + 1] = (R & 0xFF00) >> 8;
		}
		memmove(Audio16Buf, Audio16Buf + (len >> 1), audiopos - (len >> 1));
		memmove(AudioFMBuf, AudioFMBuf + (len >> 1), audiopos - (len >> 1));
		audiopos -= len;
		audiopos = audiopos < 0 ? 0 : audiopos;
	}
    else
    {
        for (J = 0; J < len; J += 4)
        {
            R = LBuf[J >> 2];
            //if (UseFIRFilter)R = FIR_calc(firL, R);
            if (UseDCFilter)R = DCF_calc(dcfL, R);
            if (UseRCFilter)R = RCF_calc(rcfL, R);
            if (playYM2413 || playY8950)
            {
                R1 = FMBuf[J >> 2];
                /* YM2413 and Y8950 calculate low volume in CalcAudio() and Increase here. This reduce noise in loud volume. */
                R1 = R1 < -21844 ? R1 : (R1 > 21844 ? R1 : R1 * 1.5f);
                if (UseFIRFilter)R1 = FIR_calc(firR, R1);
                if (UseDCFilter)R1 = DCF_calc(dcfR, R1);
                if (UseRCFilter)R1 = RCF_calc(rcfR, R1);
            }
            else if (playSCC)
            {
                R1 = FMBuf[J >> 2];
                //if (UseFIRFilter)R1 = FIR_calc(firR1, R1);
                if (UseDCFilter)R1 = DCF_calc(dcfR, R1);
                if (UseRCFilter)R1 = RCF_calc(rcfR, R1);
            }
            else R1 = 0;
            R = R < -32768 ? -32768 : (R > 32767 ? 32767 : R);
            R1 = R1 < -32768 ? -32768 : (R1 > 32767 ? 32767 : R1);
            Out[J] = R & 0xFF;
            Out[J + 1] = (R & 0xFF00) >> 8;
            Out[J + 2] = R1 & 0xFF;
            Out[J + 3] = (R1 & 0xFF00) >> 8;
        }
        memmove(Audio16Buf, Audio16Buf + (len >> 2), audiopos - (len >> 2));
        memmove(AudioFMBuf, AudioFMBuf + (len >> 2), audiopos - (len >> 2));
        audiopos -= (len>>1);
        audiopos = audiopos < 0 ? 0 : audiopos;
        return;
    }

    //Slow?
    //SDL_MixAudio(stream, AudioBuf, len, SDL_MIX_MAXVOLUME);
}


void CalcAudio(void)
{
	if ((audiopos >> 3) > SND_BUFF_SIZE)return;
    //if (TurboNow)return;

	//if ((audiopos >> 7) > SND_BUFF_SIZE)return;

    updatePrinter();

//#ifdef ALTPCM
//    updatePCM();
//#endif // ALTPCM

	register INT16 P, O, A, S;
    int R, R1;
    INT16 *LBuf, *FMBuf, *PSGBuf, *SCCBuf, *OPLLBuf, *OPLBuf;

    LBuf = Audio16Buf + (audiopos>>1);
    FMBuf = AudioFMBuf + (audiopos >> 1);

	P = PSG_calc(psg);
	//S = SCC_calc(scc);
    S = playSCC ? SCC_calc(scc) : 0;
    O = playYM2413 ? Use2413 ? OPLL_calc(opll) : 0 : 0;
	A = playY8950 ? Use8950 ? OPL_calc(opl) : 0 : 0;

    if (!SoundChannels)
    {
#ifdef DEBUG_PCM
        R = DA8bit * FactorSCC;
        //R = OPL_calc(adpcm);
        R1 = 0;
#else
        R = P * FactorPSG + S * FactorSCC + DA1bit * FactorPSG + DA8bit * FactorPCM;
        R1 = O * Factor2413 + A * Factor8950;
        //R1 = R1 < -21844 ? R1 : (R1 > 21844 ? R1 : R1 * 1.5f);
#endif // DEBUG_PCM
        LBuf[0] = R;
        FMBuf[0] = R1;
        audiopos += 2;
        return;
    }
    if (playYM2413 || playY8950)
    {
        R = P * FactorPSG + DA1bit * FactorPSG + S * FactorSCC + DA8bit * FactorPCM;
        R1 = O * Factor2413 + A * Factor8950;
        //R1 = R1 < -21844 ? R1 : (R1 > 21844 ? R1 : R1 * 1.5f);
    }
    else
    {
        R = P * FactorPSG + DA1bit * FactorPSG;
        R1 = S * FactorSCC + DA8bit * FactorPCM;
    }
    LBuf[0] = R;
    FMBuf[0] = R1;
    audiopos += 2;
}


unsigned char CheckIsVoice(void)
{
    if (IsSndRegUpd >= 5)
    {
        IsVoice = 1;
        AudioCalcCnt = 0;
    }
    else if (IsVoice)
    {
        AudioCalcCnt++;
        if (AudioCalcCnt > 5)
        {
            IsVoice = 0;
        }
    }

    if (IsVoice) return 1;
    return 0;
}


//void FillAudio(void)
//{
//    if ((audiopos >> 3) > SND_BUFF_SIZE)return;
//    Uint8* Buf;
//
//    Buf = AudioBuf + audiopos;
//
//    Buf[0] = OldR & 0xFF;
//    Buf[1] = (OldR & 0xFF00) >> 8;
//    Buf[2] = OldR1 & 0xFF;
//    Buf[3] = (OldR1 & 0xFF00) >> 8;
//    audiopos += 4;
//    Buf += 4;
//}

/*
    From libkss by Mitsutaka Okazaki.
    https://github.com/digital-sound-antiques/libkss
*/
static int16_t compressBuf(int32_t wav)
{
    const int32_t vt = 32768 * 8 / 10;

    if (wav < -vt) {
        wav = ((wav + vt) >> 2) - vt;
    }
    else if (vt < wav) {
        wav = ((wav - vt) >> 2) + vt;
    }

    if (wav < -32767) {
        wav = -32767;
    }
    else if (32767 < wav) {
        wav = 32767;
    }

    return (int16_t)wav;
}


/** AudioCallback() ******************************************/
/** Writes sound to the output buffer,                      **/
/** mixing as necessary                                     **/
/*************************************************************/
static void MixAudio(short *buffer, unsigned int length)
{
  register int J;

#if defined(FMSX) && defined(ALTSOUND)
  register int R;
  register INT16 P,O,A,S;

  /* Mix sound */
  for(J=0;J<length;J++)
  {
    S=SCC_calc(scc);
    O=Use2413? OPLL_calc(opll): 0;
    A = Use8950 ? OPL_calc(opl) : 0;
    R=P*FactorPSG+O*Factor2413+A*Factor8950+S*FactorSCC;

    /* Write to output buffer */
    *buffer++ = (R>32767)?32767:(R<-32768)?-32768:R;
  }
#else
  /* Mix sound */
  memset(MixBuffer,0,sizeof(MixBuffer));
  RenderAudio(MixBuffer,length);
  PlayAudio(MixBuffer,length);

  /* Write to output buffer */
  for(J=0;J<length;J++)
    *buffer++ = SndData[J];
#endif
}

/** GetFreeAudio() *******************************************/
/** Get the amount of free samples in the audio buffer.     **/
/*************************************************************/
unsigned int GetFreeAudio(void)
{
#if defined(FMSX) && defined(ALTSOUND)
  return(0); /* Not used; should not be called */
#else
  return(!SndRate?0:SND_BUFSIZE-WPtr);
#endif
}

/** WriteAudio() *********************************************/
/** Write up to a given number of samples to audio buffer.  **/
/** Returns the number of samples written.                  **/
/*************************************************************/
unsigned int WriteAudio(sample *Data,unsigned int Length)
{
#if defined(FMSX) && defined(ALTSOUND)
  return(0); /* Not used; should not be called */
#else
  /* Require audio to be initialized      */
  /* and buffer to have enough free space */
  if(!SndRate||WPtr+Length>SND_BUFSIZE) return(0);

  /* Copy sample data and increment buffer position */
  memcpy(SndData+WPtr*sizeof(sample),Data,Length*sizeof(sample));
  if ((WPtr+=Length)>=SND_BUFSIZE) WPtr=0;
  
  /* Return number of samples copied */
  return(Length);
#endif
}

#if defined(FMSX) && defined(ALTSOUND)
/* wrapper functions to actual sound emulation */

//void WriteOPLL (int R,int V) { OPLL_writeReg(opll,R,V); }
void WriteOPLL(int R, int V) { OPLL_writeIO(opll, R, V); }
void WriteAUDIO(int R, int V) { OPL_writeIO(opl, R + 0xC0, V); }
void Write2212 (int R,int V) { SCC_write(scc,R,V); }
//void WritePSG  (int R,int V) { PSG_writeReg(psg,R,V); }
void WritePSG(int R, int V) { PSG_writeIO(psg, R, V); }
int  ReadAUDIO(int R) { return R&1 ? OPL_readIO(opl) : OPL_status(opl); }
int  ReadPSG(int R) { return PSG_readIO(psg); }
//int  ReadPSG   (int R)       { return PSG_readReg(psg,R); }

int Read2212(int R){ return SCC_read(scc, R);}

int SCCEnabled(void){ return scc->active; }


int CheckSCC(int R)
{
    if (!scc->active)return 0;
    R -= scc->base_adr;
    if (R < 0x800 || R>0x8FF)return 0;
    return 1;
}

void SetSCCEnhanced(int IsEnhance)
{
    if (IsEnhance)
    {
        SCC_set_type(scc, SCC_ENHANCED);
        Write2212(0xBFFE, 0x20);
        Write2212(0xB000, 0x80);
        scc->mode = 1;
    }
    else
    {
        SCC_set_type(scc, SCC_STANDARD);
        scc->mode = 0;
    }
}

#endif

void OPLLChangeOption(bool IsEnable)
{
    if (IsEnable)
    {
        if(!opll)opll = OPLL_new(MSX_CLK, SndRate);
        OPLL_reset(opll);
    }
    //else
    //{
    //    OPLL_delete(opll);
    //}

    //DoReloadFMPAC();
}


void OPLChangeOption(bool IsEnable)
{
    if (IsEnable)
    {
        if(!opl)opl = OPL_new(MSX_CLK, SndRate);
        OPL_reset(opl);
    }
    //else
    //{
    //    OPL_delete(opl);
    //}
}

void DCFilterChangeOption(bool IsEnable)
{
    if (IsEnable)
    {
        if(!dcfL)dcfL = DCF_new();
        dcfL->enable = 1;
        DCF_reset(dcfL, SndRate);
        if(!dcfR)dcfR = DCF_new();
        dcfR->enable = 1;
        DCF_reset(dcfR, SndRate);
    }
    else
    {
        //DCF_delete(dcfL);
        //DCF_delete(dcfR);

        DCF_disable(dcfL);
        DCF_disable(dcfR);
    }
}


void RCFilterChangeOption(bool IsEnable)
{
    if (IsEnable)
    {
        if(!rcfL)rcfL = RCF_new();
        rcfL->enable = 1;
        //RCF_reset(rcfL, SndRate, 3870, 15);
        RCF_reset(rcfL, SndRate, 4.7e3, 0.010e-6);
        if(!rcfR)rcfR = RCF_new();
        rcfR->enable = 1;
        RCF_reset(rcfR, SndRate, 4.7e3, 0.010e-6);
        //RCF_reset(rcfL, SndRate, 3870, 15);
    }
    else
    {
        //RCF_delete(rcfL);

        RCF_disable(rcfL);
        RCF_disable(rcfR);
    }
}


void FIRFilterChangeOption(bool IsEnable)
{
    if (IsEnable)
    {
        if(!firL)firL = FIR_new();
        FIR_reset(firL, SndRate, CUT_OFF, 31);
        if(!firR)firR = FIR_new();
        FIR_reset(firR, SndRate, CUT_OFF, 31);
    }
    else
    {
        //FIR_delete(firL);
        //FIR_delete(firR);
        FIR_disable(firL);
        FIR_disable(firR);
    }
}


void EnableFMSound(int FMType)
{
    switch (FMType)
    {
    case 0:
        playYM2413 = 1;
        break;
    case 1:
        playY8950 = 1;
        break;
    default:
        break;
    }
}