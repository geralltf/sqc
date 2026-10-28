@echo off
cd /d C:\Users\Admin\OneDrive\Documents\Projects\sqc_compiler
.\sqc.exe sqc.c -o sqc2.exe > build_sqc2_out.txt 2>&1
echo SQC2_EXIT=%ERRORLEVEL% >> build_sqc2_out.txt
