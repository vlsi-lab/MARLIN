# Reconfigurable Multiplier
This directory contains two sub-directories:
- `rtl`: hosts all system-verilog source files building up the multiplier architecture.
- `tb`: contains different testbench models.
** Se lo mettiamo dire che contiene anche i report di potenza e/o netlis post-sintesi. **

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
It instantiates the entire wrapper and provides pseudo-random inputs. 
The number of simulation cycles (`N_SAMPLES`), the effective precision of the inputs (`EFF_MAC_IN_WIDTH_A`, `EFF_MAC_IN_WIDTH_B`) are defined as system verilog parameters, while the approximation level can be redefined by the user. 
