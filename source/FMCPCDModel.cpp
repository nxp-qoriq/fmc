/* =====================================================================
 *
 *  Copyright 2009-2012, Freescale Semiconductor, Inc., All Rights Reserved.
 *
 *  This file contains copyrighted material. Use of this file is restricted
 *  by the provisions of a Freescale Software License Agreement, which has
 *  either accompanied the delivery of this software in shrink wrap
 *  form or been expressly executed between the parties.
 *
 *  File Name : FMCPCDModel.cpp
 *  Author    : Serge Lamikhov-Center
 *
 * ===================================================================*/

#include <assert.h>
#include <string>
#include <string.h>
#include <cstdlib>
#include <iostream>
#include <algorithm>
#include "FMCTaskDef.h"
#include "FMCPCDModel.h"

#define UNKNOWN_NetCommField 0xFFFFFFFF


unsigned int
FMBlock::assignIndex( std::vector< Engine >& engines )
{
    engines.push_back( Engine() );
    unsigned int index = engines.size() - 1;
    engines[index].index = index;

    return index;
}


unsigned int
FMBlock::assignIndex( std::vector< Port >& ports )
{
    ports.push_back( Port() );
    unsigned int index = ports.size() - 1;
    ports[index].index = index;

    return index;
}


unsigned int
FMBlock::assignIndex( std::vector< Scheme >& schemes )
{
    schemes.push_back( Scheme() );
    unsigned int index = schemes.size() - 1;
    schemes[index].index = index;

    return index;
}


unsigned int
FMBlock::assignIndex( std::vector< CCNode >& ccnodes )
{
    ccnodes.push_back( CCNode() );
    unsigned int index = ccnodes.size() - 1;
    ccnodes[index].index = index;

    return index;
}


unsigned int
FMBlock::assignIndex( std::vector< Policer >& policers )
{
    policers.push_back( Policer() );
    unsigned int index = policers.size() - 1;
    policers[index].index = index;

    return index;
}


////////////////////////////////////////////////////////////////////////////////
/// Default constructor
////////////////////////////////////////////////////////////////////////////////
CFMCModel::CFMCModel()
{
    // Do the placement reservation to prevent containers' memory
    // reallocation and garantee validity of the pointers
    all_engines.reserve( MAX_ENGINES );
    all_ports.reserve( MAX_ENGINES*MAX_PORTS );
    all_schemes.reserve( MAX_ENGINES*MAX_SCHEMES );
    all_policers.reserve( MAX_ENGINES*MAX_POLICERS );
    all_ccnodes.reserve( MAX_ENGINES*MAX_CCNODES );
    spEnable = 0;
}


////////////////////////////////////////////////////////////////////////////////
/// Create model by building the internal database
////////////////////////////////////////////////////////////////////////////////
bool
CFMCModel::createModel( CTaskDef* pTaskDef )
{
    assert( pTaskDef );

    // For all engines
    std::map< std::string, CEngine >::iterator engIt;
    for ( engIt = pTaskDef->engines.begin();
          engIt != pTaskDef->engines.end();
          ++engIt ) {

        Engine& engine = createEngine( engIt->second, pTaskDef );

        applier.add( ApplyOrder::Entry( ApplyOrder::EngineEnd, engine.getIndex() ) );

        // For all ports of this engine
        std::vector< CPort >::iterator portIt;
        for ( portIt = engIt->second.ports.begin();
              portIt != engIt->second.ports.end();
              ++portIt ) {

            // Find the policy
            std::map< std::string, CPolicy >::iterator policyIt =
                pTaskDef->policies.find( portIt->policy );
            if ( policyIt == pTaskDef->policies.end() ) {
                throw CGenericError( ERR_POLICY_NOT_FOUND, portIt->policy,
                                     portIt->type, portIt->number );
            }

            Port& port      = createPort( engine, *portIt, pTaskDef );

            applier.add( ApplyOrder::Entry( ApplyOrder::PortEnd, port.getIndex() ) );

            unsigned int applier_pos = applier.size();

            // For all distributions in this policy
            std::vector< std::string >::reverse_iterator distRefIt;
            uint8_t scheme_index = 0;
            for ( distRefIt = policyIt->second.dist_order.rbegin();
                  distRefIt != policyIt->second.dist_order.rend();
                  ++distRefIt, ++scheme_index ) {
                unsigned int index = FindAndAddSchemeByName( pTaskDef,
                    *distRefIt, policyIt->first, port, false );
                Scheme& scheme = all_schemes[index];
                scheme.scheme_index_per_port = scheme_index;

                // Add scheme's protocols to the port's protocol list
                std::set< Protocol >::iterator protoIt;
                for ( protoIt  = scheme.used_protocols.begin();
                      protoIt != scheme.used_protocols.end();
                      ++protoIt ) {
                          // Just add an empty element - the real header will be filled in later
                          std::pair< unsigned int, DistinctionUnitElement > empty_element;
                          empty_element.first         = 0;
                          empty_element.second.hdr    = HEADER_TYPE_NONE;
                          empty_element.second.hdrStr = "HEADER_TYPE_NONE";

                          port.protocols[*protoIt] = empty_element;
                }
            }

            for (unsigned int schIndex = 0; schIndex < all_schemes.size(); schIndex++)
            {
                if (all_schemes[schIndex].scheme_index_per_port == -1)
                {
                    all_schemes[schIndex].scheme_index_per_port = scheme_index++;
                }
            }

            if ( !port.cctrees.empty() || port.reasm.size() ) {
                applier.addDelayed( ApplyOrder::Entry( ApplyOrder::CCTree, port.getIndex() ) );
            }
            applier.insertDelayedAfter( applier_pos );

            // Enumerate and fill used protocols
            std::map< Protocol, std::pair< unsigned int, DistinctionUnitElement > >::iterator protoIt;
            unsigned int i;
            for ( protoIt  = port.protocols.begin(), i = 0;
                  protoIt != port.protocols.end();
                  ++protoIt, ++i ) {
                protoIt->second.first      = i;
                protoIt->second.second.hdr = getNetCommHeaderType( protoIt->first.name );
                protoIt->second.second.opt = protoIt->first.opt;
                if ( protoIt->second.second.hdr == HEADER_TYPE_NONE ) {
                    std::string shim_protocol = pTaskDef->getShimNoFromCustom( protoIt->first.name );
                    if ( !shim_protocol.empty() ) { // Shim header is empty if custom protocol is not found
                        protoIt->second.second.hdr = getNetCommHeaderType( shim_protocol );
                    }
                    else {
                        // Header type HEADER_TYPE_NONE is not accepted as a valid value
                        throw CGenericError( ERR_UNKNOWN_PROTOCOL, protoIt->first.name );
                    }
                }
                protoIt->second.second.hdrStr = getNetCommHeaderTypeStr( protoIt->second.second.hdr );
            }

            applier.add( ApplyOrder::Entry( ApplyOrder::PortStart, port.getIndex() ) );
        }

        applier.add( ApplyOrder::Entry( ApplyOrder::EngineStart, engine.getIndex() ) );
        createSoftParse( pTaskDef );
    }

    return true;
}


////////////////////////////////////////////////////////////////////////////////
/// Converts engine parameters provided throught XML file to the internal
/// structure
////////////////////////////////////////////////////////////////////////////////
Engine&
CFMCModel::createEngine( const CEngine& xmlEngine, const CTaskDef* pTaskDef )
{
    assert( xmlEngine.name.length() > 2 );

    Engine& engine = all_engines[FMBlock::assignIndex( all_engines )];

    engine.name     = xmlEngine.name;
    engine.number   = std::strtoul( xmlEngine.name.c_str() + 2, 0, 0 );
    engine.pcd_name = "pcd:" + engine.name;

    std::map< std::string, CFragmentation >::const_iterator fragit;
    for ( fragit = pTaskDef->fragmentations.begin(); fragit != pTaskDef->fragmentations.end(); ++fragit ) {
        t_FmPcdManipParams frag;
        frag.fragOrReasm                                         = 1;
        frag.fragOrReasmParams.frag                              = 1;
        frag.fragOrReasmParams.hdr                               = HEADER_TYPE_IPv6;
        frag.fragOrReasmParams.ipFragParams.sizeForFragmentation = fragit->second.size;
        frag.fragOrReasmParams.ipFragParams.scratchBpid          = fragit->second.scratchBpid;
        frag.fragOrReasmParams.ipFragParams.dontFragAction       = (e_FmPcdManipDontFragAction)fragit->second.dontFragAction;

        engine.frags.push_back( frag );
    }

    return engine;
}


