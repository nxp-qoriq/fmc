/* =====================================================================
 *
 *  Copyright 2009, 2010, Freescale Semiconductor, Inc., All Rights Reserved.
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

#include <typeinfo>
#include <string.h>
#include <sstream>

#include "fmc.h"
#include "FMCGenericError.h"
#include "FMCTaskDef.h"
#include "FMCPDLReader.h"
#include "FMCPCDReader.h"
#include "FMCCFGReader.h"
#include "FMCPCDModel.h"
#include "FMCCModelOutput.h"
#include "FMCSP.h"

#ifdef _MSC_VER
#pragma warning(disable : 4996)
#ifdef _DEBUG
#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.VC90.CRT' version='9.0.21022.8' processorArchitecture='x86' publicKeyToken='1fc8b3b9a1e18e3b' language='*'\"")
#endif
#endif


class CDumpActions {

public:

static std::string ind( size_t count )
{
    return std::string().append( count, ' ' );
}


CDumpActions::CDumpActions( std::ofstream&  ofs_,
                            const CTaskDef& task_ ) : ofs( ofs_ ), task( task_ )
{
    indent = 0;
}


void printActions()
{
    indent = 0;

    std::map< std::string, CEngine >::const_iterator engineIt;
    for ( engineIt  = task.engines.begin();
          engineIt != task.engines.end();
          ++engineIt ) {
        ofs << ind( indent )
            << "eng:"
            << engineIt->second.name
            << std::endl;
        for ( uint32_t i = 0; i < engineIt->second.ports.size(); ++i ) {
            printPort( engineIt->second.ports[i] );
        }
    }
}


void printPort( const CPort& port )
{
    std::map< std::string, CPolicy >::const_iterator policyIt =
        task.policies.find( port.policy );
    if ( policyIt == task.policies.end() ) {
        ofs << "err: policy "
            << port.policy
            << " not found!"
            << std::endl;
        return;
    }

    indent += INDINC;

    ofs << ind( indent )
        << "port:" << port.type << "(" << port.number
        << ") -> policy:" << port.policy
        << std::endl;
    if ( !policyIt->second.reassemblyName.empty() ) {
        ofs << ind( indent )
            << "reasm:" << policyIt->second.reassemblyName
            << std::endl;
    }

    for ( uint32_t i = 0; i < policyIt->second.dist_order.size(); ++i ) {
        printAction( "distribution", policyIt->second.dist_order[i] );
    }

    indent -= INDINC;
}


void printAction( std::string action, std::string actionName )
{
    if ( action == "classification" ) {
        printCC( actionName );
    }
    else if ( action == "distribution" ) {
        printDistribution( actionName );
    }
    else if ( action == "policer" ) {
    }
    else if ( action == "drop" ) {
        printDrop();
    }
    else if ( action.empty() ) {
    }
    else {
        ofs << "err: invalid action "
            << action
            << std::endl;
    }
}


void printDrop()
{
    indent += INDINC;

    ofs << ind( indent )
        << "drop: -->"
        << std::endl;

    indent -= INDINC;
}


void printDistribution( std::string dist_name )
{
    std::map< std::string, CDistribution >::const_iterator distIt =
        task.distributions.find( dist_name );
    if ( distIt == task.distributions.end() ) {
        ofs << "err: distribution "
            << dist_name
            << " not found!"
            << std::endl;
        return;
    }

    indent += INDINC;

    const CDistribution& dist = distIt->second;

    ofs << ind( indent )
        << "dist:" << dist.name
        << "(" << dist.qbase
        << "," << dist.qcount
        << "," << dist.keyShift
        << "," << dist.symmetricHash
        << ")"
        << std::endl;

    printAction( dist.action, dist.actionName );

    indent -= INDINC;
}


void printCC( std::string cc_name )
{
    std::map< std::string, CClassification >::const_iterator ccIt =
        task.classifications.find( cc_name );
    if ( ccIt == task.classifications.end() ) {
        ofs << "err: classification "
            << cc_name
            << " not found!"
            << std::endl;
        return;
    }

    indent += INDINC;

    const CClassification& cc = ccIt->second;

    ofs << ind( indent )
        << "cc:" << cc.name
        << "(" << cc.qbase << ")"
        << std::endl;

    std::map< std::string,
              std::pair< std::string, std::string > > actions;
    for ( uint32_t i = 0; i < cc.entries.size(); ++i ) {
        actions[cc.entries[i].action + cc.entries[i].actionName] =
            std::pair< std::string, std::string >
                ( cc.entries[i].action, cc.entries[i].actionName );
    }
    actions[cc.actionOnMiss + cc.actionNameOnMiss] =
        std::pair< std::string, std::string >
            ( cc.actionOnMiss, cc.actionNameOnMiss );

    std::map< std::string,
              std::pair< std::string, std::string > >::iterator it;
    for ( it = actions.begin(); it != actions.end(); ++it ) {
        printAction( it->second.first, it->second.second );
    }
        
    indent -= INDINC;
}


private:
    static const uint32_t INDINC = 4;

    std::ofstream&     ofs;
    const    CTaskDef& task;
    uint32_t           indent;
};


std::string error_text;
std::string compile_dump;

#ifdef __cplusplus
extern "C"
#endif
int fmc_compile(
        fmc_model*   cmodel,
        const char*  nameCfg,
        const char*  namePCD,
        const char*  namePDL,
        const char*  nameSP,
        unsigned int swOffset,
        unsigned int dontWarn,
        const char** dump
)
{
    error_text   = "";
    compile_dump = "";

    try {
        if ( nameSP != 0 && std::string( nameSP ) == "nodefault" ) {
            nameSP = 0;
        }
        if ( namePCD != 0 && std::string( namePCD ) == "nodefault" ) {
            namePCD = 0;
        }
        if ( nameCfg != 0 && std::string( nameCfg ) == "nodefault" ) {
            nameCfg = 0;
        }

        CGenericError::dontWarn = dontWarn;

        CTaskDef task;

        // Process PDL and SP files
        CPDLReader pdlReader;
        pdlReader.setTaskData( &task );
        pdlReader.parseNetPDL( namePDL );

        // Process custom protocols definition - software parser portion
        if ( nameSP ) {
            pdlReader.setSoftParse(1);
            pdlReader.parseNetPDL( nameSP );
            bool keepAsm = false;
            softparser(&task, nameSP, keepAsm, swOffset);
        }

        if ( namePCD && nameCfg )
        {
            // Process PCD file
            CPCDReader pcdReader;
            pcdReader.setTaskData( &task );
            pcdReader.parseNetPCD( namePCD );

            // Process config file
            CCFGReader cfgReader;
            cfgReader.setTaskData( &task );
            cfgReader.parseCfgData( nameCfg );

            task.checkSemantics();

//            std::ofstream ofs( ".\\dumpActions.txt" );
//            CDumpActions da( ofs, task );
//            da.printActions();
//            error_text = "Error: temporary termination";
            //return 10;

            CFMCModel model;
            model.createModel( &task );

            CFMCCModelOutput   modelOut;
            std::ostringstream oss;
            modelOut.output( model, cmodel, oss, 0 );

            compile_dump = oss.str();
        }

        if ( dump != 0 ) {
            *dump        = compile_dump.c_str();
        }
    }
    catch ( const CGenericError& genError ) { // Catch known errors generated by FMC
        error_text = "Error: " + genError.getErrorMsg();
        return 3;
    }
    catch ( std::exception &e )
    {
        error_text  = "Error: ";
        error_text += e.what();
        error_text += " Type : ";
        error_text += typeid( e ).name();
        return 4;
    }
    catch ( ... ) {                     // Catch unknown errors
        error_text = "Error: "
                     "Unrecognized error occured. The program was terminated";
        return 1;
    }

    return 0;
}


#ifdef __cplusplus
extern "C"
#endif
const char* fmc_get_error()
{
    return error_text.c_str();
}
