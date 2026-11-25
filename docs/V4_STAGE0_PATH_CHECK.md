# V4 Path Check

Project root:

`E:\model_on_fpga\nanogpt_on_fpga_hls_V4`

Scope rule:

- Modified project: `nanogpt_on_fpga_hls_V4`
- Reference-only projects: `nanogpt_on_fpga_hls_V1`, `nanogpt_on_fpga_hls_V2`, `nanogpt_on_fpga_hls_V3`, `nanogpt_on_fpga_ps_V1`, `my_demo`

Checks performed:

```powershell
rg -n "GPT_V3|HLS V3|nanogpt_on_fpga_hls_V3|nanogpt_on_fpga_hls_V2|nanogpt_on_fpga_hls_V1" `
  .\nanogpt_on_fpga_hls_V4\vitis2024.2\src `
  .\nanogpt_on_fpga_hls_V4\HLS\gpt_accel `
  .\nanogpt_on_fpga_hls_V4\vivado2024.2\scripts `
  .\nanogpt_on_fpga_hls_V4\scripts
```

Result:

- No source-code or build-script references to V1/V2/V3 project paths were found.
- Some copied log files may contain old path text and should be treated as historical logs only.
- V4 scripts resolve paths relative to `nanogpt_on_fpga_hls_V4`.
- Vivado GPT IP repository points to `HLS\gpt_accel\gpt_accel_prj\solution1\impl\ip`.
- Vitis build uses `vitis2024.2\src` as the authoritative application source.

Key scripts checked:

- `build_all.bat`
- `run_hls.bat`
- `run_vivado.bat`
- `run_vitis_build.bat`
- `scripts\jtag_run_test.tcl`
- `vivado2024.2\scripts\create_bd.tcl`
- `vivado2024.2\scripts\build_bitstream.tcl`
- `vitis2024.2\scripts\build_app.tcl`

