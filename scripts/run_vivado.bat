@echo off
setlocal
set ROOT=%~dp0..
set VIVADO=G:\Vivado2024.2\Vivado\2024.2\bin\vivado.bat
if not exist "%VIVADO%" (
  echo ERROR: vivado.bat not found at %VIVADO%
  exit /b 1
)
call "%VIVADO%" -mode batch -source "%ROOT%\vivado2024.2\scripts\create_project.tcl"
if errorlevel 1 exit /b 1
call "%VIVADO%" -mode batch -source "%ROOT%\vivado2024.2\scripts\build_bitstream.tcl"
if errorlevel 1 exit /b 1
call "%VIVADO%" -mode batch -source "%ROOT%\vivado2024.2\scripts\export_xsa.tcl"
endlocal
