# Time-stamp: <05/09/09 20:59:42 ptr>
# $Id: targetsys.mak,v 1.1.2.1 2005/09/11 06:27:35 complement Exp $

SO := so

ARCH := a
ifdef TARGET_OS
AR := ${TARGET_OS}-ar
else
AR := ar
endif
AR_INS_R := -rs
AR_EXTR := -x
AR_OUT = $@
