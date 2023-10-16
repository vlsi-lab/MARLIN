# build the entire hierarchy calling vendor tool
MAKE                       = make
PYTHON = python
VENDOR = ./util/vendor.py
BASE_PATH = ./riscv_characterization/vendor/
FILE_LIST = dory.vendor.hjson nemo.vendor.hjson pulp-riscv-gnu-toolchain.vendor.hjson pulp-sdk.vendor.hjson pulp.vendor.hjson
build:
	$(foreach repo, $(FILE_LIST), $(PYTHON) $(VENDOR) $(BASE_PATH)$(repo) -v;)
update:

