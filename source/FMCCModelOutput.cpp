/* =====================================================================
 *
 *  Copyright 2009-2012, Freescale Semiconductor, Inc., All Rights Reserved.
 *
 *  This file contains copyrighted material. Use of this file is restricted
 *  by the provisions of a Freescale Software License Agreement, which has
 *  either accompanied the delivery of this software in shrink wrap
 *  form or been expressly executed between the parties.
 *
 *  File Name : FMCCModelOutput.cpp
 *  Author    : Serge Lamikhov-Center
 *
 * ===================================================================*/

#define _CRT_SECURE_NO_WARNINGS

#include <sstream>
#include <iomanip>
#include <string.h>

#include "FMCCModelOutput.h"


static std::string ind( size_t count )
{
    return std::string().append( count, ' ' );
}

#define QUOTE(x)    # x

#define EMIT1( A )              \
    oss << ind( indent ) << A << std::endl;

#define EMIT2( A, B )           \
    cmodel->A B;                \
    oss << ind( indent ) << "." << QUOTE( A ) << " " << B << "," << std::endl;

#define EMIT3( A, B, C )        \
    cmodel->A B C;              \
    oss << ind( indent ) << "." << QUOTE( A ) << B << QUOTE( C ) << "," << std::endl;

#define EMIT4( A, B, C, D )     \
    cmodel->A B C D;            \
    oss << ind( indent ) << "." << QUOTE( A ) << B << QUOTE( C ) << " " << D << "," << std::endl;

#define EMIT4STR( A, B, C, D )  \
    cmodel->A B C D;          \
    oss << ind( indent ) << "." << QUOTE( A ) << B << QUOTE( C ) << D ## Str << "," << std::endl;

#define EMIT5( A, B, C, D, E )   \
    cmodel->A B C D E;          \
    oss << ind( indent ) << "." << QUOTE( A ) << B << QUOTE( C ) << D << QUOTE( E ) << "," << std::endl;

#define EMIT5_2( A, B, C, D, E )  \
    cmodel->A B C D E;                  \
    oss << ind( indent ) << QUOTE( D ) << E << "," << std::endl;

#define EMIT5_2STR( A, B, C, D, E )  \
    cmodel->A B C D E;                  \
    oss << ind( indent ) << QUOTE( D ) << E ## Str << "," << std::endl;

#define EMIT6( A, B, C, D, E, F )   \
    cmodel->A B C D E F;          \
    oss << ind( indent ) << "." << QUOTE( A ) << B << QUOTE( C ) << D << QUOTE( E ) << F << "," << std::endl;

#define EMIT6STR( A, B, C, D, E, F )    \
    cmodel->A B C D E F;              \
    oss << ind( indent ) << "." << QUOTE( A ) << B << QUOTE( C ) << D << QUOTE( E ) << F ## Str << "," << std::endl;

#define EMIT7_2( A, B, C, D, E, F, G ) \
    cmodel->A B C D E F G;              \
    oss << ind( indent ) << QUOTE( F ) << G << "," << std::endl;

#define EMIT7_2A( A, B, C, D, E, F, G ) \
    cmodel->A B C D E F G;              \
    oss << ind( indent ) << "."                                         \
        << QUOTE( A ) << B << QUOTE( C ) << D                 \
        << QUOTE( E ) << QUOTE( F ) << G                                               \
        << "," << std::endl;

#define EMIT7_2STR( A, B, C, D, E, F, G )  \
    cmodel->A B C D E F G;                  \
    oss << ind( indent ) << QUOTE( F ) << G ## Str << "," << std::endl;

#define EMIT7SP( A, B, C, D, D1, E, F )                                 \
    cmodel->A B C D E F;                                                \
    oss << ind( indent ) << "."                                         \
        << QUOTE( A )  << B << QUOTE( C ) << QUOTE( D1 )                \
        << E << QUOTE( F )                                              \
        << "," << std::endl;

#define EMIT11SP( A, B, C, D, E, F, F1, G, H, I, J )                    \
    cmodel->A B C D E = F G H I J;                                      \
    oss << ind( indent ) << "."                                         \
        << QUOTE( A )  << B << QUOTE( C ) << D << QUOTE( E ) << " = "   \
        << QUOTE( F1 ) << G << QUOTE( H ) << I << QUOTE( J )            \
        << "," << std::endl;

#define OUT_EMPTY   \
    oss << std::endl;

void
CFMCCModelOutput::output( const CFMCModel& model, fmc_model_t* cmodel, std::ostream& oss,
                          size_t indent )
{
    if ( model.spEnable ) {
        oss << "#include \"softparse.h\"" << std::endl;
    }

    OUT_EMPTY;
    oss << ind( indent ) << "struct fmc_model_t cmodel = {" << std::endl;
    indent += 4;

    // Output format version
    EMIT2( format_version =, FMC_OUTPUT_FORMAT_VER );

    // Output Soft Parser
    EMIT2( sp_enable =, model.spEnable );
    if ( cmodel->sp_enable ) {
        cmodel->sp = model.swPrs;
        memcpy( cmodel->spCode, model.swPrs.p_Code, cmodel->sp.size );
        cmodel->sp.p_Code = (uint8_t*)&(cmodel->spCode);
        oss << ind( indent ) << "SOFT_PARSE_CODE," << std::endl;
    }
    else {
        oss << ind( indent ) << "{" << std::endl;
        oss << ind( indent ) << "}," << std::endl;
    }
    OUT_EMPTY;

    // Output all engines
    EMIT2( fman_count =, model.all_engines.size() );
    for ( unsigned int i = 0; i < cmodel->fman_count; ++i ) {
        output_fmc_fman( model, cmodel, i, oss, indent );
    }
    OUT_EMPTY;
    // Output all ports
    EMIT2( port_count =, model.all_ports.size() )
    for ( unsigned int i = 0; i < cmodel->port_count; ++i ) {
        output_fmc_port( model, cmodel, i, oss, indent );
    }
    OUT_EMPTY;
    // Output all schemes
    EMIT2( scheme_count =, model.all_schemes.size() )
    for ( unsigned int i = 0; i < cmodel->scheme_count; ++i ) {
        OUT_EMPTY;
        output_fmc_scheme( model, cmodel, i, oss, indent );
    }
    OUT_EMPTY;
    // Output CC nodes
    EMIT2( ccnode_count =, model.all_ccnodes.size() )
    for ( unsigned int i = 0; i < cmodel->ccnode_count; ++i ) {
        OUT_EMPTY;
        output_fmc_ccnode( model, cmodel, i, oss, indent );
    }
	OUT_EMPTY;
#if (DPAA_VERSION >= 11)
    // Output Replicators
    EMIT2( replicator_count =, model.all_replicators.size() )
    for ( unsigned int i = 0; i < cmodel->replicator_count; ++i ) {
        OUT_EMPTY;
        output_fmc_replicator( model, cmodel, i, oss, indent );
    }
#endif /* (DPAA_VERSION >= 11) */
    OUT_EMPTY;
    // Output policers
    EMIT2( policer_count =, model.all_policers.size() )
    for ( unsigned int i = 0; i < cmodel->policer_count; ++i ) {
        OUT_EMPTY;
        output_fmc_policer( model, cmodel, i, oss, indent );
    }
    OUT_EMPTY;
    // Output apply order
    EMIT2( ao_count =, model.applier.size() )
    for ( int i = cmodel->ao_count - 1; i >= 0; --i ) {
        output_fmc_applier( model, cmodel, i, oss, indent );
    }

    indent -= 4;
    oss << ind( indent ) << "};" << std::endl;
}


