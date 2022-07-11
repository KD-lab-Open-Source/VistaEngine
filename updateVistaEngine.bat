if exist *.sln goto shortUpdate

set TOOLS=C:\Incubator\Utils

call %TOOLS%\backup.bat

rename Game.exe Game_.exe
rename TriggerEditor.dll TriggerEditor_.dll
rename VistaEngine.exe VistaEngine_.exe
del /Q *.exe
del /Q *.pdb
del /Q *.dll
del /Q Scripts\*.*
del /Q Scripts\Engine\*.*
del /Q /S Scripts\Resource\*.*

%TOOLS%\xcopy C:\Incubator\VistaEngine\*.* .\*.* /E /Y
copy C:\Incubator\Heap\Archive\EffectTool\EffectTool.exe .

if errorlevel 1 goto bad
@wscript.exe %TOOLS%\popupMessage.wsf /JOB:popup "Версия обновилась успешно"
goto end

:bad
@wscript.exe %TOOLS%\popupMessage.wsf /JOB:popup "Версия не смогла обновиться, необходимо перезагрузить компьютер"
goto end

:shortUpdate
copy C:\Incubator\VistaEngine\*.exe .
copy C:\Incubator\VistaEngine\*.pdb .
copy C:\Incubator\VistaEngine\*.dll .

:end


