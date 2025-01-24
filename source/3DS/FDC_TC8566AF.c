/* TC8566AF FDC driver emulation.					 */
/* Based on uPD765.c from EMULib by Marat Fayzullin. */
/* https://fms.komkon.org/EMUL8/ */
/* and tc8566af.hpp from micro MSX2+ by Yoji Suzuki. */
/* https://github.com/suzukiplan/micro-msx2p */
/* and TC8566AF.c from blueMSX by Daniel Vik. */
/* http://bluemsx.msxblue.com/ */
/* based on infomation at MSX techenical guide book disk edition. */
/* http://www.ascat.jp/tg/tgdindex.html */

/* h.tomioka 2024 */

#include "FDC_TC8566AF.h"
#include <stdio.h>
#include <string.h>

#include "MSX.h"
#include "3DSLib.h"
#include "3DSConfig.h"

void ResetTC8566AF(register TC8566AF* D, FDIDisk* Disks, register unsigned char Eject)
{
	int J;
	D->Status[0] = 0;
	D->Status[1] = 0;
	D->Status[2] = 0;
	D->Status[3] = 0;
    D->MainStatus = 0x20;   /* 0x20:NDM(Non DMA Mode) */
    //D->MainStatus = 0;   /* 0x20:NDM(Non DMA Mode) */
	D->Drive = 0;
	D->Side = 0;
    D->CylinderNumber = 0;
    D->SectorNumber = 0;
	D->Wait = 0;
	D->Cmd = 0;
    D->CurrTrack = 0;
    D->SectorsPerCylinder = 0;
    D->Number = 0;
    D->CommandCode = 0;
    D->Phase = PHASE_IDLE;
    D->PhaseStep = 0;

    D->Verbose = Verbose&0x04;

    /* For all drives... */
    for (J = 0; J < 4; ++J)
    {
        /* Reset drive-dependent state */
        D->Disk[J] = Disks ? &Disks[J] : 0;
        /* Initialize disk structure, if requested */
        if ((Eject == TC8566AF_INIT) && D->Disk[J])  InitFDI(D->Disk[J]);
        /* Eject disk image, if requested */
        if ((Eject == TC8566AF_EJECT) && D->Disk[J]) EjectFDI(D->Disk[J]);
    }
}


