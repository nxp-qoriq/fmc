/* =====================================================================
 *
 *  Copyright 2009, 2010, Freescale Semiconductor, Inc., All Rights Reserved. 
 *
 *  This file contains copyrighted material. Use of this file is restricted
 *  by the provisions of a Freescale Software License Agreement, which has
 *  either accompanied the delivery of this software in shrink wrap
 *  form or been expressly executed between the parties.
 *
 *  File Name : FMCPDLReader.h
 *  Author    : Serge Lamikhov-Center
 *
 * ===================================================================*/

#ifndef PDLREADER_H
#define PDLREADER_H

#include <libxml/parser.h>
#include "FMCTaskDef.h"


class CPDLReader {
public:
    CPDLReader();

    void setTaskData( CTaskDef* pTaskData );
    void parseNetPDL( std::string filename );
    void setSoftParse( bool newSoftParse );
    bool getSoftParse () const;

    void parseProtocol     (CProtocol* protocol, xmlNodePtr pNode );    
    void parseExecute      (CProtocol* protocol, xmlNodePtr pNode );
    void parseExecuteSection(CExecuteSection* executeSection, xmlNodePtr pNode, bool virtualSection=0);
    void parseExecuteAction(CExecuteAction* executeAction, xmlNodePtr pNode );
    void parseExecuteAssign(CExecuteAssign* executeAssign, xmlNodePtr pNode );
    void parseExecuteCase  (CExecuteCase*   executeCase,   xmlNodePtr pNode );
    void parseExecuteIf    (CExecuteIf*     executeIf,     xmlNodePtr pNode );
    void parseExecuteLoop  (CExecuteLoop*   executeLoop,   xmlNodePtr pNode );
    void parseExecuteInline(CExecuteInline* executeInline, xmlNodePtr pNode );
    void parseExecuteSwitch(CExecuteSwitch* executeSwitch, xmlNodePtr pNode );	
    void parseFormat       (CProtocol* protocol, xmlNodePtr pNode );    
    void parseBlock        (CProtocol* protocol, xmlNodePtr pNode );
    void parseFields       (CProtocol* protocol, xmlNodePtr pNode );
    void parseField        (CField* field,       xmlNodePtr pNode );    
    
protected:
    static std::string getAttr( xmlNodePtr pNode, const char* attr );
    static void checkUnknownAttr( xmlNodePtr pNode, int num, ... );

private:
    CTaskDef* task;
    bool      softParse;
    CGenericError error;
};

#endif // PDLREADER_H
