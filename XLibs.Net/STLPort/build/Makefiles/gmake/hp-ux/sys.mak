# Time-stamp: <05/09/09 21:12:16 ptr>
# $Id: sys.mak,v 1.1.2.2 2005/09/11 06:27:35 complement Exp $

INSTALL := $$HOME/bin/install-sh

INSTALL_SO := ${INSTALL} -c -m 0755
INSTALL_A := ${INSTALL} -c -m 0644
INSTALL_EXE := ${INSTALL} -c -m 0755

EXT_TEST := /usr/bin/test
