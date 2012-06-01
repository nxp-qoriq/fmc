/* =====================================================================
 *
 *  Copyright 2009, 2010, Freescale Semiconductor, Inc., All Rights Reserved.
 *
 *  This file contains copyrighted material. Use of this file is restricted
 *  by the provisions of a Freescale Software License Agreement, which has
 *  either accompanied the delivery of this software in shrink wrap
 *  form or been expressly executed between the parties.
 *
 *  File Name : FMCPCDReader.h
 *  Author    : Serge Lamikhov-Center
 *
 * ===================================================================*/

#ifndef PCDREADER_H
#define PCDREADER_H

#include <libxml/parser.h>
#include "FMCTaskDef.h"

class CPCDReader {
public:
    void setTaskData( CTaskDef* pTaskData );
    void parseNetPCD( std::string filename );

    void parseAlias( xmlNodePtr pNode );
    void parseDistribution( CDistribution* distribution, xmlNodePtr pNode );
    void parseClassification( CClassification* classification, xmlNodePtr pNode );
    void parseFieldRef( CFieldRef* fieldref, xmlNodePtr pNode );
    void parseNonHeader( CNonHeaderEntry* nonHeaderEntry, xmlNodePtr pNode );
    void parsePolicy( CPolicy* policy, xmlNodePtr pNode );
    void parsePolicer( CPolicer* policer, xmlNodePtr pNode );
    void parseDistOrder( CPolicy* policy, xmlNodePtr pNode );
    void parseManipulations( xmlNodePtr pNode );
    void parseFragmentation( CFragmentation* fragmentation, xmlNodePtr pNode );
    void parseReassembly( CReassembly* reassembly, xmlNodePtr pNode );
    void parseHeaderManipulation( CHeaderManip* headerManip, xmlNodePtr pNode );
    void parseHeaderInsert( CHeaderInsert* headerInsert, xmlNodePtr pNode );
    void parseHeaderRemove( CHeaderRemove* headerRemove, xmlNodePtr pNode );

protected:
    static std::string getAttr( xmlNodePtr pNode, const char* attr );
    static std::string getXMLElement( xmlNodePtr pNode );
    static void checkUnknownAttr( xmlNodePtr pNode, int num, ... );

private:
    CTaskDef* task;
    CGenericError error;
};

#endif // PCDREADER_H
