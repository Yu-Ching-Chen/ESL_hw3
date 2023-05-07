use_hls_lib "memlib"
#*******************************************************************************
# Copyright 2015 Cadence Design Systems, Inc.
# All Rights Reserved.
#
#*******************************************************************************
#
# Stratus Project File
#
############################################################
# Project Parameters
############################################################
#
# Technology Libraries
#
set LIB_PATH "[get_install_path]/share/stratus/techlibs/GPDK045/gsclib045_svt_v4.4/gsclib045/timing"
set LIB_LEAF "slow_vdd1v2_basicCells.lib"
use_tech_lib    "$LIB_PATH/$LIB_LEAF"

#
# Global synthesis attributes.
#
set_attr clock_period           10.0
set_attr message_detail         3
#set_attr default_input_delay    0.1
#set_attr sched_aggressive_1 on
#set_attr unroll_loops on
#set_attr flatten_arrays all
#set_attr timing_aggression 0
#set_attr default_protocol true

#
# Simulation Options
#
### 1. You may add your own options for C++ compilation here.
set_attr cc_options             "-DCLOCK_PERIOD=10.0 -g"
#enable_waveform_logging -vcd
set_attr end_of_sim_command "make saySimPassed"

#
# Testbench or System Level Modules
#
### 2. Add your testbench source files here.
define_system_module main ../main.cpp
define_system_module System ../Testbench.cpp
define_system_module tb ../System.cpp

#
# SC_MODULEs to be synthesized
#
### 3. Add your design source files here (to be high-level synthesized).
define_hls_module Filter ../Filter.cpp

#
# Simulation Configurations
#
use_systemc_simulator builtin
use_verilog_simulator xcelium ;# 'xcelium' or 'vcs'
enable_waveform_logging -vcd ;# to store signal transitions vcd, shm (for Xcelium), or fsdb
set_systemc_options -version 2.3 -gcc 6.3 ;# enables C++14
set_attr end_of_sim_command "make cmp_result"   ;# Make rule to run at end of each simulation

### 4. Define your HLS configuration (arbitrary names, BASIC and DPA in this example).
define_hls_config Filter BASIC
define_hls_config Filter DPA       --dpopt_auto=op,expr

set IMAGE_DIR           ".."
set IN_FILE_NAME1       "${IMAGE_DIR}/lena_color_256_noise.bmp"
set OUT_FILE_NAME1		"lena_color_256_noise_out.bmp"
set IN_FILE_NAME2       "${IMAGE_DIR}/peppers_color_noise.bmp"
set OUT_FILE_NAME2		"peppers_color_noise_out.bmp"

### 5. Define simulation configuration for each HLS configuration
### 5.1 The behavioral simulation (C++ only).
define_sim_config B1 -argv "$IN_FILE_NAME1 $OUT_FILE_NAME1"
define_sim_config B2 -argv "$IN_FILE_NAME2 $OUT_FILE_NAME2"
### 5.2 The Verilog simulation for HLS config "BASIC".
define_sim_config V_BASIC1 "Filter RTL_V BASIC" -argv "$IN_FILE_NAME1 $OUT_FILE_NAME1"
define_sim_config V_BASIC2 "Filter RTL_V BASIC" -argv "$IN_FILE_NAME2 $OUT_FILE_NAME2"
### 5.3 The Verilog simulation for HLS config "DPA".
define_sim_config V_DPA1 "Filter RTL_V DPA" -argv "$IN_FILE_NAME1 $OUT_FILE_NAME1"
define_sim_config V_DPA2 "Filter RTL_V DPA" -argv "$IN_FILE_NAME2 $OUT_FILE_NAME2"