void
CFMCCModelOutput::output_fmc_fman( const CFMCModel& model, fmc_model_t* cmodel,
                                   unsigned int index,
                                   std::ostream& oss, size_t indent )
{
    EMIT1( std::string( "/* FMan: " + model.all_engines[index].name + " */" ) );

    strncpy( cmodel->fman[index].name, model.all_engines[index].name.c_str(), FMC_NAME_LEN - 1 );
    cmodel->fman[index].name[FMC_NAME_LEN - 1] = 0;
    oss << ind( indent ) << ".fman[" << index << "].name = \"" << model.all_engines[index].name << "\"," << std::endl;
    EMIT4( fman[, index, ].number =, model.all_engines[index].number );

    strncpy( cmodel->fman[index].pcd_name, model.all_engines[index].pcd_name.c_str(), FMC_NAME_LEN - 1 );
    cmodel->fman[index].pcd_name[FMC_NAME_LEN - 1] = 0;
    oss << ind( indent ) << ".fman[" << index << "].pcd_name = \"" << model.all_engines[index].pcd_name << "\"," << std::endl;

    EMIT4( fman[, index, ].port_count =, model.all_engines[index].ports.size() );
    for ( unsigned int i = 0; i < cmodel->fman[index].port_count; ++i ) {
        EMIT6( fman[, index, ].ports[, i, ] =, model.all_engines[index].ports[i] )
    }

    OUT_EMPTY;

#ifndef P1023
    if ( model.all_engines[index].offload_support != 0 ) {
        EMIT4( fman[, index, ].offload_support =, model.all_engines[index].offload_support );
        OUT_EMPTY;
    }

    EMIT4( fman[, index, ].reasm_count =, model.all_engines[index].reasm.size() );
    for ( unsigned int i = 0; i < model.all_engines[index].reasm.size(); i++ ) {
        strncpy( cmodel->fman[index].reasm_name[i], model.all_engines[index].reasm_names[i].c_str(), FMC_NAME_LEN - 1 );
        cmodel->fman[index].reasm_name[i][FMC_NAME_LEN - 1] = 0;
        oss << ind( indent ) << ".fman[" << index << "].reasm_name[" << i << "] = \"" << model.all_engines[index].reasm_names[i] << "\"," << std::endl;

        EMIT5( fman[, index, ].reasm[, i, ].type = e_FM_PCD_MANIP_REASSEM );
        EMIT5( fman[, index, ].reasm[, i, ].h_NextManip = 0 );
        EMIT5( fman[, index, ].reasm[, i, ].u.reassem.hdr = HEADER_TYPE_IPv6 );

        EMIT7_2A( fman[, index, ].reasm[, i, ].u.reassem.u, .ipReassem.maxNumFramesInProcess =,
            model.all_engines[index].reasm[i].u.reassem.u.ipReassem.maxNumFramesInProcess );
        EMIT7_2A( fman[, index, ].reasm[, i, ].u.reassem.u, .ipReassem.timeOutMode =,
            model.all_engines[index].reasm[i].u.reassem.u.ipReassem.timeOutMode );
        EMIT7_2A( fman[, index, ].reasm[, i, ].u.reassem.u, .ipReassem.numOfFramesPerHashEntry[0] =,
            model.all_engines[index].reasm[i].u.reassem.u.ipReassem.numOfFramesPerHashEntry[0] );
        EMIT7_2A( fman[, index, ].reasm[, i, ].u.reassem.u, .ipReassem.numOfFramesPerHashEntry[1] =,
            model.all_engines[index].reasm[i].u.reassem.u.ipReassem.numOfFramesPerHashEntry[1] );
        EMIT7_2A( fman[, index, ].reasm[, i, ].u.reassem.u, .ipReassem.timeoutThresholdForReassmProcess =,
            model.all_engines[index].reasm[i].u.reassem.u.ipReassem.timeoutThresholdForReassmProcess );
        EMIT7_2A( fman[, index, ].reasm[, i, ].u.reassem.u, .ipReassem.fqidForTimeOutFrames =,
            model.all_engines[index].reasm[i].u.reassem.u.ipReassem.fqidForTimeOutFrames );
//        EMIT7_2( fman[, index, ].reasm[, i, ].fragOrReasmParams, .ipReasmParams.relativeSchemeId[0] =, 0 );
//        EMIT7_2( fman[, index, ].reasm[, i, ].fragOrReasmParams, .ipReasmParams.relativeSchemeId[1] =, 1 );
        EMIT7_2A( fman[, index, ].reasm[, i, ].u.reassem.u, .ipReassem.minFragSize[0] =,
            model.all_engines[index].reasm[i].u.reassem.u.ipReassem.minFragSize[0] );
        EMIT7_2A( fman[, index, ].reasm[, i, ].u.reassem.u, .ipReassem.minFragSize[1] =,
            model.all_engines[index].reasm[i].u.reassem.u.ipReassem.minFragSize[1] );
#if (DPAA_VERSION >= 11)
#else
        EMIT7_2A( fman[, index, ].reasm[, i, ].u.reassem.u, .ipReassem.sgBpid =,
            (int)model.all_engines[index].reasm[i].u.reassem.u.ipReassem.sgBpid );
#endif /* (DPAA_VERSION >= 11) */
        EMIT7_2A( fman[, index, ].reasm[, i, ].u.reassem.u, .ipReassem.dataLiodnOffset =,
            model.all_engines[index].reasm[i].u.reassem.u.ipReassem.dataLiodnOffset );
        EMIT7_2A( fman[, index, ].reasm[, i, ].u.reassem.u, .ipReassem.dataMemId =,
            (int)model.all_engines[index].reasm[i].u.reassem.u.ipReassem.dataMemId );

        //indent -= 4;
        //EMIT1( "}," );
    }

    OUT_EMPTY;

    EMIT4( fman[, index, ].frag_count =, model.all_engines[index].frags.size() );
    for ( unsigned int i = 0; i < model.all_engines[index].frags.size(); i++ ) {
        strncpy( cmodel->fman[index].frag_name[i], model.all_engines[index].frag_names[i].c_str(), FMC_NAME_LEN - 1 );
        cmodel->fman[index].frag_name[i][FMC_NAME_LEN - 1] = 0;
        oss << ind( indent ) << ".fman[" << index << "].frag_name[" << i << "] = \"" << model.all_engines[index].frag_names[i] << "\"," << std::endl;

        EMIT5( fman[, index, ].frag[, i, ].type = e_FM_PCD_MANIP_FRAG );
        EMIT5( fman[, index, ].frag[, i, ].h_NextManip = 0 );
        EMIT5( fman[, index, ].frag[, i, ].u.frag.hdr = HEADER_TYPE_IPv6 );

        EMIT7_2A( fman[, index, ].frag[, i,].u.frag.u, .ipFrag.sizeForFragmentation =,
            model.all_engines[index].frags[i].u.frag.u.ipFrag.sizeForFragmentation );
#if (DPAA_VERSION == 10)
        EMIT7_2A( fman[, index, ].frag[, i,].u.frag.u, .ipFrag.scratchBpid =,
            (int)model.all_engines[index].frags[i].u.frag.u.ipFrag.scratchBpid );
#endif /* (DPAA_VERSION == 10) */
        EMIT7_2A( fman[, index, ].frag[, i,].u.frag.u, .ipFrag.dontFragAction =,
            model.all_engines[index].frags[i].u.frag.u.ipFrag.dontFragAction );
        EMIT7_2A( fman[, index, ].frag[, i,].u.frag.u, .ipFrag.sgBpidEn =,
            model.all_engines[index].frags[i].u.frag.u.ipFrag.sgBpidEn );
        EMIT7_2A( fman[, index, ].frag[, i,].u.frag.u, .ipFrag.sgBpid =,
            (int)model.all_engines[index].frags[i].u.frag.u.ipFrag.sgBpid );

        //indent -= 4;
        //EMIT1( "}," );
    }

    OUT_EMPTY;

    EMIT4( fman[, index, ].hdr_count =, model.all_engines[index].headerManips.size() );
    for ( unsigned int i = 0; i < model.all_engines[index].headerManips.size(); i++ ) {
        strncpy( cmodel->fman[index].hdr_name[i], model.all_engines[index].headerManips_names[i].c_str(), FMC_NAME_LEN - 1 );
        cmodel->fman[index].hdr_name[i][FMC_NAME_LEN - 1] = 0;
        oss << ind( indent ) << ".fman[" << index << "].hdr_name[" << i << "] = \"" << model.all_engines[index].headerManips_names[i] << "\"," << std::endl;

        EMIT5( fman[, index, ].hdr[, i, ].type = e_FM_PCD_MANIP_HDR );
        EMIT5( fman[, index, ].hdr[, i, ].h_NextManip = 0 );

        EMIT7_2A( fman[, index, ].hdr[, i,].u.hdr, .insrt =,
            model.all_engines[index].headerManips[i].u.hdr.insrt );
        EMIT7_2A( fman[, index, ].hdr[, i,].u.hdr, .rmv =,
            model.all_engines[index].headerManips[i].u.hdr.rmv );
        EMIT7_2A( fman[, index, ].hdr[, i,].u.hdr, .dontParseAfterManip =,
            model.all_engines[index].headerManips[i].u.hdr.dontParseAfterManip );

        if ( model.all_engines[index].headerManips[i].u.hdr.insrt )
        {
            EMIT7_2A( fman[, index, ].hdr[, i,].u.hdr.insrtParams, .type =,
                model.all_engines[index].headerManips[i].u.hdr.insrtParams.type );
            EMIT7_2A( fman[, index, ].hdr[, i,].u.hdr.insrtParams.u.generic, .size =,
                (int)model.all_engines[index].headerManips[i].u.hdr.insrtParams.u.generic.size );
            EMIT7_2A( fman[, index, ].hdr[, i,].u.hdr.insrtParams.u.generic, .offset =,
                (int)model.all_engines[index].headerManips[i].u.hdr.insrtParams.u.generic.offset );
            EMIT7_2A( fman[, index, ].hdr[, i,].u.hdr.insrtParams.u.generic, .replace =,
                model.all_engines[index].headerManips[i].u.hdr.insrtParams.u.generic.replace );

            oss << ind( indent ) << ".fman[" << index << "].insertData[" << i << "] = {";
            for ( unsigned int j = 0; j < (int)model.all_engines[index].headerManips[i].u.hdr.insrtParams.u.generic.size; ++j ) {
                    if ( j != 0 ) {
                        oss << ",";
                    }
                    oss << " 0x" << std::hex << std::setw( 2 ) << std::setfill( '0' ) << (unsigned int)model.all_engines[index].insertDatas[i].data[j];
                    oss << std::dec << std::resetiosflags(std::ios::basefield | std::ios::internal);
                    cmodel->fman[index].insertData[i][j] = model.all_engines[index].insertDatas[i].data[j];
            }
            oss << " }," << std::endl;
        }

        if ( model.all_engines[index].headerManips[i].u.hdr.rmv )
        {
            EMIT7_2A( fman[, index, ].hdr[, i,].u.hdr.rmvParams, .type =,
                model.all_engines[index].headerManips[i].u.hdr.rmvParams.type );
            EMIT7_2A( fman[, index, ].hdr[, i,].u.hdr.rmvParams.u.generic, .size =,
                (int)model.all_engines[index].headerManips[i].u.hdr.rmvParams.u.generic.size );
            EMIT7_2A( fman[, index, ].hdr[, i,].u.hdr.rmvParams.u.generic, .offset =,
                (int)model.all_engines[index].headerManips[i].u.hdr.rmvParams.u.generic.offset );
        }
    }

    OUT_EMPTY;
#endif /* P1023 */
}


