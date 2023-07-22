# Time-stamp: <05/12/23 23:40:08 ptr>

#INCLUDES = -I$(SRCROOT)/include
INCLUDES :=

CXX := c++
CC := gcc -ansi

ifdef TARGET_OS
CXX := ${TARGET_OS}-${CXX}
CC := ${TARGET_OS}-${CC}
endif

ifeq ($(OSNAME), darwin)
CXX_VERSION := $(shell ${CXX} -dumpversion)
# TODO: ensure PANTHER's gcc compatibility...
CXX_VERSION_MAJOR := $(shell ${CXX} -dumpversion | awk 'BEGIN { FS = "."; } { print $1; }')
CXX_VERSION_MINOR := $(shell ${CXX} -dumpversion | awk 'BEGIN { FS = "."; } { print $2; }')
CXX_VERSION_PATCH := $(shell ${CXX} -dumpversion | awk 'BEGIN { FS = "."; } { print $3; }')
# This is to differentiate Apple-builded compiler from original
# compiler (it's has different behaviour)
ifneq ("$(shell ${CXX} -v 2>&1 | grep Apple)", "")
GCC_APPLE_CC := 1
endif

else

ifneq ($(OSNAME), windows)
CXX_VERSION := $(shell ${CXX} --version | grep GCC | awk '{ print $$3; }')

ifeq ($(CXX_VERSION),)
# 2.95 report only version
CXX_VERSION := $(shell ${CXX} --version)
endif

CXX_VERSION_MAJOR := $(shell echo ${CXX_VERSION} | awk 'BEGIN { FS = "."; } { print $$1; }')
CXX_VERSION_MINOR := $(shell echo ${CXX_VERSION} | awk 'BEGIN { FS = "."; } { print $$2; }')
CXX_VERSION_PATCH := $(shell echo ${CXX_VERSION} | awk 'BEGIN { FS = "."; } { print $$3; }')
endif
endif

DEFS ?=
OPT ?=

ifdef WITHOUT_STLPORT
INCLUDES =
else
INCLUDES = -I${STLPORT_INCLUDE_DIR}
endif

OUTPUT_OPTION = -o $@
LINK_OUTPUT_OPTION = ${OUTPUT_OPTION}
CPPFLAGS = $(DEFS) $(INCLUDES)

ifeq ($(OSNAME), cygming)
release-shared : RCFLAGS = --include-dir=${STLPORT_INCLUDE_DIR} -DCOMP=gcc -DBUILD=r -DBUILD_INFOS="-O2" --output-format coff
dbg-shared : RCFLAGS = --include-dir=${STLPORT_INCLUDE_DIR} -DCOMP=gcc -DBUILD=d -DBUILD_INFOS="-g" --output-format coff
stldbg-shared : RCFLAGS = --include-dir=${STLPORT_INCLUDE_DIR} -DCOMP=gcc -DBUILD=stld -DBUILD_INFOS="-g -D_STLP_DEBUG" --output-format coff
RC_OUTPUT_OPTION = -o $@
CXXFLAGS = -Wall -Wsign-promo -fexceptions -fident
ifeq ($(OSREALNAME), mingw)
CCFLAGS += -mthreads
CFLAGS += -mthreads
CXXFLAGS += -mthreads
else
DEFS += -D_REENTRANT
endif
CCFLAGS += $(OPT)
CFLAGS += $(OPT)
CXXFLAGS += $(OPT)
COMPILE.rc = $(RC) $(RCFLAGS)
endif

ifeq ($(OSNAME), windows)
release-shared : RCFLAGS = --include-dir=${STLPORT_INCLUDE_DIR} -DCOMP=gcc -DBUILD=r -DBUILD_INFOS="-O2" --output-format coff
dbg-shared : RCFLAGS = --include-dir=${STLPORT_INCLUDE_DIR} -DCOMP=gcc -DBUILD=d -DBUILD_INFOS="-g" --output-format coff
stldbg-shared : RCFLAGS = --include-dir=${STLPORT_INCLUDE_DIR} -DCOMP=gcc -DBUILD=stld -DBUILD_INFOS="-g -D_STLP_DEBUG" --output-format coff
RC_OUTPUT_OPTION = -o $@
CXXFLAGS = -Wall -Wsign-promo -fexceptions -fident
CCFLAGS += -mthreads
CFLAGS += -mthreads
CXXFLAGS += -mthreads
CCFLAGS += $(OPT)
CFLAGS += $(OPT)
CXXFLAGS += $(OPT)
COMPILE.rc = $(RC) $(RCFLAGS)
endif

ifeq ($(OSNAME),sunos)
CCFLAGS = -pthreads $(OPT)
CFLAGS = -pthreads $(OPT)
# CXXFLAGS = -pthreads -nostdinc++ -fexceptions -fident $(OPT)
CXXFLAGS = -pthreads  -fexceptions -fident $(OPT)
endif

