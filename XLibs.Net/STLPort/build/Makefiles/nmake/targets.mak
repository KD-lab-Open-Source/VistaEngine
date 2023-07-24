# Time-stamp: <04/05/01 00:45:03 ptr>
# $Id: targets.mak,v 1.1.2.4 2005/06/01 05:39:54 ptr Exp $

# dependency output parser
#!include ${RULESBASE}/dparser-$(COMPILER_NAME).mak

# if sources disposed in several dirs, calculate
# appropriate rules; here is recursive call!

#DIRS_UNIQUE_SRC := $(dir $(SRC_CPP) $(SRC_CC) $(SRC_C) )
#DIRS_UNIQUE_SRC := $(sort $(DIRS_UNIQUE_SRC) )
#include ${RULESBASE}/dirsrc.mak
!include $(RULESBASE)/$(USE_MAKE)/rules-o.mak

#ALLBASE    := $(basename $(notdir $(SRC_CC) $(SRC_CPP) $(SRC_C)))
ALLBASE    = $(SRC_CC) $(SRC_CPP) $(SRC_C)
#ALLOBJS    := $(addsuffix .o,$(ALLBASE))

# follow tricks to avoid leading space if one of the macro undefined:
# SRC_CC, SRC_CPP or SRC_C
!ifdef SRC_CC
ALLOBJS    = $(SRC_CC:.cc=.o)
!endif
!ifdef SRC_CPP
!ifdef ALLOBJS
ALLOBJS = $(ALLOBJS) $(SRC_CPP:.cpp=.o)
!else
ALLOBJS = $(SRC_CPP:.cpp=.o)
!endif
!endif
!ifdef SRC_C
!ifdef ALLOBJS
ALLOBJS = $(ALLOBJS) $(SRC_C:.c=.o)
!else
ALLOBJS = $(SRC_C:.c=.o)
!endif
!endif

!ifdef SRC_RC
ALLRESS = $(SRC_RC:.rc=.res)
#ALLRESS = $(ALLRESS:../=)
!endif
# ALLOBJS = $(ALLOBJS:somedir/=)

!if EXIST( .\nmake-src-prefix.mak )
# Include strip of path to sources, i.e. macro like
#   ALLOBJS = $(ALLOBJS:..\..\..\..\..\..\explore/../extern/boost/libs/test/src/=)
#   ALLOBJS = $(ALLOBJS:../=)
#   ALLRESS = $(ALLRESS:../=)
# Be careful about path spelling!
# Pay attention the order of this macro! THE ORDER IS SIGNIFICANT!
!include .\nmake-src-prefix.mak
!endif

ALLDEPS    = $(SRC_CC:.cc=.d) $(SRC_CPP:.cpp=.d) $(SRC_C:.c=.d)

