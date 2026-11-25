@echo off
setlocal
set ROOT=%~dp0..
call "%ROOT%\scripts\run_hls.bat"
if errorlevel 1 exit /b 1
call "%ROOT%\scripts\run_vivado.bat"
if errorlevel 1 exit /b 1
call "%ROOT%\scripts\run_vitis_build.bat"
endlocal
