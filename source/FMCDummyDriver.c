/* =====================================================================
 *
 *  Copyright 2009, 2010, Freescale Semiconductor, Inc., All Rights Reserved. 
 *
 *  This file contains copyrighted material. Use of this file is restricted
 *  by the provisions of a Freescale Software License Agreement, which has
 *  either accompanied the delivery of this software in shrink wrap
 *  form or been expressly executed between the parties.
 *
 *  File Name : FMCDummyDriver.c
 *  Author    : Serge Lamikhov-Center
 *
 * ===================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#include <std_ext.h>
#include <error_ext.h>
#include <part_ext.h>
#include <fm_ext.h>
#include <Peripherals/fm_pcd_ext.h>
#include <Peripherals/fm_port_ext.h>

#ifdef __cplusplus
}
#endif
/*
   Dummy stub functions' definitions. The file makes it possible
   to compile/link the project without the real driver integration
*/

static unsigned int handle_no = 0;

t_Handle
FM_Open(uint8_t id)
{
    printf( "Calling IOCTL::FM_Open %d\n", handle_no + 1 );
    return (t_Handle)++handle_no;
}


void
FM_Close(t_Handle h_Fm)
{
    printf( "Calling IOCTL::FM_Close %d\n", h_Fm );
}


t_Handle
FM_PCD_Open( t_FmPcdParams *p_FmPcdParams )
{
    printf( "Calling IOCTL::FM_PCD_Open %d\n", handle_no + 1 );
    return (t_Handle)++handle_no;
}


void
FM_PCD_Close(t_Handle h_FmPcd)
{
    printf( "Calling IOCTL::FM_PCD_Close %d\n",  h_FmPcd );
}


t_Handle FM_PORT_Open(t_FmPortParams *p_FmPortParams)
{
    printf( "Calling IOCTL::FM_PORT_Open %d\n", handle_no + 1 );
    return (t_Handle)++handle_no;
}


void FM_PORT_Close(t_Handle h_FmPort)
{
    printf( "Calling IOCTL::FM_PORT_Close %d\n", h_FmPort );
}


t_Error
FM_PORT_SetPCD(t_Handle h_FmPort, t_FmPortPcdParams *p_FmPortPcd)
{
    printf( "Calling IOCTL::FM_PORT_SetPCD %d\n", h_FmPort );
    return E_OK;
}


t_Handle
FM_PCD_Config( t_FmPcdParams *p_FmPcdParams )
{
    printf( "Calling IOCTL::FM_PCD_Config %d\n", handle_no + 1 );
    return (t_Handle)++handle_no;
}

t_Error
FM_PCD_Init(t_Handle h_FmPcd)
{
    printf( "Calling IOCTL::FM_PCD_Init %d\n", h_FmPcd );
    return E_OK;
}

t_Handle
FM_PCD_NetEnvCharacteristicsSet( t_Handle h_FmPcd,
                                t_FmPcdNetEnvParams *p_Units )
{
    printf( "Calling IOCTL::FM_PCD_SetNetEnvCharacteristics %d\n", handle_no + 1 );
    return (t_Handle)++handle_no;
}

t_Handle
FM_PCD_KgSchemeSet( t_Handle h_FmPcd,
                    t_FmPcdKgSchemeParams *p_Scheme )
{
    printf( "Calling IOCTL::FM_PCD_KgSetScheme %d rel=%d\n", handle_no + 1, p_Scheme->id.relativeSchemeId );
    return (t_Handle)++handle_no;
}


t_Handle
FM_PCD_PlcrProfileSet( t_Handle  h_FmPcd,  
                       t_FmPcdPlcrProfileParams *  p_Profile )
{
    printf( "Calling IOCTL::FM_PCD_PlcrSetProfile %d\n", handle_no + 1 );
    return (t_Handle)++handle_no;
}


t_Handle
FM_PCD_MatchTableSet(t_Handle             h_FmPcd,
                     t_FmPcdCcNodeParams  *p_CcNodeParam)
{
    printf( "Calling IOCTL::FM_PCD_CcSetNode %d\n", handle_no + 1 );
    return (t_Handle)++handle_no;
}


