/* =====================================================================
 *
 *  Copyright 2009-2012, Freescale Semiconductor, Inc., All Rights Reserved. 
 *
 *  This file contains copyrighted material. Use of this file is restricted
 *  by the provisions of a Freescale Software License Agreement, which has
 *  either accompanied the delivery of this software in shrink wrap
 *  form or been expressly executed between the parties.
 *
 *  File Name : fmc.h
 *  Author    : Serge Lamikhov-Center
 *
 * ===================================================================*/

#ifndef LIBFMC_H
#define LIBFMC_H

#ifdef __cplusplus
extern "C"
{
#endif
#include <std_ext.h>
#include <error_ext.h>
#include <part_ext.h>
#include <fm_ext.h>
#include <Peripherals/fm_pcd_ext.h>
#include <Peripherals/fm_port_ext.h>

#define FMC_OUTPUT_FORMAT_VER 0x103

#define FMC_NAME_LEN             64
#define FMC_FMAN_NUM              2
#define FMC_PORTS_PER_FMAN       16
#define FMC_SCHEMES_NUM          32
#define FMC_SCHEME_PROTOCOLS_NUM 16
#define FMC_CC_NODES_NUM         64
#define FMC_PLC_NUM              64
#define MAX_SP_CODE_SIZE      0x7C0
#define FMC_MANIP_MAX             8

typedef struct fmc_fman_t {
    unsigned int       number;
    unsigned int       port_count;
    unsigned int       ports[FMC_PORTS_PER_FMAN];
    char               name[FMC_NAME_LEN];
    t_Handle           handle;
    char               pcd_name[FMC_NAME_LEN];
    t_Handle           pcd_handle;

    unsigned int       reasm_count;
    t_FmPcdManipParams reasm[FMC_MANIP_MAX];
    char               reasm_name[FMC_MANIP_MAX][FMC_NAME_LEN];
    t_Handle           reasm_handle[FMC_MANIP_MAX];

    unsigned int       frag_count;
    t_FmPcdManipParams frag[FMC_MANIP_MAX];
    char               frag_name[FMC_MANIP_MAX][FMC_NAME_LEN];
    t_Handle           frag_handle[FMC_MANIP_MAX];
} fmc_fman;


typedef struct fmc_port_t {
    e_FmPortType         type;                      ///< Port type
    unsigned int         number;                    ///< Port number
    t_FmPcdNetEnvParams  distinctionUnits;          ///< Port's network env
    t_FmPortPcdParams    pcdParam;
    t_FmPortPcdPrsParams prsParam;
    t_FmPortPcdKgParams  kgParam;
    t_FmPortPcdCcParams  ccParam;
    char                 name[FMC_NAME_LEN];
    t_Handle             handle;
    t_Handle             env_id_handle;
    t_Handle             cctree_handle;

    unsigned int         schemes_count;             ///< Number of used schemes
    unsigned int         schemes[FMC_SCHEMES_NUM];  ///< Schemes used by this port

    unsigned int         ccnodes_count;             ///< Number of used CC nodes
    unsigned int         ccnodes[FMC_CC_NODES_NUM]; ///< Class. nodes used by this port

    unsigned int         ccroot_count;
    unsigned int         ccroot[FMC_CC_NODES_NUM];

    unsigned int         reasm_index;
} fmc_port;


typedef enum fmc_apply_order_e {
    FMCEngineStart,
    FMCEngineEnd,
    FMCPortStart,
    FMCPortEnd,
    FMCScheme,
    FMCCCNode,
    FMCCCTree,
    FMCPolicer
} fmc_apply_order_e;

    
typedef struct fmc_apply_order_t {
    fmc_apply_order_e   type;
    unsigned int        index;
} fmc_apply_order;


typedef struct fmc_model_t {
    unsigned int             format_version;
    unsigned int             sp_enable;           ///< Soft parser enable flag
    t_FmPcdPrsSwParams       sp;                  ///< Soft parser configuration
    uint8_t                  spCode[MAX_SP_CODE_SIZE];

    unsigned int             fman_count;                ///< Number of used FMan engines
    fmc_fman                 fman[FMC_FMAN_NUM];

    unsigned int             port_count;                ///< Number of used FMan ports
    fmc_port                 port[FMC_FMAN_NUM * FMC_PORTS_PER_FMAN];

    unsigned int             scheme_count;     ///< Number of used KeyGen schemes
    char                     scheme_name  [FMC_SCHEMES_NUM][FMC_NAME_LEN];
    t_Handle                 scheme_handle[FMC_SCHEMES_NUM];
    t_FmPcdKgSchemeParams    scheme       [FMC_SCHEMES_NUM];

    unsigned int             ccnode_count;       ///< Number of used CC nodes
    char                     ccnode_name         [FMC_CC_NODES_NUM][FMC_NAME_LEN];
    t_Handle                 ccnode_handle       [FMC_CC_NODES_NUM];
    t_FmPcdCcNodeParams      ccnode              [FMC_CC_NODES_NUM];
    uint8_t                  cckeydata           [FMC_CC_NODES_NUM][FM_PCD_MAX_NUM_OF_KEYS]
                                                 [FM_PCD_MAX_SIZE_OF_KEY];
    unsigned char            ccmask              [FMC_CC_NODES_NUM][FM_PCD_MAX_NUM_OF_KEYS]
                                                 [FM_PCD_MAX_SIZE_OF_KEY];
    unsigned int             ccentry_action_index[FMC_CC_NODES_NUM][FM_PCD_MAX_NUM_OF_KEYS];
    unsigned char            ccentry_frag        [FMC_CC_NODES_NUM][FM_PCD_MAX_NUM_OF_KEYS];
    unsigned int             ccmiss_action_index [FMC_CC_NODES_NUM];
                                     
    unsigned int             policer_count; ///< Number of used policers
    char                     policer_name        [FMC_PLC_NUM][FMC_NAME_LEN];
    t_FmPcdPlcrProfileParams policer             [FMC_PLC_NUM];
    t_Handle                 policer_handle      [FMC_PLC_NUM];
    unsigned int             policer_action_index[FMC_PLC_NUM][3];
    
    unsigned int             ao_count;               ///< Number of 'apply order' elements
    fmc_apply_order          ao[FMC_FMAN_NUM*FMC_PORTS_PER_FMAN*(FMC_SCHEMES_NUM+FMC_CC_NODES_NUM)];
} fmc_model;


int fmc_compile(
        fmc_model*   model,
        const char*  nameCfg,
        const char*  namePCD,
        const char*  namePDL,
        const char*  nameSP,
        unsigned int swOffset,
        unsigned int dontWarn,
        const char** dump
);

int fmc_execute( fmc_model* model );

int fmc_clean( fmc_model* model );

const char* fmc_get_error( void );

t_Handle fmc_get_handle(
            fmc_model*   model,
            unsigned int engine_number,
            e_FmPortType port_type,
            unsigned int port_number,
            const char*  name
);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif // LIBFMC_H
