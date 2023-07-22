# -*- makefile -*- Time-stamp: <04/07/25 16:54:59 ptr>
# $Id: rules-a.mak,v 1.1.2.1 2004/12/24 11:30:27 ptr Exp $

# Static libraries tags

PHONY += release-static dbg-static stldbg-static

release-static: $(OUTPUT_DIR_A) ${A_NAME_OUT}

dbg-static:	$(OUTPUT_DIR_A_DBG) ${A_NAME_OUT_DBG}

stldbg-static:	$(OUTPUT_DIR_A_STLDBG) ${A_NAME_OUT_STLDBG}

${A_NAME_OUT}:	$(OBJ_A)
	$(AR) $(AR_INS_R) $(AR_OUT) $(OBJ_A)

${A_NAME_OUT_DBG}:	$(OBJ_A_DBG)
	$(AR) $(AR_INS_R) $(AR_OUT) $(OBJ_A_DBG)

${A_NAME_OUT_STLDBG}:	$(OBJ_A_STLDBG)
	$(AR) $(AR_INS_R) $(AR_OUT) $(OBJ_A_STLDBG)
