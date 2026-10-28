@echo off
cd /d C:\Users\Admin\OneDrive\Documents\Projects\sqc_compiler
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" > nul 2>&1
echo VCVARS_DONE > msvc_build_log.txt
cl /nologo /O2 /W0 sqc.c /Fe:sqc_msvc.exe >> msvc_build_log.txt 2>&1
echo CL_EXIT=%ERRORLEVEL% >> msvc_build_log.txt
if exist sqc_msvc.exe (echo FILE_OK >> msvc_build_log.txt) else (echo FILE_MISSING >> msvc_build_log.txt)