void
CFMCCModelOutput::output_fmc_port( const CFMCModel& model, fmc_model_t* cmodel,
                                   unsigned int index,
                                   std::ostream& oss, size_t indent )
{
    EMIT1( std::string( "/* FMan port: " + model.all_ports[index].name + " */" ) );

    strncpy( cmodel->port[index].name, model.all_ports[index].name.c_str(), FMC_NAME_LEN - 1 );
    cmodel->port[index].name[FMC_NAME_LEN - 1] = 0;
    oss << ind( indent ) << ".port[" << index << "].name = \"" << model.all_ports[index].name << "\"," << std::endl;

    EMIT4STR( port[, index, ].type =, model.all_ports[index].type );
    EMIT4( port[, index, ].number  = ,  model.all_ports[index].number );

    EMIT4( port[, index, ].schemes_count =, model.all_ports[index].schemes.size() );
    for ( unsigned int i = 0; i < cmodel->port[index].schemes_count; ++i ) {
        EMIT6( port[, index, ].schemes[, i, ] =, model.all_ports[index].schemes[i] );
    }

    EMIT4( port[, index, ].ccnodes_count =, model.all_ports[index].ccnodes.size() );
    for ( unsigned int i = 0; i < cmodel->port[index].ccnodes_count; ++i ) {
        EMIT6( port[, index, ].ccnodes[, i, ] =, model.all_ports[index].ccnodes[i] );
    }

    EMIT4( port[, index, ].ccroot_count =, model.all_ports[index].cctrees.size() );
    for ( unsigned int i = 0; i < cmodel->port[index].ccroot_count; ++i ) {
        EMIT6( port[, index, ].ccroot[, i, ] =, model.all_ports[index].cctrees[i] );
    }

#ifndef P1023
    for ( unsigned int i = 0; i < cmodel->port[index].ccroot_count; ++i ) {
        EMIT6( port[, index, ].ccroot_manip[, i, ] =, model.all_ports[index].hdrmanips[i] );
    }
#endif /* P1023 */

    // Distinction units
    unsigned int numOfDistUnits = model.all_ports[index].protocols.size();
    if ( model.all_ports[index].reasm_index != 0 ) {
        numOfDistUnits += 2;
    }
    EMIT4( port[, index, ].distinctionUnits.numOfDistinctionUnits =, numOfDistUnits );

    std::map< Protocol,
        std::pair< unsigned int, DistinctionUnitElement > >::const_iterator protoIt;
    for ( protoIt  = model.all_ports[index].protocols.begin();
          protoIt != model.all_ports[index].protocols.end();
          ++protoIt ) {
        EMIT6STR( port[, index, ].distinctionUnits.units[, protoIt->second.first, ].hdrs[0].hdr =, protoIt->second.second.hdr );
        if ( protoIt->second.second.opt != "" )
        {
            switch (cmodel->port[index].distinctionUnits.units[protoIt->second.first].hdrs[0].hdr)
            {
            case HEADER_TYPE_ETH:
                EMIT6( port[, index, ].distinctionUnits.units[, protoIt->second.first, ].hdrs[0].opt.ethOpt =, strtoul( protoIt->second.second.opt.c_str(), 0, 16 ) );
                break;
            case HEADER_TYPE_VLAN:
                EMIT6( port[, index, ].distinctionUnits.units[, protoIt->second.first, ].hdrs[0].opt.vlanOpt =, strtoul( protoIt->second.second.opt.c_str(), 0, 16 ) );
                break;
            case HEADER_TYPE_MPLS:
                EMIT6( port[, index, ].distinctionUnits.units[, protoIt->second.first, ].hdrs[0].opt.mplsOpt =, strtoul( protoIt->second.second.opt.c_str(), 0, 16 ) );
                break;
            case HEADER_TYPE_IPv4:
                EMIT6( port[, index, ].distinctionUnits.units[, protoIt->second.first, ].hdrs[0].opt.ipv4Opt =, strtoul( protoIt->second.second.opt.c_str(), 0, 16 ) );
                break;
            case HEADER_TYPE_IPv6:
                EMIT6( port[, index, ].distinctionUnits.units[, protoIt->second.first, ].hdrs[0].opt.ipv6Opt =, strtoul( protoIt->second.second.opt.c_str(), 0, 16 ) );
                break;
            }
        }
    }

#ifndef P1023
    if ( model.all_ports[index].reasm_index != 0 ) {
        EMIT5( port[, index, ].distinctionUnits.units[, numOfDistUnits - 2,].hdrs[0].hdr = HEADER_TYPE_IPv4 );
        EMIT5( port[, index, ].distinctionUnits.units[, numOfDistUnits - 2,].hdrs[0].opt.ipv4Opt = IPV4_FRAG_1 );
        EMIT5( port[, index, ].distinctionUnits.units[, numOfDistUnits - 1,].hdrs[0].hdr = HEADER_TYPE_IPv6 );
        EMIT5( port[, index, ].distinctionUnits.units[, numOfDistUnits - 1,].hdrs[0].opt.ipv6Opt = IPV6_FRAG_1 );
    }
#endif /* P1023 */

    // Fill PCD params
    if ( ( !model.all_ports[index].cctrees.empty() || model.all_ports[index].reasm_index != 0 )
         && !model.all_ports[index].schemes.empty() && !model.all_policers.empty() ) {
        EMIT3( port[, index, ].pcdParam.pcdSupport = e_FM_PORT_PCD_SUPPORT_PRS_AND_KG_AND_CC_AND_PLCR );
    }
    else if ( ( !model.all_ports[index].cctrees.empty() || model.all_ports[index].reasm_index != 0 )
         && !model.all_ports[index].schemes.empty() ) {
        EMIT3( port[, index, ].pcdParam.pcdSupport = e_FM_PORT_PCD_SUPPORT_PRS_AND_KG_AND_CC );
    }
    else if ( !model.all_ports[index].schemes.empty() && !model.all_policers.empty() ) {
        EMIT3( port[, index, ].pcdParam.pcdSupport = e_FM_PORT_PCD_SUPPORT_PRS_AND_KG_AND_PLCR );
    }
    else if ( !model.all_ports[index].schemes.empty() ) {
        EMIT3( port[, index, ].pcdParam.pcdSupport = e_FM_PORT_PCD_SUPPORT_PRS_AND_KG );
    }
    else {
        EMIT3( port[, index, ].pcdParam.pcdSupport = e_FM_PORT_PCD_SUPPORT_PRS_ONLY );
    }

    // Parser params
    EMIT3( port[, index, ].prsParam.parsingOffset = 0 );
    EMIT4( port[, index, ].prsParam.prsResultPrivateInfo =, model.all_ports[index].portid );
    EMIT3( port[, index, ].prsParam.firstPrsHdr = HEADER_TYPE_ETH );
    if ( model.spEnable ) {
        EMIT4( port[, index, ].prsParam.numOfHdrsWithAdditionalParams =, (unsigned int)model.swPrs.numOfLabels );
        for ( unsigned int i = 0; i < model.swPrs.numOfLabels; ++i ) {
            {
                // Special case - we don't have .hdr and corresponding .hdrStr fields in this structure
                cmodel->port[index].prsParam.additionalParams[i].hdr = model.swPrs.labelsTable[i].hdr;
                oss << ind( indent ) << ".port[" << index << "].prsParam.additionalParams[" << i << "].hdr =" <<
                    CFMCModel::getNetCommHeaderTypeStr( model.swPrs.labelsTable[i].hdr ) << "," <<std::endl;
            }
            EMIT5( port[, index, ].prsParam.additionalParams[, i, ].errDisable  = 0 );
            EMIT5( port[, index, ].prsParam.additionalParams[, i, ].swPrsEnable = 1 );
            EMIT5( port[, index, ].prsParam.additionalParams[, i, ].usePrsOpts  = 0 );
            EMIT5( port[, index, ].prsParam.additionalParams[, i, ].indexPerHdr = 0 );
            EMIT5( port[, index, ].prsParam.additionalParams[, i, ].usePrsOpts  = 0 );
        }
    }

    EMIT4( port[, index, ].reasm_index =, model.all_ports[index].reasm_index );

    OUT_EMPTY;
}


