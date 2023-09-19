# Reconfigurable Multiplier

## Table of contents
- [Table of contents](#table-of-contents)
- [Folders organization](#folders-organization)
- [Source files](#source-files)
- [Testbenches](#testbenches)
- [Results](#results)
  - [Setup](#setup)

## Folders organization
This directory contains two sub-directories:
- `rtl`: hosts all system-verilog source files building up the multiplier architecture.
- `tb`: contains different testbench models.
- `results.7z`: contains collected synthesis and post-synthesis metrics. 
- `syn`: contains basic scripts for synthesis.
- `power`: a basic script for power estimation is provided.

This folder further contains the python script `LUT_convert.py`. It defines a series of functions which are useful to convert the multiplier results extracted from the testbench into LUTs header files compatible with AdaPT library.

## Source files
They can be found in the `rtl` directory.
- `HA.sv`: HDL description of a half-adder. Component of the Dadda reduction tree.
- `FA.sv`: HDL description of a full-adder. Instantiated in the Dadda reduction tree.
- `mul_9x9_signed_bw.sv`: HDL description of the reconfigurable 9x9 multiplier.
- `appr_wrapper.sv`: set of registers to wrap the multiplier architecture in order to synchronize its inputs and the produced outputs.

## Testbenches
The folder `tb` contains some useful testbenches to test the functionality of the DUT and extract meaningful information.
- `tb.sv`: testbench used to run randomized simulation of the combinational multiplier architecture alone.
- `tb_all_comb.sv`: save to txt file a LUT containing all results for all inputs combinations and all approximation levels, given a precision level. This initial txt LUT files will then be used to build a high-level model of the multiplier.  
- `tb_sync_rec.sv`: this testbench is the one used to perform the switching activity annotatin and post-synthesis power estimation using Synopsys DC. 
It instantiates the entire wrapper and provides 100000 pseudo-random inputs. 
The number of simulation cycles (`N_SAMPLES`), the effective precision of the inputs (`EFF_MAC_IN_WIDTH_A`, `EFF_MAC_IN_WIDTH_B`) are defined as SystemVerilog parameters, while the approximation level can be redefined by the user. 


## Results
All relevant results are contained in the zip file `results.7z`.
On Linux use the following command to extract the contained folders in the current directory:
``` bash
  7z x results.7z
```
The folder contains three sub-folders organized as:
- `results_synthesis_approximate_multiplier`: contains the area and timing report extracted from the synthesis procedure.
- `results_power_approximate_multiplier`: contains post-synthesis power estimation for all 256 approximate levels in the full-precision 9-bit inputs configuration.
- `results_multiplier_designware`: contains area, timing, resource and power reports for a 9-bit inputs multiplier described in SystemVerilog using the `*` operator. 

### Setup
All RTL simulations have been performed using Siemens QuestaSim. 
In order to replicate our results, the tech library UMC 65nm has been used to synthesize our design on Synopsys Design Compiler (DC), with a clock period of 2ns.

The folder `syn` contains two scripts:
- `syn.scr`: commands to perform the synthesis in Synopsys DC with the set constraints. Reports are automatically saved
- `run_syn.sh`: this script simply calls the above one. It can be simply run using the command `source run_syn.sh`. Notice that Synopsys DC environment has to be previously set to correctly run this script.

The following code snippet provides an example of how testbench parameters and variables can be redefined when launching QuestaSim simulation. 
Furthermore it shows how to create a vcd file and store into it only the needed wafermors (i.e. the multiplier signals). 

```
 vsim -c -voptargs=+acc -t 1ns work.tb +nowarnTSCALE -sdftyp /tb/dut=../syn/netlist/appr_mul.sdf  +APPROX_LEVEL=$appr_level -GEFF_MAC_IN_WID    TH_A=$a_w -GEFF_MAC_IN_WIDTH_B=$b_w ;
vcd file ../power/vcd/approx_mul_${a_w}x${b_w}_${appr_level}.vcd
vcd add /tb/dut/mul_9x9_appr_i/*

 ```


Post-synthesis power estimation is performed with Synopsys DC using as a testbench the file  `tb_sync_rec.sv` and the vcd dump collected through QuestaSim, as shown above.

The folder `power` contains scripts to run the power estimation:
- `power.scr`: power estimation using switching activity collected in simulation. Automatically stores the power report.
- `run_power.sh`: call the previous script. It can be run using the command `source run_power.sh`. Before running it be sure to have your environment set to run Synopsys DC. The script automatically converts vcd dumps into saif format. 


