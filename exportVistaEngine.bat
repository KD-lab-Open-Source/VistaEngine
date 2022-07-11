rmdir /Q /S Output 
mkdir Output

if "%1"=="fast" goto fast_copy
xcopy Resource\*.* Output\Resource\*.* /S /Y
@goto end_copy
:fast_copy
\\Center\Incubator\Utils\HardLinkTree.exe Resource Output\Resource
:end_copy


xcopy Scripts\*.* Output\Scripts\*.* /S /Y
for /R .\Output\Scripts %%k IN (.svn) DO if exist %%k rd %%k /S /Q

copy Maelstrom.exe Output\Maelstrom.exe
copy Game.exe Output\Game.exe
copy ConfigurationTool.exe Output\ConfigurationTool.exe
copy game.pdb Output\game.pdb
copy gameFinal.pdb Output\gameFinal.pdb
copy VistaEngineExt.exe Output\VistaEngineExt.exe
copy VistaEngineExt.pdb Output\VistaEngineExt.pdb

copy TriggerEditor.dll Output\TriggerEditor.dll
copy AttribEditor.dll Output\AttribEditor.dll
copy binkw32.dll Output\binkw32.dll
copy ExcelExport.dll Output\ExcelExport.dll
copy *.ini Output\*.ini

del /Q /S Output\Scripts\TreeControlSetups\*.*
del /q Output\Scripts\Content\VistaEngine.scr
del /q Output\Scripts\Content\GameOptionsEditor
del /q /S Output\Scripts\Engine\Translations\tools

cd Output\Resource
rem call \\Center\Incubator\Utils\cleanResource.cmd
cd ..\..

cd Output
call \\CENTER\Incubator\Utils\cacheObject.cmd 
cd ..
