@echo off
setlocal
set ROOT=%~dp0..
if exist "C:\Program Files\Git\usr\bin\tee.exe" set PATH=C:\Program Files\Git\usr\bin;%PATH%
if exist "G:\Vivado2024.2\Vitis\2024.2\tps\win64\msys64\usr\bin\tee.exe" set PATH=G:\Vivado2024.2\Vitis\2024.2\tps\win64\msys64\usr\bin;%PATH%
set HLS=G:\Vivado2024.2\Vitis\2024.2\bin\vitis_hls.bat
if not exist "%HLS%" set HLS=G:\Vivado2024.2\Vitis_HLS\2024.2\bin\vitis_hls.bat
if not exist "%HLS%" set HLS=G:\Vivado2024.2\Vivado\2024.2\bin\vitis_hls.bat
if not exist "%HLS%" set HLS=G:\Vivado2024.2\Vitis\2024.2\bin\unwrapped\win64.o\vitis_hls.exe
if not exist "%HLS%" (
  echo ERROR: vitis_hls tool not found.
  exit /b 1
)
pushd "%ROOT%\python_model_export\tiny_gpt"
python export_tiny_gpt_int8.py
if errorlevel 1 exit /b 1
copy /Y exported\gpt_*.h "%ROOT%\HLS\gpt_accel\" >nul
copy /Y exported\gpt_*.h "%ROOT%\vitis2024.2\src\" >nul
copy /Y exported\gpt_*.h "%ROOT%\vitis2024.2\workspace\ai_menu_app\src\" >nul
popd
pushd "%ROOT%\HLS\lenet_accel"
call "%HLS%" -f build_prj.tcl
popd
pushd "%ROOT%\HLS\gpt_accel"
call "%HLS%" -f run_hls.tcl
popd
if exist "%ROOT%\HLS\gpt_accel\gpt_accel_prj\solution1\syn\report" xcopy /Y /I "%ROOT%\HLS\gpt_accel\gpt_accel_prj\solution1\syn\report\*.rpt" "%ROOT%\result\hls_reports\"
if exist "%ROOT%\HLS\lenet_accel\myproject_prj\solution1\syn\report" xcopy /Y /I "%ROOT%\HLS\lenet_accel\myproject_prj\solution1\syn\report\*.rpt" "%ROOT%\result\hls_reports\"
if exist "%ROOT%\HLS\lenet_accel\myproject_prj\solution1\impl\ip\component.xml" xcopy /Y /E /I "%ROOT%\HLS\lenet_accel\myproject_prj\solution1\impl\ip" "%ROOT%\vivado2024.2\ip_repo\lenet\" >nul
if exist "%ROOT%\HLS\gpt_accel\gpt_accel_prj\solution1\impl\ip\component.xml" xcopy /Y /E /I "%ROOT%\HLS\gpt_accel\gpt_accel_prj\solution1\impl\ip" "%ROOT%\vivado2024.2\ip_repo\gpt\" >nul
endlocal
