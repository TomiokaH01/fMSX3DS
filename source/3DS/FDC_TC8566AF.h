#ifndef TC8566AF_H
#define TC8566AF_H

#include "FDIDisk.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CMD_UNKNOWN					0
#define CMD_READ_DATA				1
#define	CMD_WRITE_DATA				2
#define CMD_WRITE_DELETED_DATA		3
#define	CMD_READ_DELETED_DATA		4
#define	CMD_READ_DIAGNOSTIC			5
#define CMD_READ_ID					6
#define CMD_FORMAT					7
#define	CMD_SCAN_EQUAL				8
#define CMD_SCAN_LOW_OR_EQUAL		9
#define CMD_SCAN_HIGH_OR_EQUAL		10
#define CMD_SEEK					11
#define CMD_RECALIBRATE				12
#define CMD_SENSE_INTERRUPT_STATUS	13
#define CMD_SPECIFY					14
#define CMD_SENSE_DEVICE_STATUS		15

#define TC8566AF_KEEP    0
#define TC8566AF_INIT    1
#define TC8566AF_EJECT   2

#define PHASE_IDLE			0
#define PHASE_COMMAND		1
#define PHASE_DATATRANSFER	2
#define PHASE_RESULT		3

typedef struct
{
	unsigned char Status[4];		/* Result Status Registers	*/
	unsigned char MainStatus;		/* Status Register			*/
	unsigned char ReadOnly;			/* Is write protected?		*/
	unsigned char Drive;			/* Current disk #			*/
	unsigned char Side;				/* Current side #			*/
	unsigned char CylinderNumber;	/* Current cylinder #		*/
	unsigned char SectorNumber;		/* Current Sector Number #	*/
	unsigned char OldSide;
	unsigned char OldCylinderNumber;
	unsigned char OldSectorNumber;
	unsigned char DummyWrite;
	unsigned char SectorsPerCylinder;
	unsigned char FillerByte;
	unsigned char CurrTrack;		/* Current track #			*/
	unsigned char Number;
	unsigned char Wait;				/* Expiration counter		*/
	unsigned char Cmd;				/* Last command				*/
	unsigned char CommandCode;		/* Last Command reg value	*/
	unsigned char Phase;			/* FDC Phase. (IDLE, COMMAND, DATATRANSFER,RESULT)  */
	unsigned char PhaseStep;

	unsigned char* Ptr;				/* Pointer to data */

	unsigned char Verbose;

	int SectorOffset;				/* current position of sector. */
	FDIDisk* Disk[4]; /* Disk images */
}TC8566AF;

void ResetTC8566AF(register TC8566AF* D, FDIDisk* Disks, register unsigned char Eject);

unsigned char ReadTC8566AF(register TC8566AF* D, register unsigned char A);

void WriteTC8566AF(register TC8566AF* D, register unsigned char A, register unsigned char V);

#ifdef __cplusplus
}
#endif
#endif /* TC8566AF_H */