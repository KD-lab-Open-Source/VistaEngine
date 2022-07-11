set proc="C:\Program Files\TortoiseSVN\bin\TortoiseProc.exe" 

%proc% /command:update /path:"Scripts\Content*Resource" /notempfile /closeonend:3

%proc% /command:commit /path:"Scripts\Content*Resource" /notempfile /closeonend
