/* Mega-SCSI and MB89352A emulation library. Based on MB89352.c in BlueMSX and scsi.cpp in XM6. */

/* http://bluemsx.msxblue.com/

/* http://retropc.net/pi/xm6/index.html */

#include "MSX.h"
#include "3DSLib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef MEGASCSI_HD

#include "MegaSCSI.h"

void MB89352A_Init(MB89352A* spc)
{
	spc->Buf = (unsigned char*)malloc(0x800);
	spc->Type = 1;
}


void MB89352A_DisConnect(MB89352A* spc)
{
	if (spc->Phase != SPC_PHASE_BUSFRRE)
	{
		spc->Regs[4] |= 0x20;	/* Regs[4]:INTS,  0x20:Disconnect */
		spc->Phase = SPC_PHASE_BUSFRRE;
		spc->Message = -1;
	}
	spc->Regs[5] = 0;	/* Regs[5]:PSNS */
	spc->bsy = false;
	spc->TransActive = false;
	spc->Length = 0;
	spc->Counter = 0;
	spc->tc = 0;
	spc->atn = false;
}


void MB89352A_SoftReset(MB89352A* spc)
{
	int i;
	spc->sel = false;
	for (i = 2; i < 15; ++i)spc->Regs[i] = 0;
	for (i = 0; i < 12; ++i)spc->CmdData[i] = 0;
	spc->CurrBuf = spc->Buf;
	spc->Phase = SPC_PHASE_BUSFRRE;
	MB89352A_DisConnect(spc);
}


void MB89352A_Reset(MB89352A* spc)
{
	int i;

	spc->Regs[0] = 0x80;	/* Regs[0]:BDID */
	spc->Regs[1] = 0x80;	/* Regs[1]:SCTL */
	spc->rst = false;
	spc->atn = false;
	spc->Id = 7;

	MB89352A_SoftReset(spc);
}


unsigned char MB89352A_ReadReg(MB89352A* spc, unsigned char R)
{
	unsigned char retval;
	switch (R)
	{
	case 5:		/* Regs[5]:PSNS */
		//if (spc->Phase == SPC_PHASE_EXECUTE)
		//{
		//	spc->Phase = SPC_PHASE_EXECUTE;
		//	spc->Blocks = 0;
		//	spc->Counter = 0;

		//	if ((spc->atn) && (spc->Phase != SPC_PHASE_EXECUTE))
		//	{
		//		spc->NextPhase = spc->Phase;
		//		spc->Phase = SPC_PHASE_MSGOUT;
		//		spc->Regs[5] = 0x80 | 0x08 | 0x04 | 0x02 | 0x00; /* 0x80:REQ,  0x08:BSY(Busy),  0x04:MSG(Message),  0x02:CD(Command),  0x01:IO(0:Data Output) */
		//	}
		//	else
		//	{
		//		switch (spc->Phase)
		//		{
		//		case SPC_PHASE_DATAIN:
		//			spc->Regs[5] = 0x80 | 0x08 | 0x01; /* 0x80:REQ,  0x08:BSY(Busy),  0x01:IO(0:Data Input) */
		//			break;
		//		case SPC_PHASE_DATAOUT:
		//			spc->Regs[5] = 0x80 | 0x08 | 0x00; /* 0x80:REQ,  0x08:BSY(Busy),  0x00:IO(0:Data Output) */
		//			break;
		//		case SPC_PHASE_STATUS:
		//			spc->Regs[5] = 0x80 | 0x08 | 0x02 | 0x01; /* 0x80:REQ,  0x08:BSY(Busy),  0x02:CD(Command),  0x01:IO(0:Data Input) */
		//			break;
		//		case SPC_PHASE_EXECUTE:
		//			spc->Regs[5] = 0x08; /* 0x08:BSY(Busy) */
		//			break;
		//		default:
		//			break;
		//		}
		//	}
		//}
		//return (unsigned char)((spc->Regs[5] | spc->atn ? 0x20 : 0) & 0xFF);

		retval = spc->Regs[5];
		if (spc->req)retval |= 0x80;
		if (spc->ack)retval |= 0x40;
		if (spc->atn)retval |= 0x20;
		if (spc->sel)retval |= 0x10;
		if (spc->bsy)retval |= 0x08;
		if (spc->msg)retval |= 0x04;
		if (spc->cd)retval |= 0x02;
		if (spc->io)retval |= 0x01;
		return retval;

	case 6:		/* Regs[6]:SSTS */
		retval = 1;
		if (spc->TransActive)
		{
			if (spc->Regs[5] & 0x01)		/* 0x01:IO (1:Data Input) */
			{
				if (spc->tc >= 8)retval = 2;
				else
				{
					if (spc->tc != 0)retval = 0;
				}
			}
		}
		retval = 1;
		if (spc->Phase != SPC_PHASE_BUSFRRE)retval |= 0x80;		/* 0x40,0x80:Connected */
		if (spc->bsy)retval |= 0x20;		/* 0x20:SPC BUSY */
		if ((spc->TransActive) || (spc->Phase == SPC_PHASE_COMMAND))retval |= 0x10;		/* 0x10:Transfer Progress */
		if (spc->rst)retval |= 0x8;		/* 0x08:Reset */
		if (!spc->tc)retval |= 0x04;	/* 0x04:TC = 0 */
		return retval;

	case 10:		/* Regs[10]:DREG */
		return MB89352A_ReadDREG(spc);
	case 12:		/* Regs[12]:TCH */
		return (unsigned char)((spc->tc >> 16) & 0xFF);
	case 13:		/* Regs[13]:TCM */
		return (unsigned char)((spc->tc >> 8) & 0xFF);
	case 14:		/* Regs[14]:TCL */
		return (unsigned char)(spc->tc & 0xFF);

	default:
		return ((unsigned char)(spc->Regs[R] & 0xFF));
	}
}


