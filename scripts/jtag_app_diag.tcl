set script_dir [file normalize [file dirname [info script]]]
set root_dir [file normalize [file join $script_dir ..]]
set local_tmp [file join $root_dir .tmp]
file mkdir $local_tmp
set ::env(TEMP) $local_tmp
set ::env(TMP) $local_tmp
set elf_file [file join $root_dir result xsa_bit_elf nanogpt_ps_app.elf]

puts "ELF: $elf_file"
connect -url tcp:127.0.0.1:3121
catch {targets} t0
puts $t0
targets -set -nocase -filter {name =~ "*Cortex-A53 #0*" || name =~ "*A53*#0"}
catch {stop}
catch {rst -processor -clear-registers}
after 1000
puts "Targets before dow:"
catch {targets} t1
puts $t1
dow $elf_file
puts "Registers immediately after dow:"
catch {rrd} r0
puts $r0
puts "Starting ELF for 5 seconds..."
con
after 5000
catch {stop} stop_err
puts "stop result: $stop_err"
puts "Targets after stop:"
catch {targets} t2
puts $t2
puts "Registers after 5 seconds:"
catch {rrd} r1
puts $r1
exit
