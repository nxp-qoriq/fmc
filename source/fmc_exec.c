/* =====================================================================
 *
 *  Copyright 2009-2012, Freescale Semiconductor, Inc., All Rights Reserved.
 *
 *  This file contains copyrighted material. Use of this file is restricted
 *  by the provisions of a Freescale Software License Agreement, which has
 *  either accompanied the delivery of this software in shrink wrap
 *  form or been expressly executed between the parties.
 *
 *  File Name : fmc_exec.c
 *  Author    : Serge Lamikhov-Center
 *
 * ===================================================================*/

#include <string.h>
#include "fmc.h"


// Forward declaration of static functions
static int fmc_exec_engine_start( fmc_model* model, unsigned int index,
                                  unsigned int* p_relative_scheme_index );
static int fmc_exec_engine_end  ( fmc_model* model, unsigned int index );
static int fmc_exec_port_start  ( fmc_model* model, unsigned int engine,
                                  unsigned int index );
static int fmc_exec_port_end    ( fmc_model* model, unsigned int engine,
                                  unsigned int index );
static int fmc_exec_scheme      ( fmc_model* model,  unsigned int engine,
                                  unsigned int port, unsigned int index,
                                  unsigned int relative_scheme_index );
static int fmc_exec_ccnode      ( fmc_model* model, unsigned int engine,
                                  unsigned int index );
static int fmc_exec_cctree      ( fmc_model* model,  unsigned int engine,
                                  unsigned int port );
static int fmc_exec_policer     ( fmc_model* model, unsigned int engine,
                                  unsigned int index );

static int fmc_clean_engine_start( fmc_model* model, unsigned int index );
static int fmc_clean_engine_end  ( fmc_model* model, unsigned int index );
static int fmc_clean_port_start  ( fmc_model* model, unsigned int engine,
                                   unsigned int index );
static int fmc_clean_port_end    ( fmc_model* model, unsigned int engine,
                                   unsigned int index );
static int fmc_clean_scheme      ( fmc_model* model,  unsigned int engine,
                                   unsigned int port, unsigned int index );
static int fmc_clean_ccnode      ( fmc_model* model, unsigned int engine,
                                   unsigned int index );
static int fmc_clean_cctree      ( fmc_model* model,  unsigned int engine,
                                   unsigned int port );
static int fmc_clean_policer     ( fmc_model* model, unsigned int engine,
                                   unsigned int index );


/* -------------------------------------------------------------------------- */
int
fmc_execute( fmc_model* model )
{
    int          ret = 0;
    unsigned int current_engine;
    unsigned int current_port;
    unsigned int relative_scheme_index;
    unsigned int i;

    if ( model->format_version != FMC_OUTPUT_FORMAT_VER ) {
        return 0xFFFFFFFF;
    }
    
    for ( i = 0; i < model->ao_count; i++ ) {
        switch ( model->ao[i].type ) {
            case FMCEngineStart:
                current_engine        = model->ao[i].index;
                relative_scheme_index = 0;
                ret = fmc_exec_engine_start( model, current_engine, &relative_scheme_index );
                break;
            case FMCEngineEnd:
                ret = fmc_exec_engine_end( model, current_engine );
                break;
            case FMCPortStart:
                current_port = model->ao[i].index;
                ret = fmc_exec_port_start( model, current_engine, current_port );
                break;
            case FMCPortEnd:
                ret = fmc_exec_port_end( model, current_engine, current_port );
                break;
            case FMCScheme:
                ret = fmc_exec_scheme( model, current_engine, current_port,
                                       model->ao[i].index, relative_scheme_index++ );
                break;
            case FMCCCNode:
                ret = fmc_exec_ccnode( model, current_engine, model->ao[i].index );
                break;
            case FMCCCTree:
                ret = fmc_exec_cctree( model, current_engine, model->ao[i].index );
                break;
            case FMCPolicer:
                ret = fmc_exec_policer( model, current_engine, model->ao[i].index );
                break;
        }

        // Exit the loop in case of failure
        if ( ret != 0 ) {
            break;
        }
    }

    return ret;
}