#!if [echo ALLOBJS -$(ALLOBJS)-]
#!endif
# Following trick intended to add prefix
# set marker (spaces are significant here!):
OBJ=$(ALLOBJS:.o =.o@)
#!if [echo OBJ 1 -$(OBJ)-]
#!endif
# remove trailing marker (with white space):
#OBJ=$(OBJ:.o@ =.o)
# remove unwanted space as result of line extending, like
# target: dep1.cpp dep2.cpp \
#         dep3.cpp
# (note, that if write '... dep2.cpp\', no white space happens)
OBJ=$(OBJ:.o@ =.o@)
#!if [echo OBJ 2 -$(OBJ)-]
#!endif
# replace marker by prefix:
#OBJ=$(OBJ:.o@=.o %OUTPUT_DIR%/)
# sorry, but I still not know how substitute macros in braces ();
!if "$(COMPILER_NAME)" == "evc4"
!if "$(TARGET_PROC)" == "arm"
OBJ=$(OBJ:.o@=.o obj\evc4-arm\shared\)
!elseif "$(TARGET_PROC)" == "x86"
OBJ=$(OBJ:.o@=.o obj\evc4-x86\shared\)
!elseif "$(TARGET_PROC)" == "mips"
OBJ=$(OBJ:.o@=.o obj\evc4-mips\shared\)
!elseif "$(TARGET_PROC)" == "sh3"
OBJ=$(OBJ:.o@=.o obj\evc4-sh3\shared\)
!elseif "$(TARGET_PROC)" == "sh4"
OBJ=$(OBJ:.o@=.o obj\evc4-sh4\shared\)
!else
!error No target processor configured!
!endif
!elseif "$(COMPILER_NAME)" == "evc3"
!if "$(TARGET_PROC)" == "arm"
OBJ=$(OBJ:.o@=.o obj\evc3-arm\shared\)
!elseif "$(TARGET_PROC)" == "x86"
OBJ=$(OBJ:.o@=.o obj\evc3-x86\shared\)
!elseif "$(TARGET_PROC)" == "mips"
OBJ=$(OBJ:.o@=.o obj\evc3-mips\shared\)
!elseif "$(TARGET_PROC)" == "sh3"
OBJ=$(OBJ:.o@=.o obj\evc3-sh3\shared\)
!elseif "$(TARGET_PROC)" == "sh4"
OBJ=$(OBJ:.o@=.o obj\evc3-sh4\shared\)
!else
!error No target processor configured!
!endif
!elseif "$(COMPILER_NAME)" == "vc6"
OBJ=$(OBJ:.o@=.o obj\vc6\shared\)
!elseif "$(COMPILER_NAME)" == "vc70"
OBJ=$(OBJ:.o@=.o obj\vc70\shared\)
!elseif "$(COMPILER_NAME)" == "vc71"
OBJ=$(OBJ:.o@=.o obj\vc71\shared\)
!elseif "$(COMPILER_NAME)" == "vc8"
OBJ=$(OBJ:.o@=.o obj\vc8\shared\)
!elseif "$(COMPILER_NAME)" == "icl"
OBJ=$(OBJ:.o@=.o obj\icl\shared\)
!endif
#!if [echo OBJ 3 -$(OBJ)-]
#!endif
# add prefix to first element:
OBJ=$(OUTPUT_DIR)\$(OBJ)
#!if [echo -$(OBJ)-]
#!endif

# The same trick for OBJ_DBG:
OBJ_DBG=$(ALLOBJS:.o =.o@)
#OBJ_DBG=$(OBJ_DBG:.o@ =.o)
OBJ_DBG=$(OBJ_DBG:.o@ =.o@)
#OBJ=$(OBJ:.o@=.o %OUTPUT_DIR%/)
# sorry, but I still not know how substitute macros in braces ();
!if "$(COMPILER_NAME)" == "evc4"
!if "$(TARGET_PROC)" == "arm"
OBJ_DBG=$(OBJ_DBG:.o@=.o obj\evc4-arm\shared-g\)
!elseif "$(TARGET_PROC)" == "x86"
OBJ_DBG=$(OBJ_DBG:.o@=.o obj\evc4-x86\shared-g\)
!elseif "$(TARGET_PROC)" == "mips"
OBJ_DBG=$(OBJ_DBG:.o@=.o obj\evc4-mips\shared-g\)
!elseif "$(TARGET_PROC)" == "sh3"
OBJ_DBG=$(OBJ_DBG:.o@=.o obj\evc4-sh3\shared-g\)
!elseif "$(TARGET_PROC)" == "sh4"
OBJ_DBG=$(OBJ_DBG:.o@=.o obj\evc4-sh4\shared-g\)
!else
!error No target processor configured!
!endif
!elseif "$(COMPILER_NAME)" == "evc3"
!if "$(TARGET_PROC)" == "arm"
OBJ_DBG=$(OBJ_DBG:.o@=.o obj\evc3-arm\shared-g\)
!elseif "$(TARGET_PROC)" == "x86"
OBJ_DBG=$(OBJ_DBG:.o@=.o obj\evc3-x86\shared-g\)
!elseif "$(TARGET_PROC)" == "mips"
OBJ_DBG=$(OBJ_DBG:.o@=.o obj\evc3-mips\shared-g\)
!elseif "$(TARGET_PROC)" == "sh3"
OBJ_DBG=$(OBJ_DBG:.o@=.o obj\evc3-sh3\shared-g\)
!elseif "$(TARGET_PROC)" == "sh4"
OBJ_DBG=$(OBJ_DBG:.o@=.o obj\evc3-sh4\shared-g\)
!else
!error No target processor configured!
!endif
!elseif "$(COMPILER_NAME)" == "vc6"
OBJ_DBG=$(OBJ_DBG:.o@=.o obj\vc6\shared-g\)
!elseif "$(COMPILER_NAME)" == "vc70"
OBJ_DBG=$(OBJ_DBG:.o@=.o obj\vc70\shared-g\)
!elseif "$(COMPILER_NAME)" == "vc71"
OBJ_DBG=$(OBJ_DBG:.o@=.o obj\vc71\shared-g\)
!elseif "$(COMPILER_NAME)" == "vc8"
OBJ_DBG=$(OBJ_DBG:.o@=.o obj\vc8\shared-g\)
!elseif "$(COMPILER_NAME)" == "icl"
OBJ_DBG=$(OBJ_DBG:.o@=.o obj\icl\shared-g\)
!endif
# add prefix to first element:
OBJ_DBG=$(OUTPUT_DIR_DBG)\$(OBJ_DBG)