////////////////////////////////////////////////////////////////////////////////
/// Converts Port parameters provided throught XML file to the internal
/// structure
////////////////////////////////////////////////////////////////////////////////
Port&
CFMCModel::createPort( Engine& engine, const CPort& xmlPort, const CTaskDef* pTaskDef )
{
    Port& port = all_ports[FMBlock::assignIndex( all_ports )];

    engine.ports.push_back( port.getIndex() );

    port.type      = getPortType( xmlPort.type );
    port.typeStr   = getPortTypeStr( port.type );
    port.number    = xmlPort.number;
    port.portid    = xmlPort.portid;
    std::ostringstream oss;
    oss << engine.name << port.typeStr << port.number;
    port.signature = oss.str();

    std::map< std::string, CReassembly >::const_iterator it;
    for ( it = pTaskDef->reassemblies.begin(); it != pTaskDef->reassemblies.end(); ++it ) {
        t_FmPcdManipParams reasm;
        reasm.fragOrReasm = 1;
        reasm.fragOrReasmParams.frag = 0;
        reasm.fragOrReasmParams.hdr             = HEADER_TYPE_IPv6;
        reasm.fragOrReasmParams.extBufPoolIndx  = it->second.sgBpid;
//        reasm.fragOrReasmParams.ipReasmParams.?????????               = pTaskDef->reassemblies[i].sgLiodnOffset;
        reasm.fragOrReasmParams.ipReasmParams.maxNumFramesInProcess             = it->second.maxInProcess;
        reasm.fragOrReasmParams.ipReasmParams.liodnOffset                       = it->second.dataLiodnOffset;
        reasm.fragOrReasmParams.ipReasmParams.dataMemId                         = it->second.dataMemId;
        reasm.fragOrReasmParams.ipReasmParams.minFragSize[0]                    = it->second.ipv4minFragSize;
        reasm.fragOrReasmParams.ipReasmParams.minFragSize[1]                    = it->second.ipv6minFragSize;
        reasm.fragOrReasmParams.ipReasmParams.timeOutMode                       = (e_FmPcdManipReassemTimeOutMode)it->second.timeOutMode;
        reasm.fragOrReasmParams.ipReasmParams.fqidForTimeOutFrames              = it->second.fqidForTimeOutFrames;
        reasm.fragOrReasmParams.ipReasmParams.numOfFramesPerHashEntry           = (e_FmPcdManipReassemWaysNumber)it->second.numOfFramesPerHashEntry;
        reasm.fragOrReasmParams.ipReasmParams.timeoutThresholdForReassmProcess  = it->second.timeoutThreshold;

        port.reasm.push_back( reasm );
    }

    return port;
}


////////////////////////////////////////////////////////////////////////////////
/// Converts Distribution parameters provided throught XML file to the internal
/// scheme structure
////////////////////////////////////////////////////////////////////////////////
Scheme&
CFMCModel::createScheme( const CTaskDef* pTaskDef, Port& port, const CDistribution& xmlDist,
                         bool isDirect )
{
    Scheme& scheme = all_schemes[FMBlock::assignIndex( all_schemes )];

    if ( isDirect ) {
        applier.add( ApplyOrder::Entry( ApplyOrder::Scheme, scheme.getIndex() ) );
    }
    else {
        applier.addDelayed( ApplyOrder::Entry( ApplyOrder::Scheme, scheme.getIndex() ) );
    }

    port.schemes.push_back( scheme.getIndex() );

    scheme.name                  = xmlDist.name;
    scheme.qbase                 = xmlDist.qbase;
    scheme.qcount                = xmlDist.qcount;
    scheme.relativeSchemeId      = port.schemes.size();
    scheme.hashShift             = xmlDist.keyShift;
    scheme.symmetricHash         = xmlDist.symmetricHash;
    scheme.isDirect              = isDirect ? 1 : 0;
    scheme.scheme_index_per_port = -1;
    scheme.port_signature        = port.signature;

    // For each 'key' entry
    for ( unsigned int i = 0; i < xmlDist.key.size(); ++i ) {
        ExtractData field;
        if ( xmlDist.key[i].name != "" ) {
            field.type       = e_FM_PCD_EXTRACT_BY_HDR;
            field.typeStr    = "e_FM_PCD_EXTRACT_BY_HDR";

            Protocol tProt;
            tProt.name =
                xmlDist.key[i].name.substr( 0, xmlDist.key[i].name.find_last_of( "." ) );
            tProt.opt = "";
            if ( scheme.isDirect == 0 ) {   // Only for inderect schemes
                scheme.used_protocols.insert( tProt );
            }

            field.fieldName  = xmlDist.key[i].name;
            field.hdrtype    = e_FM_PCD_EXTRACT_FULL_FIELD;
            field.hdrtypeStr = "e_FM_PCD_EXTRACT_FULL_FIELD";
            field.hdr        = getNetCommHeaderType( tProt.name );
            if ( field.hdr == HEADER_TYPE_NONE ) {
                std::string shim_protocol = pTaskDef->getShimNoFromCustom( tProt.name );
                if ( !shim_protocol.empty() ) { // Shim header is empty if a custom protocol is not found
                    field.hdrtype    = e_FM_PCD_EXTRACT_FROM_HDR;
                    field.hdrtypeStr = "e_FM_PCD_EXTRACT_FROM_HDR";
                    field.hdr = getNetCommHeaderType( shim_protocol );
                }
                else {
                    // Header type HEADER_TYPE_NONE is not accepted as a valid value
                    throw CGenericError( ERR_UNKNOWN_PROTOCOL, xmlDist.key[i].name );
                }
            }
            else {
                // Process unknown field of known header
                if ( getNetCommFieldType( field.fieldName ) == UNKNOWN_NetCommField ) {
                    field.hdrtype    = e_FM_PCD_EXTRACT_FROM_HDR;
                    field.hdrtypeStr = "e_FM_PCD_EXTRACT_FROM_HDR";
                }
            }
            field.hdrStr      = getNetCommHeaderTypeStr( field.hdr );
            field.hdrIndex    = getNetCommHeaderIndex( xmlDist.key[i].header_index );
            field.hdrIndexStr = getNetCommHeaderIndexStr( xmlDist.key[i].header_index );

            // Find the protocol and get the field's length
            unsigned int bitsize;
            unsigned int bitoffset;
            if ( pTaskDef->GetFieldProperties( field.fieldName,
                                               bitsize, bitoffset ) ) {
                field.size   = bitsize / 8;
                field.offset = bitoffset / 8;
            }

            field.fieldType    = getNetCommFieldType( field.fieldName );
            field.fieldTypeStr = getNetCommFieldTypeStr( field.fieldName );
        }
        else {
            field.type    = e_FM_PCD_EXTRACT_NON_HDR;
            field.typeStr = "e_FM_PCD_EXTRACT_NON_HDR";
            field.src     = e_FM_PCD_EXTRACT_FROM_FRAME_START;
            field.srcStr  = "e_FM_PCD_EXTRACT_FROM_FRAME_START";
        }

        scheme.key.push_back( field );
    }

    // For each 'combine' entry
    for ( unsigned int i = 0; i < xmlDist.combines.size(); ++i ) {
        ExtractData combine;

        combine.offsetInFqid = xmlDist.combines[i].offsetInFqid;
        combine.offset   = xmlDist.combines[i].offset;
        combine.size     = 1; // xmlDist.combines[i].size; All 'combine' elements
                              // have a fixed size due to the HW ability
        combine.mask     = xmlDist.combines[i].mask;
        combine.default_ = xmlDist.combines[i].default_;
        combine.validate = xmlDist.combines[i].validate;

        if ( xmlDist.combines[i].kind == CCombineEntry::PORTDATA ) {
            combine.type    = e_FM_PCD_KG_EXTRACT_PORT_PRIVATE_INFO;
            combine.typeStr = "e_FM_PCD_KG_EXTRACT_PORT_PRIVATE_INFO";
            // The next entries are initialized, but should not have effect when
            // the type is e_FM_PCD_KG_EXTRACT_PORT_PRIVATE_INFO
            combine.src     = e_FM_PCD_EXTRACT_FROM_FRAME_START;
            combine.srcStr  = "e_FM_PCD_EXTRACT_FROM_FRAME_START";
        }
        else if ( xmlDist.combines[i].kind == CCombineEntry::PARSER ) {
            combine.type    = e_FM_PCD_EXTRACT_NON_HDR;
            combine.typeStr = "e_FM_PCD_EXTRACT_NON_HDR";
            combine.src     = e_FM_PCD_EXTRACT_FROM_PARSE_RESULT;
            combine.srcStr  = "e_FM_PCD_EXTRACT_FROM_PARSE_RESULT";
        }
        else if ( xmlDist.combines[i].kind == CCombineEntry::FIELD && xmlDist.combines[i].fieldref != "" ) {
            combine.type       = e_FM_PCD_EXTRACT_BY_HDR;
            combine.typeStr    = "e_FM_PCD_EXTRACT_BY_HDR";
            combine.hdrtype    = e_FM_PCD_EXTRACT_FULL_FIELD;
            combine.hdrtypeStr = "e_FM_PCD_EXTRACT_FULL_FIELD";

            std::string protocol_name =
                xmlDist.combines[i].fieldref.substr( 0, xmlDist.combines[i].fieldref.find_last_of( "." ) );
            combine.hdr         = getNetCommHeaderType( protocol_name );
            if ( combine.hdr == HEADER_TYPE_NONE ) {
                std::string shim_protocol = pTaskDef->getShimNoFromCustom( protocol_name );
                if ( !shim_protocol.empty() ) { // Shim header is empty if custom protocol is not found
                    combine.hdr = getNetCommHeaderType( shim_protocol );
                }
                else {
                    // Header type HEADER_TYPE_NONE is not accepted as a valid value
                    throw CGenericError( ERR_UNKNOWN_PROTOCOL, xmlDist.combines[i].fieldref );
                }
            }
            combine.hdrStr      = getNetCommHeaderTypeStr( combine.hdr );
            // e_FM_PCD_HDR_INDEX_NONE is the only supported option right now
            combine.hdrIndex    = e_FM_PCD_HDR_INDEX_NONE;
            combine.hdrIndexStr = "e_FM_PCD_HDR_INDEX_NONE";

            combine.fieldName    = xmlDist.combines[i].fieldref;
            combine.fieldType    = getNetCommFieldType( combine.fieldName );
            combine.fieldTypeStr = getNetCommFieldTypeStr( combine.fieldName );

            uint32_t bitSize   = 0;
            uint32_t bitOffset = 0;
            if ( pTaskDef->GetFieldProperties( combine.fieldName, bitSize, bitOffset ) ) {
                combine.offset += bitOffset;
            }
        }
        else {  // "Extract from frame" is assumed. isFrame should be true in this case
            combine.type    = e_FM_PCD_EXTRACT_NON_HDR;
            combine.typeStr = "e_FM_PCD_EXTRACT_NON_HDR";
            combine.src     = e_FM_PCD_EXTRACT_FROM_FRAME_START;
            combine.srcStr  = "e_FM_PCD_EXTRACT_FROM_FRAME_START";
        }
        // Offset for the 'combine' elements is measured in bytes in the current HW
        if ( combine.offset % 8 != 0 ) {
            CGenericError::printWarning( WARN_EXTRACTION_BYTE_ALLIGN, scheme.name );
        }
        combine.offset /= 8;

        scheme.combines.push_back( combine );
    }

    // For each 'protocol' entry
    if ( scheme.isDirect == 0 ) {   // Only for inderect schemes
        for ( unsigned int i = 0; i < xmlDist.protocols.size(); ++i ) {
            Protocol tProt;
            tProt.name = xmlDist.protocols[i].name;
            tProt.opt = xmlDist.protocols[i].opt;
            scheme.used_protocols.insert( tProt );
        }
    }

    // Add next engine entry
    scheme.nextEngine        = getEngineByType( xmlDist.action );
    scheme.nextEngineStr     = getEngineByTypeStr( xmlDist.action );
    scheme.doneAction        = e_FM_PCD_ENQ_FRAME;
    scheme.doneActionStr     = "e_FM_PCD_ENQ_FRAME";
    scheme.actionHandleIndex = 0xFFFFFFFF;
    if ( scheme.nextEngine == e_FM_PCD_DONE &&
         xmlDist.action == "drop" ) {                   // Is it drop?
        scheme.doneAction    = e_FM_PCD_DROP_FRAME;
        scheme.doneActionStr = "e_FM_PCD_DROP_FRAME";
    }
    else if ( scheme.nextEngine == e_FM_PCD_PLCR ) {    // Is it a policer?
        // Find the policer and add it
        scheme.actionHandleIndex =
            FindAndAddPolicerByName( pTaskDef, xmlDist.actionName, xmlDist.name,
                                     port );
    }
    else if ( scheme.nextEngine == e_FM_PCD_KG ) {      // Is it a distribution?
        // Find the distribution and add it
        scheme.actionHandleIndex =
            FindAndAddSchemeByName( pTaskDef, xmlDist.actionName, xmlDist.name,
                                    port, true );
    }
    else if ( scheme.nextEngine == e_FM_PCD_CC ) {      // Is it CC node?
        // Find CC node
        scheme.actionHandleIndex =
            FindAndAddCCNodeByName( pTaskDef, xmlDist.actionName, xmlDist.name,
                                    port, true );
    }

    return scheme;
}


