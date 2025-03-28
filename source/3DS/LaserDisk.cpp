/* The Port of ld700.cpp in Common Source Code Project */
/* By Takeda.Toshiya */
/* http://takeda-toshiya.my.coocan.jp/ */
/* Port to 3DS: h.tomioka 2025 */
/* It includes 3DS Video codec code of 3ds-theoraplayer.*/
/* https://github.com/oreo639/3ds-theoraplayer */

extern "C"
{
#include "video.h"
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>

#include <theora/theoradec.h>
#include <tremor/ivorbiscodec.h>

#include <3ds.h>
#include <citro2d.h>
#include "MSX.h"
#include "3DSLib.h"
#include "3DSConfig.h"

#ifdef _LASERDISK

#define PHASE_IDLE			0
#define PHASE_HEADER_PULSE	1
#define PHASE_HEADER_SPACE	2
#define PHASE_BITS_PULSE	3
#define PHASE_BITS_SPACE	4

#define STATUS_EJECT		0
#define STATUS_STOP			1
#define STATUS_PLAY			2
#define STATUS_PAUSE		3

#define MAX_TRACKS			1024
#define MAX_PAUSES			1024

#define LDTEX_WIDTH			640
#define LDTEX_HEIGHT		480
#define LD_BUFFER_SIZE		0x1000

#define CHECK_LD700TOP5(A, B, C, D, E)	((top[0] == A) && (top[1] == B) && (top[2] == C)	\
			&& (top[3] == D) && (top[4] == E))
#define CHECK_LD700TOP8(A, B, C, D, E, F, G, H)	((top[0] == A) && (top[1] == B) && (top[2] == C)   \
            && (top[3] == D) && (top[4] == E) && (top[5] == F) && (top[6] == G) && (top[7] == H))

#define LOOP_TOP_BUFFER			for (int j = 0;;) {	\
		char c = *top++;						\
		if ((c >= '0') && (c <= '9')) {			\
		tmp[j++] = c;	}						\
		else if (c != ' ') {					\
		tmp[j] = '\0';							\
		break;		}}

#define bitCeil(x)	x<=1 ? 1: (1u << (32 - __builtin_clz(x - 1)))

bool accepted = false, preRemoteSignal = false, prevSoundSignal = false, signalBufferOK = false, soundMuteL = false, soundMuteR = false;
u32 preRemoteTime = 0, command = 0, numBits = 0;

int phase = 0, status = 0;
int seekMode = 0, seekNum = 0, currFrameRaw = 0, waitFrameRaw = 0, NumTrack = 0, NumPause = 0, SoundEventId = 0, volumeL = 0, volumeR = 0;
int TrackFramRaw[MAX_TRACKS];
int PauseFrameRaw[MAX_PAUSES];
s16 soundSampleL = 0, soundSampleR = 0;

s16* mixBufferL, * mixBufferR;
int mixBufferPtr, mixBufferLength;

FILE* videoFile;
th_ycbcr_buffer yuv;

Handle y2rEvent;

ogg_sync_state	oggSyncState;
ogg_page oggPage;
ogg_packet oggPacket;
ogg_stream_state oggStreamState;
ogg_stream_state oggStreamState1;
th_info	ThInfo;
th_comment Thcomment;
th_dec_ctx* ThDecCTX = NULL;
th_setup_info* ThSetupInfo = NULL;
ogg_int64_t granulePos = 0;
th_pixel_fmt thPixelFormat;

#include "video.h"

THEORA_Context vidCtx;
Thread vthread = NULL;

th_ycbcr_buffer yubuf;
unsigned short* LDBuf;
s16* LDSoundBuf;
float scaleframe = 1.0f;
int LDSoundMulti = 0;

int LCDisplaying = false;
THEORA_videoinfo* vinfo;
THEORA_audioinfo* ainfo;

void InitLD700()
{
	preRemoteSignal = false;
	preRemoteTime = 0;
	command = numBits = 0;

	status = STATUS_EJECT;
	phase = PHASE_IDLE;
	seekMode = seekNum = 0;
	accepted = false;
	currFrameRaw = 0;
	waitFrameRaw = 0;

	prevSoundSignal = false;
	signalBufferOK = false;
	SoundEventId = -1;
	soundSampleL = soundSampleR = 0;

	mixBufferL = mixBufferR = NULL;
	mixBufferPtr = mixBufferLength = 0;
}