# And for OBJ_STLDBG too:
OBJ_STLDBG=$(ALLOBJS:.o =.o@)
#OBJ_STLDBG=$(OBJ_STLDBG:.o@ =.o)
OBJ_STLDBG=$(OBJ_STLDBG:.o@ =.o@)
#OBJ=$(OBJ:.o@=.o %OUTPUT_DIR%/)
# sorry, but I still not know how substitute macros in braces ();
!if "$(COMPILER_NAME)" == "evc4"
!if "$(TARGET_PROC)" == "arm"
OBJ_STLDBG=$(OBJ_STLDBG:.o@=.o obj\evc4-arm\shared-stlg\)
!elseif "$(TARGET_PROC)" == "x86"
OBJ_STLDBG=$(OBJ_STLDBG:.o@=.o obj\evc4-x86\shared-stlg\)
!elseif "$(TARGET_PROC)" == "mips"
OBJ_STLDBG=$(OBJ_STLDBG:.o@=.o obj\evc4-mips\shared-stlg\)
!elseif "$(TARGET_PROC)" == "sh3"
OBJ_STLDBG=$(OBJ_STLDBG:.o@=.o obj\evc4-sh3\shared-stlg\)
!elseif "$(TARGET_PROC)" == "sh4"
OBJ_STLDBG=$(OBJ_STLDBG:.o@=.o obj\evc4-sh4\shared-stlg\)
!else
!error No target processor configured!
!endif
!elseif "$(COMPILER_NAME)" == "evc3"
!if "$(TARGET_PROC)" == "arm"
OBJ_STLDBG=$(OBJ_STLDBG:.o@=.o obj\evc3-arm\shared-stlg\)
!elseif "$(TARGET_PROC)" == "x86"
OBJ_STLDBG=$(OBJ_STLDBG:.o@=.o obj\evc3-x86\shared-stlg\)
!elseif "$(TARGET_PROC)" == "mips"
OBJ_STLDBG=$(OBJ_STLDBG:.o@=.o obj\evc3-mips\shared-stlg\)
!elseif "$(TARGET_PROC)" == "sh3"
OBJ_STLDBG=$(OBJ_STLDBG:.o@=.o obj\evc3-sh3\shared-stlg\)
!elseif "$(TARGET_PROC)" == "sh4"
OBJ_STLDBG=$(OBJ_STLDBG:.o@=.o obj\evc3-sh4\shared-stlg\)
!else
!error No target processor configured!
!endif
!elseif "$(COMPILER_NAME)" == "vc6"
OBJ_STLDBG=$(OBJ_STLDBG:.o@=.o obj\vc6\shared-stlg\)
!elseif "$(COMPILER_NAME)" == "vc70"
OBJ_STLDBG=$(OBJ_STLDBG:.o@=.o obj\vc70\shared-stlg\)
!elseif "$(COMPILER_NAME)" == "vc71"
OBJ_STLDBG=$(OBJ_STLDBG:.o@=.o obj\vc71\shared-stlg\)
!elseif "$(COMPILER_NAME)" == "vc8"
OBJ_STLDBG=$(OBJ_STLDBG:.o@=.o obj\vc8\shared-stlg\)
!elseif "$(COMPILER_NAME)" == "icl"
OBJ_STLDBG=$(OBJ_STLDBG:.o@=.o obj\icl\shared-stlg\)
!endif
# add prefix to first element:
OBJ_STLDBG=$(OUTPUT_DIR_STLDBG)\$(OBJ_STLDBG)

