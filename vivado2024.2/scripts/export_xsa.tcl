set script_dir [file normalize [file dirname [info script]]]
set root_dir [file normalize [file join $script_dir ../..]]
set prj [file join $root_dir vivado2024.2 soc_prj nanogpt_soc.xpr]
file mkdir [file join $root_dir vivado2024.2 soc_hw]
file mkdir [file join $root_dir result xsa_bit_elf]
set xsa_file [file join $root_dir vivado2024.2 soc_hw nanogpt_zynqmp.xsa]

set routed_dcp [file join $root_dir result vivado_reports system_wrapper_post_route.dcp]
if {[file exists $routed_dcp]} {
  open_checkpoint $routed_dcp
} else {
  open_project $prj
  open_run impl_1
}
write_hw_platform -fixed -include_bit -force -file $xsa_file
file copy -force $xsa_file [file join $root_dir result xsa_bit_elf nanogpt_zynqmp.xsa]