/* -------------------------------------------------------------------------- */
int
fmc_clean( fmc_model* model )
{
    int          ret = 0;
    unsigned int current_engine;
    unsigned int current_port;
    unsigned int i, j;

    if ( model->format_version != FMC_OUTPUT_FORMAT_VER ) {
        return 0xFFFFFFFF;
    }

    for ( j = 0; j < model->ao_count; j++ ) {
        // Clean entities in reverse order of applying
        i = model->ao_count - j - 1;
        switch ( model->ao[i].type ) {
            case FMCEngineStart:
                current_engine = model->ao[i].index;
                ret = fmc_clean_engine_start( model, current_engine );
                break;
            case FMCEngineEnd:
                current_engine = model->ao[i].index;
                ret = fmc_clean_engine_end( model, current_engine );
                break;
            case FMCPortStart:
                current_port = model->ao[i].index;
                ret = fmc_clean_port_start( model, current_engine, current_port );
                break;
            case FMCPortEnd:
                current_port = model->ao[i].index;
                ret = fmc_clean_port_end( model, current_engine, current_port );
                break;
            case FMCScheme:
                ret = fmc_clean_scheme( model, current_engine, current_port,
                                        model->ao[i].index );
                break;
            case FMCCCNode:
                ret = fmc_clean_ccnode( model, current_engine, model->ao[i].index );
                break;
            case FMCCCTree:
                ret = fmc_clean_cctree( model, current_engine, model->ao[i].index );
                break;
            case FMCPolicer:
                ret = fmc_clean_policer( model, current_engine, model->ao[i].index );
                break;
        }

        // Exit the loop in case of failure
        if ( ret != 0 ) {
            break;
        }
    }

    return ret;
}


/* -------------------------------------------------------------------------- */
t_Handle
fmc_get_handle(
            fmc_model*   model,
            unsigned int engine_number,
            e_FmPortType port_type,
            unsigned int port_number,
            const char*  name
)
{
    unsigned int engine  = 0;
    unsigned int port    = 0;
    unsigned int found   = 0;
    unsigned int ccindex = 0;
    unsigned int i;

    // Find engine index
    for ( engine = 0; engine < model->fman_count; engine++ ) {
        if ( model->fman[engine].number == engine_number ) {
            found = 1;
            break;
        }
    }
    if ( !found ) {
        return 0;
    }

    // Check whether it is a 'policy' name. Return PCD handle then
    if ( strcmp( model->fman[engine].pcd_name, name ) == 0 ) {
        return model->fman[engine].pcd_handle;
    }

    // Check fragmentation names
    for ( i = 0; i < model->fman[engine].frag_count; i++ ) {
        if ( strcmp( model->fman[engine].frag_name[i], name ) == 0 ) {
            return model->fman[engine].frag_handle[i];
        }
    }

    // Find port index
    found = 0;
    for ( port = 0; port < model->fman[engine].port_count; port++ ) {
        unsigned int port_index = model->fman[engine].ports[port];
        if ( model->port[port_index].type   == port_type &&
             model->port[port_index].number == port_number ) {
            found = 1;
            break;
        }
    }
    if ( !found ) {
        return 0;
    }

    // Find CC handle according to found engine and port
    for ( ccindex =0; ccindex < model->port[port].ccnodes_count; ccindex++ ) {
        unsigned int index = model->port[port].ccnodes[ccindex];
        if ( strcmp( model->ccnode_name[index], name ) == 0 ) {
            return model->ccnode_handle[index];
        }
    }

    return 0;
}


/* -------------------------------------------------------------------------- */
static int
fmc_exec_engine_start( fmc_model* model, unsigned int index, 
                       unsigned int* p_relative_scheme_index )
{
    unsigned int i;

    t_FmPcdParams fmPcdParams = {0};

    // Open FMan device
#ifndef NETCOMM_SW
    model->fman[index].handle = FM_Open( model->fman[index].number );
#else
    model->fman[index].handle = SYS_GetHandle(e_SYS_SUBMODULE_FM, model->fman[index].number );
#endif
    if ( model->fman[index].handle == 0 ) {
        return 1;
    }

    // Open FMan device
    fmPcdParams.h_Fm = model->fman[index].handle;
#ifndef NETCOMM_SW
    model->fman[index].pcd_handle = FM_PCD_Open( &fmPcdParams );
#else
    model->fman[index].handle_pcd = SYS_GetHandle(e_SYS_SUBMODULE_FM_PCD, model->fman[index].number );
#endif
    if ( model->fman[index].pcd_handle == 0 ) {
        return 2;
    }

    if ( model->sp_enable ) {
        FM_PCD_PrsLoadSw( model->fman[index].pcd_handle, &(model->sp) );
    }
    FM_PCD_Enable( model->fman[index].pcd_handle );

    for ( i = 0; i < model->fman[index].reasm_count; i++ ) {
        if ( model->fman[index].reasm[i].fragOrReasmParams.hdr == HEADER_TYPE_IPv6 ) {
            model->fman[index].reasm[i].fragOrReasmParams.ipReasmParams.relativeSchemeId[0] =
                (*p_relative_scheme_index)++;
            model->fman[index].reasm[i].fragOrReasmParams.ipReasmParams.relativeSchemeId[1] =
                (*p_relative_scheme_index)++;
        }
        else {
            model->fman[index].reasm[i].fragOrReasmParams.ipReasmParams.relativeSchemeId[0] =
                (*p_relative_scheme_index)++;
        }

        model->fman[index].reasm_handle[i] =
            FM_PCD_ManipSetNode( model->fman[index].pcd_handle, &model->fman[index].reasm[i] );
    }

    for ( i = 0; i < model->fman[index].frag_count; i++ ) {
        model->fman[index].frag_handle[i] =
            FM_PCD_ManipSetNode( model->fman[index].pcd_handle, &model->fman[index].frag[i] );
    }

    return 0;
}