OBJ_A=$(ALLOBJS:.o =.o@)
OBJ_A=$(OBJ_A:.o@ =.o@)
!if "$(COMPILER_NAME)" == "evc4"
!if "$(TARGET_PROC)" == "arm"
OBJ_A=$(OBJ_A:.o@=.o obj\evc4-arm\static\)
!elseif "$(TARGET_PROC)" == "x86"
OBJ_A=$(OBJ_A:.o@=.o obj\evc4-x86\static\)
!elseif "$(TARGET_PROC)" == "mips"
OBJ_A=$(OBJ_A:.o@=.o obj\evc4-mips\static\)
!elseif "$(TARGET_PROC)" == "sh3"
OBJ_A=$(OBJ_A:.o@=.o obj\evc4-sh3\static\)
!elseif "$(TARGET_PROC)" == "sh4"
OBJ_A=$(OBJ_A:.o@=.o obj\evc4-sh4\static\)
!else
!error No target processor configured!
!endif
!elseif "$(COMPILER_NAME)" == "evc3"
!if "$(TARGET_PROC)" == "arm"
OBJ_A=$(OBJ_A:.o@=.o obj\evc3-arm\static\)
!elseif "$(TARGET_PROC)" == "x86"
OBJ_A=$(OBJ_A:.o@=.o obj\evc3-x86\static\)
!elseif "$(TARGET_PROC)" == "mips"
OBJ_A=$(OBJ_A:.o@=.o obj\evc3-mips\static\)
!elseif "$(TARGET_PROC)" == "sh3"
OBJ_A=$(OBJ_A:.o@=.o obj\evc3-sh3\static\)
!elseif "$(TARGET_PROC)" == "sh4"
OBJ_A=$(OBJ_A:.o@=.o obj\evc3-sh4\static\)
!else
!error No target processor configured!
!endif
!elseif "$(COMPILER_NAME)" == "vc6"
OBJ_A=$(OBJ_A:.o@=.o obj\vc6\static\)
!elseif "$(COMPILER_NAME)" == "vc70"
OBJ_A=$(OBJ_A:.o@=.o obj\vc70\static\)
!elseif "$(COMPILER_NAME)" == "vc71"
OBJ_A=$(OBJ_A:.o@=.o obj\vc71\static\)
!elseif "$(COMPILER_NAME)" == "vc8"
OBJ_A=$(OBJ_A:.o@=.o obj\vc8\static\)
!elseif "$(COMPILER_NAME)" == "icl"
OBJ_A=$(OBJ_A:.o@=.o obj\icl\static\)
!endif
OBJ_A=$(OUTPUT_DIR_A)\$(OBJ_A)

OBJ_A_DBG=$(ALLOBJS:.o =.o@)
OBJ_A_DBG=$(OBJ_A_DBG:.o@ =.o@)
!if "$(COMPILER_NAME)" == "evc4"
!if "$(TARGET_PROC)" == "arm"
OBJ_A_DBG=$(OBJ_A_DBG:.o@=.o obj\evc4-arm\static-g\)
!elseif "$(TARGET_PROC)" == "x86"
OBJ_A_DBG=$(OBJ_A_DBG:.o@=.o obj\evc4-x86\static-g\)
!elseif "$(TARGET_PROC)" == "mips"
OBJ_A_DBG=$(OBJ_A_DBG:.o@=.o obj\evc4-mips\static-g\)
!elseif "$(TARGET_PROC)" == "sh3"
OBJ_A_DBG=$(OBJ_A_DBG:.o@=.o obj\evc4-sh3\static-g\)
!elseif "$(TARGET_PROC)" == "sh4"
OBJ_A_DBG=$(OBJ_A_DBG:.o@=.o obj\evc4-sh4\static-g\)
!else
!error No target processor configured!
!endif
!elseif "$(COMPILER_NAME)" == "evc3"
!if "$(TARGET_PROC)" == "arm"
OBJ_A_DBG=$(OBJ_A_DBG:.o@=.o obj\evc3-arm\static-g\)
!elseif "$(TARGET_PROC)" == "x86"
OBJ_A_DBG=$(OBJ_A_DBG:.o@=.o obj\evc3-x86\static-g\)
!elseif "$(TARGET_PROC)" == "mips"
OBJ_A_DBG=$(OBJ_A_DBG:.o@=.o obj\evc3-mips\static-g\)
!elseif "$(TARGET_PROC)" == "sh3"
OBJ_A_DBG=$(OBJ_A_DBG:.o@=.o obj\evc3-sh3\static-g\)
!elseif "$(TARGET_PROC)" == "sh4"
OBJ_A_DBG=$(OBJ_A_DBG:.o@=.o obj\evc3-sh4\static-g\)
!else
!error No target processor configured!
!endif
!elseif "$(COMPILER_NAME)" == "vc6"
OBJ_A_DBG=$(OBJ_A_DBG:.o@=.o obj\vc6\static-g\)
!elseif "$(COMPILER_NAME)" == "vc70"
OBJ_A_DBG=$(OBJ_A_DBG:.o@=.o obj\vc70\static-g\)
!elseif "$(COMPILER_NAME)" == "vc71"
OBJ_A_DBG=$(OBJ_A_DBG:.o@=.o obj\vc71\static-g\)
!elseif "$(COMPILER_NAME)" == "vc8"
OBJ_A_DBG=$(OBJ_A_DBG:.o@=.o obj\vc8\static-g\)
!elseif "$(COMPILER_NAME)" == "icl"
OBJ_A_DBG=$(OBJ_A_DBG:.o@=.o obj\icl\static-g\)
!endif
OBJ_A_DBG=$(OUTPUT_DIR_A_DBG)\$(OBJ_A_DBG)