void MB89352A_WriteReg(MB89352A* spc, unsigned char R, unsigned char V)
{
	int I, J;
	bool IsError = false;
	bool tempflag = false;
	switch (R)
	{
	case 0:			/* Regs[0]:BDID */
		V &= 0x07;
		spc->Id = V;
		spc->Regs[0] = 1 << V;		/* Regs[0]:BDID */
		break;

	case 1:			/* Regs[1]:SCTL */
		tempflag = (V & 0xE0) ? false : true;
		if (tempflag != spc->isEnabled)
		{
			spc->isEnabled = tempflag;
			if (!tempflag)MB89352A_SoftReset(spc);
		}
		spc->Regs[R] = V;
		break;

	case 2:			/* Regs[2]:SCMD */
		if (!spc->isEnabled)break;
		if (V & 0x10)		/* 0x10:RST Out */
		{
			if (!(spc->Regs[2] & 0x10) & !(spc->Regs[1]))
			{
				spc->rst = true;
				spc->Regs[4] |= 0x01;		/* Regs[4]:INTS,  0x01:Reset Condition(1:Reset) */
				MB89352A_DisConnect(spc);
			}
		}
		else
		{
			spc->rst = false;
		}

		spc->Regs[2] = V;

		switch (V>>5)		/* V>>5:Command */
		{
		case 0:			/* Bus Release */
			MB89352A_DisConnect(spc);
			break;

		case 1:			/* Select */
			if (spc->rst)
			{
				spc->Regs[4] |= 0x04;		/* Regs[4]:INTS,  0x04:TimeOut */
				break;
			}

			if (spc->Regs[8] & 0x01)		/* Regs[8]:PCTL */		/* ReSelection */
			{
				spc->Regs[4] |= 0x04;		/* Regs[4]:INTS,  0x04:TimeOut */
				MB89352A_DisConnect(spc);
				break;
			}

			I = spc->Regs[13];			/* Regs[13]:TEMP */
			if ((spc->Phase == SPC_PHASE_BUSFRRE) && (I & spc->Regs[0]) && ((I & spc->Regs[0])!=I))		/* Regs[0]:BDID */
			{
				I &= ~(spc->Regs[0]);
				for (J = 0; J < 8; J++)
				{
					I = I >> 1;
					if (!I)break;
				}


				//if (HDD[0].Data)
				if(HDDStream)
				{
					spc->Regs[4] |= 0x10;			/* Rgs[4]:INTS,  0x10:Command Complete */
					spc->bsy = true;
					spc->Message = 0;
					spc->Counter = -1;
					I = 0;
					if (spc->atn)
					{
						spc->Phase = SPC_PHASE_MSGOUT;
						spc->NextPhase = SPC_PHASE_COMMAND;
						spc->Regs[5] = 0x80 | 0x08 | 0x04 | 0x02;	/* 0x80:REQ,  0x08:BSY(Busy),  0x04:MSG(Message),  0x02:CD(Command),  0x01:IO(0:Data Output) */
					}
				}
				else
				{
					if (Verbose & 0x10)printf("SCSI not active!\n");
					IsError = true;
				}
			}

			//if ((I & spc->Regs[0]) == spc->Regs[0])		/* Regs[0]:BDID */
			//{
			//	I &= ~(spc->Regs[0]);
			//	for (J = 0; J < 8; J++)
			//	{
			//		if (I == (1 << J))
			//		{
			//			spc->TargetId = J;
			//			break;
			//		}
			//	}

			//	if (HDD[0].Data)
			//	{
			//		spc->Regs[4] |= 0x10;			/* Rgs[4]:INTS,  0x10:Command Complete */
			//		spc->bsy = true;
			//		spc->Message = 0;
			//		spc->Counter = -1;
			//		I = 0;
			//		if (spc->atn)
			//		{
			//			spc->Phase = SPC_PHASE_MSGOUT;
			//			spc->NextPhase = SPC_PHASE_COMMAND;
			//			spc->Regs[5] = 0x80 | 0x08 | 0x04 | 0x02;	/* 0x80:REQ,  0x08:BSY(Busy),  0x04:MSG(Message),  0x02:CD(Command),  0x01:IO(0:Data Output) */
			//		}
			//	}
			//	else
			//	{
			//		IsError = true;
			//	}
			//}
			else
			{
				if (Verbose & 0x10)printf("spc select failed Phase:%d  spcReg[0]:%d  spcReg[13]:%d \n", spc->Phase, spc->Regs[0], spc->Regs[13]);
				IsError = true;
				spc->TargetId = -1;
			}

			if (IsError)
			{
				if (Verbose & 0x10)printf("spc select failed!\n");
				spc->Regs[4] |= 0x04;		/* Regs[4]:INTS,  0x04:TimeOut */
				MB89352A_DisConnect(spc);
			}
			break;

		case 2:		/* Reset ATN */
			spc->atn = false;
			break;

		case 3:		/*Set ATN*/
			spc->atn = true;
			break;

		case 4:		/* Transfer */
			if ((spc->Regs[14] == ((spc->Regs[5]) & 0x07)) && (spc->Regs[5] & (0x80 | 0x08)))spc->TransActive = 1; /*  0x80:REQ,  0x08:BSY(Busy) */
			else spc->Regs[4] |= 0x08;		/* Regs[4]:INTS,  0x08:Service Requited */
			break;

		case 5:		/* Transfer Pause */
			break;

		case 6:		/* Reset ACK/REQ */
			MB89352A_ResetACKREQ(spc);
			break;

		case 7:		/* Set ACK/REQ */
			MB89352A_SetACKREQ(spc, &V);
			break;

		default:
			break;
		}
		break;

	case 3:		/* Reserved */
		break;

	case 4:		/* INTS */
		spc->Regs[4] &= ~V;
		if (spc->rst)spc->Regs[4] |= 0x01;		/* 0x01: Reset Condition */
		break;

	case 5:		/* SDGC */
		spc->Regs[R] = V;
		break;

	case 6:		/* SSTS(Read Only) */
		break;

	case 7:		/* SERR(Read Only) */
		break;

	case 8:		/* PCTL */
		spc->Regs[R] = V;
		spc->Regs[14] = V;
		break;

	case 9:		/* MBC(Read Only) */
		break;

	case 10:		/* Regs[10]:DREG */
		if ((spc->TransActive) && (spc->tc > 0))
		{
			MB89352A_SetACKREQ(spc, &V);
			MB89352A_ResetACKREQ(spc);

			--spc->tc;
			if (!spc->tc)
			{
				spc->TransActive = 0;
				spc->Regs[4] |= 0x10;		/* Regs[4]:INTS,  0x10:Command Complete */
			}
			spc->Regs[9] = (spc->Regs[9] - 1) & 0x0F;		/* Regs[9]:MBC */
		}
		break;
	case 11:		/* Regs[11]:TEMP */
		spc->Regs[13] = V;
		break;

	case 12:		/* Regs[12]:TCH */
		spc->tc = (spc->tc&0x0000FFFF)+ (V<<16);
		break;

	case 13:		/* Regs[13]:TCM */
		spc->tc = (spc->tc & 0x00FF00FF) + (V << 16);
		break;

	case 14:		/* Regs[14]:TCL */
		spc->tc = (spc->tc & 0x00FFFF00) + V;
		break;

	case 15:		/* Reserved */
		break;

	default:
		spc->Regs[R] = V;
		break;
	}
}


