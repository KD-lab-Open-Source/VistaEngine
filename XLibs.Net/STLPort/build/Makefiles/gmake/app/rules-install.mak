# -*- makefile -*- Time-stamp: <04/07/25 17:58:50 ptr>
# $Id: rules-install.mak,v 1.1.2.3 2005/11/01 07:16:44 complement Exp $

install:	install-shared

# The program name to be installed will be the same as compiled name,
# but it will be a bit altered in case of installation debug and/or
# stlport-debug program in the same catalog as 'release' program.

INSTALL_PRGNAME := ${PRGNAME}${EXE}

ifeq (${INSTALL_BIN_DIR},${INSTALL_BIN_DIR_DBG})
INSTALL_PRGNAME_DBG := ${PRGNAME}g${EXE}
else
INSTALL_PRGNAME_DBG := ${INSTALL_PRGNAME}
endif

ifeq (${INSTALL_BIN_DIR},${INSTALL_BIN_DIR_STLDBG})
INSTALL_PRGNAME_STLDBG := ${PRGNAME}stlg${EXE}
else
INSTALL_PRGNAME_STLDBG := ${INSTALL_PRGNAME}
endif

ifeq (${INSTALL_BIN_DIR_DBG},${INSTALL_BIN_DIR_STLDBG})
INSTALL_PRGNAME_DBG := ${PRGNAME}g${EXE}
INSTALL_PRGNAME_STLDBG := ${PRGNAME}stlg${EXE}
endif

install-shared: install-release-shared install-dbg-shared install-stldbg-shared

ifneq ($(OSNAME),windows)
install-release-shared: release-shared
	@if [ ! -d $(INSTALL_BIN_DIR) ] ; then \
	  mkdir -p $(INSTALL_BIN_DIR) ; \
	fi
	$(INSTALL_EXE) ${PRG} $(INSTALL_BIN_DIR)/${INSTALL_PRGNAME}

install-dbg-shared: dbg-shared
	@if [ ! -d $(INSTALL_BIN_DIR_DBG) ] ; then \
	  mkdir -p $(INSTALL_BIN_DIR_DBG) ; \
	fi
	$(INSTALL_EXE) ${PRG_DBG} $(INSTALL_BIN_DIR_DBG)/${INSTALL_PRGNAME_DBG}

install-stldbg-shared: stldbg-shared
	@if [ ! -d $(INSTALL_BIN_DIR_STLDBG) ] ; then \
	  mkdir -p $(INSTALL_BIN_DIR_STLDBG) ; \
	fi
	$(INSTALL_EXE) ${PRG_STLDBG} $(INSTALL_BIN_DIR_STLDBG)/${INSTALL_PRGNAME_STLDBG}
else
install-release-shared: release-shared
	$(INSTALL_EXE) $(subst /,\,$(PRG)) $(subst /,\,$(INSTALL_BIN_DIR)/$(INSTALL_PRGNAME))

install-dbg-shared: dbg-shared
	$(INSTALL_EXE) $(subst /,\,$(PRG_DBG)) $(subst /,\,$(INSTALL_BIN_DIR_DBG)/$(INSTALL_PRGNAME_DBG))

install-stldbg-shared: stldbg-shared
	$(INSTALL_EXE) $(subst /,\,$(PRG_STLDBG)) $(subst /,\,$(INSTALL_BIN_DIR_STLDBG)/$(INSTALL_PRGNAME_STLDBG))
endif
