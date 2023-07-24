# -*- Makefile -*- Time-stamp: <04/03/31 08:14:15 ptr>
# $Id: nmake-evc4.mak,v 1.1.2.2 2005/06/03 20:03:55 dums Exp $

COMPILER_NAME=evc4
CEVERSION=420

LDFLAGS_COMMON = /entry:"mainACRTStartup"

!include nmake-evc-common.mak

!if "$(COMPILER_NAME)" == "evc3"
!error Error: Configured for evc3 but compiling with evc4 makefiles!
!endif
