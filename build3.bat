@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" 1>C:\tmp\b3_out.txt 2>&1
echo ENV_OK >> C:\tmp\b3_out.txt
cl /nologo /O2 /W0 sqc.c /Fe:sqc_msvc.exe >> C:\tmp\b3_out.txt 2>&1
echo MSVC_EXIT=%ERRORLEVEL% >> C:\tmp\b3_out.txt
if not exist sqc_msvc.exe (echo BUILD_FAILED >> C:\tmp\b3_out.txt && exit /b 1)
sqc_msvc.exe sqc.c -o sqc2_new.exe >> C:\tmp\b3_out.txt 2>&1
echo S2_EXIT=%ERRORLEVEL% >> C:\tmp\b3_out.txt
if not exist sqc2_new.exe (echo S2_MISSING >> C:\tmp\b3_out.txt && exit /b 1)
echo --- short test via sqc2_new --- >> C:\tmp\b3_out.txt
sqc2_new.exe test_short_s2.c -o test_short_s2.exe >> C:\tmp\b3_out.txt 2>&1
echo SHORT_EXIT=%ERRORLEVEL% >> C:\tmp\b3_out.txt
test_short_s2.exe >> C:\tmp\b3_out.txt 2>&1
echo SHORT_RUN=%ERRORLEVEL% >> C:\tmp\b3_out.txt
echo --- sqc3 bootstrap --- >> C:\tmp\b3_out.txt
sqc2_new.exe sqc.c -o sqc3.exe >> C:\tmp\b3_out.txt 2>&1
echo S3_EXIT=%ERRORLEVEL% >> C:\tmp\b3_out.txt
if not exist sqc3.exe (echo S3_MISSING >> C:\tmp\b3_out.txt && exit /b 1)
echo --- test_program2 via sqc3 --- >> C:\tmp\b3_out.txt
sqc3.exe test_program2.c -o test_program2.exe >> C:\tmp\b3_out.txt 2>&1
echo TP2_EXIT=%ERRORLEVEL% >> C:\tmp\b3_out.txt
if not exist test_program2.exe (echo TP2_MISSING >> C:\tmp\b3_out.txt && exit /b 1)
test_program2.exe >> C:\tmp\b3_out.txt 2>&1
echo RUN_EXIT=%ERRORLEVEL% >> C:\tmp\b3_out.txt
echo DONE >> C:\tmp\b3_out.txt
