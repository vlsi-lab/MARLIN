# `pulp-riscv-gnu-toolchain-mod`
The repository version that has been used is: 
[pulp-riscv-gnu-toolchain](https://github.com/pulp-platform/pulp-riscv-gnu-toolchain.git)
commit 49dea1915979fdf91eb2ad4a8873e4cd88a6b811

## Paths

- `riscv-opc.h`: `pulp-riscv-gnu-toolchain/riscv-binutils-gbd/include/opcode/`

This file is updated to include as part of the ISA the new CSR mnemonic with the corresponding address. 
The file `pulp-riscv-gnu-toolchain/riscv-binutils-gbd/opcodes/riscv-opc.c` does not need to be updated in this case as no new instruction is added to the ISA using the proposed methodology.
