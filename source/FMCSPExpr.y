/* =====================================================================
 *
 *  Copyright 2009,2010, Freescale Semiconductor, Inc., All Rights Reserved. 
 *
 *  This file contains copyrighted material. Use of this file is restricted
 *  by the provisions of a Freescale Software License Agreement, which has
 *  either accompanied the delivery of this software in shrink wrap
 *  form or been expressly executed between the parties.
 *
 *  File Name : FMCSPExpr.y
 *  Author    : Levy Ro'ee
 *
 * ===================================================================*/

%{
#include "FMCSPIR.h"
#include "FMCGenericError.h"

#ifdef _MSC_VER
#pragma warning(disable : 4065)
#endif

extern char parseString[100];
extern int FMCSPExprlex();

void FMCSPExprerror(const char *str)
{
    fprintf(stderr," Error: %s.\n", str);
    throw CGenericErrorLine(ERR_LEXER_ERROR,*getLineYacc(), parseString);
}

void warning(const char *str)
{
    fprintf(stderr,"warning: %s.\n", str);
}

%}

%expect 0

%union 
{
    uint64_t	number;
    CENode	    *node;
}


%token	OBJECT VARIABLE SUB_VARIABLE FIELD PROTOCOL PROTOCOL_FIELD
%token  HEADERSIZE DEFAULT_HEADER_SIZE
%token	VARIABLE_A SUB_VARIABLE_A FIELD_A PROTOCOL_FIELD_A ACCESS_ERROR
%token	DEC HEX BIN
%token	OPEN_PARENTHESIS CLOSE_PARENTHESIS OPEN_BRACKET CLOSE_BRACKET COMMA
%token	END

%left   OR      AND
%left   GREATER GREATEREQU	LESS LESSEQU EQU NOTEQU
%left	SHR		SHL         CONCAT
%left   BIT_AND XOR			BIT_OR
%left	PLUS	MINUS       ADD_CARRY
%left   NOT     BIT_NOT     CHECKSUM

%type  <node> _object objectError objectLegal
%type  <node> _num instructions headerSize 


%%

/* -----------INSTRUCTIONS------------ */ 

expr:
    instructions    { setExpressionYacc ($1); }
    ;

instructions:
    OPEN_PARENTHESIS instructions CLOSE_PARENTHESIS {$$ = $2;}
|	_num        {$$ = $1;}
|	_object     {$$ = $1;}
|   headerSize  {$$ = $1;}
|                NOT        instructions {$$ = new CENode(ENOT,     $2);}
|                BIT_NOT    instructions {$$ = new CENode(EBITNOT,  $2);} 
|   CHECKSUM  OPEN_PARENTHESIS instructions COMMA instructions 
              COMMA instructions CLOSE_PARENTHESIS 
        {$$ = new CENode(ECHECKSUM, $3, new CENode(ECHECKSUM_LOC, $5, $7));}
|   instructions PLUS       instructions {$$ = new CENode(EADD,     $1, $3);}
|   instructions ADD_CARRY  instructions {$$ = new CENode(EADDCARRY,$1, $3);}
|   instructions MINUS      instructions {$$ = new CENode(ESUB,     $1, $3);} 
|   instructions BIT_AND    instructions {$$ = new CENode(EBITAND,  $1, $3);}    
|   instructions BIT_OR     instructions {$$ = new CENode(EBITOR,   $1, $3);}
|   instructions XOR        instructions {$$ = new CENode(EXOR,     $1, $3);}    
|	instructions EQU		instructions {$$ = new CENode(EEQU,	    $1, $3);}
|	instructions NOTEQU		instructions {$$ = new CENode(ENOTEQU,  $1, $3);}
|	instructions LESS		instructions {$$ = new CENode(ELESS,    $1, $3);}
|	instructions LESSEQU	instructions {$$ = new CENode(ELESSEQU, $1, $3);}
|	instructions GREATER	instructions {$$ = new CENode(EGREATER, $1, $3);}
|	instructions GREATEREQU instructions {$$ = new CENode(EGREATEREQU, $1,$3);}
|	instructions SHR		instructions {$$ = new CENode(ESHR,     $1,$3);}
|	instructions SHL		instructions {$$ = new CENode(ESHL,     $1,$3);}
|	instructions CONCAT		instructions {$$ = new CENode(ESHLAND,  $1,$3);}
|	instructions OR	    	instructions {$$ = new CENode(EOR,      $1,$3);}
|	instructions AND		instructions {$$ = new CENode(EAND,     $1,$3);}
    ;
    
