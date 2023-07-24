# -*- makefile -*- Time-stamp: <03/10/17 14:09:57 ptr>
# $Id: vc-common.mak,v 1.1.2.3 2005/04/07 20:44:26 dums Exp $


# Oh, the commented below work for gmake 3.78.1 and above,
# but phrase without tag not work for it. Since gmake 3.79 
# tag with assignment fail, but work assignment for all tags
# (really that more correct).

!ifndef LDLIBS
LDLIBS =
!endif

#Per default MSVC vcvars32.bat script set the LIB environment
#variable to get the native library, there is no need to add
#them here
#LDSEARCH = $(LDSEARCH) /LIBPATH:"$(MSVC_LIB_DIR)"

LDFLAGS_REL = $(LDFLAGS_REL) /DLL $(LDSEARCH)
LDFLAGS_DBG = $(LDFLAGS_DBG) /DLL $(LDSEARCH)
LDFLAGS_STLDBG = $(LDFLAGS_STLDBG) /DLL $(LDSEARCH)
# LDFLAGS_STATIC = $(LDSEARCH)

LDFLAGS_REL = $(LDFLAGS_REL) /VERSION:$(MAJOR).$(MINOR)
LDFLAGS_DBG = $(LDFLAGS_DBG) /VERSION:$(MAJOR).$(MINOR)
LDFLAGS_STLDBG = $(LDFLAGS_STLDBG) /VERSION:$(MAJOR).$(MINOR)