unsigned char ReadTC8566AF(register TC8566AF* D, register unsigned char A)
{
    unsigned char retval;
    switch (A)
    {
    case 4:
        if (!(D->MainStatus & 0x80))D->MainStatus |= 0x80;      /* 0x80:RQM(Request for Master) */
        return((D->MainStatus & ~0x20) | (D->Phase == PHASE_DATATRANSFER ? 0x20 : 0));  /* 0x20:NDM(Non DMA Mode) */
    case 5:
        switch (D->Phase)
        {
        case PHASE_DATATRANSFER:
            retval = 0xFF;
            if (D->Cmd == CMD_READ_DATA)
            {
                //if (D->Status[0] & 0x40)
                //{
                //    D->Phase = PHASE_RESULT;
                //    D->PhaseStep = 0;
                //    D->SectorOffset = 0;
                //    D->MainStatus &= 0x7F;
                //    return retval;
                //}
                if (!D->Ptr)
                {
                    D->Phase = PHASE_RESULT;
                    D->PhaseStep = 0;
                    D->SectorOffset = 0;
                }
                else if (D->SectorOffset < 512)
                {
                    /* Read data */
                    retval = D->Ptr[D->SectorOffset];
                    D->SectorOffset++;
                    if (D->SectorOffset == 512)
                    {
                        if (isLoadDer)
                        {
                            int J = D->SectorNumber - 1 + D->Disk[D->Drive]->Sectors * (D->CurrTrack * D->Disk[D->Drive]->Sides + D->Side);
                            if (derBuf[J >> 3] & (0x80 >> (J & 0x07)))
                            {
                                if (Verbose) printf("TC8566AF: ERROR Sector %d (Copy Protected)\n", J);
                                D->Status[0] |= 0x40;   /* 0x40:IC(Interrupt Code) */
                                D->Status[1] |= 0x20;   /* 0x20:DE(Data Error) */
                                D->Status[2] |= 0x20;   /* 0x20:DD(Data Error in Data Field ) */
                                D->Phase = PHASE_RESULT;
                                D->PhaseStep = 0;
                                D->SectorOffset = 0;
                                D->MainStatus &= 0x7F;
                                return retval;
                            }
                        }
                        if (D->Verbose) printf("TC8566AF: DONE reading data\n");
                        D->Phase = PHASE_RESULT;
                        D->PhaseStep = 0;
                        D->SectorOffset = 0;
                    }
                }
            }
            D->MainStatus &= 0x7F;
            return retval;
        case PHASE_RESULT:
            switch (D->Cmd)
            {
            case CMD_READ_DATA:
            case CMD_WRITE_DATA:
            case CMD_FORMAT:
                switch (D->PhaseStep++)
                {
                case 0:
                    return D->Status[0];
                case 1:
                    return D->Status[1];
                case 2:
                    return D->Status[2];
                case 3:
                    return D->CylinderNumber;
                case 4:
                    return D->Side;
                case 5:
                    return D->SectorNumber;
                case 6:
                    D->Phase = PHASE_IDLE;
                    D->MainStatus &= ~0x10; /* 0x10:CB(FDC Busy) */
                    D->MainStatus &= ~0x40; /* 0x40:DIO(Datat Input/Output) */
                    return D->Number;
                }
                break;
            case CMD_SENSE_INTERRUPT_STATUS:
                switch (D->PhaseStep++)
                {
                case 0:
                    return D->Status[0];
                case 1:
                    D->Phase = PHASE_IDLE;
                    D->MainStatus &= ~0x10; /* 0x10:CB(FDC Busy) */
                    D->MainStatus &= ~0x40; /* 0x40:DIO(Datat Input/Output) */
                    return D->CurrTrack;
                }
                break;
            case CMD_SENSE_DEVICE_STATUS:
                switch (D->PhaseStep++)
                {
                case 0:
                    D->Phase = PHASE_IDLE;
                    D->MainStatus &= ~0x10; /* 0x10:CB(FDC Busy) */
                    D->MainStatus &= ~0x40; /* 0x40:DIO(Datat Input/Output) */
                    return D->Status[3];
                    break;
                }
                break;
            }
            break;
        }
        return 0xFF;
        //return 0;
    }
    return 0xFF;
}


/* 0x01:US0(Unit Select), 0x02:US1(Unit Select) , 0x40:IC0(Interrupt Code), 0x80:IC1(Interrupt Code) */
#define SETUP_STATUS0   D->Status[0] &= ~(0x01 | 0x02 | 0x40 | 0x80); \
D->Status[0] |= D->Disk[D->Drive] ? 0 : 0x01;   \
D->Status[0] |= V & (0x01 | 0x02); \
D->Status[0] |= D->Disk[D->Drive] ? 0 : 0x80;
//D->Status[0] |= D->Disk[D->Drive]->Data ? 0 : 0x80;

/* 0x01:US0(Unit Select),0x02:US1(Unit Select), 0x10:TO(Track O), 0x04:HD(Head Address), 0x40:WP(Write Protected), 0x20:RY(Ready) */
#define SETUP_STATUS3   D->Status[3] = V & (0x01 | 0x02);   \
D->Status[3] |= !D->CurrTrack ? 0x10 : 0; \
D->Status[3] |= D->Side ? 0x04 : 0;   \
D->Status[3] |= D->Disk[D->Drive]->Sides>1 ? 0x08 : 0;   \
D->Status[3] |= D->ReadOnly ? 0x40 : 0; \
D->Status[3] |= D->Disk[D->Drive] ? 0x20 : 0;

