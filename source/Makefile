#==========================================================================
#
#  Copyright 2009-2012 Freescale Semiconductor, Inc., All Rights Reserved.
#
#  This file contains copyrighted material. Use of this file is restricted
#  by the provisions of a Freescale Software License Agreement, which has
#  either accompanied the delivery of this software in shrink wrap
#  form or been expressly executed between the parties.

#==========================================================================
# For host mode only compilation, set FMCHOSTMODE variable. For example:
#     make FMCHOSTMODE=1
#
# Also, use variables below in accordance to your environment

FMD_USPACE_HEADER_PATH ?= /usr/include/fmd
FMD_USPACE_LIB_PATH    ?= /usr/lib
LIBXML2_HEADER_PATH    ?= /usr/include/libxml2
TCLAP_HEADER_PATH      ?= /usr/include

PKG_NAME = fmc-0.9.24
PKG_RELEASE = 1

CFLAGS += -fPIC

ifeq ($(MACHINE),p1023rds)
  CFLAGS += -DP1023
else
  ifeq ($(MACHINE),B4860)
    CFLAGS += -DB4860
  else
    CFLAGS += -DP4080
  endif
endif

ifdef 64BIT5020
CFLAGS += -m64
endif
CFLAGS += -DNCSW_LINUX -O2 -D__STDC_LIMIT_MACROS\
          -I$(TCLAP_HEADER_PATH) \
          -I$(LIBXML2_HEADER_PATH) \
          -I$(FMD_USPACE_HEADER_PATH) \
          -I$(FMD_USPACE_HEADER_PATH)/etc \
          -I$(FMD_USPACE_HEADER_PATH)/Peripherals \
          -I$(FMD_USPACE_HEADER_PATH)/integrations
CXXFLAGS = $(CFLAGS)
LDLIBS   = -lfmc -lfm -lxml2 -lm

ifdef FMCHOSTMODE
LDFLAGS += -L.
else
LDFLAGS += -L. -L$(FMD_USPACE_LIB_PATH)
endif
ifdef 64BIT5020
LDFLAGS += -m64
endif

.PHONY: default
ifdef FMCHOSTMODE
default: libfm.a libfmc.a fmc
else
default: libfmc.a fmc
endif

objects = ./libfmc.o ./fmc_exec.o                                       \
     ./FMCCFGReader.o ./FMCPDLReader.o ./FMCPCDReader.o                 \
     ./FMCSPCreateCode.o ./FMCSPDumpCode.o ./FMCSPDumpIR.o              \
     ./FMCSPDumpParse.o ./FMCSPIR.o ./FMCPDLReader.o                    \
     ./FMCSPReviseCode.o ./FMCSPReviseIR.o ./FMCSP.o                    \
     ./FMCSPExpr.o ./FMCSPExprLexer.o                                   \
     ./FMCPCDModel.o ./FMCCModelOutput.o                                \
     ./FMCTaskDef.o ./FMCUtils.o ./FMCGenericError.o                    \
     ./spa/dll.o ./spa/fm_sp.o ./spa/fm_sp_assembler.tab.o              \
     ./spa/fm_sp_private.o ./spa/htbl.o ./spa/lex._fmsp_assembler_yy.o

headers = fmc.h FMCCFGReader.h FMCCModelOutput.h FMCGenericError.h \
          FMCPCDModel.h FMCPCDReader.h FMCPDLReader.h FMCSP.h      \
          FMCSPCreateCode.h FMCSPIR.h FMCTaskDef.h FMCUtils.h      \
          FMCSPExpr.hpp logger.hpp 

other_files = ./fmc ./fmc_config ./fmc_config.c ./fmc_config.o ./softparse.h \
	          ./libfm.a ./libfmc.a ./FMCDummyDriver.o ./FMC.o

fmc : ./FMC.o ./FMCUtils.o ./FMCGenericError.o libfmc.a
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(objects) : $(headers)

libfmc.a : libfmc.a($(objects))

fmc_config : fmc_config.o libfmc.a libfm.a

libfm.a : libfm.a(FMCDummyDriver.o)

.PHONY: clean
clean:
	rm -f $(other_files) $(objects)

.PHONY: dist
dist: clean
	mkdir -p $(PKG_NAME);
	mkdir -p $(PKG_NAME)/source
	mkdir -p $(PKG_NAME)/source/spa;
	mkdir -p $(PKG_NAME)/etc
	cp *.c *.cpp *.h *.hpp Makefile    $(PKG_NAME)/source
	cp spa/*.c spa/*.h                 $(PKG_NAME)/source/spa
	cp -r ../etc/*                     $(PKG_NAME)/etc
	cp ../INSTALL ../COPYING ../README $(PKG_NAME)
	find $(PKG_NAME) -name  CVS   -print0 | xargs -0 /bin/rm -R -f
	find $(PKG_NAME) -name .svn   -print0 | xargs -0 /bin/rm -R -f
	find $(PKG_NAME) -name \*.o   -print0 | xargs -0 /bin/rm -R -f
	find $(PKG_NAME) -name \*.bak -print0 | xargs -0 /bin/rm -R -f
	find $(PKG_NAME) -name \*.org -print0 | xargs -0 /bin/rm -R -f
	tar -czf $(PKG_NAME)-$(PKG_RELEASE).tgz $(PKG_NAME)
	rm -Rf $(PKG_NAME)
