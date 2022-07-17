svn up
call set.bat

if exist %XLibs% rd %XLibs% /q /s
md %XLibs% 
xcopy Heap %XLibs%\ /E



cd Util\Serialization
call build.bat copyHeaders
cd ..\..

cd Util\XMath
call build.bat copyHeaders
cd ..\..

cd Util\kdw
call build.bat copyHeaders
cd ..\..

cd Profiler
call build.bat copyHeaders
cd ..



cd STLport
call build.bat 
cd ..

cd XUtil
call build.bat 
cd ..

cd XZip
call build.bat 
cd ..


cd Util\Serialization
call build.bat 
cd ..\..

cd Util\XMath
call build.bat 
cd ..\..

cd Util\kdw
call build.bat
cd ..\..

cd Profiler
call build.bat 
cd ..


cd OGG
call build.bat 
cd ..



cd ProfUIS
call build.bat 
cd ..



cd DemonWare
call build.bat 
cd ..


cd FreeType
call build.bat 
cd ..

cd Util\XmlRpc
call build.bat
cd ..\..

cd MySQLpp
call build.bat 
cd ..

exit