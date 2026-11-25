set script_dir [file normalize [file dirname [info script]]]
set root_dir [file normalize [file join $script_dir ..]]
set local_tmp [file join $root_dir .tmp]
file mkdir $local_tmp
set ::env(TEMP) $local_tmp
set ::env(TMP) $local_tmp

set bit_file [file join $root_dir result xsa_bit_elf nanogpt_zynqmp.bit]
set app_elf [file join $root_dir result xsa_bit_elf nanogpt_ps_app.elf]
set fsbl_elf [file join $root_dir vitis2024.2 workspace nanogpt_platform export nanogpt_platform sw nanogpt_platform boot fsbl.elf]
set xsa_file [file join $root_dir result xsa_bit_elf nanogpt_zynqmp.xsa]

puts "BIT: $bit_file"
puts "APP ELF: $app_elf"
puts "FSBL: $fsbl_elf"
puts "XSA: $xsa_file"

foreach required [list $bit_file $app_elf $fsbl_elf $xsa_file] {
  if {![file exists $required]} {
    puts "ERROR: missing required file: $required"
    exit 1
  }
}

proc select_a53_0 {} {
  targets -set -nocase -filter {name =~ "*Cortex-A53 #0*" || name =~ "*A53*#0"}
}

puts "Connecting to hw_server..."
if {[catch {connect -url tcp:127.0.0.1:3121} err]} {
  puts "WARN: existing hw_server connection failed: $err"
  connect
}

set zynqmp_utils {G:/Vivado2024.2/Vitis/2024.2/scripts/vitis/util/zynqmp_utils.tcl}
if {[file exists $zynqmp_utils]} {
  source $zynqmp_utils
}

puts "Loading hardware description..."
targets -set -nocase -filter {name =~ "APU*"}
loadhw -hw $xsa_file -mem-ranges [list {0x80000000 0xbfffffff} {0x400000000 0x5ffffffff} {0x1000000000 0x7fffffffff}] -regs
configparams force-mem-access 1

puts "Programming bitstream..."
fpga -file $bit_file
puts "Bitstream programmed."

puts "Selecting Cortex-A53 #0 for FSBL..."
select_a53_0
catch {stop}
catch {rst -processor}
after 1000

puts "Downloading FSBL..."
dow $fsbl_elf
catch {bpremove -all}
set fsbl_bp ""
if {[catch {bpadd -addr &XFsbl_Exit} fsbl_bp_err]} {
  puts "WARN: bpadd &XFsbl_Exit failed: $fsbl_bp_err"
  if {[catch {bpadd -addr 0xfffccb20} fsbl_bp_err2]} {
    puts "WARN: bpadd fixed FSBL exit address failed: $fsbl_bp_err2"
  } else {
    set fsbl_bp $fsbl_bp_err2
  }
} else {
  set fsbl_bp $fsbl_bp_err
}

puts "Running FSBL for DDR initialization..."
if {[catch {con -block -timeout 60} con_err]} {
  puts "WARN: FSBL con timeout or error: $con_err"
  catch {stop}
}
if {$fsbl_bp ne ""} {
  catch {bpremove $fsbl_bp}
}
catch {bpremove -all}

puts "Selecting Cortex-A53 #0 for application..."
select_a53_0
catch {stop}
catch {rst -processor}
after 1000

puts "Downloading main application ELF..."
dow $app_elf
configparams force-mem-access 0
select_a53_0

puts "Starting main application ELF. Watch UART now."
con
puts "Application start command issued."
exit 0
