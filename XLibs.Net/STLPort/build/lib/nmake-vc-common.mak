# -*- Makefile -*- Time-stamp: <03/10/26 17:39:09 ptr>
# $Id: nmake-vc-common.mak,v 1.1.2.6 2005/04/07 20:41:35 dums Exp $

SRCROOT=..

STLPORT_INCLUDE_DIR = ../../stlport
!include Makefile.inc

INCLUDES=$(INCLUDES) /I$(STLPORT_INCLUDE_DIR) /FI vc_warning_disable.h

!ifdef STLP_BUILD_BOOST_PATH
OPT_DBG = /Zm800
INCLUDES=$(INCLUDES) /I "$(STLP_BUILD_BOOST_PATH)"
!endif

RC_FLAGS_REL = /I$(STLPORT_INCLUDE_DIR) /D "COMP=$(COMPILER_NAME)"
RC_FLAGS_DBG = /I$(STLPORT_INCLUDE_DIR) /D "COMP=$(COMPILER_NAME)"
RC_FLAGS_STLDBG = /I$(STLPORT_INCLUDE_DIR) /D "COMP=$(COMPILER_NAME)"
OPT_STLDBG = /Zm800

!include $(SRCROOT)/Makefiles/nmake/top.mak
