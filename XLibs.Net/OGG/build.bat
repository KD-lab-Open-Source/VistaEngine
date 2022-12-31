set XLibs=C:\XLibs
set devenv="C:\Program Files\Microsoft Visual Studio .NET 2003\Common7\IDE\devenv.com"

cd PlayOgg

%devenv% PlayOgg.sln /rebuild Debug
%devenv% PlayOgg.sln /rebuild Release

copy /b PlayOgg.h %XLibs%\


move PlayOggDbgMT.lib %XLibs%\VC7.1\
move PlayOggMT.lib %XLibs%\VC7.1\
move Debug\playoggdbgmt.pdb %XLibs%\VC7.1\

move vorbisfile_static_d.lib %XLibs%\VC7.1\
move vorbisfile_static.idb %XLibs%\VC7.1\
move vorbisfile_static.pdb %XLibs%\VC7.1\

move ogg_static_d.lib %XLibs%\VC7.1\
move ogg_static.idb %XLibs%\VC7.1\
move ogg_static.pdb %XLibs%\VC7.1\


:%devenv% PlayOgg.sln /clean Debug
:%devenv% PlayOgg.sln /clean Release


cd ..

