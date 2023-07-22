# -*- Makefile -*- Time-stamp: <03/10/12 20:35:49 ptr>
# $Id: aCC.mak,v 1.1.2.1 2005/05/14 08:58:25 ptr Exp $

SRCROOT := ..
COMPILER_NAME := aCC

STLPORT_INCLUDE_DIR = ../../stlport
include Makefile.inc
include ${SRCROOT}/Makefiles/top.mak


INCLUDES += -I$(STLPORT_INCLUDE_DIR)

# options for build with boost support
ifdef STLP_BUILD_BOOST_PATH
INCLUDES += -I$(STLP_BUILD_BOOST_PATH)
endif

