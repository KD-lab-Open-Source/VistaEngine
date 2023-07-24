# -*- Makefile -*- Time-stamp: <05/12/07 23:37:06 ptr>

SRCROOT := ../..
COMPILER_NAME := gcc

ALL_TAGS := release-shared stldbg-shared
STLPORT_DIR := ../../..
include Makefile.inc
include ${SRCROOT}/Makefiles/top.mak

DEFS += -D_STLP_NO_CUSTOM_IO

dbg-shared:	DEFS += -D_STLP_DEBUG_UNINITIALIZED 
stldbg-shared:	DEFS += -D_STLP_DEBUG_UNINITIALIZED 

ifeq ($(OSNAME), cygming)
release-shared:	DEFS += -D_STLP_USE_DYNAMIC_LIB
dbg-shared:	DEFS += -D_STLP_USE_DYNAMIC_LIB
stldbg-shared:	DEFS += -D_STLP_USE_DYNAMIC_LIB
endif

ifeq ($(OSNAME), windows)
release-shared:	DEFS += -D_STLP_USE_DYNAMIC_LIB
dbg-shared:	DEFS += -D_STLP_USE_DYNAMIC_LIB
stldbg-shared:	DEFS += -D_STLP_USE_DYNAMIC_LIB
endif

ifdef STLP_BUILD_BOOST_PATH
INCLUDES += -I${STLP_BUILD_BOOST_PATH}
endif

ifndef TARGET_OS
ifeq ($(OSNAME), sunos)
release-shared:	LDFLAGS += -Wl,-R${STLPORT_LIB_DIR}
dbg-shared:	LDFLAGS += -Wl,-R${STLPORT_LIB_DIR}
stldbg-shared:	LDFLAGS += -Wl,-R${STLPORT_LIB_DIR}
endif
ifeq ($(OSNAME), freebsd)
release-shared:	LDFLAGS += -Wl,-R${STLPORT_LIB_DIR}
dbg-shared:	LDFLAGS += -Wl,-R${STLPORT_LIB_DIR}
stldbg-shared:	LDFLAGS += -Wl,-R${STLPORT_LIB_DIR}
endif
ifeq ($(OSNAME), openbsd)
release-shared:	LDFLAGS += -Wl,-R${STLPORT_LIB_DIR}
dbg-shared:	LDFLAGS += -Wl,-R${STLPORT_LIB_DIR}
stldbg-shared:	LDFLAGS += -Wl,-R${STLPORT_LIB_DIR}
endif
ifeq ($(OSNAME), linux)
release-shared:	LDFLAGS += -Wl,-rpath=${STLPORT_LIB_DIR}
dbg-shared:	LDFLAGS += -Wl,-rpath=${STLPORT_LIB_DIR}
stldbg-shared:	LDFLAGS += -Wl,-rpath=${STLPORT_LIB_DIR}
endif
endif



