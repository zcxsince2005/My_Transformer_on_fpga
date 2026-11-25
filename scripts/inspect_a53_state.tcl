set script_dir [file normalize [file dirname [info script]]]
set root_dir [file normalize [file join $script_dir ..]]
set local_tmp [file join $root_dir .tmp]
file mkdir $local_tmp
set ::env(TEMP) $local_tmp
set ::env(TMP) $local_tmp

proc print_state {} {
  puts "Targets:"
  catch {targets} target_info
  puts $target_info

  puts "Selecting Cortex-A53 #0..."
  if {[catch {targets -set -nocase -filter {name =~ "*Cortex-A53 #0*" || name =~ "*A53*#0"}} err]} {
    puts "ERROR: failed to select Cortex-A53 #0: $err"
    exit 1
  }

  puts "Stopping Cortex-A53 #0..."
  catch {stop} stop_err
  if {$stop_err ne ""} {
    puts "WARN: stop returned: $stop_err"
  }

  puts "Registers:"
  catch {rrd} regs
  puts $regs
}

foreach url [list tcp:127.0.0.1:3121 TCP:127.0.0.1:3121] {
  puts "Connecting to hw_server at $url..."
  if {![catch {connect -url $url} err]} {
    print_state
    exit 0
  }
  puts "WARN: connect failed: $err"
}

puts "ERROR: could not connect to hw_server."
exit 1
