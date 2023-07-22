# -*- makefile -*- Time-stamp: <04/07/25 12:44:54 ptr>
# $Id: top.mak,v 1.1.2.1 2004/12/24 11:29:38 ptr Exp $

include ${RULESBASE}/${USE_MAKE}/app/macro.mak
include ${RULESBASE}/${USE_MAKE}/app/${COMPILER_NAME}.mak
include ${RULESBASE}/${USE_MAKE}/app/rules.mak
include ${RULESBASE}/${USE_MAKE}/app/rules-install.mak
include ${RULESBASE}/${USE_MAKE}/app/clean.mak