unsigned char MB89352A_ReadDREG(MB89352A* spc)
{
	if ((spc->TransActive) && (spc->tc > 0))
	{
		MB89352A_SetACKREQ(spc, (unsigned char*)&spc->Regs[10]);		/* Regs[10]:DREG */
		MB89352A_ResetACKREQ(spc);

		--spc->tc;
		if (!spc->tc)
		{
			spc->TransActive = 0;
			spc->Regs[4] |= 0x10;		/* Regs[4]:INTS,  0x10:Command Complete */
		}
		spc->Regs[9] = (spc->Regs[9] - 1) & 0x0F;		/* Regs[9]:MBC */
		return spc->Regs[10];			/* Regs[10]:DREG */
	}
	else
	{
		return 0xFF;
	}
}


void MB89352A_WriteDREG(MB89352A* spc, unsigned char V)
{
	if ((spc->TransActive) && (spc->tc > 0))
	{
		MB89352A_SetACKREQ(spc, &V);
		MB89352A_ResetACKREQ(spc);
	}

	--spc->tc;
	if (!spc->tc)
	{
		spc->TransActive = 0;
		spc->Regs[4] |= 0x10;		/* Regs[4]:INTS,  0x10:Command Complete */
	}
	spc->Regs[9] = (spc->Regs[9] - 1) & 0x0F;		/* Regs[9]:MBC */
}



