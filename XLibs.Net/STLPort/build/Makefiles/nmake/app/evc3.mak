# -*- makefile -*- Time-stamp: <04/05/01 00:46:25 ptr>
# $Id: evc3.mak,v 1.1.2.3 2005/06/01 05:40:09 ptr Exp $

LDFLAGS_COMMON = commctrl.lib coredll.lib corelibc.lib /nodefaultlib:"OLDNAMES.lib" /nologo /base:"0x00010000" /stack:0x10000,0x1000 /entry:"WinMainCRTStartup" /incremental:no /nodefaultlib:"libc.lib" /subsystem:WINDOWSCE /align:"4096"
!if "$(TARGET_PROC)" == "arm"
LDFLAGS_COMMON = $(LDFLAGS_COMMON) /MACHINE:ARM
!endif

!if "$(TARGET_PROC)" == "x86"
LDFLAGS_COMMON = $(LDFLAGS_COMMON) $(CEx86Corelibc) /MACHINE:IX86
!endif

!if "$(TARGET_PROC)" == "mips"
LDFLAGS_COMMON = $(LDFLAGS_COMMON) /MACHINE:MIPS
!endif

!if "$(TARGET_PROC)" == "sh3"
LDFLAGS_COMMON = $(LDFLAGS_COMMON) /MACHINE:SH3
!endif

!if "$(TARGET_PROC)" == "sh4"
LDFLAGS_COMMON = $(LDFLAGS_COMMON) /MACHINE:SH4
!endif

!include evc-common.mak
