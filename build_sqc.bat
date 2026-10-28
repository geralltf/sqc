@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" > nul 2>&1
echo ENV_OK
cd /d C:\Users\Admin\OneDrive\Documents\Projects\sqc_compiler
cl /nologo /O2 /W0 sqc.c /Fe:sqc.exe
echo BUILD_EXIT=%ERRORLEVEL%
