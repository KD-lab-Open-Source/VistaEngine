# Time-stamp: <05/09/09 21:03:45 ptr>
# $Id: targetsys.mak,v 1.1.2.2 2005/09/19 19:53:48 dums Exp $

ifndef CC
@echo CC already defined : ${CC}
endif

CC ?= gcc
CXX ?= g++

# shared library:
SO  := dll
LIB := dll.a
EXP := exp

# executable:
EXE := .exe

# static library extention:
ARCH := a

AR := ar
AR_INS_R := -rs
AR_EXTR := -x
AR_OUT = $@
