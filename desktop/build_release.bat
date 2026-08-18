@echo off
setlocal

call "D:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"

set QTDIR=D:\Qt\6.7.3\msvc2022_64
set PATH=%QTDIR%\bin;%PATH%

set BUILD_DIR=d:\Workspace\test_project\Back-Cat-Project-trae-agent-4vI8Jf\desktop\build\Desktop_Qt_6_7_3_MSVC2022_64bit-Debug

cd /d "%BUILD_DIR%"

echo === Delete old BackPet.exe in bin ===
del /f /q bin\BackPet.exe 2>nul
del /f /q bin\BackPet.ilk 2>nul
del /f /q bin\BackPet.pdb 2>nul

echo === Run nmake -f Makefile.Release ===
nmake /f Makefile.Release
if errorlevel 1 (
    echo NMAKE RELEASE FAILED
    exit /b 1
)

echo === Result ===
for %%F in (bin\BackPet.exe) do echo SIZE: %%~zF bytes  TIME: %%~tF
endlocal
