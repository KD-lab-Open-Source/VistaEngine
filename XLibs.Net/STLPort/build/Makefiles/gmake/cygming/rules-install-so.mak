# -*- makefile -*- Time-stamp: <03/07/15 18:26:22 ptr>

INSTALL_TAGS ?= install-release-shared install-dbg-shared install-stldbg-shared

PHONY += install $(INSTALL_TAGS)

install:	$(INSTALL_TAGS)

install-static: all-static install-release-static install-dbg-static install-stldbg-static
install-shared: all-shared install-release-shared install-dbg-shared install-stldbg-shared

install-release-shared:	release-shared $(INSTALL_LIB_DIR)
	@if [ ! -d $(INSTALL_BIN_DIR) ] ; then \
	  mkdir -p $(INSTALL_BIN_DIR) ; \
	fi
	$(INSTALL_SO) ${SO_NAME_OUT} $(INSTALL_BIN_DIR)/
	$(INSTALL_SO) ${LIB_NAME_OUT} $(INSTALL_LIB_DIR)/
#	$(INSTALL_SO) ${EXP_NAME_OUT} $(INSTALL_LIB_DIR)/

install-dbg-shared:	dbg-shared $(INSTALL_LIB_DIR_DBG)
	@if [ ! -d $(INSTALL_BIN_DIR) ] ; then \
	  mkdir -p $(INSTALL_BIN_DIR) ; \
	fi
	$(INSTALL_SO) ${SO_NAME_OUT_DBG} $(INSTALL_BIN_DIR)/
	$(INSTALL_SO) ${LIB_NAME_OUT_DBG} $(INSTALL_LIB_DIR_DBG)/
#	$(INSTALL_SO) ${EXP_NAME_OUT_DBG} $(INSTALL_LIB_DIR_DBG)/

install-stldbg-shared:	stldbg-shared $(INSTALL_LIB_DIR_DBG)
	@if [ ! -d $(INSTALL_BIN_DIR) ] ; then \
	  mkdir -p $(INSTALL_BIN_DIR) ; \
	fi
	$(INSTALL_SO) ${SO_NAME_OUT_STLDBG} $(INSTALL_BIN_DIR)/
	$(INSTALL_SO) ${LIB_NAME_OUT_STLDBG} $(INSTALL_LIB_DIR_STLDBG)/
#	$(INSTALL_SO) ${EXP_NAME_OUT_STLDBG} $(INSTALL_LIB_DIR_STLDBG)/

