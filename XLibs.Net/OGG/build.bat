call ..\set.bat

cd PlayOgg

%devenv% PlayOgg.sln /rebuild Debug
%devenv% PlayOgg.sln /rebuild Release

copy /b PlayOgg.h %XLibs%\


move PlayOggDbgMT.lib %XLibs%\VC7.1\
move PlayOggMT.lib %XLibs%\VC7.1\
move Debug\playoggdbgmt.pdb %XLibs%\VC7.1\

move ..\libogg-1.1\win32\Static_Debug\ogg_static_d.lib %XLibs%\VC7.1\
move ..\libogg-1.1\win32\Static_Debug\ogg_static.pdb %XLibs%\VC7.1\
move ..\libogg-1.1\win32\Static_Release\ogg_static.lib %XLibs%\VC7.1\

move ..\libvorbis-1.0.1\win32\Vorbis_Static_Debug\vorbis_static_d.lib %XLibs%\VC7.1\
move ..\libvorbis-1.0.1\win32\Vorbis_Static_Debug\vorbis_static.pdb %XLibs%\VC7.1\
move ..\libvorbis-1.0.1\win32\Vorbis_Static_Release\vorbis_static.lib %XLibs%\VC7.1\

move ..\libvorbis-1.0.1\win32\VorbisFile_Static_Debug\vorbisfile_static_d.lib %XLibs%\VC7.1\
move ..\libvorbis-1.0.1\win32\VorbisFile_Static_Debug\vorbisfile_static.pdb %XLibs%\VC7.1\
move ..\libvorbis-1.0.1\win32\VorbisFile_Static_Release\vorbisfile_static.lib %XLibs%\VC7.1\


%devenv% PlayOgg.sln /clean Debug
%devenv% PlayOgg.sln /clean Release

cd ..