void LD700Write(int id, unsigned int data, unsigned int mask)
{
	if (id == 0)
	{
		bool signal = ((data & mask) != 0);
		if (preRemoteSignal != signal)
		{
			//int usec = (int)get_passed_usec(prev_remote_time);
			int usec = 0;
			preRemoteTime = SDL_GetTicks();
			preRemoteSignal = signal;

			// from openmsx-0.10.0/src/laserdisc/
			switch (phase)
			{
			case PHASE_IDLE:
				if (signal)
				{
					//touch_sound();
					command = numBits = 0;
					phase = PHASE_HEADER_PULSE;
				}
				break;
			case PHASE_HEADER_PULSE:
				//touch_sound();
				if ((usec >= 5800) && (usec < 11200))phase = PHASE_HEADER_SPACE;
				else phase = PHASE_IDLE;
				break;
			case PHASE_HEADER_SPACE:
				//touch_sound();
				if ((usec >= 3400) && (usec < 6200))phase = PHASE_BITS_PULSE;
				else phase = PHASE_IDLE;
				break;
			case PHASE_BITS_PULSE:
				//touch_sound();
				if ((usec >= 380) && (usec < 1070))phase = PHASE_BITS_SPACE;
				else phase = PHASE_IDLE;
				break;
			case PHASE_BITS_SPACE:
				//touch_sound();
				if ((usec >= 1260) && (usec < 4720))command |= 1 << numBits;
				else if ((usec < 300) || (usec >= 1065))
				{
					phase = PHASE_IDLE;
					break;
				}
				if (numBits++ == 32)
				{
					byte custom = (command >> 0) & 0xFF;
					byte customComp = (~command >> 8) & 0xFF;
					byte code = (command >> 16) & 0xFF;
					byte codeComp = (~command >> 24) & 0xFF;
					if ((custom == customComp) && (custom == 0xA8) && (code == codeComp))
					{
						accepted = true;	/* command accepted */
					}
					phase = PHASE_IDLE;
				}
				else
				{
					phase = PHASE_BITS_PULSE;
				}
				break;
			default:
				break;
			}
		}
	}
	else if (id == 1)		/* 1:MUTE L */
	{
		//touch_sound();
		soundMuteL = ((data & mask) != 0);
	}
	else if (id == 2)		/* 2:MUTE R */
	{
		//touch_sound();
		soundMuteR = ((data & mask) != 0);
	}
}


unsigned int LD700Read(int id)
{
	return ((status == STATUS_PLAY) ? 1 : 0);
}