void
CFMCCModelOutput::output_fmc_scheme( const CFMCModel& model, fmc_model_t* cmodel,
                                     unsigned int index,
                                     std::ostream& oss, size_t indent )
{
    const Scheme& sch = model.all_schemes[index];

    EMIT1( std::string( "/* Distribution: " + sch.name + " */" ) );

    strncpy( cmodel->scheme_name[index], sch.name.c_str(), FMC_NAME_LEN - 1 );
    cmodel->scheme_name[index][FMC_NAME_LEN - 1] = 0;
    oss << ind( indent ) << ".scheme_name[" << index << "] = " << std::string( "\"" + sch.name + "\"" ) << "," << std::endl;

    EMIT4( scheme[, index, ].alwaysDirect        =, model.all_schemes[index].isDirect );
//    EMIT4( scheme[, index, ].id.relativeSchemeId =, (unsigned int)model.all_schemes[index].relativeSchemeId );
    EMIT4( scheme[, index, ].netEnvParams.numOfDistinctionUnits =, sch.used_protocols.size() );

    // Find port this scheme belongs to
    unsigned int port = 0;
    for ( unsigned int i = 0; i < model.all_ports.size(); i++ ) {
        for ( unsigned int j = 0; j < model.all_ports[i].schemes.size(); j++ ) {
            if ( model.all_ports[i].schemes[j] == index ) {
                port = i;
            }
        }
    }

    // Fill unitIds for used protocols
    int used_protocols_count = 0;
    std::set< Protocol >::const_iterator used_protocolsIt;
    for ( used_protocolsIt = sch.used_protocols.begin();
          used_protocolsIt != sch.used_protocols.end();
          ++used_protocolsIt, ++used_protocols_count ) {
        // Find index of this protocol
        unsigned int protoNum = model.all_ports[port].protocols.find( *used_protocolsIt )->second.first;
        EMIT6( scheme[, index, ].netEnvParams.unitIds[, used_protocols_count, ] =, protoNum );
    }

    unsigned int useHash = sch.key.empty() ? 0 : 1;
    EMIT4( scheme[, index, ].useHash =,  useHash );
    EMIT4( scheme[, index, ].baseFqid =, sch.qbase );

#if (DPAA_VERSION >= 11)
	EMIT4( scheme[, index, ].overrideStorageProfile =,  sch.overrideStorageProfile );
	if ( sch.overrideStorageProfile )
	{
		EMIT4( scheme[, index, ].storageProfile.direct =,  sch.storageProfile.direct );
		if ( sch.storageProfile.direct )
		{
			EMIT4( scheme[, index, ].storageProfile.profileSelect.directRelativeProfileId  =,  (unsigned int)sch.storageProfile.profileSelect.directRelativeProfileId );
		}
		else
		{
			EMIT4( scheme[, index, ].storageProfile.profileSelect.indirectProfile.fqidOffsetShift  =,  (unsigned int)sch.storageProfile.profileSelect.indirectProfile.fqidOffsetShift );
			EMIT4( scheme[, index, ].storageProfile.profileSelect.indirectProfile.fqidOffsetRelativeProfileIdBase  =,  (unsigned int)sch.storageProfile.profileSelect.indirectProfile.fqidOffsetRelativeProfileIdBase );
			EMIT4( scheme[, index, ].storageProfile.profileSelect.indirectProfile.numOfProfiles  =,  (unsigned int)sch.storageProfile.profileSelect.indirectProfile.numOfProfiles );
		}
	}
#endif /* (DPAA_VERSION >= 11) */

    EMIT4STR( scheme[, index, ].nextEngine =, sch.nextEngine );
    if ( sch.nextEngine == e_FM_PCD_DONE && sch.doneAction == e_FM_PCD_DROP_FRAME ) {
        EMIT4STR( scheme[, index, ].kgNextEngineParams.doneAction =, sch.doneAction );
    }
    if ( sch.nextEngine == e_FM_PCD_PLCR ) {
        EMIT4( scheme[, index, ].kgNextEngineParams.plcrProfile.sharedProfile =, 1 );
        EMIT4( scheme[, index, ].kgNextEngineParams.plcrProfile.direct =,        1 );
        EMIT4( scheme[, index, ].kgNextEngineParams.plcrProfile.profileSelect.
               directRelativeProfileId =, sch.actionHandleIndex );
    }
    else if ( sch.nextEngine == e_FM_PCD_CC ) {
        EMIT4( scheme[, index, ].kgNextEngineParams.cc.grpId =, sch.actionHandleIndex );
    }

    EMIT3( scheme[, index, ].schemeCounter.update = 1 );
    EMIT3( scheme[, index, ].schemeCounter.value  = 0 );
    EMIT3( scheme[, index, ].keyExtractAndHashParams.numOfUsedMasks = 0 );
    EMIT4( scheme[, index, ].keyExtractAndHashParams.hashShift =,      sch.hashShift );
    EMIT4( scheme[, index, ].keyExtractAndHashParams.symmetricHash =,  sch.symmetricHash );
    EMIT4( scheme[, index, ].keyExtractAndHashParams.hashDistributionNumOfFqids =,
           sch.qcount );

    EMIT4( scheme[, index, ].keyExtractAndHashParams.numOfUsedExtracts =, sch.key.size() );
    if (sch.privateDflt0 > 0)
    {
        EMIT4( scheme[, index, ].keyExtractAndHashParams.privateDflt0 =, sch.privateDflt0 );
    }
    if (sch.privateDflt1 > 0)
    {
        EMIT4( scheme[, index, ].keyExtractAndHashParams.privateDflt1 =, sch.privateDflt1 );
    }
    if (sch.defaults.size() > 0)
    {
        EMIT4( scheme[, index, ].keyExtractAndHashParams.numOfUsedDflts =, sch.defaults.size() );
    }
    // Fill extract parameters
    indent += 4;
    unsigned int i = 0;
    std::vector< ExtractData >::const_iterator extractIt;
    for ( extractIt = sch.key.begin();
          extractIt != sch.key.end();
          ++extractIt, ++i ) {
        if (extractIt->type == e_FM_PCD_EXTRACT_BY_HDR)
        {
            EMIT1( std::string( "/* Extract field:" ) + extractIt->fieldName + " */" );
            if ( extractIt->hdr == HEADER_TYPE_USER_DEFINED_SHIM1 ||
    #ifdef FM_SHIM3_SUPPORT
                extractIt->hdr == HEADER_TYPE_USER_DEFINED_SHIM2 ||
                extractIt->hdr == HEADER_TYPE_USER_DEFINED_SHIM3 ) {
    #else  /* FM_SHIM3_SUPPORT */
                extractIt->hdr == HEADER_TYPE_USER_DEFINED_SHIM2 ) {
    #endif /* FM_SHIM3_SUPPORT */
                if (sch.defaults.empty())
                {
                    EMIT3( scheme[, index, ].keyExtractAndHashParams.numOfUsedDflts = 1 );
                    EMIT3( scheme[, index, ].keyExtractAndHashParams.dflts[0].type = e_FM_PCD_KG_GENERIC_FROM_DATA_NO_V );
                    EMIT3( scheme[, index, ].keyExtractAndHashParams.dflts[0].dfltSelect = e_FM_PCD_KG_DFLT_GBL_0 );
                }
            }
            EMIT6STR( scheme[, index, ].keyExtractAndHashParams.extractArray[, i,
                                      ].type =, extractIt->type );

            // GCC does not compile designated init for unnamed units.
            // Put them into a block
            EMIT1( "{" );
            indent += 4;

            EMIT7_2STR( scheme[, index, ].keyExtractAndHashParams.extractArray[, i,
                                         ], .extractByHdr.hdr =,
                                         extractIt->hdr );
            EMIT7_2STR( scheme[, index, ].keyExtractAndHashParams.extractArray[, i,
                                         ], .extractByHdr.hdrIndex =,
                                         extractIt->hdrIndex );
            if ( extractIt->hdr == HEADER_TYPE_USER_DEFINED_SHIM1 ||
    #ifdef FM_SHIM3_SUPPORT
                 extractIt->hdr == HEADER_TYPE_USER_DEFINED_SHIM2 ||
                 extractIt->hdr == HEADER_TYPE_USER_DEFINED_SHIM3 ) {
    #else  /* FM_SHIM3_SUPPORT */
                extractIt->hdr == HEADER_TYPE_USER_DEFINED_SHIM2 ) {
    #endif /* FM_SHIM3_SUPPORT */
                EMIT7_2( scheme[, index, ].keyExtractAndHashParams.extractArray[, i,
                                           ],
                                           .extractByHdr.ignoreProtocolValidation =, 1 );
            }
            else {
                EMIT7_2( scheme[, index, ].keyExtractAndHashParams.extractArray[, i,
                                          ],
                                          .extractByHdr.ignoreProtocolValidation =, 0 );
            }
            EMIT7_2STR( scheme[, index, ].keyExtractAndHashParams.extractArray[, i, ],
                                          .extractByHdr.type =, extractIt->hdrtype );

            if ( extractIt->fieldType != 0xFFFFFFFF ) {
                switch( cmodel->scheme[index].keyExtractAndHashParams.extractArray[i].extractByHdr.hdr )
                {
                    case HEADER_TYPE_ETH:
                        EMIT7_2STR( scheme[, index, ].keyExtractAndHashParams.extractArray[, i,
                            ], .extractByHdr.extractByHdrType.fullField.eth =,
                            extractIt->fieldType );
                        break;
                    case HEADER_TYPE_VLAN:
                        EMIT7_2STR(
                            scheme[, index, ].keyExtractAndHashParams.extractArray[, i,
                            ], .extractByHdr.extractByHdrType.fullField.vlan =,
                            extractIt->fieldType );
                        break;
                    case HEADER_TYPE_LLC_SNAP:
                        EMIT7_2STR(
                            scheme[, index, ].keyExtractAndHashParams.extractArray[, i,
                            ], .extractByHdr.extractByHdrType.fullField.llcSnap =,
                            extractIt->fieldType );

                        break;
                    case HEADER_TYPE_PPPoE:
                        EMIT7_2STR(
                            scheme[, index, ].keyExtractAndHashParams.extractArray[, i,
                            ], .extractByHdr.extractByHdrType.fullField.pppoe=,
                            extractIt->fieldType );
                        break;
                    case HEADER_TYPE_MPLS:
                        EMIT7_2STR(
                            scheme[, index, ].keyExtractAndHashParams.extractArray[, i,
                            ], .extractByHdr.extractByHdrType.fullField.mpls=,
                            extractIt->fieldType );
                        break;
                    case HEADER_TYPE_IPv4:
                        EMIT7_2STR(
                            scheme[, index, ].keyExtractAndHashParams.extractArray[, i,
                            ], .extractByHdr.extractByHdrType.fullField.ipv4=,
                            extractIt->fieldType );
                        break;
                    case HEADER_TYPE_IPv6:
                        EMIT7_2STR(
                            scheme[, index, ].keyExtractAndHashParams.extractArray[, i,
                            ], .extractByHdr.extractByHdrType.fullField.ipv6=,
                            extractIt->fieldType );
                        break;
                    case HEADER_TYPE_TCP:
                        EMIT7_2STR(
                            scheme[, index, ].keyExtractAndHashParams.extractArray[, i,
                            ], .extractByHdr.extractByHdrType.fullField.tcp=,
                            extractIt->fieldType );
                        break;
                    case HEADER_TYPE_UDP:
                        EMIT7_2STR(
                            scheme[, index, ].keyExtractAndHashParams.extractArray[, i,
                            ], .extractByHdr.extractByHdrType.fullField.udp=,
                            extractIt->fieldType );
                        break;
                    case HEADER_TYPE_SCTP:
                        EMIT7_2STR(
                            scheme[, index, ].keyExtractAndHashParams.extractArray[, i,
                            ], .extractByHdr.extractByHdrType.fullField.sctp=,
                            extractIt->fieldType );
                        break;
                    case HEADER_TYPE_DCCP:
                        EMIT7_2STR(
                            scheme[, index, ].keyExtractAndHashParams.extractArray[, i,
                            ], .extractByHdr.extractByHdrType.fullField.dccp=,
                            extractIt->fieldType );
                        break;
                    case HEADER_TYPE_MINENCAP:
                        EMIT7_2STR(
                            scheme[, index, ].keyExtractAndHashParams.extractArray[, i,
                            ], .extractByHdr.extractByHdrType.fullField.minencap=,
                            extractIt->fieldType );
                        break;
                    case HEADER_TYPE_IPSEC_AH:
                        EMIT7_2STR(
                            scheme[, index, ].keyExtractAndHashParams.extractArray[, i,
                            ], .extractByHdr.extractByHdrType.fullField.ipsecAh=,
                            extractIt->fieldType );
                        break;
                    case HEADER_TYPE_IPSEC_ESP:
                        EMIT7_2STR(
                            scheme[, index, ].keyExtractAndHashParams.extractArray[, i,
                            ], .extractByHdr.extractByHdrType.fullField.ipsecEsp=,
                            extractIt->fieldType );
                        break;
                    case HEADER_TYPE_GRE:
                        EMIT7_2STR(
                            scheme[, index, ].keyExtractAndHashParams.extractArray[, i,
                            ], .extractByHdr.extractByHdrType.fullField.gre=,
                            extractIt->fieldType );
                        break;
                    default:
                        break;
                }
            }
            else {
                EMIT7_2( scheme[, index, ].keyExtractAndHashParams.extractArray[, i,
                          ], .extractByHdr.extractByHdrType.fromHdr.size =,
                          extractIt->size );

                EMIT7_2( scheme[, index, ].keyExtractAndHashParams.extractArray[, i,
                          ], .extractByHdr.extractByHdrType.fromHdr.offset =, extractIt->offset );

            }
        }//end of e_FM_PCD_EXTRACT_BY_HDR
        else
        {
            EMIT1( std::string( "/* Extract nonheader:" ) + extractIt->nhSourceStr + " */" );
            EMIT6STR( scheme[, index, ].keyExtractAndHashParams.extractArray[, i,
                                      ].type =, extractIt->type );

            // GCC does not compile designated init for unnamed units.
            // Put them into a block
            EMIT1( "{" );
            indent += 4;

            EMIT7_2STR( scheme[, index, ].keyExtractAndHashParams.extractArray[, i,
                                         ], .extractNonHdr.src =,
                                         extractIt->nhSource );
            EMIT7_2( scheme[, index, ].keyExtractAndHashParams.extractArray[, i,
                          ], .extractNonHdr.offset =, extractIt->nhOffset );
            EMIT7_2( scheme[, index, ].keyExtractAndHashParams.extractArray[, i,
                          ], .extractNonHdr.size =,
                          extractIt->nhSize );
        }

        indent -= 4;
        EMIT1( "}," );
    }

    indent -= 4;
    for ( i = 0; i < sch.defaults.size(); ++i ) {
        EMIT6STR( scheme[, index, ].keyExtractAndHashParams.dflts[, i, ].type =, sch.defaults[i].type );
        EMIT6STR( scheme[, index, ].keyExtractAndHashParams.dflts[, i, ].dfltSelect =, sch.defaults[i].select );
    }
    indent += 4;

    indent -= 4;
    EMIT4( scheme[, index, ].numOfUsedExtractedOrs =, sch.combines.size() );
    indent += 4;

    for ( i = 0; i < sch.combines.size(); ++i ) {
        EMIT6STR( scheme[, index, ].extractedOrs[, i, ].type =, sch.combines[i].type );

        EMIT1( "{" );
        indent += 4;
        if ( sch.combines[i].type != e_FM_PCD_EXTRACT_BY_HDR ) {
            EMIT7_2STR( scheme[, index, ].extractedOrs[, i, ],
                         .src =, sch.combines[i].src );
        }
        else {
            EMIT7_2STR( scheme[, index, ].extractedOrs[, i, ],
                         .extractByHdr.hdr =, sch.combines[i].hdr );
            EMIT7_2( scheme[, index, ].extractedOrs[, i, ],
                      .extractByHdr.ignoreProtocolValidation =, 1 );
        }
        indent -= 4;
        EMIT1( "}," );

        EMIT6( scheme[, index, ].extractedOrs[, i, ].extractionOffset =, sch.combines[i].offset );
        EMIT6( scheme[, index, ].extractedOrs[, i, ].dfltValue =,
            (e_FmPcdKgExtractDfltSelect)sch.combines[i].default_ );
        EMIT6( scheme[, index, ].extractedOrs[, i, ].mask =, sch.combines[i].mask );
        EMIT6( scheme[, index, ].extractedOrs[, i, ].bitOffsetInFqid =, sch.combines[i].offsetInFqid );
    }
    indent -= 8;
}