////////////////////////////////////////////////////////////////////////////////
/// Converts classification parameters provided throught XML file
/// to the internal representation
////////////////////////////////////////////////////////////////////////////////
CCNode&
CFMCModel::createCCNode( const CTaskDef* pTaskDef, Port& port, const CClassification& xmlCCNode )
{
    CCNode& ccNode = all_ccnodes[FMBlock::assignIndex( all_ccnodes )];

    applier.add( ApplyOrder::Entry( ApplyOrder::CCNode, ccNode.getIndex() ) );

    port.ccnodes.push_back( ccNode.getIndex() );

    ccNode.name           = xmlCCNode.name;
    ccNode.port_signature = port.signature;


    if (xmlCCNode.key.header)
    {
        ccNode.extract.type = e_FM_PCD_EXTRACT_BY_HDR;
        ccNode.extract.typeStr = "e_FM_PCD_EXTRACT_BY_HDR";
    }
    else
    {
        ccNode.extract.type = e_FM_PCD_EXTRACT_NON_HDR;
        ccNode.extract.typeStr = "e_FM_PCD_EXTRACT_NON_HDR";
    }

    ccNode.keySize = 0;

    ExtractData field;
    field.fieldType    = 0;
    field.fieldTypeStr = "";
    // For each 'extract key' entry
    if (ccNode.extract.type == e_FM_PCD_EXTRACT_BY_HDR)
    {
        for ( unsigned int i = 0; i < xmlCCNode.key.fields.size(); ++i ) {
            if ( xmlCCNode.key.fields[i].name != "" ) {
                std::string protocol_name =
                    xmlCCNode.key.fields[i].name.substr( 0, xmlCCNode.key.fields[i].name.find_last_of( "." ) );
                ccNode.used_protocols.insert( protocol_name );

                field.hdr = getNetCommHeaderType( protocol_name );
                if ( field.hdr == HEADER_TYPE_NONE ) {
                    std::string shim_protocol = pTaskDef->getShimNoFromCustom( protocol_name );
                    if ( !shim_protocol.empty() ) { // Shim header is empty if custom protocol is not found
                        field.hdr = getNetCommHeaderType( shim_protocol );
                    }
                    else {
                        // Header type HEADER_TYPE_NONE is not accepted as a valid key
                        // for coarce classification
                        throw CGenericError( ERR_UNKNOWN_PROTOCOL, xmlCCNode.key.fields[i].name );
                    }
                }
                field.hdrStr = getNetCommHeaderTypeStr( field.hdr );

                field.hdrIndex    = getNetCommHeaderIndex( xmlCCNode.key.fields[i].header_index );
                field.hdrIndexStr = getNetCommHeaderIndexStr( xmlCCNode.key.fields[i].header_index );

                field.fieldName    = xmlCCNode.key.fields[i].name;
                field.fieldType    |= getNetCommFieldType( field.fieldName );
                if ( !field.fieldTypeStr.empty() ) {
                    field.fieldTypeStr += " | ";
                }
                field.fieldTypeStr += getNetCommFieldTypeStr( field.fieldName );

                // Find the protocol and get the field's length
                unsigned int bitsize;
                unsigned int bitoffset;
                if ( pTaskDef->GetFieldProperties( xmlCCNode.key.fields[i].name,
                                                   bitsize, bitoffset ) ) {
                    ccNode.keySize += bitsize;
                }

                field.type    = e_FM_PCD_EXTRACT_BY_HDR;
                field.typeStr = "e_FM_PCD_EXTRACT_BY_HDR";
                if ( ( getNetCommFieldType( field.fieldName ) != UNKNOWN_NetCommField ) &&
                       isFullFieldForCC( field.fieldName ) ) {
                    if (xmlCCNode.key.field)
                    {
                        field.offset     = xmlCCNode.key.fields[i].offset;
                        field.size       = xmlCCNode.key.fields[i].size;
                        field.hdrtype    = e_FM_PCD_EXTRACT_FROM_FIELD;
                        field.hdrtypeStr = "e_FM_PCD_EXTRACT_FROM_FIELD";
                    } else
                    {
                        field.hdrtype    = e_FM_PCD_EXTRACT_FULL_FIELD;
                        field.hdrtypeStr = "e_FM_PCD_EXTRACT_FULL_FIELD";
                    }
                }
                else {
                    field.hdrtype    = e_FM_PCD_EXTRACT_FROM_HDR;
                    field.hdrtypeStr = "e_FM_PCD_EXTRACT_FROM_HDR";
                    // For classification purposes, only byte aligned fields are supported
                    field.size       = bitsize / 8;
                    field.offset     = bitoffset / 8;
                    // TODO: Apply mask
                }
            }
            else {
                field.type    = e_FM_PCD_EXTRACT_NON_HDR;
                field.typeStr = "e_FM_PCD_EXTRACT_NON_HDR";
            }

            ccNode.extract = field;
        }
    }
    else
    {// Non header extract
        ccNode.extract.nhSource = (e_FmPcdExtractFrom)xmlCCNode.key.nonHeaderEntry.source;
        switch (ccNode.extract.nhSource)
        {
        case e_FM_PCD_EXTRACT_FROM_FRAME_START:
            ccNode.extract.nhSourceStr = "e_FM_PCD_EXTRACT_FROM_FRAME_START";
            break;
        case e_FM_PCD_EXTRACT_FROM_KEY:
            ccNode.extract.nhSourceStr = "e_FM_PCD_EXTRACT_FROM_KEY";
            break;
        case e_FM_PCD_EXTRACT_FROM_HASH:
            ccNode.extract.nhSourceStr = "e_FM_PCD_EXTRACT_FROM_HASH";
            break;
        case e_FM_PCD_EXTRACT_FROM_PARSE_RESULT:
            ccNode.extract.nhSourceStr = "e_FM_PCD_EXTRACT_FROM_PARSE_RESULT";
            break;
        case e_FM_PCD_EXTRACT_FROM_ENQ_FQID:
            ccNode.extract.nhSourceStr = "e_FM_PCD_EXTRACT_FROM_ENQ_FQID";
            break;
        case e_FM_PCD_EXTRACT_FROM_FLOW_ID:
            ccNode.extract.nhSourceStr = "e_FM_PCD_EXTRACT_FROM_FLOW_ID";
            break;
        }

        ccNode.extract.nhAction        = (e_FmPcdAction)xmlCCNode.key.nonHeaderEntry.action;

        switch (ccNode.extract.nhAction)
        {
        case e_FM_PCD_ACTION_NONE:
            ccNode.extract.nhActionStr = "e_FM_PCD_ACTION_NONE";
            break;
        case e_FM_PCD_ACTION_EXACT_MATCH:
            ccNode.extract.nhActionStr = "e_FM_PCD_ACTION_EXACT_MATCH";
            break;
        case e_FM_PCD_ACTION_INDEXED_LOOKUP:
            ccNode.extract.nhActionStr = "e_FM_PCD_ACTION_INDEXED_LOOKUP";
            break;
        }

        ccNode.extract.nhOffset        = xmlCCNode.key.nonHeaderEntry.offset;
        ccNode.extract.nhSize        = xmlCCNode.key.nonHeaderEntry.size;
        ccNode.extract.nhIcIndxMask = xmlCCNode.key.nonHeaderEntry.icIndxMask;

        ccNode.keySize = ccNode.extract.nhSize * 8;
    }

    ccNode.keySize = ( ccNode.keySize + 7 ) / 8;    // Convert to bytes

    // For each entry ...
    for ( unsigned int i = 0; i < xmlCCNode.entries.size(); ++i ) {
        // ... copy the data according to the key size
        ccNode.keys.push_back( CCNode::CCData() );
        ccNode.masks.push_back( CCNode::CCData() );
        ccNode.indices.push_back( xmlCCNode.entries[i].index );
        ccNode.frag.push_back( !xmlCCNode.entries[i].fragmentationName.empty() );

        for ( unsigned int j = 0; j < ccNode.keySize; ++j ) {
            ccNode.keys[i].data[j] =
                xmlCCNode.entries[i].data[FM_PCD_MAX_SIZE_OF_KEY - ccNode.keySize + j];
            ccNode.masks[i].data[j] =
                xmlCCNode.entries[i].mask[FM_PCD_MAX_SIZE_OF_KEY - ccNode.keySize + j];
        }

        ccNode.nextEngines.push_back( CCNode::CCNextEngine() );
        ccNode.nextEngines[i].nextEngine        = getEngineByType( xmlCCNode.entries[i].action );
        ccNode.nextEngines[i].nextEngineStr     = getEngineByTypeStr( xmlCCNode.entries[i].action );
        ccNode.nextEngines[i].newFqid           = xmlCCNode.entries[i].qbase;
        ccNode.nextEngines[i].doneAction        = e_FM_PCD_ENQ_FRAME;
        ccNode.nextEngines[i].doneActionStr     = "e_FM_PCD_ENQ_FRAME";
        ccNode.nextEngines[i].actionHandleIndex = 0xFFFFFFFF;
        if ( ccNode.nextEngines[i].nextEngine == e_FM_PCD_DONE &&
             xmlCCNode.entries[i].action      == "drop" ) {
            ccNode.nextEngines[i].doneAction    = e_FM_PCD_DROP_FRAME;
            ccNode.nextEngines[i].doneActionStr = "e_FM_PCD_DROP_FRAME";
        }
        if ( ccNode.nextEngines[i].nextEngine == e_FM_PCD_CC ) {
            ccNode.nextEngines[i].actionHandleIndex =
                FindAndAddCCNodeByName( pTaskDef, xmlCCNode.entries[i].actionName,
                                        ccNode.name, port, false );
        }
        else if ( ccNode.nextEngines[i].nextEngine == e_FM_PCD_KG ) {
            ccNode.nextEngines[i].actionHandleIndex =
                FindAndAddSchemeByName( pTaskDef, xmlCCNode.entries[i].actionName,
                                        ccNode.name, port, true );
        }
        else if ( ccNode.nextEngines[i].nextEngine == e_FM_PCD_PLCR ) {
            ccNode.nextEngines[i].actionHandleIndex =
                FindAndAddPolicerByName( pTaskDef, xmlCCNode.entries[i].actionName,
                                        ccNode.name, port );
        }
    }

    ccNode.nextEngineOnMiss.nextEngine        = getEngineByType( xmlCCNode.actionOnMiss );
    ccNode.nextEngineOnMiss.nextEngineStr     = getEngineByTypeStr( xmlCCNode.actionOnMiss );
    ccNode.nextEngineOnMiss.newFqid           = 0;
    ccNode.nextEngineOnMiss.doneAction        = e_FM_PCD_ENQ_FRAME;
    ccNode.nextEngineOnMiss.doneActionStr     = "e_FM_PCD_ENQ_FRAME";
    ccNode.nextEngineOnMiss.actionHandleIndex = 0xFFFFFFFF;
    if ( ccNode.nextEngineOnMiss.nextEngine == e_FM_PCD_DONE &&
         xmlCCNode.actionOnMiss             == "drop" ) {
        ccNode.nextEngineOnMiss.doneAction    = e_FM_PCD_DROP_FRAME;
        ccNode.nextEngineOnMiss.doneActionStr = "e_FM_PCD_DROP_FRAME";
    }
    if ( ccNode.nextEngineOnMiss.nextEngine == e_FM_PCD_CC ) {
        ccNode.nextEngineOnMiss.actionHandleIndex =
            FindAndAddCCNodeByName( pTaskDef, xmlCCNode.actionNameOnMiss,
                                    ccNode.name, port, false );
    }
    else if ( ccNode.nextEngineOnMiss.nextEngine == e_FM_PCD_KG ) {
        ccNode.nextEngineOnMiss.actionHandleIndex =
            FindAndAddSchemeByName( pTaskDef, xmlCCNode.actionNameOnMiss,
                                    ccNode.name, port, true );
    }
    else if ( ccNode.nextEngineOnMiss.nextEngine == e_FM_PCD_PLCR ) {
        ccNode.nextEngineOnMiss.actionHandleIndex =
            FindAndAddPolicerByName( pTaskDef, xmlCCNode.actionNameOnMiss,
                                    ccNode.name, port );
    }
    else if ( ccNode.nextEngineOnMiss.nextEngine == e_FM_PCD_DONE ) {
        ccNode.nextEngineOnMiss.newFqid = xmlCCNode.qbase;
    }

    return ccNode;
}


