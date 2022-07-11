set MSV=%VS71COMNTOOLS%\..\..

call "%MSV%\Vc7\bin\"vcvars32.bat

if "%XLibs%"=="" set XLibs=C:\XLibs

set devenv="%MSV%\Common7\IDE\devenv.com"
