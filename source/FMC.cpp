/* =====================================================================
 *
 *  Copyright 2009-2012, Freescale Semiconductor, Inc., All Rights Reserved. 
 *
 *  This file contains copyrighted material. Use of this file is restricted
 *  by the provisions of a Freescale Software License Agreement, which has
 *  either accompanied the delivery of this software in shrink wrap
 *  form or been expressly executed between the parties.
 *
 *  File Name : FMC.cpp
 *  Author    : Serge Lamikhov-Center
 *
 * ===================================================================*/

#include <tclap/CmdLine.h>
#include <typeinfo>
#include <fstream>

#include "fmc.h"
#include "FMCGenericError.h"

#ifdef _MSC_VER
#pragma warning(disable : 4996)
#ifdef _DEBUG
#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.VC90.CRT' version='9.0.21022.8' processorArchitecture='x86' publicKeyToken='1fc8b3b9a1e18e3b' language='*'\"")
#endif
#endif

#define FMC_VERSION "0.9.16"

int libmain( int argc, char* argv[] );

#include <cstdlib>

fmc_model_t model = {0};

int main( int argc, char* argv[] )
{
#ifdef _DEBUG
    _CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
//    _crtBreakAlloc = 2117;
#endif    

    // Wrap everything in the 'try' block.  Do this every time, 
    // because exceptions will be thrown for problems. 
    try {
        // Get directory name
        std::string appDir( argv[0] );
        const unsigned int pos = appDir.find_last_of( "\\/" );
        if ( std::string::npos == pos ) {
            appDir = ".";
        }
        else {
            appDir = appDir.substr( 0, pos );
        }

        // Define the command line object.
        TCLAP::CmdLine cmd( "Frame Manager Configuration Tool", ' ', FMC_VERSION );

        TCLAP::ValueArg<std::string> swOffset( "t", "sp_offset",
            "Soft Parser generated code base address", false,
            "0x20", "offset" );
        cmd.add( swOffset );

        TCLAP::SwitchArg compOnly( "", "sp_only",
            "Compile the custom protocol, output generated files and exit");
        cmd.add( compOnly );

        TCLAP::SwitchArg dontWarn( "w", "no_warnings",
            "Don't report warnings");
        cmd.add( dontWarn );

        TCLAP::ValueArg<std::string> nameSP( "s", "custom_protocol",
            "Custom protocol file name", false,
            "nodefault", "custom_protocol_file" );
        cmd.add( nameSP);

#ifdef WIN32
        TCLAP::ValueArg<std::string> namePDL( "d", "pdl",
            "PDL file name", false,
            appDir + "/etc/fmc/config/hxs_pdl_v3.xml", "pdl_file" );
#else
        TCLAP::ValueArg<std::string> namePDL( "d", "pdl",
            "PDL file name", false,
            "/etc/fmc/config/hxs_pdl_v3.xml", "pdl_file" );
#endif
        cmd.add( namePDL );

        TCLAP::SwitchArg force( "f", "force",
            "Apply configuration with no regards to previous one. Same as '--apply' but doesn't clear previous configuration (target only)" );
        cmd.add( force );

        TCLAP::SwitchArg apply( "a", "apply",
            "Apply configuration (target only)" );
        cmd.add( apply );

        TCLAP::ValueArg<std::string> nameCfg( "c", "config",
            "Config data file name", false,
            "nodefault", "data_file" );
        cmd.add( nameCfg );

        TCLAP::ValueArg<std::string> namePCD( "p", "pcd",
            "PCD file name", false,
            "nodefault", "pcd_file" );
        cmd.add( namePCD );

        cmd.parse( argc, argv );

        // Check args
        if (compOnly.getValue() && !nameSP.isSet())
            throw CGenericError(ERR_SP_REQUIRED);
        if (!compOnly.getValue() && !namePCD.isSet())
            throw CGenericError(ERR_PCD_REQUIRED);
        if (!compOnly.getValue() && !nameCfg.isSet())
            throw CGenericError(ERR_CONFIG_REQUIRED);
        CGenericError::dontWarn = dontWarn.isSet();

        const char* dump = 0;
        
        int ret = fmc_compile(
                    &model,
                    nameCfg.getValue().c_str(),
                    namePCD.getValue().c_str(),
                    namePDL.getValue().c_str(),
                    nameSP.getValue().c_str(),
                    strtoul( swOffset.getValue().c_str(), 0, 0 ),
                    dontWarn.isSet(),
                    &dump
            );

        if ( ret != 0 ) {
            std::cerr << fmc_get_error() << std::endl;
            return ret;
        }
        
        if ( !compOnly.getValue() && apply.getValue() ) {
            ret = fmc_execute( &model );
            if ( ret != 0 ) {
                std::cerr << fmc_get_error() << std::endl;
                return ret;
            }
        }

        if ( !apply.getValue() ) {
            std::ofstream os( "./fmc_config_data.c" );
            os << dump;
        }
        
        return ret;
    }
    catch ( const TCLAP::ArgException& e )  // Catch exceptions for TCLAP
    {
        std::cerr << "error: "
                  << e.error()
                  << " for arg "
                  << e.argId()
                  << std::endl;
        return 2;
    } 
    catch ( const CGenericError& genError ) { // Catch known errors generated by FMC
        std::cerr << "Error: "
                  << genError.getErrorMsg()
                  << std::endl;
        return 3;
    }
    catch ( std::exception &e ) 
    {
        std::cerr << "Caught: " << e.what( ) << std::endl;
        std::cerr << "Type  : " << typeid( e ).name( ) << std::endl;
        return 4;
    }
    catch ( ... ) {                     // Catch unknown errors
        std::cerr << "Unrecognized error occured. The program was terminated"
                  << std::endl;
        return 1;
    }

    return 0;
}
