# -*- makefile -*- Time-stamp: <03/10/27 18:15:05 ptr>
# $Id: rules-so.mak,v 1.1.2.1 2004/12/24 11:29:56 ptr Exp $

# Shared libraries tags

PHONY += release-shared dbg-shared stldbg-shared

release-shared:	$(OUTPUT_DIR) ${SO_NAME_OUT}

dbg-shared:	$(OUTPUT_DIR_DBG) ${SO_NAME_OUT_DBG}

stldbg-shared:	$(OUTPUT_DIR_STLDBG) ${SO_NAME_OUT_STLDBG}

${SO_NAME_OUT}:	$(OBJ) $(RES) $(LIBSDEP)
	$(LINK.cc) $(LINK_OUTPUT_OPTION) $(OBJ) $(RES) $(LDLIBS)

${SO_NAME_OUT_DBG}:	$(OBJ_DBG) $(RES_DBG) $(LIBSDEP)
	$(LINK.cc) $(LINK_OUTPUT_OPTION) $(OBJ_DBG) $(RES_DBG) $(LDLIBS)

${SO_NAME_OUT_STLDBG}:	$(OBJ_STLDBG) $(RES_STLDBG) $(LIBSDEP)
	$(LINK.cc) $(LINK_OUTPUT_OPTION) $(OBJ_STLDBG) $(RES_STLDBG) $(LDLIBS)