void MB89352A_SetACKREQ(MB89352A* spc, unsigned char* val)
{
	/* REQ check */
	if (spc->Regs[5] & (0x80 | 0x08) != (0x80 | 0x08))	/* Regs[5]:PSNS,  0x80:REQ(Requite),  0x08:BSY(Busy) */
	{
		if (spc->Regs[5] & 0x01)	/* 0x01:IO(SCSI -> SPC) */
		{
			*val = 0xFF;
		}
		return;
	}

	/* Phase Check */
	if (spc->Regs[14] != (spc->Regs[5] & 0x07))		/* Regs[5]:PSNS,  Regs[14]:TCL */
	{
		if (spc->Regs[5] & 0x01)	/* 0x01:IO(SCSI -> SPC) */
		{
			*val = 0xFF;
		}
		if (spc->TransActive)
		{
			spc->Regs[4] |= 0x08;	/* Regs[4]:INTS   0x08:Service Requited */
		}
		return;
	}

	switch (spc->Phase)
	{
	case SPC_PHASE_DATAIN:		/* Transfer Phase(Data In) */
		//*val = spc->Buf[spc->Offset];
		//++spc->Offset;
		*val = *spc->CurrBuf;
		++spc->CurrBuf;
		spc->Regs[5] = 0x40 | 0x08 | 0x01;	/* 0x40:ACK,  0x08:BSY(Busy),  0x01:IO(1:DataIn) */
		break;
	case SPC_PHASE_DATAOUT:		/* Transfer Phase(Data Out) */
		//spc->Buf[spc->Offset] = *val;
		//++spc->Offset;
		*spc->CurrBuf = *val;
		++spc->CurrBuf;
		spc->Regs[5] = 0x40 | 0x08 | 0x00;	/* 0x40:ACK,  0x08:BSY(Busy),  0x01:IO(0:DataOut) */
		break;
	case SPC_PHASE_COMMAND:		/* Command Phase */
		if (spc->Counter < 0)
		{
			/* Initialize Command */
			spc->CmdOffset = 0;
			spc->Counter = (*val < 20) ? 6 : ((*val < 0xA0) ? 10 : 12);
		}
		spc->CmdData[spc->CmdOffset] = *val;
		++spc->CmdOffset;
		spc->Regs[5] = 0x40 | 0x08 | 0x02;	/* 0x40:ACK,  0x08:BSY(Busy),  0x02:CD(Command) */
		break;
	case SPC_PHASE_STATUS:		/* Status Phase */
		*val = 2;	/* TODO: Chnage value when CD-ROM. */
		spc->Regs[5] = 0x40 | 0x08 | 0x02 | 0x01;	/* 0x40:ACK,  0x08:BSY(Busy),  0x02:CD(Command),  0x01:IO(1:Data Input) */
		break;
	case SPC_PHASE_MSGIN:		/* Message In */
		*val = spc->Message;
		spc->Regs[5] = 0x40 | 0x08 | 0x04 | 0x02 | 0x01;	/* 0x40:ACK,  0x08:BSY(Busy),  0x04:MSG(Message),  0x02:CD(Command),  0x01:IO(1:Data Input) */
		break;
	case SPC_PHASE_MSGOUT:		/* Message Out */
		spc->Message = *val;
		spc->Regs[5] = 0x40 | 0x08 | 0x04 | 0x02 | 0x00;	/* 0x40:ACK,  0x08:BSY(Busy),  0x04:MSG(Message),  0x02:CD(Command),  0x01:IO(0:Data Output) */
		break;
	default:
		break;
	}
}


