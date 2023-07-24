# -*- makefile -*- Time-stamp: <03/10/10 16:15:53 ptr>
# $Id: top.mak,v 1.1.2.1 2004/12/24 11:30:28 ptr Exp $

LDFLAGS ?= 

include ${RULESBASE}/${USE_MAKE}/${OSNAME}/lib.mak
include ${RULESBASE}/${USE_MAKE}/lib/${COMPILER_NAME}.mak
include ${RULESBASE}/${USE_MAKE}/${OSNAME}/rules-so.mak
include ${RULESBASE}/${USE_MAKE}/lib/rules-a.mak
include ${RULESBASE}/${USE_MAKE}/${OSNAME}/rules-install-so.mak
include ${RULESBASE}/${USE_MAKE}/lib/rules-install-a.mak
include ${RULESBASE}/${USE_MAKE}/lib/clean.mak
