# -*- Makefile -*- Time-stamp: <05/05/16 22:07:32 ptr>
# $Id: Makefile.inc,v 1.1.2.4 2005/12/09 16:09:31 complement Exp $

LIBNAME = stlport
MAJOR = 5
MINOR = 0
PATCH = 1
SRC_CPP = ../../src/dll_main.cpp \
          ../../src/fstream.cpp \
          ../../src/strstream.cpp \
          ../../src/sstream.cpp \
          ../../src/ios.cpp \
          ../../src/stdio_streambuf.cpp \
          ../../src/istream.cpp \
          ../../src/ostream.cpp \
          ../../src/iostream.cpp \
          ../../src/codecvt.cpp \
          ../../src/collate.cpp \
          ../../src/ctype.cpp \
          ../../src/monetary.cpp \
          ../../src/num_get.cpp \
          ../../src/num_put.cpp \
          ../../src/num_get_float.cpp \
          ../../src/num_put_float.cpp \
          ../../src/numpunct.cpp \
          ../../src/time_facets.cpp \
          ../../src/messages.cpp \
          ../../src/locale_impl.cpp \
          ../../src/locale_catalog.cpp \
          ../../src/facets_byname.cpp \
          ../../src/complex.cpp \
          ../../src/complex_exp.cpp \
          ../../src/complex_io.cpp \
          ../../src/complex_trig.cpp \
          ../../src/complex_io_w.cpp \
          ../../src/string_w.cpp

SRC_C = ../../src/c_locale.c \
        ../../src/cxa.c

SRC_RC = ../../src/stlport.rc