OBJ_A_STLDBG=$(ALLOBJS:.o =.o@)
OBJ_A_STLDBG=$(OBJ_A_STLDBG:.o@ =.o@)
!if "$(COMPILER_NAME)" == "evc4"
!if "$(TARGET_PROC)" == "arm"
OBJ_A_STLDBG=$(OBJ_A_STLDBG:.o@=.o obj\evc4-arm\static-stlg\)
!elseif "$(TARGET_PROC)" == "x86"
OBJ_A_STLDBG=$(OBJ_A_STLDBG:.o@=.o obj\evc4-x86\static-stlg\)
!elseif "$(TARGET_PROC)" == "mips"
OBJ_A_STLDBG=$(OBJ_A_STLDBG:.o@=.o obj\evc4-mips\static-stlg\)
!elseif "$(TARGET_PROC)" == "sh3"
OBJ_A_STLDBG=$(OBJ_A_STLDBG:.o@=.o obj\evc4-sh3\static-stlg\)
!elseif "$(TARGET_PROC)" == "sh4"
OBJ_A_STLDBG=$(OBJ_A_STLDBG:.o@=.o obj\evc4-sh4\static-stlg\)
!else
!error No target processor configured!
!endif
!elseif "$(COMPILER_NAME)" == "evc3"
!if "$(TARGET_PROC)" == "arm"
OBJ_A_STLDBG=$(OBJ_A_STLDBG:.o@=.o obj\evc3-arm\static-stlg\)
!elseif "$(TARGET_PROC)" == "x86"
OBJ_A_STLDBG=$(OBJ_A_STLDBG:.o@=.o obj\evc3-x86\static-stlg\)
!elseif "$(TARGET_PROC)" == "mips"
OBJ_A_STLDBG=$(OBJ_A_STLDBG:.o@=.o obj\evc3-mips\static-stlg\)
!elseif "$(TARGET_PROC)" == "sh3"
OBJ_A_STLDBG=$(OBJ_A_STLDBG:.o@=.o obj\evc3-sh3\static-stlg\)
!elseif "$(TARGET_PROC)" == "sh4"
OBJ_A_STLDBG=$(OBJ_A_STLDBG:.o@=.o obj\evc3-sh4\static-stlg\)
!else
!error No target processor configured!
!endif
!elseif "$(COMPILER_NAME)" == "vc6"
OBJ_A_STLDBG=$(OBJ_A_STLDBG:.o@=.o obj\vc6\static-stlg\)
!elseif "$(COMPILER_NAME)" == "vc70"
OBJ_A_STLDBG=$(OBJ_A_STLDBG:.o@=.o obj\vc70\static-stlg\)
!elseif "$(COMPILER_NAME)" == "vc71"
OBJ_A_STLDBG=$(OBJ_A_STLDBG:.o@=.o obj\vc71\static-stlg\)
!elseif "$(COMPILER_NAME)" == "vc8"
OBJ_A_STLDBG=$(OBJ_A_STLDBG:.o@=.o obj\vc8\static-stlg\)
!elseif "$(COMPILER_NAME)" == "icl"
OBJ_A_STLDBG=$(OBJ_A_STLDBG:.o@=.o obj\icl\static-stlg\)
!endif
OBJ_A_STLDBG=$(OUTPUT_DIR_A_STLDBG)\$(OBJ_A_STLDBG)

