# `repo_changes`
This folder groups all the files that have been modified or added to the employed repositories belonging to `pulp-platform` project, at https://github.com/pulp-platform .
Since changes have been performed to several publicly available repositories, the preferred approach is to provide only the files that have been added with the required instructions to correctly include them in the related github projects.
**NOTE** In the new release all connected repositories have been vendorized. The files contained in the sub-directories are therefore not needed. However, they are still kept in this version so that they can be consulted. 

All the repo with the exact version that has been employed are here reported:
- `pulp-riscv-gnu-toolchain`: https://github.com/pulp-platform/pulp-riscv-gnu-toolchain.git commit 49dea1915979fdf91eb2ad4a8873e4cd88a6b811
- `nemo`: https://github.com/pulp-platform/nemo.git commit ed32239efaa256cea061a5426d0c1507c5f005ee (tag: v0.0.8) 
- `dory`: https://github.com/pulp-platform/dory.git commit 0e9011e752476c66c12770cfd283f826bdd5428a
- `cv32e40p`: https://github.com/pulp-platform/cv32e40p.git commit 8d58109ab61e1fb6c9dcbafefb8f3a56ee596427
- `pulp`: https://github.com/pulp-platform/pulp.git commit b6ae54700b76395b049742ebfc52c5aaf6e148a5 (master) 
- `pulp-sdk`: https://github.com/pulp-platform/pulp-sdk.git commit 8b84cbb30604aa01cf0824855d98e55f0d43d7ce 


## Setup
The file `repo_changes.json` stores a dictionary containing, for every of the mentioned repositories, the github address, the commit number, and the list of files modified or added with the relative path where they have to be copied to. 
The listed files are contained in folders named as the original repo plus `-mod`. 

The setup procedure, dependencies and packages required to correctly install the PULP framework are not reported here. Refer to each specific repo to find the exact instructions and workflow to follow.

In the new release, dependencies are managed through [vendor](https://opentitan.org/book/util/doc/vendor.html#utilvendorpy-vendor-in-components).