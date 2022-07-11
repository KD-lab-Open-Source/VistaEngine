if "%1"=="pass2" goto pass2

rmdir /Q /S Output 
mkdir Output

del /Q /S CacheData\Models\*.*
del /Q /S CacheData\Textures\*.*


xcopy Resource\*.* Output\Resource\*.* /T /E 
rmdir /Q /S Output\Resource\Worlds
mkdir Output\Resource\Worlds
rmdir /Q /S Output\Resource\Saves
mkdir Output\Resource\Saves
xcopy Resource\LocData\*.* Output\Resource\LocData\*.* /S /Y


xcopy Scripts\*.* Output\Scripts\*.* /S /Y
rmdir /Q /S Output\Scripts\contentLocal
rmdir /Q /S Output\Scripts\Engine\NSIS
del /Q /S Output\Scripts\TreeControlSetups\*.*
del /q Output\Scripts\Content\VistaEngine.scr
del /q Output\Scripts\Content\GameOptionsEditor
del /q /S Output\Scripts\Engine\Translations\tools
del /q Output\Scripts\Content\*.psf 


for /R .\Output\Scripts %%k IN (.svn) DO if exist %%k rd %%k /S /Q

goto end


:pass2

xcopy CacheData\*.* Output\CacheData\*.* /S /Y
copy Resource\Worlds\qswl.scr Output\Resource\Worlds\qswl.scr


:end