void LD700EventFrame()
{
	int prevFrameRaw = currFrameRaw;
	bool seekDone = false;

	//cur_frame_raw = get_cur_frame_raw();

	if (accepted)
	{
		command = (command >> 16) & 0xFF;
		if (Verbose & 0x08)
		{
			printf("LCD Command = %04Xh ", command);
		}
		switch (command)
		{
		case 0x00:
		case 0x01:
		case 0x02:
		case 0x03:
		case 0x04:
		case 0x05:
		case 0x06:
		case 0x07:
		case 0x08:
		case 0x09:
			if (status != STATUS_EJECT)
			{
				seekNum = seekNum * 10 + command;
				if (Verbose & 0x08)
				{
					printf("LCD Seek Number:%d \n", seekNum);
				}
			}
			break;
		case 0x16:
			if (status != STATUS_EJECT)
			{
				if (status == STATUS_STOP)
				{
				}
				else
				{
					//emu->stop_movie();
					//emu->set_cur_movie_frame(0, false);
					LD700SetStatus(STATUS_STOP);
					if (Verbose & 0x08)printf("LD700: STOP\n");
				}
			}
			break;
		case 0x17:
			if ((status != STATUS_EJECT) && (status != STATUS_PLAY))
			{
				//emu->mute_video_dev(true, true);
				//emu->play_movie();
				LD700SetStatus(STATUS_PLAY);
				if (Verbose & 0x08)printf("LD700: PLAY\n");
			}
			break;
		case 0x18:
			if (status != STATUS_EJECT)
			{
				//emu->pause_movie();
				LD700SetStatus(STATUS_PAUSE);
				if (Verbose & 0x08)printf("LD700: PAUSE\n");
			}
			break;
		case 0x40:
		case 0x41:
		case 0x5F:
			if (status != STATUS_EJECT)
			{
				seekMode = command;
				seekNum = 0;
			}
			break;
		case 0x42:
			if (status != STATUS_EJECT)
			{
				int temp = seekNum, num[3];
				bool flag = true;
				memset(num, 0, sizeof(num));
				for (int i = 0; i < 3; i++)
				{
					int n0 = temp % 10;
					temp /= 10;
					int n1 = temp % 10;
					temp /= 10;
					int n2 = temp % 10;
					temp /= 10;
					if (n0 == n1 && n0 == n2)
					{
						num[i] = n0;
					}
					else
					{
						flag = false;
						break;
					}

				}
				if (flag && (num[1] != 0 || num[2] != 0))
				{
					seekNum = num[0] + num[1] * 10 + num[2] * 100;
				}
				if (seekMode == 0x5F)	/* 0x5F: SEEK WAIT */
				{
					if (seekNum >= 101 && seekNum < 200)waitFrameRaw = TrackFramRaw[seekNum - 100];
					else waitFrameRaw = (int)((float)seekNum / 29.97 * 60 + 5);
					//wait_frame_raw = (int)((double)seek_num / 29.97 * emu->get_movie_frame_rate() + 0.5);
					if (Verbose & 0x08)printf("LD700: WAIT FRAME %d\n", seekNum);
				}
				else
				{
					if (seekMode == 0x40)	/* 0x40: SEEK CHAPTER */
					{
						if (Verbose & 0x08)printf("LD700:SEEK TRACK %d\n", seekNum);
						LD700SetCurTrack(seekNum);
						if (seekNum >= 0 && seekNum <= NumTrack)
						{
							//emu->set_cur_movie_frame(track_frame_raw[track], false);
						}
					}
					else if (seekMode == 0x41)	/* 0x41: SEEK FRAME */
					{
						if (Verbose & 0x08)printf("LD700:SEEK FRAME %d\n", seekNum);
						LD700SetCurFrame(seekNum, 0);
						if (seekNum >= 0)
						{
							int frame = (int)((float)seekNum / 29.97 * 60 + 5);
						}
					}
					if (status == STATUS_PAUSE)
					{
						//emu->mute_video_dev(true, true);
						//emu->play_movie();
						LD700SetStatus(STATUS_PLAY);
						if (Verbose & 0x08)printf("LD700: PLAY\n");
					}
					seekDone = true;
				}
				seekMode = 0;
			}
			break;
		case 0x45:
			if (status != STATUS_EJECT)seekNum = 0;
			break;
		default:
			if (Verbose & 0x08)printf("LaserDisc Unknown Command %02X\n", command);
			break;
		}
		accepted = false;
		LD700SetAck(1);
	}

	if ((!seekDone) && (status == STATUS_PLAY))
	{
		if ((waitFrameRaw != 0) && (prevFrameRaw < waitFrameRaw) && (currFrameRaw >= waitFrameRaw))
		{
			if (Verbose & 0x08)printf("LD700: WAIT RAW FRAME %d (%d)\n", waitFrameRaw, currFrameRaw);
			LD700SetAck(1);
			waitFrameRaw = 0;
		}
		for (int i = 0; i < NumPause; i++)
		{
			if ((prevFrameRaw < PauseFrameRaw[i]) && (currFrameRaw >= PauseFrameRaw[i]))
			{
				//emu->pause_movie();
				LD700SetStatus(STATUS_PAUSE);
				if (Verbose & 0x08)printf("LD700: PAUSE RAW FRAME %d (%d->%d)\n", PauseFrameRaw[i]
					, prevFrameRaw, currFrameRaw);
				break;
			}
		}
	}
}


void LD700EventCallback(int eventId, int err)
{
	if (eventId == 0)	/* 0:EVENT ACK */
	{
		LD700SetAck(0);
	}
	else if (eventId == 1)	/* 1:EVENT SOUND */
	{
		if (signalBufferOK)
		{
			//int sample = signal_buffer->read();
			int sample = mixBufferL[0];
			bool signal = sample > 100 ? true : (sample < -100 ? false : prevSoundSignal);
			prevSoundSignal = signal;
			//LD700Write(0,)
			//write_signals(&outputs_sound, signal ? 0xffffffff : 0);
		}
		//sound_sample_l = sound_buffer_l->read();
		//sound_sample_r = sound_buffer_r->read();
	}
	else if (eventId == 2)	/* EVENT MIX */
	{
		if (mixBufferPtr < mixBufferLength)
		{
			mixBufferL[mixBufferPtr] = soundMuteL ? 0 : soundSampleL;
			mixBufferR[mixBufferPtr] = soundMuteR ? 0 : soundSampleR;
			mixBufferPtr++;
		}
	}
}


