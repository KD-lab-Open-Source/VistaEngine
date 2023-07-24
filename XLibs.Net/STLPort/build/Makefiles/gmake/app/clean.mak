# -*- makefile -*- Time-stamp: <03/07/10 00:20:54 ptr>
# $Id: clean.mak,v 1.1.2.2 2005/09/19 19:53:48 dums Exp $

ifneq ($(OSNAME),windows)
clobber::
	@-rm -f ${PRG}
	@-rm -f ${PRG_DBG}
	@-rm -f ${PRG_STLDBG}

distclean::
	@-rm -f $(INSTALL_BIN_DIR)/$(PRG)
	@-rm -f $(INSTALL_BIN_DIR_DBG)/$(PRG_DBG)
	@-rm -f $(INSTALL_BIN_DIR_STLDBG)/$(PRG_STLDBG)
else
clobber::
	@if exist $(PRG) del /f /q $(PRG)
	@if exist $(PRG_DBG) del /f /q $(PRG_DBG)
	@if exist $(PRG_STLDBG) del /f /q $(PRG_STLDBG)

distclean::
	@if exist $(INSTALL_BIN_DIR)/$(PRG) del /f /q $(INSTALL_BIN_DIR)/$(PRG)
	@if exist $(INSTALL_BIN_DIR_DBG)/$(PRG_DBG) del /f /q $(INSTALL_BIN_DIR_DBG)/$(PRG_DBG)
	@if exist $(INSTALL_BIN_DIR_STLDBG)/$(PRG_STLDBG) del /f /q $(INSTALL_BIN_DIR_STLDBG)/$(PRG_STLDBG)
endif