!ifdef ALLRESS
RES=$(ALLRESS:.res =.res@)
RES=$(RES:.res@ =.res@)
!if "$(COMPILER_NAME)" == "evc4"
!if "$(TARGET_PROC)" == "arm"
RES=$(RES:.res@=.res obj\evc4-arm\shared\)
!elseif "$(TARGET_PROC)" == "x86"
RES=$(RES:.res@=.res obj\evc4-x86\shared\)
!elseif "$(TARGET_PROC)" == "mips"
RES=$(RES:.res@=.res obj\evc4-mips\shared\)
!elseif "$(TARGET_PROC)" == "sh3"
RES=$(RES:.res@=.res obj\evc4-sh3\shared\)
!elseif "$(TARGET_PROC)" == "sh4"
RES=$(RES:.res@=.res obj\evc4-sh4\shared\)
!else
!error No target processor configured!
!endif
!elseif "$(COMPILER_NAME)" == "evc3"
!if "$(TARGET_PROC)" == "arm"
RES=$(RES:.res@=.res obj\evc3-arm\shared\)
!elseif "$(TARGET_PROC)" == "x86"
RES=$(RES:.res@=.res obj\evc3-x86\shared\)
!elseif "$(TARGET_PROC)" == "mips"
RES=$(RES:.res@=.res obj\evc3-mips\shared\)
!elseif "$(TARGET_PROC)" == "sh3"
RES=$(RES:.res@=.res obj\evc3-sh3\shared\)
!elseif "$(TARGET_PROC)" == "sh4"
RES=$(RES:.res@=.res obj\evc3-sh4\shared\)
!else
!error No target processor configured!
!endif
!elseif "$(COMPILER_NAME)" == "vc6"
RES=$(RES:.res@=.res obj\vc6\shared\)
!elseif "$(COMPILER_NAME)" == "vc70"
RES=$(RES:.res@=.res obj\vc70\shared\)
!elseif "$(COMPILER_NAME)" == "vc71"
RES=$(RES:.res@=.res obj\vc71\shared\)
!elseif "$(COMPILER_NAME)" == "vc8"
RES=$(RES:.res@=.res obj\vc8\shared\)
!elseif "$(COMPILER_NAME)" == "icl"
RES=$(RES:.res@=.res obj\icl\shared\)
!endif
RES=$(OUTPUT_DIR)\$(RES)

RES_DBG=$(ALLRESS:.res =.res@)
RES_DBG=$(RES_DBG:.res@ =.res@)
!if "$(COMPILER_NAME)" == "evc4"
!if "$(TARGET_PROC)" == "arm"
RES_DBG=$(RES_DBG:.res@=.res obj\evc4-arm\shared-g\)
!elseif "$(TARGET_PROC)" == "x86"
RES_DBG=$(RES_DBG:.res@=.res obj\evc4-x86\shared-g\)
!elseif "$(TARGET_PROC)" == "mips"
RES_DBG=$(RES_DBG:.res@=.res obj\evc4-mips\shared-g\)
!elseif "$(TARGET_PROC)" == "sh3"
RES_DBG=$(RES_DBG:.res@=.res obj\evc4-sh3\shared-g\)
!elseif "$(TARGET_PROC)" == "sh4"
RES_DBG=$(RES_DBG:.res@=.res obj\evc4-sh4\shared-g\)
!else
!error No target processor configured!
!endif
!elseif "$(COMPILER_NAME)" == "evc3"
!if "$(TARGET_PROC)" == "arm"
RES_DBG=$(RES_DBG:.res@=.res obj\evc3-arm\shared-g\)
!elseif "$(TARGET_PROC)" == "x86"
RES_DBG=$(RES_DBG:.res@=.res obj\evc3-x86\shared-g\)
!elseif "$(TARGET_PROC)" == "mips"
RES_DBG=$(RES_DBG:.res@=.res obj\evc3-mips\shared-g\)
!elseif "$(TARGET_PROC)" == "sh3"
RES_DBG=$(RES_DBG:.res@=.res obj\evc3-sh3\shared-g\)
!elseif "$(TARGET_PROC)" == "sh4"
RES_DBG=$(RES_DBG:.res@=.res obj\evc3-sh4\shared-g\)
!else
!error No target processor configured!
!endif
!elseif "$(COMPILER_NAME)" == "vc6"
RES_DBG=$(RES_DBG:.res@=.res obj\vc6\shared-g\)
!elseif "$(COMPILER_NAME)" == "vc70"
RES_DBG=$(RES_DBG:.res@=.res obj\vc70\shared-g\)
!elseif "$(COMPILER_NAME)" == "vc71"
RES_DBG=$(RES_DBG:.res@=.res obj\vc71\shared-g\)
!elseif "$(COMPILER_NAME)" == "vc8"
RES_DBG=$(RES_DBG:.res@=.res obj\vc8\shared-g\)
!elseif "$(COMPILER_NAME)" == "icl"
RES_DBG=$(RES_DBG:.res@=.res obj\icl\shared-g\)
!endif
RES_DBG=$(OUTPUT_DIR_DBG)\$(RES_DBG)

