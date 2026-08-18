@echo off
setlocal

call "D:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"

set QTDIR=D:\Qt\6.7.3\msvc2022_64
set PATH=%QTDIR%\bin;%PATH%

set BUILD_DIR=d:\Workspace\test_project\Back-Cat-Project-trae-agent-4vI8Jf\desktop\build\Desktop_Qt_6_7_3_MSVC2022_64bit-Release

cd /d "%BUILD_DIR%"

echo === Test direct cl invocation via qmake system ===
echo cl path:
where cl

echo === Run qmake with -d (debug) briefly ===
qmake "d:\Workspace\test_project\Back-Cat-Project-trae-agent-4vI8Jf\desktop\BackPet.pro" "CONFIG+=release" -spec win32-msvc -d 2>&1 | findstr /C:"system" /C:"cl" /C:"ERROR" /C:"macros" | head -20

endlocal
