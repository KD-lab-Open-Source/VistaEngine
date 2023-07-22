# Time-stamp: <05/09/09 21:07:10 ptr>
# $Id: targetsys.mak,v 1.1.2.1 2005/09/11 06:27:35 complement Exp $

SO := nlm

ARCH := lib
AR := mwldnlm -type library -msgstyle gcc -w off
AR_INS_R := 
AR_EXTR :=
AR_OUT = -o $@

EXE := .nlm
