# -*- makefile -*- Time-stamp: <04/03/31 08:10:19 ptr>
# $Id: evc4.mak,v 1.1.2.5 2005/11/24 05:45:47 complement Exp $

LDFLAGS_COMMON = commctrl.lib coredll.lib corelibc.lib /nologo /base:"0x00100000" /stack:0x10000,0x1000 /incremental:no /subsystem:WINDOWSCE /align:"4096" /nodefaultlib:LIBC.lib /nodefaultlib:OLDNAMES.lib

!if "$(TARGET_PROC)" == "arm"
LDFLAGS_COMMON = $(LDFLAGS_COMMON) /MACHINE:ARM
!if "$(PLATFORM)" == "POCKET PC 2003"
LDFLAGS_COMMON = $(LDFLAGS_COMMON) ccrtrtti.lib
!endif
!endif

!if "$(TARGET_PROC)" == "x86"
LDFLAGS_COMMON = $(LDFLAGS_COMMON) $(CEx86Corelibc) /MACHINE:IX86
!if "$(PLATFORM)" == "POCKET PC 2003"
LDFLAGS_COMMON = $(LDFLAGS_COMMON) ccrtrtti.lib
!endif
!endif

!if "$(TARGET_PROC)" == "mips"
!if "$(TARGET_PROC_SUBTYPE)" == "MIPS16"
LDFLAGS_COMMON = $(LDFLAGS_COMMON) /MACHINE:MIPS
!elseif "$(TARGET_PROC_SUBTYPE)" == "MIPSII"
LDFLAGS_COMMON = $(LDFLAGS_COMMON) /MACHINE:MIPS
!elseif "$(TARGET_PROC_SUBTYPE)" == "MIPSII_FP"
LDFLAGS_COMMON = $(LDFLAGS_COMMON) /MACHINE:MIPS
!elseif "$(TARGET_PROC_SUBTYPE)" == "MIPSIV"
LDFLAGS_COMMON = $(LDFLAGS_COMMON) /MACHINE:MIPSFPU
!elseif "$(TARGET_PROC_SUBTYPE)" == "MIPSIV_FP"
LDFLAGS_COMMON = $(LDFLAGS_COMMON) /MACHINE:MIPSFPU
!endif
!endif

!if "$(TARGET_PROC)" == "sh3"
LDFLAGS_COMMON = $(LDFLAGS_COMMON) /MACHINE:SH3
!endif

!if "$(TARGET_PROC)" == "sh4"
LDFLAGS_COMMON = $(LDFLAGS_COMMON) /MACHINE:SH4
!endif

!include evc-common.mak
