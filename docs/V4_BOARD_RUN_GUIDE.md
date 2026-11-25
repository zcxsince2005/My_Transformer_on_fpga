# V4 Board Run Guide

This guide is for manual board programming and serial validation.

## 1. Clean Stale JTAG Processes

Close Vitis, Vivado Hardware Manager, old XSCT windows, and any serial program that may hold the wrong port.

If XSCT or hw_server is stuck, run:

```powershell
Get-Process | Where-Object { $_.ProcessName -match 'hw_server|cs_server|xsct|xsdb|xvc' } | Stop-Process -Force -ErrorAction SilentlyContinue
```

## 2. Power Cycle

1. Power off the board.
2. Wait at least 5 seconds.
3. Power on the board.

## 3. Open Serial

Use:

- Baud: `115200`
- Data bits: `8`
- Stop bits: `1`
- Parity: none
- Flow control: none

## 4. Run Main Application

```powershell
cd E:\model_on_fpga\nanogpt_on_fpga_hls_V4
G:\Vivado2024.2\Vitis\2024.2\bin\xsct.bat scripts\run_main_elf.tcl
```

Compatibility command:

```powershell
G:\Vivado2024.2\Vitis\2024.2\bin\xsct.bat scripts\jtag_run_test.tcl full-init
```

The compatibility command redirects to `scripts\run_main_elf.tcl`.

Expected menu:

```text
UART INIT DONE
LeNet + tiny GPT FPGA demo

========================================
Zynq MPSoC AI Demo HLS V4
Target: XCZU7EV-FFVC1156-2-I

Menu:
1. Run digit recognition demo
2. Print digit recognition performance
3. Enter tiny GPT dialogue
4. Print last tiny GPT generation performance
5. Print FPGA resource utilization
# Please select:
```

## 5. Validation Sequence

LeNet:

```text
1
```

Expected:

```text
accuracy = 10 / 10
```

GPT dialogue:

```text
3
hello
what is fpga
why use gpu
exit
```

Expected: readable English replies generated from the exported V4 tiny GPT weights.

GPT performance:

```text
4
```

Important lines:

```text
GPT mode: LINEAR_ROLLING_KV
GPT V4 n_embd: 96
GPT V4 n_head: 4
GPT V4 n_layer: 2
GPT V4 mlp_hidden: 384
GPT V4 HLS linear parallel factor: 64
GPT V4 HLS matmul output parallel factor: 4
GPT HLS linear fallbacks: 0
```

Resource report:

```text
5
```

Expected V4 values:

```text
GPT HLS DSP: 204
Full routed DSP: 1026 / 1728
Full routed LUT: 117328 / 230400
Timing met at 150 MHz PL clock
```

## 6. If JTAG Hangs

Stop stale processes, power-cycle the board, and rerun `scripts\run_main_elf.tcl`.

Only use `scripts\run_main_elf.tcl` or the compatibility `full-init` command for normal board programming.
