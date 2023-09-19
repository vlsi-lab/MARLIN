# `pulp-mod`
The repository version employed is: 
 https://github.com/pulp-platform/pulp.git commit b6ae54700b76395b049742ebfc52c5aaf6e148a5 (master) 

## Paths
-`tb_pulp`: `pulp/rtl/tb/`
-`compile.tcl`: `pulp/rtl/tb/`
-`vsim.tcl`: `pulp/sim/tcl_files/config/`


## Changes
-`tb_pulp`: modified to simulate with HYPER-FLASH and HYPER-RAM models, to be properly retrieved. 
-`export_vcd.tcl`: to be copied in `pulp/sim/tcl_files/config/`. Support automatic saving of vcd files for power estimation.
-`run.tcl`: can be found at `pulp/sim/tcl_files/`. The library for post-synthesis simulation has to be added.
-`vsim.tcl`: customization to run `export_vcd`. 

**Qui io rimanderei a seguire tutte le istruzioni di settaggio nelle relative repository, se no non si finisce più**