void MB89352A_ResetACKREQ(MB89352A* spc)
{
	int I;

	/* ACK Check */
	if ((spc->Regs[5] & (0x40 | 0x08)) != (0x40 | 0x08))	/* Regs[5]:PSNS,  0x40:ACK,  0x08:BSY(Busy) */
	{
		return;
	}

	/* Phase Check */
	if (spc->Regs[14] != (spc->Regs[5] & 0x07))		/* Regs[5]:PSNS,  Regs[14]:TCL */
	{
		if (spc->TransActive)
		{
			spc->Regs[4] |= 0x08;	/* Regs[4]:INTS,   0x08:Service Requited */
		}
		return;
	}

	switch (spc->Phase)
	{
	case SPC_PHASE_DATAIN:		/* Transfer Phase(Data In) */
		if (--spc->Counter > 0)
		{
			spc->Regs[5] = 0x80 | 0x08 | 0x01;		/* Regs[5]:PSNS,  0x80:REQ,  0x08:BSY(Busy),  0x01:IO(1:Data Input)*/
		}
		else
		{
			if (spc->Blocks > 0)
			{
				if (spc->Type == 1)		/* 1: Hard Disk Drive */
				{
					spc->Counter = HDDReadSector_SCSI(spc);
					if (spc->Counter)
					{
						spc->Regs[5] = 0x80 | 0x08 | 0x01;		/* Regs[5]:PSNS,  0x80:REQ,  0x08:BSY(Busy),  0x01:IO(1:Data Input)*/
						spc->CurrBuf = spc->Buf;
						break;
					}
					spc->Regs[5] = 0x80 | 0x08 | 0x03;		/* Regs[5]:PSNS, 0x80:ACK,  0x08:BSY(Busy),  0x02:CD(Command),  0x01:IO(1:Data Input) */
					spc->Phase = SPC_PHASE_STATUS;
				}
			}
		}
		break;

	case SPC_PHASE_DATAOUT:		/* Transfer Phase(Data out) */
		if (--spc->Counter > 0)
		{
			spc->Regs[5] = 0x80 | 0x08 | 0x00;		/* Regs[5]:PSNS,  0x80:REQ,  0x08:BSY(Busy),  0x01:IO(0:Data Output)*/
		}
		else
		{
			if (spc->Blocks > 0)
			{
				if (spc->Type == 1)		/* 1: Hard Disk Drive */
				{
					spc->Counter = HDDWriteSector_SCSI(spc);
					if (spc->Counter)
					{
						spc->Regs[5] = 0x80 | 0x08 | 0x00;		/* Regs[5]:PSNS,  0x80:REQ,  0x08:BSY(Busy),  0x01:IO(0:Data Output)*/
						spc->CurrBuf = spc->Buf;
						break;
					}
					spc->Regs[5] = 0x80 | 0x08 | 0x03;		/* Regs[5]:PSNS, 0x80:REQ,  0x08:BSY(Busy),  0x02:CD(Command),  0x01:IO(1:Data Input) */
					spc->Phase = SPC_PHASE_STATUS;
				}
			}

		}
		break;

	case SPC_PHASE_COMMAND:		/* Command Phase */
		if (--spc->Counter > 0)
		{
			spc->Regs[5] = 0x80 | 0x08 | 0x02;		/* Regs[5]:PSNS,  0x80:REQ,  0x08:BSY(Busy),  0x02:CD(Command) */
		}
		else
		{
			spc->CurrBuf = spc->Buf;
			//spc->CDROMBusy = 1;	/* To written later for CD-ROM */
			spc->Message = 0;
			spc->Phase = SPC_PHASE_STATUS;
			spc->Blocks = 0;

			if (spc->CmdData[0] != 0x03)spc->KeyCode = 0;	/* 0x03:REQUEST SENSE */

			if (spc->CmdData[0] < 0x20)
			{
				spc->Sector = ((spc->CmdData[1] & 0x1F) << 16) || (spc->CmdData[2] << 8) || (spc->CmdData[3]);
				spc->Length = spc->CmdData[4];
				switch (spc->CmdData[0])
				{
				case 0x00:		/* TEST UNIT READY */
					if (spc->isChanged)
					{
						spc->KeyCode = 0x62900;		/* 0x62900:Unit Attention - POR or device reset occurred */
					}
					spc->Counter = 0;
					break;

				case 0x03:		/* REQUEST SENSE */
					I = SCSI_REQUEST_SENSE(spc);
					if (I)spc->Phase = SPC_PHASE_DATAIN;
					spc->Counter = I;
					break;

				case 0x04:
					memset(spc->Buf, 0, 512);
					if (HDDWrite_SCSI(spc->Buf, 0, 1))
					{
						spc->rst = true;
						spc->isChanged = true;
					}
					else
					{
						spc->KeyCode = 0x40300;		/* 0x40300:Hardware Error - write fault */
					}
					break;

				case 0x08:		/* READ(6) */
					if (!spc->Length)
					{
						spc->Length = 256;
					}
					if (SCSICheckAddress(spc))
					{
						I = HDDReadSector_SCSI(spc);
						if (I)
						{
							spc->CmdData[0] = 0x28;		/* 0x28:READ(10) */
							spc->Phase = SPC_PHASE_DATAIN;
							spc->Counter = I;
						}
					}
					else
					{
						spc->Counter = 0;
					}
					break;

				case 0x0A:		/* WRITE(6) */
					if (!spc->Length)spc->Length = 256;
					if (SCSICheckAddress(spc))
					{
						if (spc->Length >= 128)
						{
							spc->Blocks = spc->Length - 128;
							I = 0x10000;
						}
						else
						{
							I = spc->Length * 512;
						}
						spc->CmdData[0] = 0x2A;		/* 0x2A:WRITE(10) */
						spc->Phase = SPC_PHASE_DATAOUT;
						spc->Counter = I;
					}
					spc->Counter = 0;
					break;

				case 0x0B:		/* SEEK(6) */
					spc->Length = 1;
					SCSICheckAddress(spc);
					spc->Counter = 0;
					break;

				case 0x1A:		/* MODE SENSE */
					I = SCSIModeSense(spc);
					if (I)spc->Phase = SPC_PHASE_DATAIN;
					spc->Counter = I;
					break;

				case 0x1B:		/* START STOP UNIT */
					if (spc->CmdData[4] == 2)		/* Eject */
					{
						HDD[0].Data = 0;
					}
					break;

				case 0x01:		/* REZERO */
				case 0x07:		/* REASSIGN BLOCKS */
				case 0x16:		/* RESERVE UNIT */
				case 0x17:		/* RELEASE UNIT */
				case 0x1D:		/* SEND DIAGNOSTIC */
					spc->Counter = 0;
					break;

				default:
					spc->KeyCode = 0x52000;		/* 0x52000:Illegal Request - invalid/unsupported command code */
					spc->Counter = 0;
					break;
				}
			}
			else
			{
				spc->Sector = (spc->CmdData[2] << 24) | (spc->CmdData[3] << 16) | (spc->CmdData[4] << 8) | (spc->CmdData[5]);
				spc->Length = (spc->CmdData[7] << 8) + spc->CmdData[8];
				switch (spc->CmdData[0])
				{
				case 0x25:		/* READ CAPACITY */
					if(spc->Type==1)I = HDD[0].DataSize / 512;		/* Type 1:Hard Disk Drive */
					if (!I)
					{
						spc->KeyCode = 0x23A00;			/* 	0x23A00:Not Ready - medium not present */
						spc->Counter = 0;
					}
					else
					{
						--I;
						spc->Buf[0] = (unsigned char)((I >> 24) & 0xFF);
						spc->Buf[1] = (unsigned char)((I >> 16) & 0xFF);
						spc->Buf[2] = (unsigned char)((I >> 8) & 0xFF);
						spc->Buf[3] = (unsigned char)(I & 0xFF);
						spc->Buf[4] = 0;
						spc->Buf[5] = 0;
						spc->Buf[6] = 2;		/* 512(SectorSize)>>8 */
						spc->Buf[7] = 0;
						spc->Counter = 8;
					}
					break;

					case 0x28:		/* READ(10) */
						if (SCSICheckAddress(spc))
						{
							I = HDDReadSector_SCSI(spc);
							if (I)
							{
								spc->Phase = SPC_PHASE_DATAIN;
								spc->Counter = I;
							}
						}
					break;

					case 0x2A:		/* WRITE(10) */
						if (SCSICheckAddress(spc))
						{
							if (spc->Length >= 128)
							{
								spc->Blocks = spc->Length - 128;
								spc->Counter = 0x10000;
							}
							else
							{
								spc->Counter = spc->Length * 512;
							}
							spc->Phase = SPC_PHASE_DATAOUT;
							spc->Counter = I;
						}
						spc->Counter = 0;
						break;

					case 0x2B:		/* SEEK(10) */
						spc->Length = 1;
						SCSICheckAddress(spc);
						spc->Counter = 0;
						break;
					default:
						spc->KeyCode = 0x52000;		/* 0x52000:Illegal Request - invalid/unsupported command code */
						spc->Counter = 0;
					break;
				}
			}
		}

		switch (spc->Phase)
		{
		case SPC_PHASE_DATAIN:
			spc->Regs[5] = 0x80 | 0x08 | 0x01;		/* Regs[5]:PSNS,  0x80:REQ,  0x08:BSY(Busy),  0x01:IO(1:Data In)*/
			break;
		case SPC_PHASE_DATAOUT:
			spc->Regs[5] = 0x80 | 0x08 | 0x00;		/* Regs[5]:PSNS,  0x80:REQ,  0x08:BSY(Busy),  0x01:IO(0:Data In)*/
			break;
		case SPC_PHASE_STATUS:
			spc->Regs[5] = 0x80 | 0x08 | 0x02 | 0x01;		/* Regs[5]:PSNS,  0x80:REQ,  0x08:BSY(Busy),  0x02:CD(Command), 0x01:IO(1:Data In)*/
			break;
		case SPC_PHASE_EXECUTE:
			spc->Regs[5] = 0x08;		/* Regs[5]:PSNS, 0x08:BSY(Busy) */
			break;
		default:
			break;
		}
		break;

		case SPC_PHASE_STATUS:
			spc->Regs[5] = 0x80 | 0x08 | 0x04 |0x02 | 0x01;	/* Regs[5]:PSNS,  0x80:REQ,  0x08:BSY(Busy), 0x04:MSG, 0x02:CD(Command), 0x01:IO(1:Data In)*/
			spc->Phase = SPC_PHASE_MSGIN;
			break;

		case SPC_PHASE_MSGIN:
			if (spc->Message <= 0)
			{
				MB89352A_DisConnect(spc);
				break;
			}
			spc->Message = 0;
			/* Continue with MSGOUT */

		case SPC_PHASE_MSGOUT:
			if (spc->Message == -1)
			{
				MB89352A_DisConnect(spc);
				return;
			}

			if (spc->atn)
			{
				if (spc->Message & 0x02)
				{
					MB89352A_DisConnect(spc);
					return;
				}
				spc->Regs[5] = 0x80 | 0x08 | 0x04 | 0x02;	/* Regs[5]:PSNS,  0x80:REQ,  0x08:BSY(Busy), 0x04:MSG, 0x02:CD(Command), 0x01:IO(0:Data Out)*/
			}

			if (spc->Message & 0x01)
			{
				spc->Phase = SPC_PHASE_MSGIN;
			}
			else
			{
				if (spc->Message & 0x04)
				{
					spc->Phase = SPC_PHASE_STATUS;
					spc->NextPhase = -1;
				}
				else
				{
					spc->Phase = spc->NextPhase;
					spc->NextPhase = -1;
				}
			}

			spc->Message = 0;

			switch (spc->Phase)
			{
			case SPC_PHASE_COMMAND:
				spc->Regs[5] = 0x80 | 0x08 | 0x02;	/* Regs[5]:PSNS,  0x80:REQ,  0x08:BSY(Busy), 0x02:CD(Command) */
				break;
			case SPC_PHASE_DATAIN:
				spc->Regs[5] = 0x80 | 0x08 | 0x01;	/* Regs[5]:PSNS,  0x80:REQ,  0x08:BSY(Busy), 0x01:IO(1:Data In) */
				break;
			case SPC_PHASE_DATAOUT:
				spc->Regs[5] = 0x80 | 0x08 | 0x01;	/* Regs[5]:PSNS,  0x80:REQ,  0x08:BSY(Busy), 0x00:IO(1:Data Out) */
				break;
			case SPC_PHASE_STATUS:
				spc->Regs[5] = 0x80 | 0x08 | 0x02 | 0x01;	/* Regs[5]:PSNS,  0x80:REQ,  0x08:BSY(Busy), 0x02:CD(Command), 0x01:IO(1:Data In)*/
				break;
			case SPC_PHASE_MSGIN:
				spc->Regs[5] = 0x80 | 0x08 | 0x04 | 0x02 | 0x01; /* Regs[5]:PSNS,  0x80:REQ,  0x08:BSY(Busy), 0x04:MSG, 0x02:CD(Command), 0x01:IO(1:Data In)*/
				break;
			default:
				break;
			}
			return;

		default:
			break;
	}

	if (spc->atn)
	{
		spc->NextPhase = spc->Phase;
		spc->Phase = SPC_PHASE_MSGOUT;
		spc->Regs[5] = 0x80 | 0x08 | 0x04 | 0x02 | 0x00; /* Regs[5]:PSNS,  0x80:REQ,  0x08:BSY(Busy), 0x04:MSG, 0x02:CD(Command), 0x00:IO(0:Data Out)*/
	}
}


