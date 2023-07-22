call ..\set.bat

%devenv% Profiler.sln /project Profiler /rebuild Debug
%devenv% Profiler.sln /project Profiler /rebuild Release
%devenv% Profiler.sln /project Profiler /rebuild DebugDll
%devenv% Profiler.sln /project Profiler /rebuild ReleaseDll

copy /b Profiler\Profiler.h %XLibs%\

move Profiler\ProfilerMT.lib %XLibs%\VC7.1\
move Profiler\ProfilerMTD.lib %XLibs%\VC7.1\
move Profiler\ProfilerMTDll.lib %XLibs%\VC7.1\
move Profiler\ProfilerMTDDll.lib %XLibs%\VC7.1\

%devenv% Profiler.sln /project Profiler /clean Debug
%devenv% Profiler.sln /project Profiler /clean Release
%devenv% Profiler.sln /project Profiler /clean DebugDll
%devenv% Profiler.sln /project Profiler /clean ReleaseDll