void
CFMCCModelOutput::output_fmc_ccnode( const CFMCModel& model, fmc_model_t* cmodel,
                                     unsigned int index,
                                     std::ostream& oss, size_t indent )
{
    const CCNode& node = model.all_ccnodes[index];

    EMIT1( std::string( "/* Coarse classification node: " ) + node.name + " */" );

    strncpy( cmodel->ccnode_name[index], node.name.c_str(), FMC_NAME_LEN - 1 );
    cmodel->ccnode_name[index][FMC_NAME_LEN - 1] = 0;
    oss << ind( indent ) << ".ccnode_name[" << index << "] = " << std::string( "\"" + node.name + "\"" ) << "," << std::endl;

    // Fill extract parameters
    EMIT4STR( ccnode[, index, ].extractCcParams.type =, node.extract.type );

    EMIT1( "{" );
    indent += 4;

    if ( cmodel->ccnode[index].extractCcParams.type == e_FM_PCD_EXTRACT_BY_HDR )
    {
        EMIT5_2STR( ccnode[, index, ].extractCcParams, .extractByHdr.hdr =,
                     node.extract.hdr );
        EMIT5_2STR( ccnode[, index, ].extractCcParams, .extractByHdr.hdrIndex =,
                     node.extract.hdrIndex );
        EMIT5_2STR( ccnode[, index, ].extractCcParams, .extractByHdr.type =,
                     node.extract.hdrtype );

        if ( node.extract.hdrtype == e_FM_PCD_EXTRACT_FULL_FIELD ) {
            switch ( node.extract.hdr ) {
                case HEADER_TYPE_ETH:
                    EMIT5_2STR( ccnode[, index, ].extractCcParams, .extractByHdr.extractByHdrType.fullField.eth =,
                        node.extract.fieldType );
                    break;
                case HEADER_TYPE_VLAN:
                    EMIT5_2STR( ccnode[, index, ].extractCcParams, .extractByHdr.extractByHdrType.fullField.vlan =,
                        node.extract.fieldType );
                    break;
                case HEADER_TYPE_LLC_SNAP:
                    EMIT5_2STR( ccnode[, index, ].extractCcParams, .extractByHdr.extractByHdrType.fullField.llcSnap =,
                        node.extract.fieldType );
                    break;
                case HEADER_TYPE_PPPoE:
                    EMIT5_2STR( ccnode[, index, ].extractCcParams, .extractByHdr.extractByHdrType.fullField.pppoe =,
                        node.extract.fieldType );
                    break;
                case HEADER_TYPE_MPLS:
                    EMIT5_2STR( ccnode[, index, ].extractCcParams, .extractByHdr.extractByHdrType.fullField.mpls =,
                        node.extract.fieldType );
                    break;
                case HEADER_TYPE_IPv4:
                    EMIT5_2STR( ccnode[, index, ].extractCcParams, .extractByHdr.extractByHdrType.fullField.ipv4 =,
                        node.extract.fieldType );
                    break;
                case HEADER_TYPE_IPv6:
                    EMIT5_2STR( ccnode[, index, ].extractCcParams, .extractByHdr.extractByHdrType.fullField.ipv6 =,
                        node.extract.fieldType );
                    break;
                case HEADER_TYPE_UDP:
                    EMIT5_2STR( ccnode[, index, ].extractCcParams, .extractByHdr.extractByHdrType.fullField.udp =,
                        node.extract.fieldType );
                    break;
                case HEADER_TYPE_TCP:
                    EMIT5_2STR( ccnode[, index, ].extractCcParams, .extractByHdr.extractByHdrType.fullField.tcp =,
                        node.extract.fieldType );
                    break;
                case HEADER_TYPE_SCTP:
                    EMIT5_2STR( ccnode[, index, ].extractCcParams, .extractByHdr.extractByHdrType.fullField.sctp =,
                        node.extract.fieldType );
                    break;
                case HEADER_TYPE_DCCP:
                    EMIT5_2STR( ccnode[, index, ].extractCcParams, .extractByHdr.extractByHdrType.fullField.dccp =,
                        node.extract.fieldType );
                    break;
                case HEADER_TYPE_GRE:
                    EMIT5_2STR( ccnode[, index, ].extractCcParams, .extractByHdr.extractByHdrType.fullField.gre =,
                        node.extract.fieldType );
                    break;
                case HEADER_TYPE_MINENCAP:
                    EMIT5_2STR( ccnode[, index, ].extractCcParams, .extractByHdr.extractByHdrType.fullField.minencap =,
                        node.extract.fieldType );
                    break;
                case HEADER_TYPE_IPSEC_AH:
                    EMIT5_2STR( ccnode[, index, ].extractCcParams, .extractByHdr.extractByHdrType.fullField.ipsecAh =,
                        node.extract.fieldType );
                    break;
                case HEADER_TYPE_IPSEC_ESP:
                    EMIT5_2STR( ccnode[, index, ].extractCcParams, .extractByHdr.extractByHdrType.fullField.ipsecEsp =,
                        node.extract.fieldType );
                    break;
                default:
                    break;

            }
        }
        else if ( node.extract.hdrtype == e_FM_PCD_EXTRACT_FROM_HDR ) {
            EMIT5_2( ccnode[, index, ].extractCcParams, .extractByHdr.extractByHdrType.fromHdr.size =,
                node.extract.size );
            EMIT5_2( ccnode[, index, ].extractCcParams, .extractByHdr.extractByHdrType.fromHdr.offset =,
                node.extract.offset );
        }
        else if ( node.extract.hdrtype == e_FM_PCD_EXTRACT_FROM_FIELD ) {
            switch ( node.extract.hdr ) {
                case HEADER_TYPE_ETH:
                    EMIT5_2STR( ccnode[, index, ].extractCcParams, .extractByHdr.extractByHdrType.fromField.field.eth =,
                        node.extract.fieldType );
                    break;
                case HEADER_TYPE_VLAN:
                    EMIT5_2STR( ccnode[, index, ].extractCcParams, .extractByHdr.extractByHdrType.fromField.field.vlan =,
                        node.extract.fieldType );
                    break;
                case HEADER_TYPE_LLC_SNAP:
                    EMIT5_2STR( ccnode[, index, ].extractCcParams, .extractByHdr.extractByHdrType.fromField.field.llcSnap =,
                        node.extract.fieldType );
                    break;
                case HEADER_TYPE_PPPoE:
                    EMIT5_2STR( ccnode[, index, ].extractCcParams, .extractByHdr.extractByHdrType.fromField.field.pppoe =,
                        node.extract.fieldType );
                    break;
                case HEADER_TYPE_MPLS:
                    EMIT5_2STR( ccnode[, index, ].extractCcParams, .extractByHdr.extractByHdrType.fromField.field.mpls =,
                        node.extract.fieldType );
                    break;
                case HEADER_TYPE_IPv4:
                    EMIT5_2STR( ccnode[, index, ].extractCcParams, .extractByHdr.extractByHdrType.fromField.field.ipv4 =,
                        node.extract.fieldType );
                    break;
                case HEADER_TYPE_IPv6:
                    EMIT5_2STR( ccnode[, index, ].extractCcParams, .extractByHdr.extractByHdrType.fromField.field.ipv6 =,
                        node.extract.fieldType );
                    break;
                case HEADER_TYPE_UDP:
                    EMIT5_2STR( ccnode[, index, ].extractCcParams, .extractByHdr.extractByHdrType.fromField.field.udp =,
                        node.extract.fieldType );
                    break;
                case HEADER_TYPE_TCP:
                    EMIT5_2STR( ccnode[, index, ].extractCcParams, .extractByHdr.extractByHdrType.fromField.field.tcp =,
                        node.extract.fieldType );
                    break;
                case HEADER_TYPE_SCTP:
                    EMIT5_2STR( ccnode[, index, ].extractCcParams, .extractByHdr.extractByHdrType.fromField.field.sctp =,
                        node.extract.fieldType );
                    break;
                case HEADER_TYPE_DCCP:
                    EMIT5_2STR( ccnode[, index, ].extractCcParams, .extractByHdr.extractByHdrType.fromField.field.dccp =,
                        node.extract.fieldType );
                    break;
                case HEADER_TYPE_GRE:
                    EMIT5_2STR( ccnode[, index, ].extractCcParams, .extractByHdr.extractByHdrType.fromField.field.gre =,
                        node.extract.fieldType );
                    break;
                case HEADER_TYPE_MINENCAP:
                    EMIT5_2STR( ccnode[, index, ].extractCcParams, .extractByHdr.extractByHdrType.fromField.field.minencap =,
                        node.extract.fieldType );
                    break;
                case HEADER_TYPE_IPSEC_AH:
                    EMIT5_2STR( ccnode[, index, ].extractCcParams, .extractByHdr.extractByHdrType.fromField.field.ipsecAh =,
                        node.extract.fieldType );
                    break;
                case HEADER_TYPE_IPSEC_ESP:
                    EMIT5_2STR( ccnode[, index, ].extractCcParams, .extractByHdr.extractByHdrType.fromField.field.ipsecEsp =,
                        node.extract.fieldType );
                    break;
                default:
                    break;

            }
            EMIT5_2( ccnode[, index, ].extractCcParams, .extractByHdr.extractByHdrType.fromField.size =,
                node.extract.size );
            EMIT5_2( ccnode[, index, ].extractCcParams, .extractByHdr.extractByHdrType.fromField.offset =,
                node.extract.offset );
        }
    }
    else
    {
        EMIT5_2STR( ccnode[, index, ].extractCcParams, .extractNonHdr.src =,
            node.extract.nhSource );
        if (node.extract.nhAction != e_FM_PCD_ACTION_NONE)
        {
            EMIT5_2STR( ccnode[, index, ].extractCcParams, .extractNonHdr.action =,
                node.extract.nhAction );
        }

        EMIT5_2( ccnode[, index, ].extractCcParams, .extractNonHdr.offset =,
            node.extract.nhOffset );
        EMIT5_2( ccnode[, index, ].extractCcParams, .extractNonHdr.size =,
            node.extract.nhSize );

        if (node.extract.nhAction == e_FM_PCD_ACTION_INDEXED_LOOKUP)
        {
            EMIT5_2( ccnode[, index, ].extractCcParams, .extractNonHdr.icIndxMask =,
                node.extract.nhIcIndxMask );
        }
    }

    indent -= 4;
    EMIT1( "}," );

    if (node.maxNumOfKeys > 0)
    {
        EMIT4( ccnode[, index, ].keysParams.maxNumOfKeys =, node.maxNumOfKeys );
        EMIT4( ccnode[, index, ].keysParams.maskSupport =, node.maskSupport );
    }

	EMIT4( ccnode[, index, ].keysParams.statisticsMode =, node.statistics );

    if ( ( cmodel->ccnode[index].extractCcParams.type != e_FM_PCD_EXTRACT_BY_HDR ) &&
         ( node.extract.nhAction == e_FM_PCD_ACTION_INDEXED_LOOKUP ) ) {
        // Take the upper bound of amount of keys. The number of entries
        // should be a power of 2
        unsigned int upper_bound = node.extract.nhIcIndxMask >> 4;

        // Make upper bound to be a power of 2
        uint32_t x = upper_bound;
        {
            x |= (x >> 1); x |= (x >> 2); x |= (x >> 4);
            x |= (x >> 8); x |= (x >> 16);
            x -= (x >> 1);
        }
        upper_bound = x << 1;

        EMIT4( ccnode[, index, ].keysParams.numOfKeys =, upper_bound );
    }
    else {
        EMIT4( ccnode[, index, ].keysParams.numOfKeys =, node.keys.size() );
    }
    EMIT4( ccnode[, index, ].keysParams.keySize =, node.keySize );

    if ( node.extract.nhAction != e_FM_PCD_ACTION_INDEXED_LOOKUP ) {
        EMIT4STR( ccnode[, index, ].keysParams.ccNextEngineParamsForMiss.nextEngine =, node.nextEngineOnMiss.nextEngine );
		EMIT4( ccnode[, index, ].keysParams.ccNextEngineParamsForMiss.statisticsEn =, node.nextEngineOnMiss.statistics );
        if ( node.nextEngineOnMiss.nextEngine == e_FM_PCD_PLCR ) {
            EMIT4( ccnode[, index, ].keysParams.ccNextEngineParamsForMiss.params.plcrParams.overrideParams =, 1 );
            EMIT4( ccnode[, index, ].keysParams.ccNextEngineParamsForMiss.params.plcrParams.sharedProfile =, 1 );
            EMIT4( ccnode[, index, ].keysParams.ccNextEngineParamsForMiss.params.plcrParams.newRelativeProfileId =, node.nextEngineOnMiss.actionHandleIndex );
            EMIT4( ccnode[, index, ].keysParams.ccNextEngineParamsForMiss.params.plcrParams.newFqid =, node.nextEngineOnMiss.newFqid );
#if (DPAA_VERSION >= 11)
			EMIT4( ccnode[, index, ].keysParams.ccNextEngineParamsForMiss.params.plcrParams.newRelativeStorageProfileId =, node.nextEngineOnMiss.newRelativeStorageProfileId );
#endif /* (DPAA_VERSION >= 11) */
        }
        else if ( node.nextEngineOnMiss.nextEngine == e_FM_PCD_CC ) {
            EMIT4( ccmiss_action_index[, index, ] =, node.nextEngineOnMiss.actionHandleIndex );
        }
        else if ( node.nextEngineOnMiss.nextEngine == e_FM_PCD_KG ) {
            EMIT4( ccnode[, index, ].keysParams.ccNextEngineParamsForMiss.params.kgParams.overrideFqid =, 1 );
            EMIT4( ccmiss_action_index[, index, ] =, node.nextEngineOnMiss.actionHandleIndex );
#if (DPAA_VERSION >= 11)
			EMIT4( ccnode[, index, ].keysParams.ccNextEngineParamsForMiss.params.kgParams.newRelativeStorageProfileId =, node.nextEngineOnMiss.newRelativeStorageProfileId );
#endif /* (DPAA_VERSION >= 11) */
        }
        else if ( node.nextEngineOnMiss.nextEngine == e_FM_PCD_DONE ) {
            if ( node.nextEngineOnMiss.doneAction == e_FM_PCD_DROP_FRAME ) {
                EMIT4STR( ccnode[, index, ].keysParams.ccNextEngineParamsForMiss.params.enqueueParams.action =, node.nextEngineOnMiss.doneAction );
            }
            else if ( node.nextEngineOnMiss.newFqid != 0 ) {
                EMIT4( ccnode[, index, ].keysParams.ccNextEngineParamsForMiss.params.enqueueParams.overrideFqid =, 1 );
                EMIT4( ccnode[, index, ].keysParams.ccNextEngineParamsForMiss.params.enqueueParams.newFqid =,
                        node.nextEngineOnMiss.newFqid );
#if (DPAA_VERSION >= 11)
				EMIT4( ccnode[, index, ].keysParams.ccNextEngineParamsForMiss.params.enqueueParams.newRelativeStorageProfileId =,
					 node.nextEngineOnMiss.newRelativeStorageProfileId );
#endif /* (DPAA_VERSION >= 11) */
            }
            else {
                EMIT4( ccnode[, index, ].keysParams.ccNextEngineParamsForMiss.params.enqueueParams.overrideFqid =, 0 );
            }
        }
    }

    oss << ind( indent ) << ".cckeydata[" << index << "] = {" << std::endl;
    for ( unsigned int i = 0; i < node.keys.size(); ++i ) {
        oss << ind( indent + 4 ) << "{";
        for ( unsigned int j = 0; j < node.keySize; ++j ) {
            if ( j != 0 ) {
                oss << ",";
            }
            oss << " 0x" << std::hex << std::setw( 2 ) << std::setfill( '0' ) << (unsigned int)node.keys[i].data[j];
            oss << std::dec << std::resetiosflags(std::ios::basefield | std::ios::internal);
            cmodel->cckeydata[index][i][j] = node.keys[i].data[j];
        }
        oss << " }," << std::endl;
    }
    EMIT1( "}," );

    oss << ind( indent ) << ".ccmask[" << index << "] = {" << std::endl;
    for ( unsigned int i = 0; i < node.masks.size(); ++i ) {
        oss << ind( indent + 4 ) << "{";
        for ( unsigned int j = 0; j < node.keySize; ++j ) {
            if ( j != 0 ) {
                oss << ",";
            }
            oss << " 0x" << std::hex << std::setw( 2 ) << std::setfill( '0' ) << (unsigned int)node.masks[i].data[j];
            oss << std::dec << std::resetiosflags(std::ios::basefield | std::ios::internal);
            cmodel->ccmask[index][i][j] = node.masks[i].data[j];
        }
        oss << " }," << std::endl;
    }
    EMIT1( "}," );

    for ( unsigned int i = 0; i < node.keys.size(); ++i ) {
        int node_num = node.indices[i];

        if ( node.frag[i] != 0 ) {
            EMIT6( ccentry_frag[, index, ][, node_num, ] =, node.frag[i] );
        }

        if ( node.header[i] != 0 ) {
            EMIT6( ccentry_manip[, index, ][, node_num, ] =, node.header[i] );
        }

        EMIT6STR( ccnode[, index, ].keysParams.keyParams[, node_num, ].ccNextEngineParams.nextEngine =, node.nextEngines[i].nextEngine );
		EMIT6( ccnode[, index, ].keysParams.keyParams[, node_num, ].ccNextEngineParams.statisticsEn =, node.nextEngines[i].statistics );
        if ( node.nextEngines[i].nextEngine == e_FM_PCD_PLCR ) {
            EMIT5( ccnode[, index, ].keysParams.keyParams[, node_num, ].ccNextEngineParams.params.plcrParams.overrideParams = 1 );
            EMIT5( ccnode[, index, ].keysParams.keyParams[, node_num, ].ccNextEngineParams.params.plcrParams.sharedProfile = 1 );
            EMIT6( ccnode[, index, ].keysParams.keyParams[, node_num, ].ccNextEngineParams.params.plcrParams.newRelativeProfileId =, node.nextEngines[i].actionHandleIndex );
            EMIT6( ccnode[, index, ].keysParams.keyParams[, node_num, ].ccNextEngineParams.params.plcrParams.newFqid =, node.nextEngines[i].newFqid );
#if (DPAA_VERSION >= 11)
			EMIT6( ccnode[, index, ].keysParams.keyParams[, node_num, ].ccNextEngineParams.params.plcrParams.newRelativeStorageProfileId =, node.nextEngines[i].newRelativeStorageProfileId );
#endif /* (DPAA_VERSION >= 11) */
        }
        else if ( node.nextEngines[i].nextEngine == e_FM_PCD_KG ) {
            if ( node.nextEngines[i].newFqid != 0 ) {
                EMIT5( ccnode[, index, ].keysParams.keyParams[, node_num, ].ccNextEngineParams.params.kgParams.overrideFqid = 1 );
#if (DPAA_VERSION >= 11)
				EMIT6( ccnode[, index, ].keysParams.keyParams[, node_num, ].ccNextEngineParams.params.kgParams.newRelativeStorageProfileId =, node.nextEngines[i].newRelativeStorageProfileId );
#endif /* (DPAA_VERSION >= 11) */
            }
            else {
                EMIT5( ccnode[, index, ].keysParams.keyParams[, node_num, ].ccNextEngineParams.params.kgParams.overrideFqid = 0 );
            }
            EMIT6( ccentry_action_index[, index, ][, node_num, ] =, node.nextEngines[i].actionHandleIndex );
        }
        else if ( node.nextEngines[i].nextEngine == e_FM_PCD_CC ) {
            EMIT6( ccentry_action_index[, index, ][, node_num, ] =, node.nextEngines[i].actionHandleIndex );
        }
#if (DPAA_VERSION >= 11)
		else if ( node.nextEngines[i].nextEngine == e_FM_PCD_FR ) {
            EMIT6( ccentry_action_index[, index, ][, node_num, ] =, node.nextEngines[i].actionHandleIndex );
        }
#endif /* (DPAA_VERSION >= 11) */
        else if ( node.nextEngines[i].nextEngine == e_FM_PCD_DONE ) {
            if ( node.nextEngines[i].doneAction == e_FM_PCD_DROP_FRAME ) {
                EMIT6STR( ccnode[, index, ].keysParams.keyParams[, node_num, ].ccNextEngineParams.params.enqueueParams.action =, node.nextEngineOnMiss.doneAction );
            }
            else if ( node.nextEngines[i].newFqid != 0 ) {
                EMIT6( ccnode[, index, ].keysParams.keyParams[, node_num, ].ccNextEngineParams.params.enqueueParams.overrideFqid =, 1 );
                EMIT6( ccnode[, index, ].keysParams.keyParams[, node_num, ].ccNextEngineParams.params.enqueueParams.newFqid =, node.nextEngines[i].newFqid );
#if (DPAA_VERSION >= 11)
				EMIT6( ccnode[, index, ].keysParams.keyParams[, node_num, ].ccNextEngineParams.params.enqueueParams.newRelativeStorageProfileId =, node.nextEngines[i].newRelativeStorageProfileId );
#endif /* (DPAA_VERSION >= 11) */
            }
            else {
                EMIT6( ccnode[, index, ].keysParams.keyParams[, node_num, ].ccNextEngineParams.params.enqueueParams.overrideFqid =, 0 );
            }
        }
    }
}


