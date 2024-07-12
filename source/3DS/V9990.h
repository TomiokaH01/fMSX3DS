/** fMSX: portable MSX emulator ******************************/
/**                                                         **/
/**                         V9938.h                         **/
/**                                                         **/
/** This file contains declarations for V9938 special       **/
/** graphical operations support implemented in V9938.c.    **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1994-2020                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/

/** Modified for V9990 VDP h.tomioka 2024         **/

#ifndef V9990_H
#define V9990_H

#include "MSX.h"
void VDPReadV9990(void);

void VDPWriteV9990(register byte V);

byte VDPDrawV9990(register byte Op);

void InitV9990CMD(void);

void LoopVDPV9990(void);

void FlushV9990(void);

void ExecV9990(void);

void ResetV9990VDPRegister(void);

#endif /* V9990_H */