ifeq ($(OSNAME),linux)
CCFLAGS = -pthread $(OPT)
CFLAGS = -pthread $(OPT)
# CXXFLAGS = -pthread -nostdinc++ -fexceptions -fident $(OPT)
CXXFLAGS = -pthread -fexceptions -fident $(OPT)
endif

ifeq ($(OSNAME),openbsd)
CCFLAGS = -pthread $(OPT)
CFLAGS = -pthread $(OPT)
# CXXFLAGS = -pthread -nostdinc++ -fexceptions -fident $(OPT)
CXXFLAGS = -pthread -fexceptions -fident $(OPT)
endif

ifeq ($(OSNAME),freebsd)
CCFLAGS = -pthread $(OPT)
CFLAGS = -pthread $(OPT)
DEFS += -D_REENTRANT
# CXXFLAGS = -pthread -nostdinc++ -fexceptions -fident $(OPT)
CXXFLAGS = -pthread -fexceptions -fident $(OPT)
endif

ifeq ($(OSNAME),darwin)
CCFLAGS = $(OPT)
CFLAGS = $(OPT)
DEFS += -D_REENTRANT
CXXFLAGS = -fexceptions $(OPT)
# This is here due to bug in GNU make 3.79 from MacOS build:
stldbg-static :	CPPFLAGS = -D_STLP_DEBUG ${CPPFLAGS}
stldbg-shared :	CPPFLAGS = -D_STLP_DEBUG ${CPPFLAGS}
stldbg-static-dep : CPPFLAGS = -D_STLP_DEBUG ${CPPFLAGS}
stldbg-shared-dep : CPPFLAGS = -D_STLP_DEBUG ${CPPFLAGS}
endif

ifeq ($(OSNAME),hp-ux)
CCFLAGS = -pthread $(OPT)
CFLAGS = -pthread $(OPT)
# CXXFLAGS = -pthread -nostdinc++ -fexceptions -fident $(OPT)
CXXFLAGS = -pthread -fexceptions -fident $(OPT)
endif

#ifeq ($(CXX_VERSION_MAJOR),3)
#ifeq ($(CXX_VERSION_MINOR),2)
#CXXFLAGS += -ftemplate-depth-32
#endif
#ifeq ($(CXX_VERSION_MINOR),1)
#CXXFLAGS += -ftemplate-depth-32
#endif
#ifeq ($(CXX_VERSION_MINOR),0)
#CXXFLAGS += -ftemplate-depth-32
#endif
#endif
ifeq ($(CXX_VERSION_MAJOR),2)
CXXFLAGS += -ftemplate-depth-32
endif

# Required for correct order of static objects dtors calls:
ifneq ($(OSNAME),cygming)
ifneq ($(OSNAME),windows)
ifneq ($(OSNAME),darwin)
ifneq ($(CXX_VERSION_MAJOR),2)
CXXFLAGS += -fuse-cxa-atexit
endif
endif
endif
endif

ifdef EXTRA_CXXFLAGS
CXXFLAGS += ${EXTRA_CXXFLAGS}
endif

CDEPFLAGS = -E -M
CCDEPFLAGS = -E -M

# STLport DEBUG mode specific defines
stldbg-static :	DEFS += -D_STLP_DEBUG
stldbg-shared :	DEFS += -D_STLP_DEBUG
stldbg-static-dep : DEFS += -D_STLP_DEBUG
stldbg-shared-dep : DEFS += -D_STLP_DEBUG

# optimization and debug compiler flags
release-static : OPT += -O2
release-shared : OPT += -O2

dbg-static : OPT += -g
dbg-shared : OPT += -g
#dbg-static-dep : OPT += -g
#dbg-shared-dep : OPT += -g

stldbg-static : OPT += -g
stldbg-shared : OPT += -g
#stldbg-static-dep : OPT += -g
#stldbg-shared-dep : OPT += -g

# dependency output parser (dependencies collector)
DP_OUTPUT_DIR = | sed 's|\($*\)\.o[ :]*|$(OUTPUT_DIR)/\1.o $@ : |g' > $@; \
                           [ -s $@ ] || rm -f $@

DP_OUTPUT_DIR_DBG = | sed 's|\($*\)\.o[ :]*|$(OUTPUT_DIR_DBG)/\1.o $@ : |g' > $@; \
                           [ -s $@ ] || rm -f $@

DP_OUTPUT_DIR_STLDBG = | sed 's|\($*\)\.o[ :]*|$(OUTPUT_DIR_STLDBG)/\1.o $@ : |g' > $@; \
                           [ -s $@ ] || rm -f $@