void
CFMCCModelOutput::output_fmc_policer( const CFMCModel& model, fmc_model_t* cmodel,
                                      unsigned int index,
                                      std::ostream& oss, size_t indent )
{
    const Policer& plc = model.all_policers[index];

    EMIT1( std::string( "/* Policer: " ) + plc.name + " */" );

    strncpy( cmodel->policer_name[index], plc.name.c_str(), FMC_NAME_LEN - 1 );
    cmodel->policer_name[index][FMC_NAME_LEN - 1] = 0;
    oss << ind( indent ) << ".policer_name[" << index << "] = " << std::string( "\"" + plc.name + "\"" ) << "," << std::endl;

    EMIT3( policer[, index, ].modify = 0 );
    EMIT3( policer[, index, ].id.newParams.profileType = e_FM_PCD_PLCR_SHARED );
    EMIT4( policer[, index, ].id.newParams.relativeProfileId =, plc.getIndex() );
    EMIT4STR( policer[, index, ].algSelection = , plc.algorithm );
    EMIT4STR( policer[, index, ].colorMode = , plc.colorMode );

    EMIT4( policer[, index, ].nonPassthroughAlgParams.comittedInfoRate =, plc.comittedInfoRate );
    EMIT4( policer[, index, ].nonPassthroughAlgParams.comittedBurstSize =, plc.comittedBurstSize );
    EMIT4( policer[, index, ].nonPassthroughAlgParams.peakOrAccessiveInfoRate =, plc.peakOrAccessiveInfoRate );
    EMIT4( policer[, index, ].nonPassthroughAlgParams.peakOrAccessiveBurstSize =, plc.peakOrAccessiveBurstSize );
    EMIT4STR( policer[, index, ].nonPassthroughAlgParams.rateMode =, plc.rateMode );
    EMIT3( policer[, index, ].nonPassthroughAlgParams.byteModeParams.frameLengthSelection = e_FM_PCD_PLCR_L2_FRM_LEN );
    EMIT3( policer[, index, ].nonPassthroughAlgParams.byteModeParams.rollBackFrameSelection = e_FM_PCD_PLCR_ROLLBACK_FULL_FRM_LEN );

    EMIT4STR( policer[, index, ].color.dfltColor =, plc.dfltColor );

    EMIT4STR( policer[, index, ].nextEngineOnGreen =, plc.nextEngineOnGreen );
    EMIT4STR( policer[, index, ].paramsOnGreen.action =, plc.onGreenAction );

    EMIT4STR( policer[, index, ].nextEngineOnYellow =, plc.nextEngineOnYellow );
    EMIT4STR( policer[, index, ].paramsOnYellow.action =, plc.onYellowAction );

    EMIT4STR( policer[, index, ].nextEngineOnRed =, plc.nextEngineOnRed );
    EMIT4STR( policer[, index, ].paramsOnRed.action =, plc.onRedAction );

    EMIT4( policer_action_index[, index, ][0] =, plc.onGreenActionHandleIndex );
    EMIT4( policer_action_index[, index, ][1] =, plc.onYellowActionHandleIndex );
    EMIT4( policer_action_index[, index, ][2] =, plc.onRedActionHandleIndex );
}