/* -------------------------------------------------------------------------- */
static int
fmc_exec_engine_end( fmc_model* model, unsigned int index )
{
    return 0;
}


/* -------------------------------------------------------------------------- */
static int
fmc_exec_port_start( fmc_model* model, unsigned int engine, unsigned int port )
{
    t_FmPortParams  fmPortParam      = {0};

    fmc_fman* pengine = &model->fman[engine];
    fmc_port* pport   = &model->port[port];

    fmPortParam.h_Fm     = pengine->handle;
    fmPortParam.portId   = pport->number;
    fmPortParam.portType = pport->type;

#ifndef NETCOMM_SW
    model->port[port].handle = FM_PORT_Open( &fmPortParam );
#else
    if (fmPortParam.portType == e_FM_PORT_TYPE_OH_OFFLINE_PARSING)
        model->port[port].handle = SYS_GetHandle(e_SYS_SUBMODULE_FM_PORT_HO, fmPortParam.portId );
    else
        model->port[port].handle = SYS_GetHandle(e_SYS_SUBMODULE_FM_PORT_1GRx, fmPortParam.portId );
#endif
    if ( pport->handle == 0 ) {
        return 3;
    }

    pport->env_id_handle = FM_PCD_SetNetEnvCharacteristics(
                                pengine->pcd_handle,
                                &pport->distinctionUnits );
    if ( pport->env_id_handle == 0 ) {
        return 4;
    }

    return 0;
}


/* -------------------------------------------------------------------------- */
static int
fmc_exec_port_end( fmc_model* model, unsigned int engine, unsigned int port )
{
    t_Error err;
    fmc_port* pport = &model->port[port];
    unsigned int i;

    pport->pcdParam.h_NetEnv    = pport->env_id_handle;
    pport->pcdParam.p_PrsParams = &pport->prsParam;

    // Add KeyGen runtime parameters
    if ( pport->schemes_count != 0 ) {
        pport->pcdParam.p_KgParams  = &pport->kgParam;
        for ( i = 0; i < pport->schemes_count; ++i ) {
            pport->pcdParam.p_KgParams->h_Schemes[ i ] =
                model->scheme_handle[ pport->schemes[i] ];
        }
    }

    // Add CC runtime parameters
    if ( pport->ccnodes_count != 0 || pport->reasm_index > 0 ) {
        pport->pcdParam.p_CcParams           = &pport->ccParam;
        pport->pcdParam.p_CcParams->h_CcTree = pport->cctree_handle;
    }

    err = FM_PORT_Disable( pport->handle );
    if ( err ) { return 5; }
    err = FM_PORT_SetPCD( pport->handle, &pport->pcdParam );
    if ( err ) { return 6; }
    err = FM_PORT_Enable( pport->handle );
    if ( err ) { return 7; }

    return 0;
}


/* -------------------------------------------------------------------------- */
static int
fmc_exec_scheme( fmc_model* model,  unsigned int engine,
                 unsigned int port, unsigned int index,
                 unsigned int relative_scheme_index )
{
    model->scheme[index].netEnvParams.h_NetEnv = model->port[port].env_id_handle;
    model->scheme[index].id.relativeSchemeId   = relative_scheme_index;

    // Fill next engine handles
    if ( model->scheme[index].nextEngine == e_FM_PCD_CC ) {
        model->scheme[index].kgNextEngineParams.cc.h_CcTree =
                                                model->port[port].cctree_handle;
    }

    model->scheme_handle[index] =
        FM_PCD_KgSetScheme( model->fman[engine].pcd_handle,
                            &(model->scheme[index]) );
    if ( model->scheme_handle[index] == 0 ) {
        return 5;
    }

    return 0;
}


