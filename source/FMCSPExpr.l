/* =====================================================================
 *
 *  Copyright 2009,2010, Freescale Semiconductor, Inc., All Rights Reserved. 
 *
 *  This file contains copyrighted material. Use of this file is restricted
 *  by the provisions of a Freescale Software License Agreement, which has
 *  either accompanied the delivery of this software in shrink wrap
 *  form or been expressly executed between the parties.
 *
 *  File Name : FMCSPExpr.l
 *  Author    : Levy Ro'ee
 *
 * ===================================================================*/

%{

#include <string>
#include "FMCSPIR.h"
#include "FMCSPExpr.hpp"

#ifdef _MSC_VER
#pragma warning(disable : 4996)
#pragma warning(disable : 4003)
#pragma warning(disable : 4018)
#endif

char parseString[100];

%}

%option noyywrap
%option stack 
%option never-interactive
 /*%option debug*/

access		    "["[0-9]*:[0-9]*"]"		
accessErr	    "["[^:]*:[^\]]*"]"		

%x ERROR

%% 

\(              return OPEN_PARENTHESIS;
\)              return CLOSE_PARENTHESIS;
\+              return PLUS;
\-              return MINUS;
\[              return OPEN_BRACKET;
\]              return CLOSE_BRACKET;
addc            return ADD_CARRY;
bitwxor         return XOR;
bitwor          return BIT_OR;
bitwand         return BIT_AND;
bitwnot         return BIT_NOT;
or              return OR;
and             return AND;
not             return NOT;
==				return EQU;
!=				return NOTEQU;
lt				return LESS;
le				return LESSEQU;
gt				return GREATER;
ge				return GREATEREQU;
shr 			return SHR;
shl 			return SHL;
concat          return CONCAT;
checksum        return CHECKSUM;
,               return COMMA;
$[dD][eE][fF][aA][uU][lL][tT][hH][eE][aA][dD][eE][rR][sS][iI][zZ][eE]      return DEFAULT_HEADER_SIZE;
$[hH][eE][aA][dD][eE][rR][sS][iI][zZ][eE]       return HEADERSIZE;
[0-9]+			{ strcpy (parseString, yytext); return DEC;	}
0x[0-9a-fA-F]+	{ strcpy (parseString, yytext); return HEX; }
0b[01]+			{ strcpy (parseString, yytext); return BIN; }
$[0-9a-zA-Z_]+\.[0-9a-zA-Z_]+ { strcpy (parseString, yytext); 
                                return SUB_VARIABLE;}
$[0-9a-zA-Z_]+\.[0-9a-zA-Z_]+{access} { strcpy (parseString, yytext); 
                                        return SUB_VARIABLE_A;}                                    
$[fF][wW]+{access}            { strcpy (parseString, yytext);
                                return PROTOCOL_FIELD_A; }
$[fF][wW]+{accessErr}         { strcpy (parseString, yytext);
                                return ACCESS_ERROR; }
$[0-9a-zA-Z_]+                { strcpy (parseString, yytext);   
                                return VARIABLE;}
$[0-9a-zA-Z_]+{access}        { strcpy (parseString, yytext);   
                                return VARIABLE_A;}
$[0-9a-zA-Z_]+{accessErr}     { strcpy (parseString, yytext);   
                                return ACCESS_ERROR;}
[0-9a-zA-Z_]+                 { strcpy (parseString, yytext);   
                                return FIELD;}
[0-9a-zA-Z_]+{access}         { strcpy (parseString, yytext);   
                                return FIELD_A;}
#[0-9a-zA-Z_]+                { strcpy (parseString, yytext); 
                                return PROTOCOL; } 
[0-9a-zA-Z_]+\.[0-9a-zA-Z_]+  { strcpy (parseString, yytext); 
                                return PROTOCOL_FIELD; }
[ \t\r\n]+			              /* ignore whitespace */;
.                               yyless(0); BEGIN ERROR;
<ERROR>[^ \t\r\n]*            { throw CGenericErrorLine(ERR_UNRECOGNIZED_LEXER,
                                    *getLineYacc(), yytext);}

