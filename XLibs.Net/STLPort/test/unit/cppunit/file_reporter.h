/*
 * Copyright (c) 2003, 2004
 * Zdenek Nemec
 *
 * This material is provided "as is", with absolutely no warranty expressed
 * or implied. Any use is at your own risk.
 *
 * Permission to use or copy this software for any purpose is hereby granted 
 * without fee, provided the above notices are retained on all copies.
 * Permission to modify the code and to distribute modified code is granted,
 * provided the above notices are retained, and a notice that the code was
 * modified is included with the above copyright notice.
 *
 */

/* $Id: file_reporter.h,v 1.1.2.2 2005/01/22 13:01:06 dums Exp $ */

#ifndef _CPPUNITMINIFILEREPORTERINTERFACE_H_
#   define _CPPUNITMINIFILEREPORTERINTERFACE_H_

#include <stdio.h>
//
// CppUnit mini file(stream) reporter
//
class FileReporter : public CPPUNIT_NS::Reporter
{
private:
    FileReporter(const FileReporter&) {}
    const FileReporter& operator=(const FileReporter&) { return *this; }
public:
    FileReporter() : m_numErrors(0), m_numTests(0), _myStream(false) { _file=stderr; }
    FileReporter(const char* file) : m_numErrors(0), m_numTests(0), _myStream(true) { _file=fopen(file, "w");  }
    FileReporter(FILE* stream) : m_numErrors(0), m_numTests(0), _myStream(false) { _file=stream;  }

    virtual ~FileReporter() { if(_myStream) fclose(_file); else fflush(_file);  }
    
    virtual void error(const char *in_macroName, const char *in_macro, const char *in_file, int in_line) 
    {
        m_numErrors++;
        fprintf(_file, "\n%s(%d) : %s(%s);\n", in_file, in_line, in_macroName, in_macro);
    }

    virtual void message( const char *msg )
    {
      fprintf(_file, "\t%s\n", msg );
    }
  
    virtual void progress(const char *in_className, const char *in_shortTestName) 
    {
        m_numTests++;
        fprintf(_file, "%s::%s\n", in_className, in_shortTestName);
    }
    virtual void printSummary() 
    {
        if(m_numErrors > 0) {
            fprintf(_file, "There were errors! (%d of %d)\n", m_numErrors, m_numTests);
        }
        else {
            fprintf(_file, "\nOK (%d)\n\n", m_numTests);
        }
    }
private:
    int m_numErrors;
    int m_numTests;
    bool  _myStream;
    FILE* _file;
};

#endif /*_CPPUNITMINIFILEREPORTERINTERFACE_H_*/
