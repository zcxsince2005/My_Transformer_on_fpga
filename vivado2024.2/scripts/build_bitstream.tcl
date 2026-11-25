set script_dir [file normalize [file dirname [info script]]]
set root_dir [file normalize [file join $script_dir ../..]]
set prj [file join $root_dir vivado2024.2 soc_prj nanogpt_soc.xpr]

if {![file exists $prj]} {
  source [file join $script_dir create_project.tcl]
} else {
  open_project $prj
}

set bd_file [get_files [file join [get_property DIRECTORY [current_project]] nanogpt_soc.srcs sources_1 bd system system.bd]]
if {[llength $bd_file] == 0} {
  return -code error "system.bd not found in project"
}

# Keep the flow in this Vivado process. Some Windows environments block
# Vivado's generated rundef.js scripts, which makes launch_runs hang.
set_property synth_checkpoint_mode None $bd_file
generate_target all $bd_file
export_ip_user_files -of_objects $bd_file -no_script -sync -force -quiet
update_compile_order -fileset sources_1
set_property top system_wrapper [current_fileset]

file mkdir [file join $root_dir result vivado_reports]
file mkdir [file join $root_dir result xsa_bit_elf]
set rpt_dir [file join $root_dir result vivado_reports]

puts "INFO: Running in-process synth_design..."
synth_design -top system_wrapper -part xczu7ev-ffvc1156-2-i
write_checkpoint -force [file join $rpt_dir system_wrapper_synth.dcp]
report_utilization -file [file join $rpt_dir utilization_synth.rpt]

puts "INFO: Running opt_design..."
opt_design
write_checkpoint -force [file join $rpt_dir system_wrapper_opt.dcp]

puts "INFO: Running place_design..."
place_design
write_checkpoint -force [file join $rpt_dir system_wrapper_place.dcp]
report_utilization -file [file join $rpt_dir utilization_placed.rpt]

puts "INFO: Running phys_opt_design..."
catch {phys_opt_design} phys_opt_msg
puts $phys_opt_msg

puts "INFO: Running route_design..."
route_design
write_checkpoint -force [file join $rpt_dir system_wrapper_post_route.dcp]

report_utilization -file [file join $rpt_dir utilization_impl.rpt]
report_timing_summary -file [file join $rpt_dir timing_summary_impl.rpt]
report_power -file [file join $rpt_dir power_impl.rpt]

puts "INFO: Writing bitstream..."
write_bitstream -force [file join $rpt_dir system_wrapper.bit]
file copy -force [file join $rpt_dir system_wrapper.bit] [file join $root_dir result xsa_bit_elf nanogpt_zynqmp.bit]

set ltx_file [file join $rpt_dir system_wrapper.ltx]
if {[file exists $ltx_file]} {
  file copy -force $ltx_file [file join $root_dir result xsa_bit_elf nanogpt_zynqmp.ltx]
}

puts "INFO: Bitstream copied to [file join $root_dir result xsa_bit_elf nanogpt_zynqmp.bit]"
