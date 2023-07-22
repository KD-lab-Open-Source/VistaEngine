# -*- makefile -*- Time-stamp: <04/03/16 17:23:52 ptr>
# $Id: rules-install-a.mak,v 1.1.2.2 2005/11/01 07:17:21 complement Exp $

PHONY += install-release-static install-dbg-static install-stldbg-static

install-static: install-release-static install-dbg-static install-stldbg-static

install-release-static:	release-static
	@if [ ! -d $(INSTALL_LIB_DIR) ] ; then \
	  mkdir -p $(INSTALL_LIB_DIR) ; \
	fi
	$(INSTALL_A) ${A_NAME_OUT} $(INSTALL_LIB_DIR)

install-dbg-static:	dbg-static
	@if [ ! -d $(INSTALL_LIB_DIR_DBG) ] ; then \
	  mkdir -p $(INSTALL_LIB_DIR_DBG) ; \
	fi
	$(INSTALL_A) ${A_NAME_OUT_DBG} $(INSTALL_LIB_DIR_DBG)

install-stldbg-static:	stldbg-static
	@if [ ! -d $(INSTALL_LIB_DIR_STLDBG) ] ; then \
	  mkdir -p $(INSTALL_LIB_DIR_STLDBG) ; \
	fi
	$(INSTALL_A) ${A_NAME_OUT_STLDBG} $(INSTALL_LIB_DIR_STLDBG)
