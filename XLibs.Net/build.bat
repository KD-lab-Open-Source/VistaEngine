call set.bat

del %XLibs% /q /s
rd %XLibs% /q /s
md %XLibs% 
xcopy Heap %XLibs%\ /E

cd STLport
call build.bat 
cd ..

cd XUtil
call build.bat 
cd ..

cd XZip
call build.bat 
cd ..


cd Serialization
call build.bat 
cd ..

cd AttribEditor
call build.bat 
cd ..


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