////////////////////////////////////////////////////////////////////////////////
/// Finds XML definition for a coarse classification node and adds it
////////////////////////////////////////////////////////////////////////////////
unsigned int
CFMCModel::FindAndAddCCNodeByName( const CTaskDef* pTaskDef, std::string name,
                                   std::string from, Port& port, bool isRoot )
{
    std::map< std::string, CClassification >::const_iterator nodeIt;
    nodeIt = pTaskDef->classifications.find( name );
    // Does such node exist?
    if ( nodeIt == pTaskDef->classifications.end() ) {
        throw CGenericError( ERR_CC_NOT_FOUND, name, from );
    }

    // Check whether this node was already created
    bool         found = false;
    unsigned int index;
    for ( unsigned int i = 0; i < all_ccnodes.size(); ++i ) {
        if ( ( all_ccnodes[i].name == name ) &&
             ( all_ccnodes[i].port_signature == port.signature ) ) {
            found = true;
            index = all_ccnodes[i].getIndex();

            applier.move( ApplyOrder::Entry( ApplyOrder::CCNode,
                                             index ) );
        }
    }

    if ( !found ) {
        CCNode& ccnode = createCCNode( pTaskDef, port, nodeIt->second );
        index = ccnode.getIndex();
    }

    if ( isRoot ) {
        port.cctrees.push_back( index );
        return port.cctrees.size() - 1;
    }

    return index;
}


////////////////////////////////////////////////////////////////////////////////
/// Converts Distribution parameters provided throught XML file to the internal
/// softparse result structure
////////////////////////////////////////////////////////////////////////////////
void
CFMCModel::createSoftParse( const CTaskDef* pTaskDef )
{
    const CSoftParseResult& taskSpr = pTaskDef->spr;

    if (!taskSpr.softParseEnabled)
        spEnable = 0;
    else
    {
        spEnable = 1;
        swPrs.override    = TRUE;
        swPrs.size        = taskSpr.size;
        swPrs.base        = taskSpr.base;
        swPrs.numOfLabels = taskSpr.numOfLabels;
        swPrs.p_Code      = spCode;

        // Copy SP code
        unsigned int i, j;
        memset( spCode, 0, CODE_SIZE );
        for ( i = 2*( taskSpr.base - ASSEMBLER_BASE ), j = 0; i < CODE_SIZE - 4; ++i, ++j ) {
            spCode[j] = taskSpr.p_Code[i];
        }

        // Copy SP parameters
        for ( i = 0; i < FM_PCD_PRS_NUM_OF_HDRS; ++i ) {
            swPrs.swPrsDataParams[i] = 0;
        }

        // Copy SP label info
        for ( i = 0; i < swPrs.numOfLabels; ++i ) {
            swPrs.labelsTable[i].indexPerHdr =
                taskSpr.labelsTable[i].indexPerHdr;
            swPrs.labelsTable[i].instructionOffset =
                taskSpr.labelsTable[i].position;
            swPrs.labelsTable[i].hdr =
                getNetCommHeaderType(taskSpr.labelsTable[i].prevNames[0]);
            if ( swPrs.labelsTable[i].hdr == HEADER_TYPE_NONE ) {
                std::string shim_protocol =
                    pTaskDef->getShimNoFromCustom( taskSpr.labelsTable[i].prevNames[0] );
                if ( !shim_protocol.empty() ) { // Shim header is empty if custom protocol is not found
                    swPrs.labelsTable[i].hdr = getNetCommHeaderType( shim_protocol );
                }
                else {
                    // Header type HEADER_TYPE_NONE is not accepted as a valid value
                    throw CGenericError( ERR_UNKNOWN_PROTOCOL, taskSpr.labelsTable[i].prevNames[0] );
                }
            }
        }
    }
};


