# -*- makefile -*- Time-stamp: <03/07/15 18:26:22 ptr>
# $Id: rules-install-so.mak,v 1.1.2.1 2005/09/19 19:53:48 dums Exp $

INSTALL_TAGS ?= install-release-shared install-dbg-shared install-stldbg-shared

PHONY += install $(INSTALL_TAGS)

install:	$(INSTALL_TAGS)

install-static: all-static install-release-static install-dbg-static install-stldbg-static
install-shared: all-shared install-release-shared install-dbg-shared install-stldbg-shared

install-release-shared:	release-shared
	@if not exist $(subst /,\,$(INSTALL_BIN_DIR)/) mkdir $(subst /,\,$(INSTALL_BIN_DIR)/)
	@if not exist $(subst /,\,$(INSTALL_LIB_DIR)/) mkdir $(subst /,\,$(INSTALL_LIB_DIR)/)
	$(INSTALL_SO) $(subst /,\,$(SO_NAME_OUT)) $(subst /,\,$(INSTALL_BIN_DIR)/)
	$(INSTALL_SO) $(subst /,\,$(LIB_NAME_OUT)) $(subst /,\,$(INSTALL_LIB_DIR)/)

install-dbg-shared:	dbg-shared
	@if not exist $(subst /,\,$(INSTALL_BIN_DIR)/) mkdir $(subst /,\,$(INSTALL_BIN_DIR)/)
	@if not exist $(subst /,\,$(INSTALL_LIB_DIR_DBG)/) mkdir $(subst /,\,$(INSTALL_LIB_DIR_DBG)/)
	$(INSTALL_SO) $(subst /,\,$(SO_NAME_OUT_DBG)) $(subst /,\,$(INSTALL_BIN_DIR)/)
	$(INSTALL_SO) $(subst /,\,$(LIB_NAME_OUT_DBG)) $(subst /,\,$(INSTALL_LIB_DIR_DBG)/)

install-stldbg-shared:	stldbg-shared
	@if not exist $(subst /,\,$(INSTALL_BIN_DIR)/) mkdir $(subst /,\,$(INSTALL_BIN_DIR)/)
	@if not exist $(subst /,\,$(INSTALL_LIB_DIR_STLDBG)/) mkdir $(subst /,\,$(INSTALL_LIB_DIR_STLDBG)/)
	$(INSTALL_SO) $(subst /,\,$(SO_NAME_OUT_STLDBG)) $(subst /,\,$(INSTALL_BIN_DIR)/)
	$(INSTALL_SO) $(subst /,\,$(LIB_NAME_OUT_STLDBG)) $(subst /,\,$(INSTALL_LIB_DIR_STLDBG)/)

