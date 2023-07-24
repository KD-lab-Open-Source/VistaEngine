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

#include "cppunit_proxy.h"
#include "file_reporter.h"

#ifdef UNDER_CE
# include <windows.h>
#endif

namespace CPPUNIT_NS
{
  int CPPUNIT_NS::TestCase::m_numErrors = 0;
  int CPPUNIT_NS::TestCase::m_numTests = 0;

  CPPUNIT_NS::TestCase *CPPUNIT_NS::TestCase::m_root = 0;
  CPPUNIT_NS::Reporter *CPPUNIT_NS::TestCase::m_reporter = 0;

  void CPPUNIT_NS::TestCase::registerTestCase(TestCase *in_testCase)
  {
    in_testCase->m_next = m_root;
    m_root = in_testCase;
  }

  int CPPUNIT_NS::TestCase::run(Reporter *in_reporter, const char *in_testName, bool invert )
  {
    TestCase::m_reporter = in_reporter;

    m_numErrors = 0;
    m_numTests = 0;

    TestCase *tmp = m_root;
    while(tmp != 0)
    {
      tmp->myRun(in_testName, invert);
      tmp = tmp->m_next;
    }
    return m_numErrors;
  }  
}

#ifdef __SUNPRO_CC
using namespace std;
#endif // __SUNPRO_CC

# ifdef UNDER_CE
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
  //MessageBox(NULL, TEXT("Press Ok to start STLport testing..."), TEXT("STLP test suite"), MB_OK | MB_ICONASTERISK);

  int argc=1;
  size_t size=wcslen(lpCmdLine);
  char* buff=new char[size+1];
  wcstombs(buff, lpCmdLine, size);
  buff[size]=0;
  char* bp=buff;

  char* argv[3];  // limit to three args
  argv[0]=0;
  argv[1]=0;
  argv[2]=0;
  while(*bp && argc<3) {
    if(*bp==' ' ||* bp=='\t') {
      bp++;
      continue;
    }
    if(*bp=='-') {
      char* start=bp;
      while(*bp && *bp!=' ' && *bp!='\t')
        bp++;
      argv[argc]=new char[bp-start+1];
      strncpy(argv[argc], start, bp-start);
      argv[argc][bp-start]=0;
      argc++;
    }
  }

  // set default output to stlp_test.txt if -f is not defined
  bool f_supplied=false;
  for(int i2=0; i2<argc; i2++){
    if(argv[i2] && argv[i2][1] == 'f') {
      f_supplied=true;
      break;
    }
  }
  if(!f_supplied) {
    argc++;
    argv[argc-1]=new char[18];
    strncpy(argv[argc-1], "-f=/stlp_test.txt", 18);
  }
  
  delete[] buff;

# else
int main(int argc, char** argv)
{
# endif

  // CppUnit(mini) test launcher
  // command line option syntax:
  // test [OPTIONS]
  // where OPTIONS are
  //  -t=CLASS[::TEST]    run the test class CLASS or member test CLASS::TEST
  //  -x=CLASS[::TEST]    run all except the test class CLASS or member test CLASS::TEST
  //  -f=FILE             save output in file FILE instead of stdout

  int num_errors=0;
  char *fileName=0;
  char *testName="";
  char *xtestName="";

  for(int i=1; i<argc; i++) {
    if(argv[i][0]!='-')
      break;
    if(!strncmp(argv[i], "-t=", 3)) {
      testName=argv[i]+3;
    }
    else if(!strncmp(argv[i], "-f=", 3)) {
      fileName=argv[i]+3;
    } else if ( !strncmp(argv[i], "-x=", 3) ) {
      xtestName=argv[i]+3;
    }
  }
  CPPUNIT_NS::Reporter*   reporter=0;
  if(fileName)
      reporter=new FileReporter(fileName);
  else
      reporter=new FileReporter(stdout);

  if ( xtestName[0] != 0 ) {
    num_errors=CPPUNIT_NS::TestCase::run(reporter, xtestName, true );
  } else {
    num_errors=CPPUNIT_NS::TestCase::run(reporter, testName);
  }
  reporter->printSummary();
  delete reporter;

# ifdef UNDER_CE
  // let the user know we are done
  /*
  if(!num_errors)
    MessageBox(NULL, TEXT("All STLport tests passed!"), TEXT("STLP test suite"), MB_OK | MB_ICONASTERISK);
  else
    MessageBox(NULL, TEXT("Some STLport tests failed! Check the output."), TEXT("STLP test suite"), MB_OK | MB_ICONASTERISK);
  */

  // free args
  delete[] argv[1];
  delete[] argv[2];
# endif  

  return num_errors;
}