/* -------------------------------------------------------------------------- */
static int
fmc_exec_ccnode( fmc_model* model, unsigned int engine,
                 unsigned int index )
{
    unsigned int i;
    unsigned int action_index;

    for ( i = 0; i < model->ccnode[index].keysParams.numOfKeys; ++i ) {
        action_index = model->ccentry_action_index[index][i];
        if ( model->ccnode[index].keysParams.keyParams[i]
                               .ccNextEngineParams.nextEngine == e_FM_PCD_KG ) {
            model->ccnode[index].keysParams.keyParams[i]
                .ccNextEngineParams.params.kgParams.h_DirectScheme =
                                             model->scheme_handle[action_index];
        }
        else if ( model->ccnode[index].keysParams.keyParams[i]
                               .ccNextEngineParams.nextEngine == e_FM_PCD_CC ) {
            model->ccnode[index].keysParams.keyParams[i]
                .ccNextEngineParams.params.ccParams.h_CcNode =
                                             model->ccnode_handle[action_index];
        }

        if ( model->ccentry_frag[index][i] != 0 ) {
            model->ccnode[index].keysParams.keyParams[i].ccNextEngineParams.h_Manip =
                model->fman[engine].frag_handle[ model->ccentry_frag[index][i] - 1 ];
        }
    }
    action_index = model->ccmiss_action_index[index];
    if ( model->ccnode[index].keysParams.ccNextEngineParamsForMiss
                           .nextEngine == e_FM_PCD_KG ) {
        model->ccnode[index].keysParams.ccNextEngineParamsForMiss
            .params.kgParams.h_DirectScheme =
                                         model->scheme_handle[action_index];
    }
    else if ( model->ccnode[index].keysParams.ccNextEngineParamsForMiss
                           .nextEngine == e_FM_PCD_CC ) {
        model->ccnode[index].keysParams.ccNextEngineParamsForMiss
            .params.ccParams.h_CcNode = model->ccnode_handle[action_index];
    }


    model->ccnode_handle[index] =
        FM_PCD_CcSetNode( model->fman[engine].pcd_handle,
                          &(model->ccnode[index]) );

    if ( model->ccnode_handle[index] == 0 ) {
        return 6;
    }

    return 0;
}


/* -------------------------------------------------------------------------- */
static int
fmc_exec_cctree( fmc_model* model, unsigned int engine,
                 unsigned int port )
{
    t_FmPcdCcTreeParams ccTreeParams = { 0 };
    unsigned int        reasm_index;
    unsigned int        i;

    ccTreeParams.numOfGrps = model->port[port].ccroot_count;
    ccTreeParams.h_NetEnv  = model->port[port].env_id_handle;
    reasm_index            = model->port[port].reasm_index;
    if ( reasm_index != 0 ) {
        ccTreeParams.h_IpReassemblyManip =
            model->fman[engine].reasm_handle[reasm_index - 1];
    }

    for ( i = 0; i < model->port[port].ccroot_count; ++i ) {
        ccTreeParams.ccGrpParams[i].numOfDistinctionUnits = 0;
        ccTreeParams.ccGrpParams[i].nextEnginePerEntriesInGrp[0].
            nextEngine = e_FM_PCD_CC;
        ccTreeParams.ccGrpParams[i].nextEnginePerEntriesInGrp[0].
            params.ccParams.h_CcNode =
                model->ccnode_handle[model->port[port].ccroot[i]];
    }

    model->port[port].cctree_handle =
        FM_PCD_CcBuildTree( model->fman[engine].pcd_handle,
                            &ccTreeParams );
    if ( model->port[port].cctree_handle == 0 ) {
        return 7;
    }

    return 0;
}


