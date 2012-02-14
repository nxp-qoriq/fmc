/* =====================================================================
 *
 *  Copyright 2009-2012, Freescale Semiconductor, Inc., All Rights Reserved. 
 *
 *  This file contains copyrighted material. Use of this file is restricted
 *  by the provisions of a Freescale Software License Agreement, which has
 *  either accompanied the delivery of this software in shrink wrap
 *  form or been expressly executed between the parties.
 *
 *  File Name : FMCCModelOutput.h
 *  Author    : Serge Lamikhov-Center
 *
 * ===================================================================*/

#ifndef FMCCMODELOUTPUT_H
#define FMCCMODELOUTPUT_H

#include "FMCPCDModel.h"

class CFMCCModelOutput
{
public:
    void output( const CFMCModel& model, fmc_model_t* cmodel,
                 std::ostream& oss, size_t indent );
protected:
    void output_fmc_fman( const CFMCModel& model, fmc_model_t* cmodel,
                          unsigned int index,
                          std::ostream& oss, size_t indent );
    void output_fmc_port( const CFMCModel& model, fmc_model_t* cmodel,
                          unsigned int index,
                          std::ostream& oss, size_t indent );
    void output_fmc_scheme( const CFMCModel& model, fmc_model_t* cmodel,
                            unsigned int index,
                            std::ostream& oss, size_t indent );
    void output_fmc_ccnode( const CFMCModel& model, fmc_model_t* cmodel,
                            unsigned int index,
                            std::ostream& oss, size_t indent );
    void output_fmc_policer( const CFMCModel& model, fmc_model_t* cmodel,
                             unsigned int index,
                             std::ostream& oss, size_t indent );
    void output_fmc_applier( const CFMCModel& model, fmc_model_t* cmodel,
                             unsigned int index,
                             std::ostream& oss, size_t indent );
};

#endif