t_Error
FM_PCD_ConfigKgDfltValue(t_Handle h_FmPcd, uint8_t valueId, uint32_t value)
{
    printf( "Calling IOCTL::FM_PCD_ConfigKgDfltValue\n" );
    return E_OK;
}


t_Error
FM_PORT_Enable(t_Handle h_FmPort)
{
    printf( "Calling IOCTL::FM_PORT_Enable %d\n", h_FmPort );
    return E_OK;
}


t_Error
FM_PORT_Disable(t_Handle h_FmPort)
{
    printf( "Calling IOCTL::FM_PORT_Disable %d\n", h_FmPort );
    return E_OK;
}


t_Error
FM_PCD_Enable( t_Handle h_FmPcd )
{
    printf( "Calling IOCTL::FM_PCD_Enable %d\n", h_FmPcd );
    return E_OK;
}


t_Error
FM_PCD_Disable( t_Handle h_FmPcd )
{
    printf( "Calling IOCTL::FM_PCD_Disable %d\n", h_FmPcd );
    return E_OK;
}


t_Error
FM_PORT_DeletePCD(t_Handle h_FmPort)
{
    printf( "Calling IOCTL::FM_PORT_DeletePCD %d\n", h_FmPort );
    return E_OK;
}


t_Error
FM_PCD_KgSchemeDelete(t_Handle h_FmPcd, t_Handle h_Scheme)
{
    printf( "Calling IOCTL::FM_PCD_KgDeleteScheme %d\n", h_Scheme );
    return E_OK;
}


t_Error
FM_PCD_MatchTableDelete(t_Handle h_FmPcd, t_Handle h_CcNode)
{
    printf( "Calling IOCTL::FM_PCD_CcDeleteNode %d\n", h_CcNode );
    return E_OK;
}


t_Error
FM_PCD_PlcrProfileDelete(t_Handle h_FmPcd, t_Handle h_Profile)
{
    printf( "Calling IOCTL::FM_PCD_PlcrDeleteProfile %d\n", h_Profile );
    return E_OK;
}


t_Error
FM_PCD_NetEnvCharacteristicsDelete(t_Handle h_FmPcd, t_Handle h_NetEnv)
{
    printf( "Calling IOCTL::FM_PCD_DeleteNetEnvCharacteristics %d\n", h_NetEnv );
    return E_OK;
}


t_Handle
FM_PCD_CcRootBuild(t_Handle             h_FmPcd,
                   t_FmPcdCcTreeParams  *p_FmPcdCcTreeParams)
{
    printf( "Calling IOCTL::FM_PCD_CcBuildTree %d\n", handle_no + 1 );
    return (t_Handle)++handle_no;
}


t_Error
FM_PCD_CcRootDelete(t_Handle h_FmPcd, t_Handle h_CcTree)
{
    printf( "Calling IOCTL::FM_PCD_CcDeleteTree %d\n", h_CcTree );
    return E_OK;
}


t_Error
FM_PCD_PrsLoadSw(t_Handle h_FmPcd, t_FmPcdPrsSwParams *p_SwPrs)
{
    printf( "Calling IOCTL::FM_PCD_PrsLoadSw\n" );
    return E_OK;
}


#ifndef P1023
t_Handle
FM_PCD_ManipNodeSet(t_Handle h_FmPcd, t_FmPcdManipParams *p_FmPcdManipParams)
{
    printf( "Calling IOCTL::FM_PCD_ManipSetNode %d\n", handle_no + 1 );
	if ( p_FmPcdManipParams->type == e_FM_PCD_MANIP_REASSEM ) {
		printf( "rel= %d %d\n", p_FmPcdManipParams->u.reassem.u.ipReassem.relativeSchemeId[0],
                                p_FmPcdManipParams->u.reassem.u.ipReassem.relativeSchemeId[1] );
    }
    return (t_Handle)++handle_no;
}


t_Error
FM_PCD_ManipNodeDelete(t_Handle h_FmPcd, t_Handle h_HdrManipNode)
{
    printf( "Calling IOCTL::FM_PCD_ManipDeleteNode %d\n", h_HdrManipNode );
    return E_OK;
}
#endif /* P1023 */
