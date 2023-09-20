# `nemo-mod`
The repository version that was employed is
[nemo](https://github.com/pulp-platform/nemo.git) , commit ed32239efaa256cea061a5426d0c1507c5f005ee (tag: v0.0.8).

## Paths
- `graph.py`: `nemo/`
- `transform.py`: `nemo/`
- `utils.py`: `nemo/`
- `pact.py`: `nemo/quant/`
- `deploy.py`: `nemo/transf/`
- `export.py`: `nemo/transf/`
- `utils.py`: `nemo/transf/` (saved here as `transf-utils.py`)
**Notice: there could be an additional `nemo/` folder in the hierarchy**

## Changes
- `graph.py`: removed unexpected function argument.
- `transform.py`: additional parameters to carry out correct bias quantization.
- `utils.py`: added some line to fix occuring bugs. 
- `pact.py`: additional functions and parameters to support bias quantization + small bugs fixing.
- `deploy.py`: some code to support bias quantization.
- `export.py`: additional code snippets for bias quantization.
- `transf-utils.py`: additional code to support correct bias quantization.

Notice that NEMO has been extended to correctly support bias quantization, producing a model that is compliant with DORY. 
The methodology used for bias quantization is simply specular with respect to the methodoly followed by NEMO for weight quantization.
This is done not to spoil the NEMO methodoly even if it could be a sub-optimal strategy.
However, this feature has not been explored for the purpose of the current work. 