_num: num
    {
        CENode* eNode  = newENode(EINTCONST);
        stringToInt(parseString, eNode->intval, *getLineYacc());
        $$ = eNode; 
    }
    ;
    
num: DEC
|    HEX 
|    BIN
    ;
    
headerSize: HEADERSIZE
    {   
        if (getIRYacc()->status.currentAfter)            
            $$ = getIRYacc()->protocolsIRs[getIRYacc()->status.currentProtoIndex].
                                    headerSize->newDeepCopy();
        else
            $$ = getIRYacc()->protocolsIRs[getIRYacc()->status.currentProtoIndex].
                                    prevHeaderSize->newDeepCopy();
    }
|   DEFAULT_HEADER_SIZE
    {
        if (getIRYacc()->status.currentAfter)            
        {
            /*In after section defaultHeaderSize is calculated according to 
              the <format> section of the custom protocol*/
            CENode* eNode  = newENode(EINTCONST);
            if (getIRYacc()->status.currentAfter)            
                eNode->intval=getIRYacc()->calculateFormatSize(*getLineYacc());
            $$ = eNode; 
        }
        else 
        {
            /*in before section 
              defaultHeaderSize = nxtHdrOffset - prevProtoOffset*/
            CENode* prevProto = newObjectENode();
            getIRYacc()->createIRVariable("prevprotoOffset", prevProto->objref, 
                                  *getLineYacc());
            CENode* nxtHeader = newObjectENode();
            getIRYacc()->createIRVariable("NxtHdrOffset", nxtHeader->objref, 
                                  *getLineYacc());
            CENode* eNode  = new CENode(ESUB,     nxtHeader, prevProto);
            $$ = eNode;
        }
    }
   ;
    
_object: objectLegal  { $$->objref->name = parseString; }
 |       objectError  { throw CGenericErrorLine(ERR_UNRECOGNIZED_OBJ_DOT,
                                                *getLineYacc(), parseString);}
 |       ACCESS_ERROR { throw CGenericErrorLine(ERR_INVALID_INDEX, 
                                                *getLineYacc(), parseString);}
 |       FIELD_A      { throw CGenericErrorLine(ERR_ACCESS_BYTE_IN_FIELD,
                                                *getLineYacc(), parseString);}
    ;

objectError:
        SUB_VARIABLE_A {}
    |   SUB_VARIABLE   {}
    ;
    
objectLegal:
  FIELD
 {
    CENode* eNode = newObjectENode();
    getIRYacc()->createIRField(parseString,eNode->objref, *getLineYacc());
    $$ = eNode;
 }
| VARIABLE            
 {
    CENode* eNode = newObjectENode();
    getIRYacc()->createIRVariable(parseString, eNode->objref, 
                                  *getLineYacc());
    $$ = eNode;
 } 
 |	PROTOCOL_FIELD
 {
    CENode* eNode = newObjectENode();
    getIRYacc()->createIRProtocolField(parseString, eNode->objref,
                                       *getLineYacc());
    $$ = eNode;
 }  
 |	VARIABLE_A          
 {
    CENode* eNode = newObjectENode();
    getIRYacc()->createIRVariableAccess(parseString, eNode->objref, 
                                        *getLineYacc());
    $$ = eNode;
 } 
 |	PROTOCOL_FIELD_A    
 {
    CENode* eNode = newObjectENode();
    getIRYacc()->createIRFWAccess(parseString, eNode->objref, 
                                  *getLineYacc());
    $$ = eNode;
 }
    ;
    