////////////////////////////////////////////////////////////////////////////////
/// Finds XML definition for a scheme and adds it
////////////////////////////////////////////////////////////////////////////////
unsigned int
CFMCModel::FindAndAddSchemeByName( const CTaskDef* pTaskDef, std::string name,
                                   std::string from, Port& port, bool isDirect )
{
    std::map< std::string, CDistribution >::const_iterator distIt;
    distIt = pTaskDef->distributions.find( name );
    // Does such distribution exist?
    if ( distIt == pTaskDef->distributions.end() ) {
        throw CGenericError( ERR_DISTR_NOT_FOUND, name, from );
    }

    // Check whether this scheme was already created
    bool         found = false;
    unsigned int index;
    for ( unsigned int i = 0; i < all_schemes.size(); ++i ) {
        if ( ( all_schemes[i].name           == name )                 &&
             ( all_schemes[i].isDirect       == ( isDirect ? 1 : 0 ) ) &&
             ( all_schemes[i].port_signature == port.signature ) ) {
            found = true;
            index = all_schemes[i].getIndex();

            applier.move( ApplyOrder::Entry( ApplyOrder::Scheme,
                                             index ) );
        }
    }

    if ( !found ) {
        Scheme& scheme = createScheme( pTaskDef, port, distIt->second, isDirect );
        index = scheme.getIndex();
    }

    return index;
}


////////////////////////////////////////////////////////////////////////////////
/// Converts Policer parameters provided throught XML file to the internal
/// Policer structure
////////////////////////////////////////////////////////////////////////////////
Policer&
CFMCModel::createPolicer( const CTaskDef* pTaskDef, Port& port, const CPolicer& xmlPolicer )
{
    Policer& policer = all_policers[FMBlock::assignIndex( all_policers )];

    applier.add( ApplyOrder::Entry( ApplyOrder::Policer, policer.getIndex() ) );

    policer.name = xmlPolicer.name;

    switch( xmlPolicer.algorithm ) {
        case 1:
            policer.algorithm    = e_FM_PCD_PLCR_RFC_2698;
            policer.algorithmStr = "e_FM_PCD_PLCR_RFC_2698";
            break;
        case 2:
            policer.algorithm    = e_FM_PCD_PLCR_RFC_4115;
            policer.algorithmStr = "e_FM_PCD_PLCR_RFC_4115";
            break;
        default:
            policer.algorithm    = e_FM_PCD_PLCR_PASS_THROUGH;
            policer.algorithmStr = "e_FM_PCD_PLCR_PASS_THROUGH";
            break;
    }

    if ( xmlPolicer.color_mode == 1 ) {
        policer.colorMode    = e_FM_PCD_PLCR_COLOR_AWARE;
        policer.colorModeStr = "e_FM_PCD_PLCR_COLOR_AWARE";
    }
    else {
        policer.colorMode    = e_FM_PCD_PLCR_COLOR_BLIND;
        policer.colorModeStr = "e_FM_PCD_PLCR_COLOR_BLIND";
    }

    if ( xmlPolicer.unit == 1 ) {
        policer.rateMode    = e_FM_PCD_PLCR_PACKET_MODE;
        policer.rateModeStr = "e_FM_PCD_PLCR_PACKET_MODE";
    }
    else {
        policer.rateMode    = e_FM_PCD_PLCR_BYTE_MODE;
        policer.rateModeStr = "e_FM_PCD_PLCR_BYTE_MODE";
    }

    policer.comittedInfoRate         = xmlPolicer.CIR;
    policer.peakOrAccessiveInfoRate  = xmlPolicer.EIR;
    policer.comittedBurstSize        = xmlPolicer.CBS;
    policer.peakOrAccessiveBurstSize = xmlPolicer.EBS;

    policer.dfltColor    = getPlcrColor( xmlPolicer.dfltColor );
    policer.dfltColorStr = getPlcrColorStr( xmlPolicer.dfltColor );

    policer.nextEngineOnGreen        = getEngineByType( xmlPolicer.actionOnGreen );
    policer.nextEngineOnGreenStr     = getEngineByTypeStr( xmlPolicer.actionOnGreen );
    policer.onGreenAction            = e_FM_PCD_ENQ_FRAME;
    policer.onGreenActionStr         = "e_FM_PCD_ENQ_FRAME";
    policer.onGreenActionHandleIndex = 0xFFFFFFFF;
    if ( xmlPolicer.actionOnGreen == "drop" ) {
        policer.onGreenAction    = e_FM_PCD_DROP_FRAME;
        policer.onGreenActionStr = "e_FM_PCD_DROP_FRAME";
    }
    if ( policer.nextEngineOnGreen == e_FM_PCD_KG ) {
        policer.onGreenActionHandleIndex =
            FindAndAddSchemeByName( pTaskDef, xmlPolicer.actionNameOnGreen,
                                    xmlPolicer.name, port, true );
    } else if ( policer.nextEngineOnGreen == e_FM_PCD_PLCR ) {
        policer.onGreenActionHandleIndex =
            FindAndAddPolicerByName( pTaskDef, xmlPolicer.actionNameOnGreen,
                                     xmlPolicer.name, port );
    }

    policer.nextEngineOnYellow        = getEngineByType( xmlPolicer.actionOnYellow );
    policer.nextEngineOnYellowStr     = getEngineByTypeStr( xmlPolicer.actionOnYellow );
    policer.onYellowAction            = e_FM_PCD_ENQ_FRAME;
    policer.onYellowActionStr         = "e_FM_PCD_ENQ_FRAME";
    policer.onYellowActionHandleIndex = 0xFFFFFFFF;
    if ( xmlPolicer.actionOnYellow == "drop" ) {
        policer.onYellowAction    = e_FM_PCD_DROP_FRAME;
        policer.onYellowActionStr = "e_FM_PCD_DROP_FRAME";
    }
    if ( policer.nextEngineOnYellow == e_FM_PCD_KG ) {
        policer.onYellowActionHandleIndex =
            FindAndAddSchemeByName( pTaskDef, xmlPolicer.actionNameOnYellow,
                                    xmlPolicer.name, port, true );
    } else if ( policer.nextEngineOnYellow == e_FM_PCD_PLCR ) {
        policer.onYellowActionHandleIndex =
            FindAndAddPolicerByName( pTaskDef, xmlPolicer.actionNameOnYellow,
                                     xmlPolicer.name, port );
    }

    policer.nextEngineOnRed        = getEngineByType( xmlPolicer.actionOnRed );
    policer.nextEngineOnRedStr     = getEngineByTypeStr( xmlPolicer.actionOnRed );
    policer.onRedAction            = e_FM_PCD_ENQ_FRAME;
    policer.onRedActionStr         = "e_FM_PCD_ENQ_FRAME";
    policer.onRedActionHandleIndex = 0xFFFFFFFF;
    if ( xmlPolicer.actionOnRed == "drop" ) {
        policer.onRedAction    = e_FM_PCD_DROP_FRAME;
        policer.onRedActionStr = "e_FM_PCD_DROP_FRAME";
    }
    if ( policer.nextEngineOnRed == e_FM_PCD_KG ) {
        policer.onRedActionHandleIndex =
            FindAndAddSchemeByName( pTaskDef, xmlPolicer.actionNameOnRed,
                                    xmlPolicer.name, port, true );
    } else if ( policer.nextEngineOnRed == e_FM_PCD_PLCR ) {
        policer.onRedActionHandleIndex =
            FindAndAddPolicerByName( pTaskDef, xmlPolicer.actionNameOnRed,
                                     xmlPolicer.name, port );
    }

    return policer;
}


////////////////////////////////////////////////////////////////////////////////
/// Finds XML definition for a policer and adds it
////////////////////////////////////////////////////////////////////////////////
unsigned int
CFMCModel::FindAndAddPolicerByName( const CTaskDef* pTaskDef, std::string name,
                                    std::string from, Port& port )
{
    std::map< std::string, CPolicer >::const_iterator policerIt;
    policerIt = pTaskDef->policers.find( name );
    // Does such policer exist?
    if ( policerIt == pTaskDef->policers.end() ) {
        throw CGenericError( ERR_POLICER_NOT_FOUND, name, from );
    }
    Policer& policer = createPolicer( pTaskDef, port, policerIt->second );

    return policer.getIndex();
}


