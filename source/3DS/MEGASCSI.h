#ifndef MB89352A_H
#define MB89352A_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef MEGASCSI_HD

#define SPC_PHASE_BUSFRRE		0
#define	SPC_PHASE_ARBITRATION	1
#define SPC_PHASE_SELECTION		2
#define SPC_PHASE_RESELECTION	3
#define SPC_PHASE_COMMAND		4		
#define SPC_PHASE_EXECUTE		5
#define SPC_PHASE_DATAIN		6
#define SPC_PHASE_DATAOUT		7
#define SPC_PHASE_STATUS		8
#define SPC_PHASE_MSGOUT		9
#define SPC_PHASE_MSGIN			10

typedef struct
{
	unsigned char Id;			/* SCSI ID 0..7	*/
	unsigned char TargetId;		/* Target ID 0..7	*/
	int Regs[16];				/* SPC register	*/
	unsigned char CmdData[13];	/* Command Data	*/
	unsigned char bsy;					/* Busy Signal	*/
	unsigned char sel;					/* Select Signal	*/
	unsigned char atn;					/* Attention Signal	*/
	unsigned char msg;					/* Message Signal	*/
	unsigned char cd;					/* Command/Data Signal	*/
	unsigned char io;					/* Input/Output Signal	*/
	unsigned char req;					/* Request Signal	*/
	unsigned char ack;					/* Ack Signal	*/
	unsigned char rst;					/* Reset Signal	*/
	unsigned char isChanged;				/* Is SCSI device changed?  */
	unsigned char isEnabled;				/* spc enable flag */
	unsigned char Phase;		/* SCSI Phase	*/
	char NextPhase;	/* Next SCSI Phase */
	unsigned int Message;		/* SCSI Message */
	int Type;					/* SCSI Type.  1: Hard Disk Drive */

	unsigned int tc;			/* Counter for hardware counter */
	unsigned char TransActive;	/* 1:transfer active */
	unsigned char* Buf;			/* Buffer for transfer start pointer */
	unsigned char* CurrBuf;		/* Buffer for transfer*/
	unsigned int Blocks;		/* Number of blocks outside buffer */
	unsigned int Offset;		/* Buffer Offset */
	unsigned int CmdOffset;		/* Command Offset */
	unsigned int Length;		/* Read and Written number */
	unsigned int Sector;		/* Current Sector */
	int Counter;				/* SCSI counter */
	int	KeyCode;				/* Sense Key, ASC, ASCQ */

	unsigned char Verbose;
}MB89352A;

void MB89352A_Init(MB89352A* spc);
void MB89352A_DisConnect(MB89352A* spc);
void MB89352A_SoftReset(MB89352A* spc);
void MB89352A_Reset(MB89352A* spc);
unsigned char MB89352A_ReadReg(MB89352A* spc, unsigned char R);
void MB89352A_WriteReg(MB89352A* spc, unsigned char R, unsigned char V);
unsigned char MB89352A_ReadDREG(MB89352A* spc);
void MB89352A_WriteDREG(MB89352A* spc, unsigned char V);
void MB89352A_SetACKREQ(MB89352A* spc, unsigned char* val);
void MB89352A_ResetACKREQ(MB89352A* spc);
int SCSI_REQUEST_SENSE(MB89352A* spc);
int SCSICheckAddress(MB89352A* spc);
int SCSIModeSense(MB89352A* spc);
unsigned char HDDRead_SCSI(unsigned char* Buf, int Sector, int Length);
int HDDReadSector_SCSI(MB89352A* spc);
unsigned char HDDWrite_SCSI(unsigned char* Buf, int Sector, int Length);
int HDDWriteSector_SCSI(MB89352A* spc);

#endif // MEGASCSI_HD

#ifdef __cplusplus
}
#endif
#endif /* MB89352A_H */