void LD700SetStatus(int value)
{
	if (status == value)return;
	if (value == STATUS_PLAY)
	{
		if (SoundEventId == -1)
		{
			//register_event(this, EVENT_SOUND, 1000000.0 / emu->get_movie_sound_rate(), true, &sound_event_id);
		}
		//sound_buffer_l->clear();
		//sound_buffer_r->clear();
		//signal_buffer->clear();
		signalBufferOK = false;
	}
	else
	{
		if (SoundEventId != -1)
		{
			//cancel_event(this, sound_event_id);
			SoundEventId = -1;
			soundSampleL = soundSampleR = 0;
		}
	}
	//write_signals(&outputs_exv, !(value == STATUS_EJECT || value == STATUS_STOP) ? 0xffffffff : 0);
	status = value;
}


void LD700SetAck(unsigned char value)
{
	if (value)
	{
		//register_event(this, EVENT_ACK, 46000, false, NULL);
	}
	//write_signals(&outputs_ack, value ? 0xffffffff : 0);
}


void LD700SetCurFrame(int frame, unsigned char relative)
{
	if (relative)
	{
		if (!frame)return;
	}
	else
	{
		if (frame < 0)return;
	}
	bool sign = (frame >= 0);
	frame = (int)((float)abs(frame) / 29.97 * 60 + 0.5);
	if ((relative) && (!frame))frame = 1;
	//emu->set_cur_movie_frame(sign ? frame : -frame, relative);
	if (Verbose & 0x08)printf("LD700: SEEK RAW FRAME = %d RELATIVE = %d\n", sign ? frame : -frame, relative);
}


void LD700SetCurTrack(int track)
{
	if ((track >= 0) && (track <= NumTrack))
	{
		//emu->set_cur_movie_frame(track_frame_raw[track], false);
	}
}


void LD700OpenDisk(const char* filePath)
{
	FILE* F = zipfopen(filePath, "rb");
	if (!F)
	{
		LD700SetStatus(STATUS_STOP);
		LD700CloseDisk();
	}
	NumTrack = -1;
	memset(TrackFramRaw, 0, sizeof(TrackFramRaw));
	NumPause = 0;
	memset(PauseFrameRaw, 0, sizeof(PauseFrameRaw));
	byte buffer[0x1000 + 1];
	fread(buffer, sizeof(buffer), 1, F);
	fclose(F);
	buffer[0x1000] = 0;

	for (int i = 0; i < 0x1000; i++)
	{
		char* top = (char*)(buffer + i), tmp[128];
		if (CHECK_LD700TOP8('c', 'h', 'a', 'p', 't', 'e', 'r', ':'))
		{
			top += 8;
			LOOP_TOP_BUFFER
				int track = atoi(tmp);
			LOOP_TOP_BUFFER
				if ((track >= 0) && (track <= MAX_TRACKS))
				{
					if (track > NumTrack)
					{
						NumTrack = track;
					}
					TrackFramRaw[track] = atoi(tmp);
					if (Verbose & 0x08)printf("LD700:TRACK %d: %d\n", track, TrackFramRaw[track]);
				}
		}
		else if (CHECK_LD700TOP5('s', 't', 'o', 'p', ':'))
		{
			top += 5;
			LOOP_TOP_BUFFER
				if (NumPause < MAX_PAUSES)
				{
					PauseFrameRaw[NumPause] = atoi(tmp) > 300 ? atoi(tmp) : 285;
					if (Verbose & 0x08)printf("LD700: PAUSE%d\n", PauseFrameRaw[NumPause]);
					NumPause++;
				}
		}
		else if (CHECK_LD700TOP8('E', 'N', 'C', 'O', 'D', 'E', 'R', '='))
		{
			break;
		}
	}
}


void LD700CloseDisk()
{
	//emu->close_movie_file();
	NumTrack = -1;
	LD700SetStatus(STATUS_EJECT);
}


static inline float getFrameScalef(float wi, float hi, float targetw, float targeth)
{
	float w = targetw / wi;
	float h = targeth / hi;
	return fabs(w) > fabs(h) ? h : w;
}


