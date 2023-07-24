call ..\set.bat

%devenv% XTool7.1.sln /rebuild Debug
%devenv% XTool7.1.sln /rebuild DebugMultithreaded 
%devenv% XTool7.1.sln /rebuild "DebugMultithreaded Dll"
%devenv% XTool7.1.sln /rebuild Release 
%devenv% XTool7.1.sln /rebuild ReleaseMultithreaded 
%devenv% XTool7.1.sln /rebuild "ReleaseMultithreaded Dll"

copy /b xutil.h %XLibs%\xutil.h
copy /b XMath\xmath.h %XLibs%\xmath.h
copy /b Profiler\statistics.h %XLibs%\statistics.h
copy /b CRC\crc.h %XLibs%\crc.h
copy /b mtsection.h %XLibs%\MTSection.h

move libs\*.* %XLibs%\VC7.1\


%devenv% XTool7.1.sln /clean Debug
%devenv% XTool7.1.sln /clean DebugMultithreaded 
%devenv% XTool7.1.sln /clean "DebugMultithreaded Dll"
%devenv% XTool7.1.sln /clean Release 
%devenv% XTool7.1.sln /clean ReleaseMultithreaded 
%devenv% XTool7.1.sln /clean "ReleaseMultithreaded Dll"