int SCSI_REQUEST_SENSE(MB89352A* spc)
{
	int KeyCode;

	if (spc->rst)
	{
		spc->rst = 0;
		KeyCode = 0x62900;		/* 0x62900 : POWER ON */
	}
	else
	{
		KeyCode = spc->KeyCode;
	}

	spc->KeyCode = 0;

	memset(spc->Buf + 1, 0, 17);
	if (spc->Length == 0)return 0;	/* In SCSI2(MEGASCSI), Return if length is 0. */
	else
	{
		spc->Buf[0] = 0x70;
		spc->Buf[2] = (unsigned char)((KeyCode >> 16) & 0xFF);		/* Sense Key */
		spc->Buf[7] = 10;											/* Additional sense length */
		spc->Buf[12] = (unsigned char)((KeyCode >> 8) & 0xFF);		/* Additional sense code  */
		spc->Buf[13] = (unsigned char)(KeyCode & 0xFF);				/* Additional sense code qualifier */
		if (spc->Length > 18)return 18;
	}
	return spc->Length;
}


int SCSICheckAddress(MB89352A* spc)
{
	int I = HDD[0].Data ? HDD[0].DataSize / 512 : 0;
	if (!I)
	{
		spc->KeyCode = 0x23A00;		/* 0x23A00:Not Ready - medium not present */
		return 0;
	}

	if ((spc->Sector >= 0) && (spc->Length > 0) && (spc->Sector + spc->Length <= I))
	{
		return 1;
	}
	spc->KeyCode = 0x52100;		/* 0x52100:Illegal Request - LBA out of range */
	return 0;
}