void WriteTC8566AF(register TC8566AF* D, register unsigned char A, register unsigned char V)
{
    switch (A)
    {
    case 2:
        D->Drive = V & 0x03;
        break;
    case 5:
        switch (D->Phase)
        {
        case PHASE_IDLE:
            D->Cmd = CMD_UNKNOWN;
            if ((V & 0x1F) == 0x06)D->Cmd = CMD_READ_DATA;
            if ((V & 0x3F) == 0x05)D->Cmd = CMD_WRITE_DATA;
            if ((V & 0x3F) == 0x09)D->Cmd = CMD_WRITE_DELETED_DATA;
            if ((V & 0x1F) == 0x0C)D->Cmd = CMD_READ_DELETED_DATA;
            if ((V & 0xBF) == 0x02)D->Cmd = CMD_READ_DIAGNOSTIC;
            if ((V & 0xBF) == 0x0A)D->Cmd = CMD_READ_ID;
            if ((V & 0xBF) == 0x0D)D->Cmd = CMD_FORMAT;
            if ((V & 0x1F) == 0x11)D->Cmd = CMD_SCAN_EQUAL;
            if ((V & 0x1F) == 0x19)D->Cmd = CMD_SCAN_LOW_OR_EQUAL;
            if ((V & 0x1F) == 0x1D)D->Cmd = CMD_SCAN_HIGH_OR_EQUAL;
            if ((V & 0xFF) == 0x0F)D->Cmd = CMD_SEEK;
            if ((V & 0xFF) == 0x07)D->Cmd = CMD_RECALIBRATE;
            if ((V & 0xFF) == 0x08)D->Cmd = CMD_SENSE_INTERRUPT_STATUS;
            if ((V & 0xFF) == 0x03)D->Cmd = CMD_SPECIFY;
            if ((V & 0xFF) == 0x04)D->Cmd = CMD_SENSE_DEVICE_STATUS;
            D->CommandCode = V;
            D->Phase = PHASE_COMMAND;
            D->PhaseStep = 0;
            D->MainStatus |= 0x10;      /* 0x10:CB(FDC Busy) */
            switch (D->Cmd)
            {
            case CMD_READ_DATA:
            case CMD_WRITE_DATA:
            case CMD_FORMAT:
                D->Status[0] &= ~(0x40 | 0x80); /* 0x40:IC0(Interrupt Code 0), 0x80:IC1(Interrupt Code 1) */
                D->Status[1] &= ~(0x04 | 0x02); /* 0x04:ND(No Data), 0x02:NW(Not Writable) */
                D->Status[2] &= ~0x20;  /* 0x20:DD(Data Error in Data Field) */
                break;
            case CMD_RECALIBRATE:
                D->Status[0] &= ~0x20;  /* 0x20:SE(Seek Error) */
                break;
            case CMD_SENSE_INTERRUPT_STATUS:
                D->Phase = PHASE_RESULT;
                D->MainStatus |= 0x40;  /* 0x40:DIO(Data Input/Output) */
                break;
            case CMD_SEEK:
            case CMD_SPECIFY:
            case CMD_SENSE_DEVICE_STATUS:
                //if (!D->Drive)D->MainStatus |= 0x01;
                //else D->MainStatus |= 0x02;
                break;
            default:
                D->MainStatus &= ~0x10;  /* 0x10:CB(FDC Busy) */
                D->Phase = PHASE_IDLE;
                break;
            }
            break;

        case PHASE_COMMAND:
            switch (D->Cmd)
            {
            case CMD_READ_DATA:
            case CMD_WRITE_DATA:
                /* command Setup RW */
                switch (D->PhaseStep++)
                {
                case 0:
                    SETUP_STATUS0
                    SETUP_STATUS3
                    break;
                case 1:
                    D->CylinderNumber = V;
                    break;
                case 2:
                    D->Side = V & 0x01;
                    break;
                case 3:
                    D->SectorNumber = V;
                    break;
                case 4:
                    D->Number = V;
                    D->SectorOffset = (V == 2 && (D->CommandCode & 0xC0) == 0x40) ? 0 : 512;
                    break;
                case  7:
                    /* Temporary fix fof unexpectd PHYDIO write in MSXTurboR.(False detection of RAMDisk?) Need improve. */
                    D->DummyWrite =((D->OldSide == D->Side) && (D->OldCylinderNumber == D->CylinderNumber) && (D->OldSectorNumber == D->SectorNumber)
                        && (D->Cmd == CMD_WRITE_DATA)) ? 1 : 0;
                    D->OldSide = D->Side;
                    D->OldCylinderNumber = D->CylinderNumber;
                    D->OldSectorNumber = D->SectorNumber;
                    if(D->Cmd == CMD_READ_DATA)
                    //if ((D->Cmd == CMD_READ_DATA) || (D->Cmd == CMD_WRITE_DATA))
                    {
                        if (D->Verbose) printf("TC8566AF: Read-SECTOR %c:%d:%d:%d Track:%d\n", D->Drive + 'A', D->Side
                            , D->CylinderNumber, D->SectorNumber, D->CurrTrack);

                        /* Seek to the requested sector */
                        D->Ptr = SeekFDI(D->Disk[D->Drive], D->Side, D->CurrTrack,
                            D->Side, D->CylinderNumber, D->SectorNumber);
                        /* If seek successful, set up reading operation */
                        if (!D->Ptr)
                        {
                            if (D->Verbose) printf("TC8566AF: Read ERROR\n");
                            D->Status[0] |= 0x40;   /* 0x40:IC(Interrupt Code) */
                            D->Status[1] |= 0x04;   /* 0x04:ND(No Data) */
                        }
                        D->MainStatus |= 0x40;  /* 0x40:DIO(Data Input/Output) */
                    }
                    else
                    {
                        if (D->Verbose) printf("TC8566AF: Write-SECTOR %c:%d:%d:%d Track:%d\n", D->Drive + 'A', D->Side
                            , D->CylinderNumber, D->SectorNumber, D->CurrTrack);
                        /* Seek to the requested sector */
                        D->Ptr = SeekFDI(D->Disk[D->Drive], D->Side, D->CurrTrack,
                            D->Side, D->CylinderNumber, D->SectorNumber);
                        /* If seek successful, set up write operation */
                        if (!D->Ptr)
                        {
                            if (D->Verbose) printf("TC8566AF: Write ERROR\n");
                            D->Status[0] |= 0x40;   /* 0x40:IC(Interrupt Code) */
                            D->Status[1] |= 0x04;   /* 0x04:ND(No Data) */
                        }
                        D->MainStatus &= ~0x40;  /* 0x40:DIO(Data Input/Output) */
                    }
                    D->Phase = PHASE_DATATRANSFER;
                    D->PhaseStep = 0;
                    break;
                }
                break;
            case CMD_FORMAT:
                /* command Setup Format */
                switch (D->PhaseStep++)
                {
                case 0:
                    SETUP_STATUS0
                    SETUP_STATUS3
                    break;
                case 1:
                    D->Number = V;
                    break;
                case 2:
                    D->SectorsPerCylinder = V;
                    D->SectorNumber = V;
                    break;
                case 4:
                    D->FillerByte = V;
                    D->SectorOffset = 0;
                    D->Ptr = D->Disk[D->Drive]->Data+0;
                    D->MainStatus &= ~0x40;  /* 0x40:DIO(Data Input/Output) */
                    D->Phase = PHASE_DATATRANSFER;
                    D->PhaseStep = 0;
                    break;
                }
                break;
            case CMD_SEEK:
                /* commandSetupSeek */
                switch (D->PhaseStep++)
                {
                case 0:
                    SETUP_STATUS0
                    SETUP_STATUS3
                    break;
                case 1:
                    D->CurrTrack = V;
                    if (!V)D->Status[3] |= 0x10; else D->Status[3] &= ~0x10;    /* 0x10:T0(Track0) */
                    D->Status[0] |= 0x20;   /* 0x20:SE(Seek End) */
                    D->MainStatus &= ~0x10; /* 0x10:CB(FDC Busy) */
                    //D->MainStatus &= ~0x0F;
                    D->Phase = PHASE_IDLE;
                    break;
                }
                break;
            case CMD_RECALIBRATE:
                /* commandSetupRecalibrate */
                switch (D->PhaseStep++)
                {
                case 0:
                    if (D->Verbose) printf("TC8566AF: RECALIBRATE drive %c:\n", D->Drive + 'A');
                    //D->CurrTrack = 0;
                    SETUP_STATUS0
                    SETUP_STATUS3
                    D->Status[0] |= 0x20;   /* 0x20:SE(Seek End) */
                    D->MainStatus &= ~0x10; /* 0x10:CB(FDC Busy) */
                    D->Phase = PHASE_IDLE;
                    D->CurrTrack = 0;
                    break;
                }
                break;
            case CMD_SPECIFY:
                /* commandSetupSpecify */
                switch (D->PhaseStep++)
                {
                case 1:
                    if (D->Verbose) printf("TC8566AF: SPECIFY %d\n",V);
                    D->MainStatus &= ~0x10; /* 0x10:CB(FDC Busy) */
                    D->Phase = PHASE_IDLE;
                    break;
                }
                break;
            case CMD_SENSE_DEVICE_STATUS:
                if (D->Verbose) printf("TC8566AF: DRIVE-STATUS drive %c:%d\n", D->Drive + 'A', D->Side);
                /* commandSetupSenseDeviceStatus */
                switch (D->PhaseStep++)
                {
                case 0:
                    SETUP_STATUS0
                    SETUP_STATUS3
                    D->Phase = PHASE_RESULT;
                    D->PhaseStep = 0;
                    D->MainStatus |= 0x40;  /* 0x40:DIO(Disk Input/Output) */
                    break;
                }
                break;
            }
            break;

        case PHASE_DATATRANSFER:
            switch (D->Cmd)
            {
            case CMD_WRITE_DATA:
                if(D->SectorOffset<512)
                {
                    /* Write data */
                    D->Ptr[D->SectorOffset] = V;
                    D->SectorOffset++;
                    if (D->SectorOffset == 512)
                    {
                        if (D->Verbose) printf("TC8566AF: DONE writing data\n");
                        D->Ptr = SeekFDI(D->Disk[D->Drive], D->Side, D->CurrTrack,
                            D->Side, D->CylinderNumber, D->SectorNumber);
                        D->Phase = PHASE_RESULT;
                        D->PhaseStep = 0;
                        D->SectorOffset = 0;
                        D->MainStatus |= 0x40;  /* 0x40:DIO(Disk Input/Output) */
                        if(!D->DummyWrite) DiskWrited[D->Drive] = 1;
                        D->DummyWrite = 0;
                    }
                }
                break;
            case CMD_FORMAT:
                switch (D->PhaseStep&0x03)
                {
                case 0:
                    D->CurrTrack = V;
                    break;
                case 1:
                    D->Ptr = LinearFDI(D->Disk[D->Drive], D->SectorNumber - 1 + D->Disk[D->Drive]->Sectors
                        * (D->CurrTrack * D->Disk[D->Drive]->Sides + V));
                    if (D->Ptr)memset(D->Ptr, D->FillerByte, 512);
                    else D->Status[1] |= 0x02;  /* 0x02:NW(Not Writable) */
                    break;
                case 2:
                    D->SectorNumber = V;
                    break;
                default:
                    break;
                }
                if (++D->PhaseStep == 4 * D->SectorsPerCylinder - 2)
                {
                    if (D->Verbose) printf("TC8566AF: DONE format disk\n");
                    D->Phase = PHASE_RESULT;
                    D->PhaseStep = 0;
                    D->MainStatus |= 0x40;  /* 0x40:DIO(Data Input/Output) */
                }
                break;
            }
            D->MainStatus &= ~0x80;
            break;
        }
        break;
    }
}