call ..\set.bat

%devenv% XZip.sln /project xziplib /rebuild Debug
%devenv% XZip.sln /project xziplib /rebuild Release 

copy /b XZip.h %XLibs%\XZip.h

move xzipmt.lib %XLibs%\VC7.1\
move xzipmtd.lib %XLibs%\VC7.1\

%devenv% XZip.sln /project xziplib /clean Debug
%devenv% XZip.sln /project xziplib /clean Release 
