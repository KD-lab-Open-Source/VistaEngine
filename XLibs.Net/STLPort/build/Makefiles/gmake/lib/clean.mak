# -*- makefile -*- Time-stamp: <04/03/03 15:34:48 ptr>
# $Id: clean.mak,v 1.1.2.2 2005/09/19 19:53:48 dums Exp $

ifneq ($(OSNAME),windows)
clobber::
	@-rm -f ${SO_NAME_OUT}
	@-rm -f ${SO_NAME_OUTx}
	@-rm -f ${SO_NAME_OUTxx}
	@-rm -f ${SO_NAME_OUTxxx}
	@-rm -f ${SO_NAME_OUT_DBG}
	@-rm -f ${SO_NAME_OUT_DBGx}
	@-rm -f ${SO_NAME_OUT_DBGxx}
	@-rm -f ${SO_NAME_OUT_DBGxxx}
	@-rm -f ${SO_NAME_OUT_STLDBG}
	@-rm -f ${SO_NAME_OUT_STLDBGx}
	@-rm -f ${SO_NAME_OUT_STLDBGxx}
	@-rm -f ${SO_NAME_OUT_STLDBGxxx}
	@-rm -f ${A_NAME_OUT}
	@-rm -f ${A_NAME_OUT_DBG}
	@-rm -f ${A_NAME_OUT_STLDBG}
ifeq ($(OSNAME), cygming)
	@-rm -f ${LIB_NAME_OUT}
	@-rm -f ${LIB_NAME_OUT_DBG}
	@-rm -f ${LIB_NAME_OUT_STLDBG}
	@-rm -f ${RES}
	@-rm -f ${RES_DBG}
	@-rm -f ${RES_STLDBG}
endif

distclean::
	@-rm -f $(INSTALL_LIB_DIR)/$(SO_NAME)
	@-rm -f $(INSTALL_LIB_DIR)/$(SO_NAMEx)
	@-rm -f $(INSTALL_LIB_DIR)/$(SO_NAMExx)
	@-rm -f $(INSTALL_LIB_DIR)/$(SO_NAMExxx)
	@-rm -f $(INSTALL_LIB_DIR_DBG)/$(SO_NAME_DBG)
	@-rm -f $(INSTALL_LIB_DIR_DBG)/$(SO_NAME_DBGx)
	@-rm -f $(INSTALL_LIB_DIR_DBG)/$(SO_NAME_DBGxx)
	@-rm -f $(INSTALL_LIB_DIR_DBG)/$(SO_NAME_DBGxxx)
	@-rm -f $(INSTALL_LIB_DIR_STLDBG)/$(SO_NAME_STLDBG)
	@-rm -f $(INSTALL_LIB_DIR_STLDBG)/$(SO_NAME_STLDBGx)
	@-rm -f $(INSTALL_LIB_DIR_STLDBG)/$(SO_NAME_STLDBGxx)
	@-rm -f $(INSTALL_LIB_DIR_STLDBG)/$(SO_NAME_STLDBGxxx)
	@-rm -f $(INSTALL_LIB_DIR)/${A_NAME_OUT}
	@-rm -f $(INSTALL_LIB_DIR_DBG)/${A_NAME_OUT_DBG}
	@-rm -f $(INSTALL_LIB_DIR_STLDBG)/${A_NAME_OUT_STLDBG}
else
clobber::
	@if exist $(SO_NAME_OUT) del /f /q $(SO_NAME_OUT)
	@if exist $(SO_NAME_OUT_DBG) del /f /q $(SO_NAME_OUT_DBG)
	@if exist $(SO_NAME_OUT_STLDBG) del /f /q $(SO_NAME_OUT_STLDBG)
	@if exist $(A_NAME_OUT) del /f /q $(A_NAME_OUT)
	@if exist $(A_NAME_OUT_DBG) del /f /q $(A_NAME_OUT_DBG)
	@if exist $(A_NAME_OUT_STLDBG) del /f /q $(A_NAME_OUT_STLDBG)
	@if exist $(LIB_NAME_OUT) del /f /q $(LIB_NAME_OUT)
	@if exist $(LIB_NAME_OUT_DBG) del /f /q $(LIB_NAME_OUT_DBG)
	@if exist $(LIB_NAME_OUT_STLDBG) del /f /q $(LIB_NAME_OUT_STLDBG)
	@if exist $(RES) del /f /q $(RES)
	@if exist $(RES_DBG) del /f /q $(RES_DBG)
	@if exist $(RES_STLDBG) del /f /q $(RES_STLDBG)

distclean::
	@if exist $(INSTALL_LIB_DIR)/$(SO_NAME) del /f /q $(INSTALL_LIB_DIR)/$(SO_NAME)
	@if exist $(INSTALL_LIB_DIR_DBG)/$(SO_NAME_DBG) del /f /q $(INSTALL_LIB_DIR_DBG)/$(SO_NAME_DBG)
	@if exist $(INSTALL_LIB_DIR_STLDBG)/$(SO_NAME_STLDBG) del /f /q $(INSTALL_LIB_DIR_STLDBG)/$(SO_NAME_STLDBG)
	@if exist $(INSTALL_LIB_DIR)/$(A_NAME) del /f /q $(INSTALL_LIB_DIR)/$(A_NAME)
	@if exist $(INSTALL_LIB_DIR_DBG)/$(A_NAME_DBG) del /f /q $(INSTALL_LIB_DIR_DBG)/$(A_NAME_DBG)
	@if exist $(INSTALL_LIB_DIR_STLDBG)/$(A_NAME_STLDBG) del /f /q $(INSTALL_LIB_DIR_STLDBG)/$(A_NAME_STLDBG)
endif
