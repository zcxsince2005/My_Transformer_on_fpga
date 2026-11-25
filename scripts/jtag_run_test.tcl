set script_dir [file normalize [file dirname [info script]]]
set root_dir [file normalize [file join $script_dir ..]]

set mode "main"
if {[llength $argv] > 0} {
  set mode [lindex $argv 0]
}

puts "jtag_run_test.tcl compatibility wrapper"
puts "Project: $root_dir"
puts "Mode: $mode"

if {$mode eq "main" || $mode eq "full-init" || $mode eq "app"} {
  puts "Launching stable V4 main application flow via scripts/run_main_elf.tcl"
  source [file join $script_dir run_main_elf.tcl]
  exit 0
}

puts "ERROR: unsupported mode: $mode"
puts "Supported modes: main, full-init, app"
puts "Use scripts/run_main_elf.tcl directly for normal board programming."
exit 1
