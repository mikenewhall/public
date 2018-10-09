@echo off
setlocal
setlocal enableextensions
set pi_path="."
if not "%~1" == "" set pi_path="%~1"
set pi_path=%pi_path:"=%\prompt-init.bat
if not exist "%pi_path%" (
echo ERROR: File "%pi_path%" not found
exit /b 2
)
echo Installing file "%pi_path%" as Windows Command Prompt startup script...
reg add "HKCU\Software\Microsoft\Command Processor" /v AutoRun /t REG_EXPAND_SZ /d "c:\commands\prompt-init.bat" /f
if errorlevel 1 (
echo "ERROR: Failed with code %errorlevel%"
exit /b %errorlevel%
) else (
echo SUcceeded.
"%pi_path%"
)