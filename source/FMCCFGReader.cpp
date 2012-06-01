/* =====================================================================
 *
 *  Copyright 2009, 2010, Freescale Semiconductor, Inc., All Rights Reserved.
 *
 *  This file contains copyrighted material. Use of this file is restricted
 *  by the provisions of a Freescale Software License Agreement, which has
 *  either accompanied the delivery of this software in shrink wrap
 *  form or been expressly executed between the parties.
 *
 *  File Name : FMCCFGReader.cpp
 *  Author    : Serge Lamikhov-Center
 *
 * ===================================================================*/

#include <cstdlib>
#include <sstream>
#include <libxml/parser.h>

#include "FMCTaskDef.h"
#include "FMCCFGReader.h"
#include "FMCGenericError.h"
#include "FMCUtils.h"


// Aid function to retrieve XML element's attribute
std::string
CCFGReader::getXMLAttr( xmlNodePtr pNode, const char* attr )
{
    char* pAttr = (char*)xmlGetProp( pNode, (const xmlChar*)attr );
    return (pAttr) ? pAttr : "";
}


std::string
CCFGReader::getXMLElement( xmlNodePtr pNode )
{
    std::string ret = "";

    if ( pNode->children != 0 ) {
        if ( pNode->children->content != 0 ) {
            ret = (const char*)pNode->children->content;
        }
    }

    return ret;
}


void
CCFGReader::setTaskData( CTaskDef* pTaskData )
{
    task = pTaskData;
}


/////////////////////////////////////////////////////////////////////////////
// CCFGReader::parseNetPCD
// Process the root node of the NetPCD
/////////////////////////////////////////////////////////////////////////////
void
CCFGReader::parseCfgData( std::string filename )
{
    // LibXML preparations
    xmlInitParser();
    LIBXML_TEST_VERSION;
    xmlSetStructuredErrorFunc( &error, errorFuncHandler );
    xmlKeepBlanksDefault(0);

    // Parse the XML file
    xmlDocPtr doc = xmlParseFile( filename.c_str() );

    if ( !error.getErrorMsg().empty() ) {
        throw CGenericError( ERR_XML_PARSE_ERROR1, error.getErrorMsg() );
    }

    if ( 0 == doc ) {   // Was the parsing successful?
        throw CGenericError( ERR_CANT_OPEN_FILE, filename );
    }

    // Get the root node
    xmlNodePtr cur = xmlDocGetRootElement( doc );
    if ( 0 == cur ) {   // Is the document empty?
        xmlFreeDoc( doc );
        throw CGenericError( ERR_EMPTY_DOCUMENT, filename );
    }

    // Is the document of the correct type?
    if ( xmlStrcmp( cur->name, (const xmlChar *)"cfgdata" ) ) {
        xmlFreeDoc( doc );
        throw CGenericError( ERR_EMPTY_DOCUMENT, filename );
    }

    // Make sure we process the right node
    if ( xmlStrcmp( cur->name, (const xmlChar*)"cfgdata" ) ) {
        throw CGenericError( ERR_WRONG_TYPE1, (char*)cur->name );
    }

    // Parse children nodes
    cur = cur->xmlChildrenNode;
    while ( 0 != cur ) {
        // config
        if ( !xmlStrcmp( cur->name, (const xmlChar*)"config" ) ) {
            xmlNodePtr engineNode = cur->xmlChildrenNode;
            while ( engineNode ) {
                if ( !xmlStrcmp( engineNode->name, (const xmlChar*)"engine")) {
                    parseEngine( engineNode );
                }
                engineNode = engineNode->next;
            }
        }
        // comment
        else if ( !xmlStrcmp( cur->name, (const xmlChar*)"comment" ) ) {
        }
        // other
        else {
            CGenericError::printWarning(WARN_UNEXPECTED_NODE,(char*)cur->name);
        }

        cur = cur->next;
    }

    xmlFreeDoc( doc );
    xmlCleanupParser();
}


void
CCFGReader::parseEngine( xmlNodePtr pNode )
{
    // Make sure we process the right node
    if ( xmlStrcmp( pNode->name, (const xmlChar*)"engine" ) ) {
        throw CGenericError( ERR_WRONG_TYPE1, (char*)pNode->name );
    }

    CEngine engine;
    engine.name = getXMLAttr( pNode, "name" );
    if ( engine.name != "fm0" && engine.name != "fm1" ) {
        throw CGenericError( ERR_INVALID_ENGINE_NAME, engine.name );
    }

    std::string offload = stripBlanks( getXMLAttr( pNode, "offload" ) );
    engine.offload_support = std::strtol( offload.c_str(), 0, 0 );

    // Parse children nodes
    xmlNodePtr cur = pNode->xmlChildrenNode;
    while ( 0 != cur ) {
        // port
        if ( !xmlStrcmp( cur->name, (const xmlChar*)"port" ) ) {
            CPort port;
            port.type   = stripBlanks( getXMLAttr( cur, "type" ) );
            port.number = std::strtol( getXMLAttr( cur, "number" ).c_str(),
                                       0, 0 );
            port.policy = stripBlanks( getXMLAttr( cur, "policy" ) );
            port.portid = std::strtol( getXMLAttr( cur, "portid" ).c_str(),
                                       0, 0 );

            // Find the policy entry
            if ( task->policies.find( port.policy ) == task->policies.end() ) {
                throw CGenericError( ERR_POLICY_NOT_FOUND, port.policy,
                                     port.type, port.number );
            }

            // Add the port to the engine
            if ( !port.type.empty() && !port.policy.empty() ) {
                engine.ports.push_back( port );
            }
        }
        else if ( !xmlStrcmp( cur->name, (const xmlChar*)"default_queue1" ) ) {
            std::string text = getXMLElement( cur );
            engine.default_queue1 = std::strtol( text.c_str(), 0, 0 );
        }
        else if ( !xmlStrcmp( cur->name, (const xmlChar*)"default_queue2" ) ) {
            std::string text = getXMLElement( cur );
            engine.default_queue2 = std::strtol( text.c_str(), 0, 0 );
        }
        // comment
        else if ( !xmlStrcmp( cur->name, (const xmlChar*)"comment" ) ) {
        }
        // other
        else
            CGenericError::printWarning(WARN_UNEXPECTED_NODE,(char*)cur->name);


        cur = cur->next;
    }

    if ( !engine.name.empty() && ( 0 != engine.ports.size() ) ) {
        task->engines[engine.name] = engine;
    }
}