RES_STLDBG=$(ALLRESS:.res =.res@)
RES_STLDBG=$(RES_STLDBG:.res@ =.res@)
!if "$(COMPILER_NAME)" == "evc4"
!if "$(TARGET_PROC)" == "arm"
RES_STLDBG=$(RES_STLDBG:.res@=.res obj\evc4-arm\shared-stlg\)
!elseif "$(TARGET_PROC)" == "x86"
RES_STLDBG=$(RES_STLDBG:.res@=.res obj\evc4-x86\shared-stlg\)
!elseif "$(TARGET_PROC)" == "mips"
RES_STLDBG=$(RES_STLDBG:.res@=.res obj\evc4-mips\shared-stlg\)
!elseif "$(TARGET_PROC)" == "sh3"
RES_STLDBG=$(RES_STLDBG:.res@=.res obj\evc4-sh3\shared-stlg\)
!elseif "$(TARGET_PROC)" == "sh4"
RES_STLDBG=$(RES_STLDBG:.res@=.res obj\evc4-sh4\shared-stlg\)
!else
!error No target processor configured!
!endif
!elseif "$(COMPILER_NAME)" == "evc3"
!if "$(TARGET_PROC)" == "arm"
RES_STLDBG=$(RES_STLDBG:.res@=.res obj\evc3-arm\shared-stlg\)
!elseif "$(TARGET_PROC)" == "x86"
RES_STLDBG=$(RES_STLDBG:.res@=.res obj\evc3-x86\shared-stlg\)
!elseif "$(TARGET_PROC)" == "mips"
RES_STLDBG=$(RES_STLDBG:.res@=.res obj\evc3-mips\shared-stlg\)
!elseif "$(TARGET_PROC)" == "sh3"
RES_STLDBG=$(RES_STLDBG:.res@=.res obj\evc3-sh3\shared-stlg\)
!elseif "$(TARGET_PROC)" == "sh4"
RES_STLDBG=$(RES_STLDBG:.res@=.res obj\evc3-sh4\shared-stlg\)
!else
!error No target processor configured!
!endif
!elseif "$(COMPILER_NAME)" == "vc6"
RES_STLDBG=$(RES_STLDBG:.res@=.res obj\vc6\shared-stlg\)
!elseif "$(COMPILER_NAME)" == "vc70"
RES_STLDBG=$(RES_STLDBG:.res@=.res obj\vc70\shared-stlg\)
!elseif "$(COMPILER_NAME)" == "vc71"
RES_STLDBG=$(RES_STLDBG:.res@=.res obj\vc71\shared-stlg\)
!elseif "$(COMPILER_NAME)" == "vc8"
RES_STLDBG=$(RES_STLDBG:.res@=.res obj\vc8\shared-stlg\)
!elseif "$(COMPILER_NAME)" == "icl"
RES_STLDBG=$(RES_STLDBG:.res@=.res obj\icl\shared-stlg\)
!endif
RES_STLDBG=$(OUTPUT_DIR_STLDBG)\$(RES_STLDBG)
!endif
