set script_dir [file normalize [file dirname [info script]]]
set root_dir [file normalize [file join $script_dir ../..]]
set lenet_ip [file join $root_dir vivado2024.2 ip_repo lenet]
set gpt_ip [file join $root_dir vivado2024.2 ip_repo gpt]
set ps_cfg_tcl [file join $script_dir ps_config_from_mydemo.tcl]

if {![file exists [file join $lenet_ip component.xml]]} {
  puts "ERROR: LeNet IP not found: $lenet_ip"
  puts "Copy or export xilinx.com:hls:myproject_axi:1.0 into vivado2024.2/ip_repo/lenet first."
  return -code error "missing LeNet IP"
}
if {![file exists [file join $gpt_ip component.xml]]} {
  puts "ERROR: GPT IP not found: $gpt_ip"
  puts "Run scripts/run_hls.bat first."
  return -code error "missing GPT IP"
}

set_property ip_repo_paths [list $lenet_ip $gpt_ip] [current_project]
update_ip_catalog

set lenet_vlnv "xilinx.com:hls:myproject_axi:1.0"
set gpt_vlnv "xilinx.com:hls:gpt_accel:1.0"
if {[llength [get_ipdefs -all $lenet_vlnv]] == 0} {
  return -code error "Vivado cannot find LeNet IP VLNV $lenet_vlnv in $lenet_ip"
}
if {[llength [get_ipdefs -all $gpt_vlnv]] == 0} {
  return -code error "Vivado cannot find GPT IP VLNV $gpt_vlnv in $gpt_ip"
}
puts "INFO: Using LeNet IP repo: $lenet_ip"
puts "INFO: Using GPT IP repo: $gpt_ip"

create_bd_design "system"

create_bd_cell -type ip -vlnv xilinx.com:ip:zynq_ultra_ps_e:3.5 zynq_ultra_ps_e_0
if {![file exists $ps_cfg_tcl]} {
  return -code error "Missing verified PS configuration: $ps_cfg_tcl"
}
source $ps_cfg_tcl
apply_verified_zynqmp_ps_config [get_bd_cells zynq_ultra_ps_e_0]
set ps_masters [get_property CONFIG.PSU__PROTECTION__MASTERS [get_bd_cells zynq_ultra_ps_e_0]]
set ps_masters [string map {"S_AXI_HP1_FPD:NA;0" "S_AXI_HP1_FPD:NA;1"} $ps_masters]
set_property -dict [list \
  CONFIG.PSU__USE__S_AXI_GP3 {1} \
  CONFIG.PSU__SAXIGP3__DATA_WIDTH {128} \
  CONFIG.PSU__PROTECTION__MASTERS $ps_masters \
] [get_bd_cells zynq_ultra_ps_e_0]
if {[llength [get_bd_intf_pins -quiet zynq_ultra_ps_e_0/S_AXI_HP1_FPD]] == 0} {
  return -code error "PS HP1 port was not created. Check ZynqMP PS HP1 properties."
}

create_bd_cell -type ip -vlnv xilinx.com:ip:proc_sys_reset:5.0 rst_ps8_0_100M
connect_bd_net [get_bd_pins zynq_ultra_ps_e_0/pl_clk0] [get_bd_pins rst_ps8_0_100M/slowest_sync_clk]
connect_bd_net [get_bd_pins zynq_ultra_ps_e_0/pl_resetn0] [get_bd_pins rst_ps8_0_100M/ext_reset_in]

create_bd_cell -type ip -vlnv xilinx.com:ip:smartconnect:1.0 axi_ctrl
set_property -dict [list CONFIG.NUM_SI {1} CONFIG.NUM_MI {2}] [get_bd_cells axi_ctrl]
create_bd_cell -type ip -vlnv xilinx.com:ip:smartconnect:1.0 axi_lenet_ddr
set_property -dict [list CONFIG.NUM_SI {2} CONFIG.NUM_MI {1}] [get_bd_cells axi_lenet_ddr]
create_bd_cell -type ip -vlnv xilinx.com:ip:smartconnect:1.0 axi_gpt_ddr
set_property -dict [list CONFIG.NUM_SI {4} CONFIG.NUM_MI {1}] [get_bd_cells axi_gpt_ddr]

