# -*- Makefile -*- Time-stamp: <05/03/10 17:51:53 ptr>
# $Id: aCC.mak,v 1.1.2.2 2005/09/26 19:34:19 dums Exp $

SRCROOT := ../..
COMPILER_NAME := aCC

ALL_TAGS := release-shared dbg-shared stldbg-shared
STLPORT_DIR := ../../..
include Makefile.inc
include ${SRCROOT}/Makefiles/top.mak

INCLUDES += -I${STLPORT_INCLUDE_DIR}
DEFS += -D_STLP_NO_CUSTOM_IO

dbg-shared:	DEFS += -D_STLP_DEBUG_UNINITIALIZED 
stldbg-shared:	DEFS += -D_STLP_DEBUG_UNINITIALIZED 

ifdef STLP_BUILD_BOOST_PATH
INCLUDES += -I${STLP_BUILD_BOOST_PATH}
endif

release-shared:	LDSEARCH = -L${STLPORT_LIB_DIR}
dbg-shared:	LDSEARCH = -L${STLPORT_LIB_DIR}
stldbg-shared:	LDSEARCH = -L${STLPORT_LIB_DIR}
