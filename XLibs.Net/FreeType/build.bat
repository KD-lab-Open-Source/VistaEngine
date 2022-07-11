call ..\set.bat

cd builds\win32\visualc

call ..\..\..\..\buildVcProj.bat freetype

cd ..\..\..

del objs\*.* /s /q
rd objs /s /q

:: copying headers
del %XLibs%\freetype\*.* /q /s
rd %XLibs%\freetype\ /q /s

xcopy include %XLibs%\ /E /Y
