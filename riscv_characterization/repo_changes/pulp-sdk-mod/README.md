# `pulp-sdk-mod`
The repository version is: 
[pulp-sdk](https://github.com/pulp-platform/pulp-sdk.git),  commit 8b84cbb30604aa01cf0824855d98e55f0d43d7ce

## Paths
- `common.sh` : `pulp-sdk/configs/`
- `default_rules.mk` : `pulp-sdk/rtos/pulpos/common/rules/pulpos/`
- `hyperflash.json` : `pulp-sdk/tools/gap-configs/configs/devices/`
- `runner.json` : `pulp-sdk/tools/gap-configs/configs/tools/runner/`
- `rtl.json` : `pulp-sdk/tools/gapy/configs/platforms/`
- `gen_flash_image.py`: `pulp-sdk/tools/gapy/`
- `flash.mk`: `pulp-sdk/tools/gapy/rules/`
- `rtl_runner.py`: `pulp-sdk/tools/gapy/runner/rtl/`
- `pulp.json`: `pulp-sdk/tools/gapy/targets/`

## Changes
- `hyperflash.json`,`runner.json`, `flash.mk`, `rtl_runner.py`, `pulp.json` : enable hyperflash.
- `rtl.json` : launch vsim simulation.
- `gen_flash_image-py`: save words correctly in the flash.


Most of the modifications made are necessary in order to run simulations using the rtl (either pre or post-synthesis) model, rather than the high-level GVSOC simulator developed by PULP research group.
