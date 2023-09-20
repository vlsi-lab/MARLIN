# `cv32e40p-mod` 
The employed repository version is [cv32e40p](https://github.com/pulp-platform/cv32e40p.git), commit 8d58109ab61e1fb6c9dcbafefb8f3a56ee596427.
It is the version that is part of the pulp project.

## Paths

- `riscv_defines.sv`: `cv32e40p/rtl/include/`
- `riscv_core.sv`: `cv32e40p/rtl/`
- `riscv_id_stage.sv`: `cv32e40p/rtl/`
- `riscv_ex_stage.sv`: `cv32e40p/rtl/`
- `riscv_decoder.sv`: `cv32e40p/rtl/`
- `riscv_cs_register.sv`: `cv32e40p/rtl/`
- `riscv_alu_approx.sv`: `cv32e40p/rtl/`

## Changes

- `riscv_defines.sv`: add parameters for the encoding of approximate operations and for the CSR address.
- `riscv_core.sv`: add connections between ID stage and approximate alu in the EXE stage and connection between ID stage and approx CSR in EXE.
- `riscv_id_stage.sv`: add signals to correctly decode and manage approximate ALU.
- `riscv_ex_stage.sv`: add approximate ALU to the building blocks.
- `riscv_decoder.sv`: decoding of instructions to be dispatched to the approx unit. 
- `riscv_cs_register.sv`: new CSR register definition with its fields.
- `riscv_alu_approx.sv`: unit performing approximate multiplication. The main components are 4 approx multiplier whose code is stored in `approximate_multiplier`.
