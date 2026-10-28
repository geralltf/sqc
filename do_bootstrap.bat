@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" > /dev/null 2>&1
echo VCVARS_DONE
cl /nologo /O2 /W0 sqc.c /Fe:sqc_msvc.exe
echo CL_EXIT=%ERRORLEVEL%
if not exist sqc_msvc.exe (echo MSVC BUILD FAILED && exit /b 1)
echo Building sqc2.exe...
sqc_msvc.exe sqc.c -o sqc2.exe
echo S2_EXIT=%ERRORLEVEL%
if not exist sqc2.exe (echo S2 BUILD FAILED && exit /b 1)
echo Building sqc3.exe...
sqc2.exe sqc.c -o sqc3.exe
echo S3_EXIT=%ERRORLEVEL%
echo Bootstrap done.