int SCSIModeSense(MB89352A* spc)
{
	if ((spc->Length > 0) && (spc->CmdData[2] == 3))
	{
		unsigned char* Buf = spc->Buf + 0;
		int Total = HDD[0].Data ? HDD[0].DataSize / 512 : 0;
		int Media = 0x00;
		int Size = 28;

		memset(Buf + 2, 0, 34);

		if (!Total)Media = 0x70;		/* 0x70:No Disk */

		Buf[1] = Media;				/* Media Type */
		Buf[3] = 8;					/* Block descripter length */

		if (!(spc->CmdData[1] & 0x08))
		{
			Buf[5] = (unsigned char)((Total >> 16) & 0xFF);
			Buf[6] = (unsigned char)((Total >> 8) & 0xFF);
			Buf[7] = (unsigned char)(Total & 0xFF);

			Buf[9] = 0;		/* (SectorSize>>8) >>16 */
			Buf[10] = 0;	/* (SectorSize>>8)>>8 */
			Buf[11] = 2;	/* (SectorSize>>8) */
			Size += 8;
		}

		Buf[12] = 0x03;		/* Format Page 0 code length */
		Buf[13] = 0x16;		/* Format Page 1 code length */
		Buf[15] = 8;		/* Tracks */
		Buf[23] = 64;		/* Buf[23], Buf[24]: Sectors per track */
		Buf[24] = 2;		/* Buf[24], Buf[25]: Data bytes per physical sector */
		Buf[32] = 0xA0;		/* Removable. True in MB89352A.(true:0xA0 false:0x80) */

		spc->Buf[0] = Size - 1;
		if (spc->Length > Size)return Size;
		return spc->Length;
	}
	spc->KeyCode = 0x52000;		/* 0x52000:Illegal Request - LBA out of range */
	return 0;
}


