call ..\set.bat

cd build\lib

nmake /fnmake-vc71.mak clean
nmake /fnmake-vc71.mak install

del obj\*.* /s /q
rd obj /s /q

cd ..\..

move bin\*.* %XLIBS%\Vc7.1
move lib\*.* %XLIBS%\Vc7.1


del %XLibs%\stl /q /s
rd %XLibs%\stl /q /s
xcopy stlport %XLibs%\stl\ /E