#if (DPAA_VERSION >= 11)
void
CFMCCModelOutput::output_fmc_replicator( const CFMCModel& model, fmc_model_t* cmodel,
                                      unsigned int index,
                                      std::ostream& oss, size_t indent )
{
    const CRepl& repl = model.all_replicators[index];

    EMIT1( std::string( "/* Replicator: " ) + repl.name + " */" );

    strncpy( cmodel->replicator_name[index], repl.name.c_str(), FMC_NAME_LEN - 1 );
    cmodel->replicator_name[index][FMC_NAME_LEN - 1] = 0;
    oss << ind( indent ) << ".replicator_name[" << index << "] = " << std::string( "\"" + repl.name + "\"" ) << "," << std::endl;

	EMIT4( replicator[, index, ].maxNumOfEntries =, repl.maxNumOfEntries );
	EMIT4( replicator[, index, ].numOfEntries =, repl.nextEngines.size() );

	for ( unsigned int i = 0; i < repl.nextEngines.size(); ++i ) {
        int entry_num = repl.indices[i];

        if ( repl.frag[i] != 0 ) {
            EMIT6( repentry_frag[, index, ][, entry_num, ] =, repl.frag[i] );
        }

        if ( repl.header[i] != 0 ) {
            EMIT6( repentry_manip[, index, ][, entry_num, ] =, repl.header[i] );
        }

        EMIT6STR( replicator[, index, ].nextEngineParams[, entry_num, ].nextEngine =, repl.nextEngines[i].nextEngine );
        if ( repl.nextEngines[i].nextEngine == e_FM_PCD_PLCR ) {
            EMIT5( replicator[, index, ].nextEngineParams[, entry_num, ].params.plcrParams.overrideParams = 1 );
            EMIT5( replicator[, index, ].nextEngineParams[, entry_num, ].params.plcrParams.sharedProfile = 1 );
            EMIT6( replicator[, index, ].nextEngineParams[, entry_num, ].params.plcrParams.newRelativeProfileId =, repl.nextEngines[i].actionHandleIndex );
            EMIT6( replicator[, index, ].nextEngineParams[, entry_num, ].params.plcrParams.newFqid =, repl.nextEngines[i].newFqid );
			EMIT6( replicator[, index, ].nextEngineParams[, entry_num, ].params.plcrParams.newRelativeStorageProfileId =, repl.nextEngines[i].newRelativeStorageProfileId );
        }
        else if ( repl.nextEngines[i].nextEngine == e_FM_PCD_KG ) {
            if ( repl.nextEngines[i].newFqid != 0 ) {
                EMIT5( replicator[, index, ].nextEngineParams[, entry_num, ].params.kgParams.overrideFqid = 1 );
				EMIT6( replicator[, index, ].nextEngineParams[, entry_num, ].params.kgParams.newFqid =, repl.nextEngines[i].newFqid );
				EMIT6( replicator[, index, ].nextEngineParams[, entry_num, ].params.kgParams.newRelativeStorageProfileId =, repl.nextEngines[i].newRelativeStorageProfileId );
            }
            else {
                EMIT5( replicator[, index, ].nextEngineParams[, entry_num, ].params.kgParams.overrideFqid = 0 );
            }
            EMIT6( repentry_action_index[, index, ][, entry_num, ] =, repl.nextEngines[i].actionHandleIndex );
        }
        else if ( repl.nextEngines[i].nextEngine == e_FM_PCD_CC ) {
            EMIT6( repentry_action_index[, index, ][, entry_num, ] =, repl.nextEngines[i].actionHandleIndex );
        }
		else if ( repl.nextEngines[i].nextEngine == e_FM_PCD_FR ) {
            EMIT6( repentry_action_index[, index, ][, entry_num, ] =, repl.nextEngines[i].actionHandleIndex );
        }
        else if ( repl.nextEngines[i].nextEngine == e_FM_PCD_DONE ) {
            if ( repl.nextEngines[i].newFqid != 0 ) {
                EMIT6( replicator[, index, ].nextEngineParams[, entry_num, ].params.enqueueParams.overrideFqid =, 1 );
                EMIT6( replicator[, index, ].nextEngineParams[, entry_num, ].params.enqueueParams.newFqid =, repl.nextEngines[i].newFqid );
				EMIT6( replicator[, index, ].nextEngineParams[, entry_num, ].params.enqueueParams.newRelativeStorageProfileId =, repl.nextEngines[i].newRelativeStorageProfileId );
            }

            else {
                EMIT6( replicator[, index, ].nextEngineParams[, entry_num, ].params.enqueueParams.overrideFqid =, 0 );
            }
        }
    }
}
#endif /* (DPAA_VERSION >= 11) */


