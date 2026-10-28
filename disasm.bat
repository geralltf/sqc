@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" > nul 2>&1
dumpbin /DISASM sqc2.exe > disasm_sqc2.txt 2>&1
echo DONE