////////////////////////////////////////////////////////////////////////////////
/// Returns port type as e_FmPortType
////////////////////////////////////////////////////////////////////////////////
e_FmPortType
CFMCModel::getPortType( std::string type )
{
    if ( "1G" == type ) {
        return e_FM_PORT_TYPE_RX;
    }
    else if ( "10G" == type ) {
        return e_FM_PORT_TYPE_RX_10G;
    }
    else if ( "OFFLINE" == type ) {
        return e_FM_PORT_TYPE_OH_OFFLINE_PARSING;
    }
    else if ( "HOST_CMD" == type ) {
        return e_FM_PORT_TYPE_OH_HOST_COMMAND;
    }
    else if ( "TX_1G" == type ) {
        return e_FM_PORT_TYPE_TX;
    }
    else if ( "TX_10G" == type ) {
        return e_FM_PORT_TYPE_TX_10G;
    }
    else {
        return e_FM_PORT_TYPE_DUMMY;
    }
}


////////////////////////////////////////////////////////////////////////////////
/// Returns port type as e_FmPortType converted to string
////////////////////////////////////////////////////////////////////////////////
std::string
CFMCModel::getPortTypeStr( e_FmPortType type )
{
    switch ( type ) {
        case e_FM_PORT_TYPE_OH_OFFLINE_PARSING:
            return "e_FM_PORT_TYPE_OH_OFFLINE_PARSING";
        case e_FM_PORT_TYPE_OH_HOST_COMMAND:
            return "e_FM_PORT_TYPE_OH_HOST_COMMAND";
        case e_FM_PORT_TYPE_RX:
            return "e_FM_PORT_TYPE_RX";
        case e_FM_PORT_TYPE_RX_10G:
            return "e_FM_PORT_TYPE_RX_10G";
        case e_FM_PORT_TYPE_TX:
            return "e_FM_PORT_TYPE_TX";
        case e_FM_PORT_TYPE_TX_10G:
            return "e_FM_PORT_TYPE_TX_10G";
        case e_FM_PORT_TYPE_DUMMY:
            return "e_FM_PORT_TYPE_DUMMY";
    }

    return "e_FM_PORT_TYPE_DUMMY";
}


////////////////////////////////////////////////////////////////////////////////
/// Returns NetComm header type as e_NetHeaderType
////////////////////////////////////////////////////////////////////////////////
e_NetHeaderType
CFMCModel::getNetCommHeaderType( std::string protoname )
{
    std::map< std::string, e_NetHeaderType > net_types;
    net_types[ "ethernet" ]  = HEADER_TYPE_ETH;
    net_types[ "vlan" ]      = HEADER_TYPE_VLAN;
    net_types[ "llc_snap" ]  = HEADER_TYPE_LLC_SNAP;
    net_types[ "mpls" ]      = HEADER_TYPE_MPLS;
    net_types[ "ipv4" ]      = HEADER_TYPE_IPv4;
    net_types[ "ipv6" ]      = HEADER_TYPE_IPv6;
    net_types[ "tcp" ]       = HEADER_TYPE_TCP;
    net_types[ "udp" ]       = HEADER_TYPE_UDP;
    net_types[ "gre" ]       = HEADER_TYPE_GRE;
    net_types[ "pppoe" ]     = HEADER_TYPE_PPPoE;
    net_types[ "minencap" ]  = HEADER_TYPE_MINENCAP;
    net_types[ "sctp" ]      = HEADER_TYPE_SCTP;
    net_types[ "dccp" ]      = HEADER_TYPE_DCCP;
    net_types[ "ipsec_ah" ]  = HEADER_TYPE_IPSEC_AH;
    net_types[ "ipsec_esp" ] = HEADER_TYPE_IPSEC_ESP;
    net_types[ "shim1" ]     = HEADER_TYPE_USER_DEFINED_SHIM1;
    net_types[ "shim2" ]     = HEADER_TYPE_USER_DEFINED_SHIM2;
#ifdef FM_SHIM3_SUPPORT
    net_types[ "shim3" ]     = HEADER_TYPE_USER_DEFINED_SHIM3;
#endif /* FM_SHIM3_SUPPORT */


    if ( net_types.find( protoname ) != net_types.end() ) {
        return net_types[protoname];
    }

    return HEADER_TYPE_NONE;
}


////////////////////////////////////////////////////////////////////////////////
/// Returns NetComm header type as e_NetHeaderType converted to string
////////////////////////////////////////////////////////////////////////////////
std::string
CFMCModel::getNetCommHeaderTypeStr( std::string protoname )
{
    std::map< std::string, std::string > net_types;
    net_types[ "ethernet" ]  = "HEADER_TYPE_ETH";
    net_types[ "vlan" ]      = "HEADER_TYPE_VLAN";
    net_types[ "llc_snap" ]  = "HEADER_TYPE_LLC_SNAP";
    net_types[ "mpls" ]      = "HEADER_TYPE_MPLS";
    net_types[ "ipv4" ]      = "HEADER_TYPE_IPv4";
    net_types[ "ipv6" ]      = "HEADER_TYPE_IPv6";
    net_types[ "tcp" ]       = "HEADER_TYPE_TCP";
    net_types[ "udp" ]       = "HEADER_TYPE_UDP";
    net_types[ "gre" ]       = "HEADER_TYPE_GRE";
    net_types[ "pppoe" ]     = "HEADER_TYPE_PPPoE";
    net_types[ "minencap" ]  = "HEADER_TYPE_MINENCAP";
    net_types[ "sctp" ]      = "HEADER_TYPE_SCTP";
    net_types[ "dccp" ]      = "HEADER_TYPE_DCCP";
    net_types[ "ipsec_ah" ]  = "HEADER_TYPE_IPSEC_AH";
    net_types[ "ipsec_esp" ] = "HEADER_TYPE_IPSEC_ESP";
    net_types[ "shim1" ]     = "HEADER_TYPE_USER_DEFINED_SHIM1";
    net_types[ "shim2" ]     = "HEADER_TYPE_USER_DEFINED_SHIM2";
#ifdef FM_SHIM3_SUPPORT
    net_types[ "shim3" ]     = "HEADER_TYPE_USER_DEFINED_SHIM3";
#endif /* FM_SHIM3_SUPPORT */

    if ( net_types.find( protoname ) != net_types.end() ) {
        return net_types[protoname];
    }

    return "HEADER_TYPE_NONE";
}


////////////////////////////////////////////////////////////////////////////////
/// Returns NetComm header type as e_NetHeaderType converted to string
////////////////////////////////////////////////////////////////////////////////
std::string
CFMCModel::getNetCommHeaderTypeStr( e_NetHeaderType proto )
{
    std::map< e_NetHeaderType, std::string > net_types;
    net_types[ HEADER_TYPE_ETH ]                = "HEADER_TYPE_ETH";
    net_types[ HEADER_TYPE_VLAN ]               = "HEADER_TYPE_VLAN";
    net_types[ HEADER_TYPE_LLC_SNAP ]           = "HEADER_TYPE_LLC_SNAP";
    net_types[ HEADER_TYPE_MPLS ]               = "HEADER_TYPE_MPLS";
    net_types[ HEADER_TYPE_IPv4 ]               = "HEADER_TYPE_IPv4";
    net_types[ HEADER_TYPE_IPv6 ]               = "HEADER_TYPE_IPv6";
    net_types[ HEADER_TYPE_TCP ]                = "HEADER_TYPE_TCP";
    net_types[ HEADER_TYPE_UDP ]                = "HEADER_TYPE_UDP";
    net_types[ HEADER_TYPE_GRE ]                = "HEADER_TYPE_GRE";
    net_types[ HEADER_TYPE_PPPoE ]              = "HEADER_TYPE_PPPoE";
    net_types[ HEADER_TYPE_MINENCAP ]           = "HEADER_TYPE_MINENCAP";
    net_types[ HEADER_TYPE_SCTP ]               = "HEADER_TYPE_SCTP";
    net_types[ HEADER_TYPE_DCCP ]               = "HEADER_TYPE_DCCP";
    net_types[ HEADER_TYPE_IPSEC_AH ]           = "HEADER_TYPE_IPSEC_AH";
    net_types[ HEADER_TYPE_IPSEC_ESP ]          = "HEADER_TYPE_IPSEC_ESP";
    net_types[ HEADER_TYPE_USER_DEFINED_SHIM1 ] = "HEADER_TYPE_USER_DEFINED_SHIM1";
    net_types[ HEADER_TYPE_USER_DEFINED_SHIM2 ] = "HEADER_TYPE_USER_DEFINED_SHIM2";
#ifdef FM_SHIM3_SUPPORT
    net_types[ HEADER_TYPE_USER_DEFINED_SHIM3 ] = "HEADER_TYPE_USER_DEFINED_SHIM3";
#endif /* FM_SHIM3_SUPPORT */

    if ( net_types.find( proto ) != net_types.end() ) {
        return net_types[proto];
    }

    return "HEADER_TYPE_NONE";
}