create_bd_cell -type ip -vlnv $lenet_vlnv lenet_accel_0
create_bd_cell -type ip -vlnv $gpt_vlnv gpt_accel_0

connect_bd_net [get_bd_pins zynq_ultra_ps_e_0/pl_clk0] [get_bd_pins axi_ctrl/aclk] [get_bd_pins axi_lenet_ddr/aclk] [get_bd_pins axi_gpt_ddr/aclk]
connect_bd_net [get_bd_pins zynq_ultra_ps_e_0/pl_clk0] [get_bd_pins lenet_accel_0/ap_clk] [get_bd_pins gpt_accel_0/ap_clk]
connect_bd_net [get_bd_pins rst_ps8_0_100M/peripheral_aresetn] [get_bd_pins axi_ctrl/aresetn] [get_bd_pins axi_lenet_ddr/aresetn] [get_bd_pins axi_gpt_ddr/aresetn]
connect_bd_net [get_bd_pins rst_ps8_0_100M/peripheral_aresetn] [get_bd_pins lenet_accel_0/ap_rst_n] [get_bd_pins gpt_accel_0/ap_rst_n]
connect_bd_net [get_bd_pins zynq_ultra_ps_e_0/pl_clk0] [get_bd_pins zynq_ultra_ps_e_0/maxihpm0_lpd_aclk] [get_bd_pins zynq_ultra_ps_e_0/saxihp0_fpd_aclk] [get_bd_pins zynq_ultra_ps_e_0/saxihp1_fpd_aclk]

connect_bd_intf_net [get_bd_intf_pins zynq_ultra_ps_e_0/M_AXI_HPM0_LPD] [get_bd_intf_pins axi_ctrl/S00_AXI]
connect_bd_intf_net [get_bd_intf_pins axi_ctrl/M00_AXI] [get_bd_intf_pins lenet_accel_0/s_axi_control]
connect_bd_intf_net [get_bd_intf_pins axi_ctrl/M01_AXI] [get_bd_intf_pins gpt_accel_0/s_axi_control]

connect_bd_intf_net [get_bd_intf_pins lenet_accel_0/m_axi_gmem0] [get_bd_intf_pins axi_lenet_ddr/S00_AXI]
connect_bd_intf_net [get_bd_intf_pins lenet_accel_0/m_axi_gmem1] [get_bd_intf_pins axi_lenet_ddr/S01_AXI]
connect_bd_intf_net [get_bd_intf_pins axi_lenet_ddr/M00_AXI] [get_bd_intf_pins zynq_ultra_ps_e_0/S_AXI_HP0_FPD]

connect_bd_intf_net [get_bd_intf_pins gpt_accel_0/m_axi_gmem0] [get_bd_intf_pins axi_gpt_ddr/S00_AXI]
connect_bd_intf_net [get_bd_intf_pins gpt_accel_0/m_axi_gmem1] [get_bd_intf_pins axi_gpt_ddr/S01_AXI]
connect_bd_intf_net [get_bd_intf_pins gpt_accel_0/m_axi_gmem2] [get_bd_intf_pins axi_gpt_ddr/S02_AXI]
connect_bd_intf_net [get_bd_intf_pins gpt_accel_0/m_axi_gmem3] [get_bd_intf_pins axi_gpt_ddr/S03_AXI]
connect_bd_intf_net [get_bd_intf_pins axi_gpt_ddr/M00_AXI] [get_bd_intf_pins zynq_ultra_ps_e_0/S_AXI_HP1_FPD]

assign_bd_address -offset 0x80000000 -range 0x00010000 [get_bd_addr_segs lenet_accel_0/s_axi_control/Reg]
assign_bd_address -offset 0x80010000 -range 0x00010000 [get_bd_addr_segs gpt_accel_0/s_axi_control/Reg]
assign_bd_address

validate_bd_design
save_bd_design
set bd_file [get_files [file join [get_property DIRECTORY [current_project]] nanogpt_soc.srcs sources_1 bd system system.bd]]
set_property synth_checkpoint_mode None $bd_file
generate_target all $bd_file
export_ip_user_files -of_objects $bd_file -no_script -sync -force -quiet
set wrapper_file [make_wrapper -files $bd_file -top]
add_files -norecurse $wrapper_file
