set script_dir [file normalize [file dirname [info script]]]
set root_dir [file normalize [file join $script_dir ../..]]
set prj_dir [file join $root_dir vivado2024.2 soc_prj]
file mkdir $prj_dir
create_project nanogpt_soc $prj_dir -part xczu7ev-ffvc1156-2-i -force
set_property board_part "" [current_project]
source [file join $script_dir create_bd.tcl]
