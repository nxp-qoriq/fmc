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

static unsigned int hanle_no = 0;

t_Handle
FM_Open(uint8_t id)
{
    printf( "Calling IOCTL::FM_Open\n" );
    return (t_Handle)++hanle_no;
}


void
FM_Close(t_Handle h_Fm)
{
    printf( "Calling IOCTL::FM_Close\n" );
}


t_Handle
FM_PCD_Open( t_FmPcdParams *p_FmPcdParams )
{
    printf( "Calling IOCTL::FM_PCD_Open\n" );
    return (t_Handle)++hanle_no;
}


void
FM_PCD_Close(t_Handle h_FmPcd)
{
    printf( "Calling IOCTL::FM_PCD_Close\n" );
}


t_Handle FM_PORT_Open(t_FmPortParams *p_FmPortParams)
{
    printf( "Calling IOCTL::FM_PORT_Open\n" );
    return (t_Handle)++hanle_no;
}


void FM_PORT_Close(t_Handle h_FmPort)
{
    printf( "Calling IOCTL::FM_PORT_Close\n" );
}


t_Error
FM_PORT_SetPCD(t_Handle h_FmPort, t_FmPortPcdParams *p_FmPortPcd)
{
    printf( "Calling IOCTL::FM_PORT_SetPCD\n" );
    return E_OK;
}


t_Handle
FM_PCD_Config( t_FmPcdParams *p_FmPcdParams )
{
    printf( "Calling IOCTL::FM_PCD_Config\n" );
    return (t_Handle)++hanle_no;
}

t_Error
FM_PCD_Init(t_Handle h_FmPcd)
{
    printf( "Calling IOCTL::FM_PCD_Init\n" );
    return E_OK;
}

t_Handle
FM_PCD_SetNetEnvCharacteristics( t_Handle h_FmPcd,
                                t_FmPcdNetEnvParams *p_Units )
{
    printf( "Calling IOCTL::FM_PCD_SetNetEnvCharacteristics\n" );
    return (t_Handle)++hanle_no;
}

t_Handle
FM_PCD_KgSetScheme( t_Handle h_FmPcd,
                    t_FmPcdKgSchemeParams *p_Scheme )
{
    printf( "Calling IOCTL::FM_PCD_KgSetScheme\n" );
    return (t_Handle)++hanle_no;
}


t_Handle
FM_PCD_PlcrSetProfile( t_Handle  h_FmPcd,  
                       t_FmPcdPlcrProfileParams *  p_Profile )
{
    static num = 10;
    printf( "Calling IOCTL::FM_PCD_PlcrSetProfile\n" );
    return (t_Handle)++hanle_no;
}


t_Handle   FM_PCD_CcSetNode(t_Handle             h_FmPcd,
                            t_FmPcdCcNodeParams  *p_CcNodeParam)
{
    printf( "Calling IOCTL::FM_PCD_CcSetNode\n" );
    return (t_Handle)++hanle_no;
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
    printf( "Calling IOCTL::FM_PORT_Enable\n" );
    return E_OK;
}


t_Error
FM_PORT_Disable(t_Handle h_FmPort)
{
    printf( "Calling IOCTL::FM_PORT_Disable\n" );
    return E_OK;
}


t_Error
FM_PCD_Enable( t_Handle h_FmPcd )
{
    printf( "Calling IOCTL::FM_PCD_Enable\n" );
    return E_OK;
}


t_Error
FM_PCD_Disable( t_Handle h_FmPcd )
{
    printf( "Calling IOCTL::FM_PCD_Disable\n" );
    return E_OK;
}


t_Error
FM_PORT_DeletePCD(t_Handle h_FmPort)
{
    printf( "Calling IOCTL::FM_PORT_DeletePCD\n" );
    return E_OK;
}


t_Error
FM_PCD_KgDeleteScheme(t_Handle h_FmPcd, t_Handle h_Scheme)
{
    printf( "Calling IOCTL::FM_PCD_KgDeleteScheme\n" );
    return E_OK;
}


t_Error
FM_PCD_CcDeleteNode(t_Handle h_FmPcd, t_Handle h_CcNode)
{
    printf( "Calling IOCTL::FM_PCD_CcDeleteNode\n" );
    return E_OK;
}


t_Error
FM_PCD_PlcrDeleteProfile(t_Handle h_FmPcd, t_Handle h_Profile)
{
    printf( "Calling IOCTL::FM_PCD_PlcrDeleteProfile\n" );
    return E_OK;
}


t_Error
FM_PCD_DeleteNetEnvCharacteristics(t_Handle h_FmPcd, t_Handle h_NetEnv)
{
    printf( "Calling IOCTL::FM_PCD_DeleteNetEnvCharacteristics\n" );
    return E_OK;
}


t_Handle
FM_PCD_CcBuildTree (t_Handle             h_FmPcd,
                    t_FmPcdCcTreeParams  *p_FmPcdCcTreeParams)
{
    printf( "Calling IOCTL::FM_PCD_CcBuildTree\n" );
    return (t_Handle)++hanle_no;
}


t_Error
FM_PCD_CcDeleteTree(t_Handle h_FmPcd, t_Handle h_CcTree)
{
    printf( "Calling IOCTL::FM_PCD_CcDeleteTree\n" );
    return E_OK;
}


t_Error
FM_PCD_PrsLoadSw(t_Handle h_FmPcd, t_FmPcdPrsSwParams *p_SwPrs)
{
    printf( "Calling IOCTL::FM_PCD_PrsLoadSw\n" );
    return E_OK;
}


t_Handle
FM_PCD_ManipSetNode(t_Handle h_FmPcd, t_FmPcdManipParams *p_FmPcdManipParams)
{
    printf( "Calling IOCTL::FM_PCD_ManipSetNode\n" );
    return (t_Handle)++hanle_no;
}


t_Error
FM_PCD_ManipDeleteNode(t_Handle h_FmPcd, t_Handle h_HdrManipNode)
{
    printf( "Calling IOCTL::FM_PCD_ManipDeleteNode\n" );
    return E_OK;
}
