@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" > nul 2>&1
echo Building sqc_msvc.exe...
cl /nologo /O2 /W0 sqc.c /Fe:sqc_msvc.exe 2>&1
echo MSVC_EXIT=%ERRORLEVEL%
if not exist sqc_msvc.exe (echo MSVC BUILD FAILED && exit /b 1)
echo Building sqc2_new.exe...
sqc_msvc.exe sqc.c -o sqc2_new.exe
echo S2_EXIT=%ERRORLEVEL%
echo Testing sizeof short...
sqc2_new.exe C:\Users\Admin\OneDrive\Documents\Projects\sqc_compiler\test_short_s2.c -o test_short_s2.exe
echo TP_EXIT=%ERRORLEVEL%
test_short_s2.exe
