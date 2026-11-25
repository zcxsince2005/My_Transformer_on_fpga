set script_dir [file normalize [file dirname [info script]]]
set root_dir [file normalize [file join $script_dir ../..]]
set ws [file join $root_dir vitis2024.2 workspace]
set xsa [file join $root_dir vivado2024.2 soc_hw nanogpt_zynqmp.xsa]
if {![file exists $xsa]} {
  set xsa [file join $root_dir result xsa_bit_elf nanogpt_zynqmp.xsa]
}
if {![file exists $xsa]} {
  puts "ERROR: XSA not found. Run scripts/run_vivado.bat first."
  exit 1
}

set app_src [file join $root_dir vitis2024.2 src]
set out_dir [file join $root_dir result xsa_bit_elf]
file mkdir $out_dir

setws $ws
catch {platform remove nanogpt_platform}
platform create -name nanogpt_platform -hw $xsa -proc psu_cortexa53_0 -os standalone
platform active nanogpt_platform
platform generate

catch {app remove ai_menu_app}
set app_create_rc [catch {app create -name ai_menu_app -platform nanogpt_platform -domain standalone_domain -template {Empty Application}} app_create_msg]
if {$app_create_rc != 0} {
  puts "WARNING: Vitis app create failed, using existing/manual app directory."
  puts $app_create_msg
}
catch {importsources -name ai_menu_app -path $app_src -soft-link}
set generated_src [file join $ws ai_menu_app src]
file mkdir $generated_src
foreach f [glob -nocomplain -directory $app_src *] {
  if {[file isfile $f]} {
    file copy -force $f $generated_src
  }
}
set ide_build_rc [catch {app build ai_menu_app} ide_build_msg]
if {$ide_build_rc != 0} {
  puts "WARNING: Vitis app build failed, falling back to manual GCC build."
  puts $ide_build_msg
}

set elf [file join $ws ai_menu_app Debug ai_menu_app.elf]
if {![file exists $elf]} {
  set manual_dir [file join $ws ai_menu_manual_build]
  file mkdir $manual_dir

  set gcc [auto_execok aarch64-none-elf-gcc]
  if {$gcc eq ""} {
    foreach p [list \
      {G:/Vivado2024.2/Vitis/2024.2/gnu/aarch64/nt/aarch64-none/bin/aarch64-none-elf-gcc.exe} \
      {C:/Xilinx/Vitis/2024.2/gnu/aarch64/nt/aarch64-none/bin/aarch64-none-elf-gcc.exe} \
      {D:/Xilinx/Vitis/2024.2/gnu/aarch64/nt/aarch64-none/bin/aarch64-none-elf-gcc.exe}] {
      if {[file exists $p]} {
        set gcc $p
        break
      }
    }
  }
  if {$gcc eq ""} {
    puts "ERROR: aarch64-none-elf-gcc not found."
    exit 1
  }

  set domain_dir [file join $ws nanogpt_platform export nanogpt_platform sw nanogpt_platform standalone_domain]
  set inc_dir [file join $domain_dir bspinclude include]
  set lib_dir [file join $domain_dir bsplib lib]
  set lscript [file join $ws ai_menu_app src lscript.ld]
  set c_files {main.c menu.c uart_io.c lenet_demo.c lenet_perf.c gpt_tokenizer.c gpt_hw_accel.c gpt_runtime.c gpt_dialog.c gpt_perf.c resource_info.c}
  set objs {}
  set log_file [file join $out_dir vitis_manual_build.log]
  set log_fp [open $log_file w]

  foreach c $c_files {
    set src_file [file join $app_src $c]
    set obj_file [file join $manual_dir [file rootname $c].o]
    set cmd [list $gcc -O2 -g3 -Wall -Wextra -U__clang__ -I$app_src -I$inc_dir -c $src_file -o $obj_file]
    puts $log_fp $cmd
    set rc [catch {exec -ignorestderr {*}$cmd} output]
    puts $log_fp $output
    if {$rc != 0} {
      close $log_fp
      puts "ERROR: manual compile failed for $c. Log: $log_file"
      exit 1
    }
    lappend objs $obj_file
  }

  set elf [file join $manual_dir nanogpt_ps_app.elf]
  set cmd [concat [list $gcc -Wl,-T -Wl,$lscript -L$lib_dir -o $elf] $objs [list -Wl,--start-group -lxil -lgcc -lc -lm -Wl,--end-group]]
  puts $log_fp $cmd
  set rc [catch {exec -ignorestderr {*}$cmd} output]
  puts $log_fp $output
  close $log_fp
  if {$rc != 0} {
    puts "ERROR: manual link failed. Log: $log_file"
    exit 1
  }
}
if {![file exists $elf]} {
  set elf [file join $ws ai_menu_app build ai_menu_app.elf]
}
if {[file exists $elf]} {
  file copy -force $elf [file join $out_dir nanogpt_ps_app.elf]
  puts "ELF copied to [file join $out_dir nanogpt_ps_app.elf]"
} else {
  puts "ERROR: ELF not found after build."
  exit 1
}
