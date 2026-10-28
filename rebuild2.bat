@echo off
cd /d C:\Users\Admin\OneDrive\Documents\Projects\sqc_compiler
.\sqc.exe sqc.c -o sqc2.exe
echo SQC2_EXIT=%ERRORLEVEL%
