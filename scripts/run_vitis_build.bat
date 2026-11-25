@echo off
setlocal
set ROOT=%~dp0..
set XSCT=G:\Vivado2024.2\Vitis\2024.2\bin\xsct.bat
if not exist "%XSCT%" (
  echo ERROR: xsct.bat not found at %XSCT%
  exit /b 1
)
call "%XSCT%" "%ROOT%\vitis2024.2\scripts\build_app.tcl"
endlocal
