#vcd file -compress export.vcd.gz
#vcd add -r -optcells -internal -ports {sim:/tb_pulp/i_dut/*}
set input_idx $::env(INPUT_IDX)
puts "----------------------------------------------------"
puts "               VCD var = $input_idx                 "
puts "----------------------------------------------------"
### For the approx architecture
set REPORT_DIR "reports"
exec ln -s /data/shared/flavia.guella/hermione/syn-pwr-reports $REPORT_DIR

vcd file $REPORT_DIR/core_${input_idx}.vcd $REPORT_DIR/mult_approx_${input_idx}.vcd $REPORT_DIR/mult_exact_${input_idx}.vcd
vcd add -file $REPORT_DIR/core_${input_idx}.vcd {sim:/tb_pulp/i_dut/cluster_domain_i/cluster_i/CORE[0]/core_region_i/CL_CORE/RISCV_CORE/*}
vcd add -file $REPORT_DIR/mult_approx_${input_idx}.vcd {sim:/tb_pulp/i_dut/cluster_domain_i/cluster_i/CORE[0]/core_region_i/CL_CORE/RISCV_CORE/ex_stage_i/alu_approx_i/*}
vcd add -file $REPORT_DIR/mult_exact_${input_idx}.vcd {sim:/tb_pulp/i_dut/cluster_domain_i/cluster_i/CORE[0]/core_region_i/CL_CORE/RISCV_CORE/ex_stage_i/mult_i/*}

### For the exact architecture
#vcd file core_${input_idx}.vcd mult_exact_${input_idx}.vcd
#vcd add -file core_${input_idx}.vcd {sim:/tb_pulp/i_dut/cluster_domain_i/cluster_i/CORE[0]/core_region_i/CL_CORE/RISCV_CORE/*}
#vcd add -file mult_exact_${input_idx}.vcd {sim:/tb_pulp/i_dut/cluster_domain_i/cluster_i/CORE[0]/core_region_i/CL_CORE/RISCV_CORE/ex_stage_i/mult_i/*}