/* -------------------------------------------------------------------------- */
static int
fmc_exec_policer( fmc_model* model, unsigned int engine,
                  unsigned int index )
{
    unsigned int action_index;

    // Fill next engine handles
    action_index = model->policer_action_index[index][0];
    switch ( model->policer[index].nextEngineOnGreen ) {
        case e_FM_PCD_PLCR:
            model->policer[index].paramsOnGreen.h_Profile =
                model->policer_handle[action_index];
            break;
        case e_FM_PCD_KG:
            model->policer[index].paramsOnGreen.h_DirectScheme =
                model->scheme_handle[action_index];
            break;
    }
    action_index = model->policer_action_index[index][1];
    switch ( model->policer[index].nextEngineOnYellow ) {
        case e_FM_PCD_PLCR:
            model->policer[index].paramsOnYellow.h_Profile =
                model->policer_handle[action_index];
            break;
        case e_FM_PCD_KG:
            model->policer[index].paramsOnYellow.h_DirectScheme =
                model->scheme_handle[action_index];
            break;
    }
    action_index = model->policer_action_index[index][2];
    switch ( model->policer[index].nextEngineOnRed ) {
        case e_FM_PCD_PLCR:
            model->policer[index].paramsOnRed.h_Profile =
                model->policer_handle[action_index];
            break;
        case e_FM_PCD_KG:
            model->policer[index].paramsOnRed.h_DirectScheme =
                model->scheme_handle[action_index];
            break;
    }

    model->policer_handle[index] =
        FM_PCD_PlcrSetProfile( model->fman[engine].pcd_handle,
                               &(model->policer[index]) );
    if ( model->policer_handle[index] == 0 ) {
        return 8;
    }

    return 0;
}


/* -------------------------------------------------------------------------- */
static int
fmc_clean_engine_start( fmc_model* model, unsigned int index )
{
    unsigned int i;

    for ( i = 0; i < model->fman[index].frag_count; i++ ) {
        FM_PCD_ManipDeleteNode( model->fman[index].pcd_handle, model->fman[index].frag_handle[i] );
    }

    for ( i = 0; i < model->fman[index].reasm_count; i++ ) {
        FM_PCD_ManipDeleteNode( model->fman[index].pcd_handle, model->fman[index].reasm_handle[i] );
    }

    if ( model->fman[index].pcd_handle != 0 ) {
#ifndef NETCOMM_SW
        FM_PCD_Close( model->fman[index].pcd_handle );
#endif
    }
    if ( model->fman[index].handle != 0 ) {
#ifndef NETCOMM_SW
        FM_Close( model->fman[index].handle );
#endif
    }

    return 0;
}


/* -------------------------------------------------------------------------- */
static int
fmc_clean_engine_end( fmc_model* model, unsigned int index )
{
    FM_PCD_Disable( model->fman[index].pcd_handle );
    return 0;
}


/* -------------------------------------------------------------------------- */
static int
fmc_clean_port_start( fmc_model* model, unsigned int engine, unsigned int port )
{
    fmc_port* pport = &model->port[port];

    if ( pport->handle == 0 ) {
        return 0;
    }

    FM_PCD_DeleteNetEnvCharacteristics( model->fman[engine].pcd_handle,
                                        pport->env_id_handle );

#ifndef NETCOMM_SW
    FM_PORT_Close( pport->handle );
#endif

    return 0;
}


/* -------------------------------------------------------------------------- */
static int
fmc_clean_port_end( fmc_model* model, unsigned int engine, unsigned int port )
{
    t_Error err;
    fmc_port* pport = &model->port[port];

    if ( pport->handle == 0 ) {
        return 0;
    }

    err = FM_PORT_DeletePCD( pport->handle );
    if ( err ) { return 6; }

    return 0;
}


/* -------------------------------------------------------------------------- */
static int
fmc_clean_scheme( fmc_model* model,  unsigned int engine,
                 unsigned int port, unsigned int index )
{
    if ( model->scheme_handle[index] != 0 ) {
        FM_PCD_KgDeleteScheme( model->fman[engine].pcd_handle,
                               model->scheme_handle[index] );
    }

    return 0;
}


/* -------------------------------------------------------------------------- */
static int
fmc_clean_ccnode( fmc_model* model, unsigned int engine,
                 unsigned int index )
{
    if ( model->ccnode_handle[index] != 0 ) {
        FM_PCD_CcDeleteNode( model->fman[engine].pcd_handle,
                             model->ccnode_handle[index] );
    }

    return 0;
}


/* -------------------------------------------------------------------------- */
static int
fmc_clean_cctree( fmc_model* model, unsigned int engine,
                 unsigned int port )
{
    if ( model->port[port].cctree_handle != 0 ) {
        FM_PCD_CcDeleteTree( model->fman[engine].pcd_handle,
                             model->port[port].cctree_handle );
    }

    return 0;
}


/* -------------------------------------------------------------------------- */
static int
fmc_clean_policer( fmc_model* model, unsigned int engine,
                  unsigned int index )
{
    if ( model->policer_handle[index] != 0 ) {
        FM_PCD_PlcrDeleteProfile( model->fman[engine].pcd_handle,
                                  model->policer_handle[index] );
    }

    return 0;
}
