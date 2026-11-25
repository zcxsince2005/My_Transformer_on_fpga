open_project -reset gpt_accel_csim_prj
set_top gpt_accel
add_files gpt_accel.cpp -cflags "-std=c++17"
add_files -tb gpt_accel_tb.cpp -cflags "-std=c++17"
open_solution -reset solution1
set_part {xczu7ev-ffvc1156-2-i}
create_clock -period 5 -name default
config_compile -name_max_length 80
csim_design
exit
