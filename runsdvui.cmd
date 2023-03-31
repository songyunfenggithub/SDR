cd /d "D:\my\my.cpps\my.Wave\myWave" &msbuild "myWave.vcxproj" /t:sdvViewer /p:configuration="Debug" /p:platform="x64" /p:SolutionDir="D:\my\my.cpps\my.Wave\myWave" 
exit %errorlevel% 