////////////////////////////////////////////////////////////////////////////////
/// Returns NetComm field type converted to string
////////////////////////////////////////////////////////////////////////////////
unsigned int
CFMCModel::getNetCommFieldType( std::string fieldname )
{
    std::map< std::string, unsigned int > fields;

    fields["ethernet.dst"]        = NET_HEADER_FIELD_ETH_DA;
    fields["ethernet.src"]        = NET_HEADER_FIELD_ETH_SA;
    fields["ethernet.type"]       = NET_HEADER_FIELD_ETH_TYPE;
    fields["vlan.pri"]            = NET_HEADER_FIELD_VLAN_VPRI;
    fields["vlan.cfi"]            = NET_HEADER_FIELD_VLAN_CFI;
    fields["vlan.vlanid"]         = NET_HEADER_FIELD_VLAN_VID;
    fields["vlan.type"]           = NET_HEADER_FIELD_VLAN_TYPE;
    fields["vlan.tci"]            = NET_HEADER_FIELD_VLAN_TCI;
    fields["llc_snap.OUI"]        = NET_HEADER_FIELD_SNAP_OUI;
    fields["llc_snap.type"]       = NET_HEADER_FIELD_LLC_SNAP_TYPE;
    fields["llc_snap.dsap"]       = NET_HEADER_FIELD_LLC_DSAP;
    fields["llc_snap.ssap"]       = NET_HEADER_FIELD_LLC_SSAP;
    fields["mpls.label"]          = NET_HEADER_FIELD_MPLS_LABEL_STACK;
    fields["ipv4.ver"]            = NET_HEADER_FIELD_IPv4_VER;
    fields["ipv4.hlen"]           = NET_HEADER_FIELD_IPv4_HDR_LEN;
    fields["ipv4.tos"]            = NET_HEADER_FIELD_IPv4_TOS;
    fields["ipv4.tlen"]           = NET_HEADER_FIELD_IPv4_TOTAL_LEN;
    fields["ipv4.identification"] = NET_HEADER_FIELD_IPv4_ID;
    fields["ipv4.df"]             = NET_HEADER_FIELD_IPv4_FLAG_D;
    fields["ipv4.mf"]             = NET_HEADER_FIELD_IPv4_FLAG_M;
    fields["ipv4.foffset"]        = NET_HEADER_FIELD_IPv4_OFFSET;
    fields["ipv4.ttl"]            = NET_HEADER_FIELD_IPv4_TTL;
    fields["ipv4.nextp"]          = NET_HEADER_FIELD_IPv4_PROTO;
    fields["ipv4.hchecksum"]      = NET_HEADER_FIELD_IPv4_CKSUM;
    fields["ipv4.src"]            = NET_HEADER_FIELD_IPv4_SRC_IP;
    fields["ipv4.dst"]            = NET_HEADER_FIELD_IPv4_DST_IP;
    fields["ipv6.nexthdr"]        = NET_HEADER_FIELD_IPv6_NEXT_HDR;
    fields["ipv6.src"]            = NET_HEADER_FIELD_IPv6_SRC_IP;
    fields["ipv6.dst"]            = NET_HEADER_FIELD_IPv6_DST_IP;
    fields["ipv6.hop"]            = NET_HEADER_FIELD_IPv6_HOP_LIMIT;
    fields["tcp.sport"]           = NET_HEADER_FIELD_TCP_PORT_SRC;
    fields["tcp.dport"]           = NET_HEADER_FIELD_TCP_PORT_DST;
    fields["tcp.seq"]             = NET_HEADER_FIELD_TCP_SEQ;
    fields["tcp.ack"]             = NET_HEADER_FIELD_TCP_ACK;
    fields["tcp.flags"]           = NET_HEADER_FIELD_TCP_FLAGS;
    fields["tcp.win"]             = NET_HEADER_FIELD_TCP_WINDOW;
    fields["tcp.crc"]             = NET_HEADER_FIELD_TCP_CKSUM;
    fields["tcp.urg"]             = NET_HEADER_FIELD_TCP_URGPTR;
    fields["udp.sport"]           = NET_HEADER_FIELD_UDP_PORT_SRC;
    fields["udp.dport"]           = NET_HEADER_FIELD_UDP_PORT_DST;
    fields["udp.len"]             = NET_HEADER_FIELD_UDP_LEN;
    fields["udp.crc"]             = NET_HEADER_FIELD_UDP_CKSUM;
    fields["gre.type"]            = NET_HEADER_FIELD_GRE_TYPE;
    fields["pppoe.ver"]           = NET_HEADER_FIELD_PPPoE_VER;
    fields["pppoe.type"]          = NET_HEADER_FIELD_PPPoE_TYPE;
    fields["pppoe.code"]          = NET_HEADER_FIELD_PPPoE_CODE;
    fields["pppoe.session_ID"]    = NET_HEADER_FIELD_PPPoE_SID;
    fields["pppoe.hlen"]          = NET_HEADER_FIELD_PPPoE_LEN;
    fields["pppoe.nextp"]         = NET_HEADER_FIELD_PPP_PID;
    fields["minencap.dst"]        = NET_HEADER_FIELD_MINENCAP_DST_IP;
    fields["sctp.sport"]          = NET_HEADER_FIELD_SCTP_PORT_SRC;
    fields["sctp.dport"]          = NET_HEADER_FIELD_SCTP_PORT_DST;
    fields["sctp.ver_tag"]        = NET_HEADER_FIELD_SCTP_VER_TAG;
    fields["sctp.crc"]            = NET_HEADER_FIELD_SCTP_CKSUM;
    fields["dccp.sport"]          = NET_HEADER_FIELD_DCCP_PORT_SRC;
    fields["dccp.dport"]          = NET_HEADER_FIELD_DCCP_PORT_DST;
    fields["ipsec_ah.nexthdr"]    = NET_HEADER_FIELD_IPSEC_AH_NH;
    fields["ipsec_ah.spi"]        = NET_HEADER_FIELD_IPSEC_AH_SPI;
    fields["ipsec_esp.spi"]       = NET_HEADER_FIELD_IPSEC_ESP_SPI;

    if ( fields.find( fieldname ) != fields.end() ) {
        return fields[fieldname];
    }

    return UNKNOWN_NetCommField;
}


////////////////////////////////////////////////////////////////////////////////
/// Returns NetComm field type converted to string
////////////////////////////////////////////////////////////////////////////////
std::string
CFMCModel::getNetCommFieldTypeStr( std::string fieldname )
{
    std::map< std::string, std::string > fields;

    fields["ethernet.dst"]        = "NET_HEADER_FIELD_ETH_DA";
    fields["ethernet.src"]        = "NET_HEADER_FIELD_ETH_SA";
    fields["ethernet.type"]       = "NET_HEADER_FIELD_ETH_TYPE";
    fields["vlan.pri"]            = "NET_HEADER_FIELD_VLAN_VPRI";
    fields["vlan.cfi"]            = "NET_HEADER_FIELD_VLAN_CFI";
    fields["vlan.vlanid"]         = "NET_HEADER_FIELD_VLAN_VID";
    fields["vlan.type"]           = "NET_HEADER_FIELD_VLAN_TYPE";
    fields["vlan.tci"]            = "NET_HEADER_FIELD_VLAN_TCI";
    fields["llc_snap.OUI"]        = "NET_HEADER_FIELD_SNAP_OUI";
    fields["llc_snap.type"]       = "NET_HEADER_FIELD_LLC_SNAP_TYPE";
    fields["llc_snap.dsap"]       = "NET_HEADER_FIELD_LLC_DSAP";
    fields["llc_snap.ssap"]       = "NET_HEADER_FIELD_LLC_SSAP";
    fields["mpls.label"]          = "NET_HEADER_FIELD_MPLS_LABEL_STACK";
    fields["ipv4.ver"]            = "NET_HEADER_FIELD_IPv4_VER";
    fields["ipv4.hlen"]           = "NET_HEADER_FIELD_IPv4_HDR_LEN";
    fields["ipv4.tos"]            = "NET_HEADER_FIELD_IPv4_TOS";
    fields["ipv4.tlen"]           = "NET_HEADER_FIELD_IPv4_TOTAL_LEN";
    fields["ipv4.identification"] = "NET_HEADER_FIELD_IPv4_ID";
    fields["ipv4.df"]             = "NET_HEADER_FIELD_IPv4_FLAG_D";
    fields["ipv4.mf"]             = "NET_HEADER_FIELD_IPv4_FLAG_M";
    fields["ipv4.foffset"]        = "NET_HEADER_FIELD_IPv4_OFFSET";
    fields["ipv4.ttl"]            = "NET_HEADER_FIELD_IPv4_TTL";
    fields["ipv4.nextp"]          = "NET_HEADER_FIELD_IPv4_PROTO";
    fields["ipv4.hchecksum"]      = "NET_HEADER_FIELD_IPv4_CKSUM";
    fields["ipv4.src"]            = "NET_HEADER_FIELD_IPv4_SRC_IP";
    fields["ipv4.dst"]            = "NET_HEADER_FIELD_IPv4_DST_IP";
    fields["ipv6.nexthdr"]        = "NET_HEADER_FIELD_IPv6_NEXT_HDR";
    fields["ipv6.src"]            = "NET_HEADER_FIELD_IPv6_SRC_IP";
    fields["ipv6.dst"]            = "NET_HEADER_FIELD_IPv6_DST_IP";
    fields["ipv6.hop"]            = "NET_HEADER_FIELD_IPv6_HOP_LIMIT";
    fields["tcp.sport"]           = "NET_HEADER_FIELD_TCP_PORT_SRC";
    fields["tcp.dport"]           = "NET_HEADER_FIELD_TCP_PORT_DST";
    fields["tcp.seq"]             = "NET_HEADER_FIELD_TCP_SEQ";
    fields["tcp.ack"]             = "NET_HEADER_FIELD_TCP_ACK";
    fields["tcp.flags"]           = "NET_HEADER_FIELD_TCP_FLAGS";
    fields["tcp.win"]             = "NET_HEADER_FIELD_TCP_WINDOW";
    fields["tcp.crc"]             = "NET_HEADER_FIELD_TCP_CKSUM";
    fields["tcp.urg"]             = "NET_HEADER_FIELD_TCP_URGPTR";
    fields["udp.sport"]           = "NET_HEADER_FIELD_UDP_PORT_SRC";
    fields["udp.dport"]           = "NET_HEADER_FIELD_UDP_PORT_DST";
    fields["udp.len"]             = "NET_HEADER_FIELD_UDP_LEN";
    fields["udp.crc"]             = "NET_HEADER_FIELD_UDP_CKSUM";
    fields["gre.type"]            = "NET_HEADER_FIELD_GRE_TYPE";
    fields["pppoe.ver"]           = "NET_HEADER_FIELD_PPPoE_VER";
    fields["pppoe.type"]          = "NET_HEADER_FIELD_PPPoE_TYPE";
    fields["pppoe.code"]          = "NET_HEADER_FIELD_PPPoE_CODE";
    fields["pppoe.session_ID"]    = "NET_HEADER_FIELD_PPPoE_SID";
    fields["pppoe.hlen"]          = "NET_HEADER_FIELD_PPPoE_LEN";
    fields["pppoe.nextp"]         = "NET_HEADER_FIELD_PPP_PID";
    fields["minencap.dst"]        = "NET_HEADER_FIELD_MINENCAP_DST_IP";
    fields["sctp.sport"]          = "NET_HEADER_FIELD_SCTP_PORT_SRC";
    fields["sctp.dport"]          = "NET_HEADER_FIELD_SCTP_PORT_DST";
    fields["sctp.ver_tag"]        = "NET_HEADER_FIELD_SCTP_VER_TAG";
    fields["sctp.crc"]            = "NET_HEADER_FIELD_SCTP_CKSUM";
    fields["dccp.sport"]          = "NET_HEADER_FIELD_DCCP_PORT_SRC";
    fields["dccp.dport"]          = "NET_HEADER_FIELD_DCCP_PORT_DST";
    fields["ipsec_ah.nexthdr"]    = "NET_HEADER_FIELD_IPSEC_AH_NH";
    fields["ipsec_ah.spi"]        = "NET_HEADER_FIELD_IPSEC_AH_SPI";
    fields["ipsec_esp.spi"]       = "NET_HEADER_FIELD_IPSEC_ESP_SPI";

    if ( fields.find( fieldname ) != fields.end() ) {
        return fields[fieldname];
    }

    return "UNKNOWN_FIELD";
}


