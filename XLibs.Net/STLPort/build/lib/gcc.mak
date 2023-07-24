# -*- Makefile -*- Time-stamp: <03/10/12 20:35:49 ptr>

SRCROOT := ..
COMPILER_NAME := gcc

STLPORT_INCLUDE_DIR = ../../stlport
include Makefile.inc
include ${SRCROOT}/Makefiles/top.mak

ifeq ($(OSNAME),linux)
DEFS += -D_STLP_REAL_LOCALE_IMPLEMENTED -D_GNU_SOURCE
endif

# options for build with boost support
ifdef STLP_BUILD_BOOST_PATH
INCLUDES += -I$(STLP_BUILD_BOOST_PATH)
endif