unsigned char HDDRead_SCSI(unsigned char* Buf, int Sector, int Length)
{
	if (!HDDStream)return 0;
	if (Verbose & 0x10)printf("HDD Read\n");
	if (fseek(HDDStream, Sector * 512, SEEK_SET)==0)
	{
		return (fread(Buf, 1, Length, HDDStream) == Length);
	}
	return 0;
}


int HDDReadSector_SCSI(MB89352A* spc)
{
	bool isReadError = false;
	int retval;
	int numSectors;

	if (Verbose & 0x10)printf("HDD Read Sector\n");

	if (spc->Length >= 128)
	{
		numSectors = 128;
		retval = 0x10000;		/* numSectors*512 */
	}
	else
	{
		numSectors = spc->Length;
		retval = spc->Length * 512;
	}
	if (!HDDStream)isReadError = true;
	else if (fseek(HDDStream, spc->Sector * 512, SEEK_SET) != 0)isReadError = true;
	else if (fread(spc->CurrBuf, 1, numSectors*512, HDDStream) != numSectors*512)isReadError = true;
	if (isReadError)
	{
		spc->Blocks = 0;
		return 0;
	}
	spc->Sector += numSectors;
	spc->Length -= numSectors;
	spc->Blocks = spc->Length;
	return retval;
}


unsigned char HDDWrite_SCSI(unsigned char* Buf, int Sector, int Length)
{
	if (!HDDStream)return 0;
	if (fseek(HDDStream, Sector * 512, SEEK_SET) == 0)
	{
		return (fwrite(Buf, 1, Length, HDDStream) == Length);
	}
	return 0;
}


int HDDWriteSector_SCSI(MB89352A* spc)
{
	bool isWriteError = false;
	int retval;
	int numSectors;

	if (spc->Length >= 128)numSectors = 128;
	else numSectors = spc->Length;

	if (!HDDStream)isWriteError = true;
	else if (fseek(HDDStream, spc->Length * 512, SEEK_SET) != 0)isWriteError = true;
	else if (fwrite(spc->CurrBuf, 1, spc->Blocks, HDDStream) != spc->Blocks)isWriteError = true;
	if (isWriteError)
	{
		spc->Blocks = 0;
		return 0;
	}
	spc->Sector += numSectors;
	spc->Length -= numSectors;

	if (spc->Length >= 128)
	{
		spc->Blocks = spc->Length - 128;
		retval = 0x10000;		/* numSectors*512 */
	}
	else
	{
		spc->Blocks = 0;
		retval = spc->Length * 512;
	}
	return retval;
}


//unsigned char* LinearHDD_MB89352A(unsigned char ID, int SectorN, unsigned char PerTrack, unsigned char Heads)
//{
//	if (SectorN < 0) return(0);
//	else
//	{
//		int Sector = SectorN % PerTrack;
//		int Track = SectorN / PerTrack >> (Heads - 1);
//		int Side = (SectorN / PerTrack) & (Heads - 1);
//		return(SeekFDI(&HDD[0], Side, Track, Side, Track, Sector + 1));
//	}
//}

#endif // MEGASCSI_HD