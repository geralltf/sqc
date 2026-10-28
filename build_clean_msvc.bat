@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" > nul 2>&1
echo VCVARS_DONE
cl /nologo /O2 /W0 "C:\Users\Admin\OneDrive\Documents\Projects\sqc_compiler\sqc.c" /Fe:"C:\Users\Admin\OneDrive\Documents\Projects\sqc_compiler\sqc_msvc.exe"
echo CL_EXIT=%ERRORLEVEL%
if exist "C:\Users\Admin\OneDrive\Documents\Projects\sqc_compiler\sqc_msvc.exe" (echo FILE_OK) else (echo FILE_MISSING)
