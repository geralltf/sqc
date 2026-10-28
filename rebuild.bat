@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" > nul 2>&1
echo VCVARS_DONE
cl /nologo /O2 /W0 sqc.c /Fe:sqc.exe
echo CL_EXIT=%ERRORLEVEL%
