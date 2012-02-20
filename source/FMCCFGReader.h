/* =====================================================================
 *
 *  Copyright 2009, 2010, Freescale Semiconductor, Inc., All Rights Reserved. 
 *
 *  This file contains copyrighted material. Use of this file is restricted
 *  by the provisions of a Freescale Software License Agreement, which has
 *  either accompanied the delivery of this software in shrink wrap
 *  form or been expressly executed between the parties.
 *
 *  File Name : FMCCFGReader.h
 *  Author    : Serge Lamikhov-Center
 *
 * ===================================================================*/

#ifndef CFGREADER_H
#define CFGREADER_H

#include <libxml/parser.h>
#include "FMCTaskDef.h"

class CCFGReader {
public:
    void setTaskData( CTaskDef* pTaskData );
    void parseCfgData( std::string filename );
    void parseEngine( xmlNodePtr pNode );

protected:
    static std::string getXMLAttr( xmlNodePtr pNode, const char* attr );
    static std::string getXMLElement( xmlNodePtr pNode );

private:
    CTaskDef* task;
    CGenericError error;
};

#endif // CFGREADER_H
