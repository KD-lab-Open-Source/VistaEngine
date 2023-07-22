#!ifndef MSVC_DIR
#MSVC_DIR = c:\Program Files\Microsoft Visual Studio .NET\VC7
#!endif

CFLAGS_COMMON = /nologo /W4 /GR /GX
CXXFLAGS_COMMON = /nologo /W4 /GR /GX


!include $(RULESBASE)/$(USE_MAKE)/vc-common.mak