std::string
CFMCCModelOutput::get_apply_item_name( fmc_model_t* cmodel,
                                       ApplyOrder::Entry e )
{
    switch ( get_fmc_type( e.type ) ) {
    case FMCEngineStart:
    case FMCEngineEnd:
        return std::string( " /* " ) +
            cmodel->fman[e.index].name + " */";
    case FMCPortStart:
    case FMCPortEnd:
    case FMCCCTree:
        return std::string( " /* " ) +
            cmodel->port[e.index].name + " */";
    case FMCScheme:
        return std::string( " /* " ) +
            cmodel->scheme_name[e.index] + " */";
    case FMCCCNode:
        return std::string( " /* " ) +
            cmodel->ccnode_name[e.index] + " */";
    case FMCPolicer:
        return std::string( " /* " ) +
            cmodel->policer_name[e.index] + " */";
#if (DPAA_VERSION >= 11)
	case FMCReplicator:
        return std::string( " /* " ) +
            cmodel->replicator_name[e.index] + " */";
#endif /* (DPAA_VERSION >= 11) */
    default:
        return "";
    }
    return "";
}


void
CFMCCModelOutput::output_fmc_applier( const CFMCModel& model, fmc_model_t* cmodel,
                                      unsigned int index,
                                      std::ostream& oss, size_t indent )
{
    ApplyOrder::Entry e = model.applier.get( index );

    cmodel->ao[cmodel->ao_count - index - 1].type  = get_fmc_type( e.type );
    cmodel->ao[cmodel->ao_count - index - 1].index = e.index;

    oss << ind( indent )
        << "FMC_APPLY_ORDER("
        << std::setfill(' ') << std::setw( 3 )
        << cmodel->ao_count - index - 1
        << ", "
        << get_fmc_type_str( e.type )
        << ","
        << std::setfill(' ') << std::setw( 3 )
        << e.index
        << " ),"
        << get_apply_item_name( cmodel, e )
        << std::endl;
}


fmc_apply_order_e
CFMCCModelOutput::get_fmc_type( ApplyOrder::Type t ) const
{
    switch ( t ) {
        case ApplyOrder::EngineStart:
            return FMCEngineStart;
        case ApplyOrder::EngineEnd:
            return FMCEngineEnd;
        case ApplyOrder::PortStart:
            return FMCPortStart;
        case ApplyOrder::PortEnd:
            return FMCPortEnd;
        case ApplyOrder::Scheme:
            return FMCScheme;
        case ApplyOrder::CCNode:
            return FMCCCNode;
        case ApplyOrder::CCTree:
            return FMCCCTree;
		case ApplyOrder::Replicator:
            return FMCReplicator;
        case ApplyOrder::Policer:
            return FMCPolicer;
    }
    return FMCEngineStart;
}


std::string
CFMCCModelOutput::get_fmc_type_str( ApplyOrder::Type t ) const
{
    switch ( t ) {
        case ApplyOrder::EngineStart:
            return "FMCEngineStart";
        case ApplyOrder::EngineEnd:
            return "FMCEngineEnd  ";
        case ApplyOrder::PortStart:
            return "FMCPortStart  ";
        case ApplyOrder::PortEnd:
            return "FMCPortEnd    ";
        case ApplyOrder::Scheme:
            return "FMCScheme     ";
        case ApplyOrder::CCNode:
            return "FMCCCNode     ";
        case ApplyOrder::CCTree:
            return "FMCCCTree     ";
        case ApplyOrder::Policer:
            return "FMCPolicer    ";
		case ApplyOrder::Replicator:
            return "FMCReplicator ";
    }
    return "";
}