void LD700changeFile(const char* filepath)
{
	int ret = 0;

	if (!isOgg(filepath))
	{
		printf("The file is not an ogg file.\n");
		return;
	}

	if ((ret = THEORA_Create(&vidCtx, filepath)))
	{
		printf("THEORA_Create exited with error, %d.\n", ret);
		return;
	}

	if (!THEORA_HasVideo(&vidCtx) && !THEORA_HasAudio(&vidCtx))
	{
		printf("No audio or video stream could be found.\n");
		return;
	}

	vinfo = THEORA_vidinfo(&vidCtx);
	ainfo = THEORA_audinfo(&vidCtx);

	if (THEORA_HasVideo(&vidCtx))
	{
		scaleframe = getFrameScalef(vinfo->width, vinfo->height, 320, 240);
	}

	printf("Theora Create sucessful.\n");

	if (Verbose)printf("frame width:%d", vinfo->width);
	if (Verbose)printf("frame height%d", vinfo->height);
	if (Verbose)printf("FPS:%d", vinfo->fps);
	if (Verbose)printf("Color Space:%d", vinfo->colorspace);
	if (Verbose)printf("Pixel Format%d\n", vinfo->fmt);

	if (Verbose)printf("Audio Rate %d", ainfo->rate);
	if (Verbose)printf("Audio Channels %d\n", ainfo->channels);

	granulePos = 0;
	InitLaserDiskTexture(bitCeil((float)vinfo->width), bitCeil((float)vinfo->height));
	if (!IsLaserDisk)
	{
		y2rInit();
		LDSoundBuf = (s16*)malloc(4096*128*sizeof(s16));
		IsLaserDisk = 1;
		SoundChannels = 1;
		SDL_CloseAudio();
		InitAudio(SoundSampRate ? 44100 : 22050, 150);
		if ((ainfo->rate >= 44100) && !(SoundSampRate))LDSoundMulti = 1;
	}
	IsPlayLaserDisk = 1;
}


void LD700UpdateVideo(C3D_Tex ldtex)
{
	if (THEORA_eos(&vidCtx))
	{
		IsPlayLaserDisk = 0;
		return;
	}

	if (THEORA_HasVideo(&vidCtx))
	{
		//th_ycbcr_buffer ybr;
		if (THEORA_getvideo(&vidCtx, yuv))
		{
			bool isBusy = true;
			Y2RU_StopConversion();
			while (isBusy)Y2RU_IsBusyConversion(&isBusy);
			switch (vinfo->fmt)
			{
			case TH_PF_420:
				Y2RU_SetInputFormat(INPUT_YUV420_INDIV_8);
				break;
			case TH_PF_422:
				Y2RU_SetInputFormat(INPUT_YUV422_INDIV_8);
				break;
			default:
				break;
			}
			Y2RU_SetOutputFormat(OUTPUT_RGB_16_565);
			Y2RU_SetRotation(ROTATION_NONE);
			Y2RU_SetBlockAlignment(BLOCK_8_BY_8);
			Y2RU_SetTransferEndInterrupt(true);
			Y2RU_SetInputLineWidth(vinfo->width);
			Y2RU_SetInputLines(vinfo->height);
			Y2RU_SetStandardCoefficient(COEFFICIENT_ITU_R_BT_601);
			Y2RU_SetAlpha(0xFF);


			Y2RU_SetSendingY(yuv[0].data, vinfo->width * vinfo->height, vinfo->width, yuv[0].stride - vinfo->width);
			Y2RU_SetSendingU(yuv[1].data, (vinfo->width / 2) * (vinfo->height / 2), vinfo->width / 2,
				yuv[1].stride - (vinfo->width >> 1));
			Y2RU_SetSendingV(yuv[2].data, (vinfo->width / 2) * (vinfo->height / 2), vinfo->width / 2,
				yuv[2].stride - (vinfo->width >> 1));

			Y2RU_SetReceiving(ldtex.data, vinfo->width * vinfo->height * 2, vinfo->width * 8 * 2,
				(bitCeil((float)vinfo->width) - vinfo->width) * 8 * 2);

			Y2RU_StartConversion();

			Y2RU_GetTransferEndEvent(&y2rEvent);
			if (svcWaitSynchronization(y2rEvent, 6e7))
			{
				if (Verbose)
				{
					printf("Y2R timed out");
				}
			}// DEBUG

			PutImageLaserDisk(vinfo->width, vinfo->height, scaleframe, scaleframe);
		}
	}
}


void DoCalcAudioLD(int size)
{
	if (THEORA_HasAudio(&vidCtx))
	{
		THEORA_readaudio(&vidCtx, (char*)LDSoundBuf, size);
	}
}


void TrashLaserDisk(void)
{
	//free(LDBuf);
	if(videoFile)fclose(videoFile);
	if (IsLaserDisk)
	{
		free(LDSoundBuf);
		y2rExit();
	}
}

/* Taken from main.c of 3ds-theoraplayer */
/* https://github.com/oreo639/3ds-theoraplayer */
int isOgg(const char* filepath)
{
	FILE* fp = fopen(filepath, "r");
	char magic[16];

	if (!fp)
	{
		printf("Could not open %s. Please make sure file exists.\n", filepath);
		return 0;
	}

	fseek(fp, 0, SEEK_SET);
	fread(magic, 1, 16, fp);
	fclose(fp);

	if (!strncmp(magic, "OggS", 4))
		return 1;

	return 0;
}
#endif // _LASERDISK