# =====================================================================
#
#  Copyright 2009, 2010, Freescale Semiconductor, Inc., All Rights Reserved. 
#
#  This file contains copyrighted material. Use of this file is restricted
#  by the provisions of a Freescale Software License Agreement, which has
#  either accompanied the delivery of this software in shrink wrap
#  form or been expressly executed between the parties.

# Makefile for Soft Parse Assembler.
#
# This file creates both the stand alone compiler (fm_spa) and
# the assembler library (fmsp.a).
#

YACC  := bison
YARGS := -d
LEX   := flex

all: fmsp fm_spa

%.tab.c %.tab.h: %.y
	$(YACC) $(YARGS) $<

lex._fmsp_assembler_yy.c: fm_sp_assembler.lex
	$(LEX) -P_fmsp_assembler_yy fm_sp_assembler.lex

.c.o:
	cc -g -c -m32 -o $@ $<

fmsp: fm_sp_assembler.tab.o lex._fmsp_assembler_yy.o dll.o htbl.o  fm_sp_private.o fm_sp.o
	ar -crs fmsp.a fm_sp_assembler.tab.o lex._fmsp_assembler_yy.o dll.o htbl.o  fm_sp_private.o fm_sp.o

fm_spa: fm_spa.o fmsp.a
	cc -g -m32 -o $@ $^

cleanup:
	rm -f *.o lex.*.c *.tab.h *.tab.c 

clean:
	rm -f fm_spa __assembled__* fmsp.a