////////////////////////////////////////////////////////////////////////////////
/// Returns true if a field can be extracted as a full field by coarse classific.
////////////////////////////////////////////////////////////////////////////////
bool
CFMCModel::isFullFieldForCC( std::string fieldName )
{
    const char* supported[] =
    {
        "ethernet.dst",
        "ethernet.src",
        "ethernet.type",
        "vlan.tci",
        "mpls.label",
        "ipv4.dst",
        "ipv4.tos",
        "ipv4.nextp",
        "ipv4.src",
        // "ipv4.dst" | "ipv4.src"
        "ipv4.ttl",
        "ipv6.nexthdr",
        "ipv6.dst",
        "ipv6.src",
        "ipv6.hop",
        "gre.type",
        "minencap.dst",
        // NET_HEADER_FIELD_MINENCAP_TYPE
        // NET_HEADER_FIELD_MINENCAP_SRC_IP
        "tcp.sport",
        "tcp.dport",
        // "tcp.sport" | "tcp.dport"
        "pppoe.nextp",
        "udp.sport",
        "udp.dport",
        // "udp.sport" | "udp.dport"
    };

    const size_t supported_len = sizeof(supported)/sizeof(supported[0]);
    const char** find_result = std::find( &supported[0],
                                          &supported[supported_len],
                                          fieldName );

    return ( find_result != &supported[supported_len] );
}


////////////////////////////////////////////////////////////////////////////////
/// Returns NetComm header index as e_FmPcdHdrIndex
////////////////////////////////////////////////////////////////////////////////
e_FmPcdHdrIndex
CFMCModel::getNetCommHeaderIndex( std::string indexstr )
{
    if ( indexstr == "0" ) {
        return e_FM_PCD_HDR_INDEX_NONE;
    }
    if ( indexstr == "1" ) {
        return e_FM_PCD_HDR_INDEX_1;
    }
    if ( indexstr == "2" ) {
        return e_FM_PCD_HDR_INDEX_2;
    }
    if ( indexstr == "3" ) {
        return e_FM_PCD_HDR_INDEX_3;
    }
    if ( indexstr == "last" ) {
        return e_FM_PCD_HDR_INDEX_LAST;
    }

    return e_FM_PCD_HDR_INDEX_NONE;
}


////////////////////////////////////////////////////////////////////////////////
/// Returns NetComm header index as string
////////////////////////////////////////////////////////////////////////////////
std::string
CFMCModel::getNetCommHeaderIndexStr( std::string indexstr )
{
    if ( indexstr == "0" ) {
        return "e_FM_PCD_HDR_INDEX_NONE";
    }
    if ( indexstr == "1" ) {
        return "e_FM_PCD_HDR_INDEX_1";
    }
    if ( indexstr == "2" ) {
        return "e_FM_PCD_HDR_INDEX_2";
    }
    if ( indexstr == "3" ) {
        return "e_FM_PCD_HDR_INDEX_3";
    }
    if ( indexstr == "last" ) {
        return "e_FM_PCD_HDR_INDEX_LAST";
    }

    return "e_FM_PCD_HDR_INDEX_NONE";
}


e_FmPcdPlcrColor
CFMCModel::getPlcrColor( std::string color )
{
    if ( color == "yellow" ) {
        return e_FM_PCD_PLCR_YELLOW;
    }
    else if ( color == "red" ) {
        return e_FM_PCD_PLCR_RED;
    }
    else if ( color == "override" ) {
        return e_FM_PCD_PLCR_OVERRIDE;
    }

    // Default color
    return e_FM_PCD_PLCR_GREEN;
}


std::string
CFMCModel::getPlcrColorStr( std::string color )
{
    if ( color == "yellow" ) {
        return "e_FM_PCD_PLCR_YELLOW";
    }
    else if ( color == "red" ) {
        return "e_FM_PCD_PLCR_RED";
    }
    else if ( color == "override" ) {
        return "e_FM_PCD_PLCR_OVERRIDE";
    }

    // Default color
    return "e_FM_PCD_PLCR_GREEN";
}


e_FmPcdEngine
CFMCModel::getEngineByType( std::string enginename )
{
    if ( enginename == "distribution" ) {
        return e_FM_PCD_KG;
    }
    else if (enginename == "classification" ) {
        return e_FM_PCD_CC;
    }
    else if (enginename == "policer" ) {
        return e_FM_PCD_PLCR;
    }
    else if (enginename == "parser" ) {
        return e_FM_PCD_PRS;
    }

    return e_FM_PCD_DONE;
}


std::string
CFMCModel::getEngineByTypeStr( std::string enginename )
{
    if ( enginename == "distribution" ) {
        return "e_FM_PCD_KG";
    }
    else if (enginename == "classification" ) {
        return "e_FM_PCD_CC";
    }
    else if (enginename == "policer" ) {
        return "e_FM_PCD_PLCR";
    }
    else if (enginename == "parser" ) {
        return "e_FM_PCD_PRS";
    }

    return "e_FM_PCD_DONE";
}


ApplyOrder::Entry::Entry( Type typePrm, unsigned int indexPrm ) :
    type( typePrm ), index( indexPrm )
{
}


void
ApplyOrder::add( Entry entry )
{
    entries.push_back( entry );
}


void
ApplyOrder::move( Entry entry )
{
    // Find existing entry
    std::vector< Entry >::iterator entryIt = entries.begin();
    for ( entryIt = entries.begin(); entryIt != entries.end(); ++entryIt ) {
        if ( entryIt->index == entry.index &&
             entryIt->type  == entry.type ) {
            entries.erase( entryIt );
            entryIt = entries.begin();
        }
    }
    add( entry );
}


void
ApplyOrder::addDelayed( Entry entry )
{
    delayed_entries.push_back( entry );
}


void
ApplyOrder::insertDelayedAfter( unsigned int after )
{
    entries.insert( entries.begin() + after,
        delayed_entries.begin(),
        delayed_entries.end() );

    delayed_entries.clear();
}


ApplyOrder::Entry
ApplyOrder::getAt( unsigned int index ) const
{
    return entries[index];
}


unsigned int
ApplyOrder::size() const
{
    return entries.size();
}


std::string
ApplyOrder::getTypeAsStr( Type t ) const
{
    switch ( t ) {
        case EngineStart:
            return "FMCEngineStart";
        case EngineEnd:
            return "FMCEngineEnd";
        case PortStart:
            return "FMCPortStart";
        case PortEnd:
            return "FMCPortEnd";
        case Scheme:
            return "FMCScheme";
        case CCNode:
            return "FMCCCNode";
        case CCTree:
            return "FMCCCTree";
        case Policer:
            return "FMCPolicer";
    }
    return "";
}
