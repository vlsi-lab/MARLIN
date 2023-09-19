#!/bin/bash

#set TB "vopt_tb -L models_lib -L vip_lib -L /software/dk/nangate45/verilog/msim6.2g"
#set TB "vopt_tb -L models_lib -L vip_lib"
#set TB "vopt_tb -L models_lib -L vip_lib -L /software/dk/nangate45/verilog/msim6.2g -sdftyp /tb_pulp/i_dut/cluster_domain_i/cluster_i/CORE[0]/core_region_i/CL_CORE/RISCV_CORE/=/home/flavia.guella/PULP/pulp_vip_syn/working_dir/cv32e40p_pulpissimo_postsyn/approx_riscy.sdf "
#set TB "vopt_tb -L models_lib -L vip_lib -L /software/dk/nangate45/verilog/msim6.2g -sdftyp /tb_pulp/i_dut/cluster_domain_i/cluster_i/CORE[0]/core_region_i/CL_CORE/RISCV_CORE/riscv_core_int_i/=/home/flavia.guella/PULP/pulp_vip_syn/working_dir/cv32e40p_pulpissimo_syn/syn_cluster_param/netlist/approx_riscy_latch_56.sdf "
#set TB "vopt_tb -L models_lib -L vip_lib -L /software/dk/umc65/Core-lib_LL_Multi-Voltage_Reg.Vt/synopsys/uk65lscllmvbbr_120c25_tc.dc -sdftyp /tb_pulp/i_dut/cluster_domain_i/cluster_i/CORE[0]/core_region_i/CL_CORE/RISCV_CORE/riscv_core_int_i/=/home/flavia.guella/PULP/pulp_vip_syn/working_dir/cv32e40p_pulpissimo_syn/syn_new_lib/netlist/approx_riscy_latch_56.sdf "
set TB "vopt_tb -L models_lib -L vip_lib -L /software/dk/umc65/Core-lib_LL_Multi-Voltage_Reg.Vt/synopsys/uk65lscllmvbbr_120c25_tc.dc"

#set TB "vopt_tb +neg_tcheck -L models_lib -L vip_lib -L /software/dk/nangate45/verilog/msim6.2g -sdftyp /tb_pulp/i_dut/cluster_domain_i/cluster_i/CORE[0]/core_region_i/CL_CORE/RISCV_CORE/riscv_core_int_i/=/home/flavia.guella/PULP/pulp_vip_syn/working_dir/cv32e40p_pulpissimo_syn/syn_cluster_param/netlist/approx_riscy_nolatch_10.sdf "
#set TB "vopt_tb -L models_lib -L vip_lib -L /software/dk/umc65/Core-lib_LL_Multi-Voltage_Reg.Vt/synopsys/uk65lscllmvbbr_120c25_tc.db -sdftyp /tb_pulp/i_dut/cluster_domain_i/cluster_i/CORE[0]/core_region_i/CL_CORE/RISCV_CORE/=/home/flavia.guella/PULP/pulp_vip_syn/working_dir/cv32e40p_pulpissimo_postsyn/syn_new/approx_riscy.sdf"
#set TB "vopt_tb -L models_lib -L vip_lib -L /software/dk/umc65/Core-lib_LL_Multi-Voltage_Reg.Vt/synopsys/uk65lscllmvbbr_120c25_tc.db"

#set TB "vopt_tb -L models_lib -L vip_lib -L /software/dk/umc65/Core-lib_LL_Multi-Voltage_Reg.Vt/synopsys/uk65lscllmvbbr_120c25_tc.db  -sdftyp /tb_pulp/i_dut/cluster_domain_i/cluster_i/CORE[0]/core_region_i/CL_CORE/RISCV_CORE/riscv_core_int_i/=/home/flavia.guella/PULP/pulp_vip_syn/working_dir/cv32e40p_pulpissimo_syn/syn_new_lib/netlist/approx_riscy_nolatch_2.45892735.sdf "
#set TB "vopt_tb -L models_lib -L vip_lib -L /software/dk/umc65/Core-lib_LL_Multi-Voltage_Reg.Vt/synopsys/uk65lscllmvbbr_120c25_tc.db -sdftyp /tb_pulp/i_dut/cluster_domain_i/cluster_i/CORE[0]/core_region_i/CL_CORE/RISCV_CORE/riscv_core_int_i/=/home/flavia.guella/PULP/pulp_vip_syn/working_dir/cv32e40p_pulpissimo_postsyn/syn_new/approx_riscy.sdf "
#vcd add /tb_pulp/i_dut/cluster_domain_i/cluster_i/CORE[0]/core_region_i/CL_CORE/RISCV_CORE/*

source ./tcl_files/config/vsim.tcl
