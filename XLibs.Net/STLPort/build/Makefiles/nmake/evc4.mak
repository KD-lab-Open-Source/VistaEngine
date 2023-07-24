# Time-stamp: <04/03/31 07:55:19 ptr>
# $Id: evc4.mak,v 1.1.2.9 2005/11/24 05:45:47 complement Exp $

DEFS_COMMON = $(DEFS_COMMON) /D _WIN32_WCE=$(CEVERSION) /D UNDER_CE=$(CEVERSION) /D "UNICODE"

!if "$(TARGET_PROC)" == ""
!error No target processor configured! Please rerun configure.bat!
!endif

!if "$(CC)" == ""
!error CC not set, run the proper WCE*.bat from this shell to set it!
!endif

# All the batchfiles to setup the environment yield different
# compilers which they put into CC.
CXX = $(CC)

!if "$(TARGET_PROC)" == "arm"
DEFS_COMMON = $(DEFS_COMMON) /D "ARM" /D "_ARM_" /D "ARMV4"
OPT_COMMON =
!endif

!if "$(TARGET_PROC)" == "x86"
DEFS_COMMON = $(DEFS_COMMON) /D "x86" /D "_X86_" /D "_i386_"
OPT_COMMON = /Gs8192
!if "$(TARGET_PROC_SUBTYPE)" == "emulator"
DEFS_COMMON = $(DEFS_COMMON) /D "emulator"
!endif
!endif

!if "$(TARGET_PROC)" == "mips"
DEFS_COMMON = $(DEFS_COMMON) /D "_MIPS_" /D "MIPS"
OPT_COMMON =

!if "$(TARGET_PROC_SUBTYPE)" == "MIPSII"
OPT_COMMON = $(OPT_COMMON) /QMmips2 /QMFPE
!elseif "$(TARGET_PROC_SUBTYPE)" == "MIPSII_FP"
OPT_COMMON = $(OPT_COMMON) /QMmips2 /QMFPE-
!elseif "$(TARGET_PROC_SUBTYPE)" == "MIPSIV"
OPT_COMMON = $(OPT_COMMON) /QMmips4 /QMn32 /QMFPE
!elseif "$(TARGET_PROC_SUBTYPE)" == "MIPSIV_FP"
OPT_COMMON = $(OPT_COMMON) /QMmips4 /QMn32 /QMFPE-
!endif
!endif

!if "$(TARGET_PROC)" == "sh3"
DEFS_COMMON = $(DEFS_COMMON) /D "SH3" /D "_SH3_" /D "SHx"
OPT_COMMON =
!endif

!if "$(TARGET_PROC)" == "sh4"
DEFS_COMMON = $(DEFS_COMMON) /D "SH4" /D "_SH4_" /D "SHx"
OPT_COMMON = /Qsh4
!endif


# exception handling support
CFLAGS_COMMON = /nologo /TC /W4 /GF /GR /GX
CFLAGS_REL = $(CFLAGS_COMMON) $(OPT_REL)
CFLAGS_STATIC_REL = $(CFLAGS_COMMON) $(OPT_STATIC_REL)
CFLAGS_DBG = $(CFLAGS_COMMON) $(OPT_DBG)
CFLAGS_STATIC_DBG = $(CFLAGS_COMMON) $(OPT_STATIC_DBG)
CFLAGS_STLDBG = $(CFLAGS_COMMON) $(OPT_STLDBG)
CFLAGS_STATIC_STLDBG = $(CFLAGS_COMMON) $(OPT_STATIC_STLDBG)
CXXFLAGS_COMMON = /nologo /TP /W4 /GF /GR /GX
CXXFLAGS_REL = $(CXXFLAGS_COMMON) $(OPT_REL)
CXXFLAGS_STATIC_REL = $(CXXFLAGS_COMMON) $(OPT_STATIC_REL)
CXXFLAGS_DBG = $(CXXFLAGS_COMMON) $(OPT_DBG)
CXXFLAGS_STATIC_DBG = $(CXXFLAGS_COMMON) $(OPT_STATIC_DBG)
CXXFLAGS_STLDBG = $(CXXFLAGS_COMMON) $(OPT_STLDBG)
CXXFLAGS_STATIC_STLDBG = $(CXXFLAGS_COMMON) $(OPT_STATIC_STLDBG)

!include evc-common.mak
