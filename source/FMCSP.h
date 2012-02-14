/* =====================================================================
 *
 *  Copyright 2009, 2010, Freescale Semiconductor, Inc., All Rights Reserved. 
 *
 *  This file contains copyrighted material. Use of this file is restricted
 *  by the provisions of a Freescale Software License Agreement, which has
 *  either accompanied the delivery of this software in shrink wrap
 *  form or been expressly executed between the parties.
 *
 *  File Name : FMCSP.h
 *  Author    : Serge Lamikhov-Center, Levy Ro'ee
 *
 * ===================================================================*/

#ifndef SP_H
#define SP_H

#include "FMCTaskDef.h"
#include "FMCSPIR.h"
#include "FMCSPCreateCode.h"
extern "C" {
#include "spa/fm_sp.h"
}


void softparser (CTaskDef *task, std::string currentPath, bool keepAsm, unsigned int baseAddress);
unsigned int assemble   (char* asmResult, unsigned char* binary, fmsp_label_list_t **labels, bool debug, int baseAddress);
void createExtensions (std::vector<CExtension> &extns, CCode code, fmsp_label_list_t *labels);

#endif // SP_H
