```
# This file is part of the dune-xt project:
#   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
# Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
# License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
#      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
#          with "runtime exception" (http://www.dune-project.org/license.html)
# Authors:
```

---
jupytext:
  text_representation:
   format_name: myst
jupyter:
  jupytext:
    cell_metadata_filter: -all
    formats: ipynb,myst
    main_language: python
    text_representation:
      format_name: myst
      extension: .md
      format_version: '1.3'
      jupytext_version: 1.11.2
kernelspec:
  display_name: Python 3
  name: python3
---

```{try_on_binder}
```

```{code-cell}
:tags: [remove-cell]
:load: myst_code_init.py
```

# Example: Grid adaptation

```{code-cell}
# wurlitzer: display dune's output in the notebook
%load_ext wurlitzer
%matplotlib notebook

import numpy as np
np.warnings.filterwarnings('ignore') # silence numpys warnings
```

```{code-cell}
from dune.xt.grid import Dim, Simplex, make_cube_grid, visualize_grid

grid = make_cube_grid(Dim(2), Simplex(), [-1, -1], [1, 1], [1, 1])
print('initial grid')
_ = visualize_grid(grid)
print(f'grid has {grid.size(0)} elements')

# we require one global refinement for simlexgrids to obtain a symmetric grid
grid.global_refine(1)

print('once refined to obtain a symmetric grid')
_ = visualize_grid(grid)

print(f'grid has {grid.size(0)} elements')
```

```{code-cell}
from dune.gdt import ContinuousLagrangeSpace, DiscreteFunction, AdaptationHelper

V_h = ContinuousLagrangeSpace(grid, order=1)
u_h = DiscreteFunction(V_h, name='u_h')

print(f'space has {V_h.num_DoFs} DoFs')
```

```{code-cell}
adaptation_helper = AdaptationHelper(grid)
adaptation_helper.append(V_h, u_h)
```

```{code-cell}
markers = np.array(adaptation_helper.markers, copy=False) # direct access to dune vector without copy
centers = np.array(grid.centers(0), copy=False)

elements_in_the_left_half = np.where(centers[:, 0] < 0)[0]
markers[elements_in_the_left_half] = 1
adaptation_helper.mark()
```

Lets have a look at the markers:

```{code-cell}
print(markers)
```

```{code-cell}
adaptation_helper.pre_adapt()
adaptation_helper.adapt()
adaptation_helper.post_adapt()
```

```{code-cell}
print(f'grid has {grid.size(0)} elements')

_ = visualize_grid(grid)

print(f'space has {V_h.num_DoFs} DoFs')
```

Let us now have another look at the markers:

```{code-cell}
print(markers)
print(len(markers))
print(grid.size(0))
```

Since the vector of markers has been resized along with the grid, the wrapped numpy array now contains garbage and/or is of wrong size.

**So after each adapt, we need to wrap it anew!**

```{code-cell}
markers = np.array(adaptation_helper.markers, copy=False)

print(markers)
```

## keeping track of refined elements

We can also tell the helper to keep track of those elements which have been refined. Therefore we mark the elements in the left half again. **Note that we also need to wrap the centers again!**

```{code-cell}
centers = np.array(grid.centers(0), copy=False)

elements_in_the_left_half = np.where(centers[:, 0] > 0)[0]
markers[elements_in_the_left_half] = 1
print(markers)
adaptation_helper.mark()
```

After marking we adapt the grid as before, but use the optional `indicate_new_elements` parameter.

```{code-cell}
adaptation_helper.pre_adapt()
adaptation_helper.adapt()
adaptation_helper.post_adapt(indicate_new_elements=True)
```

```{code-cell}
_ = visualize_grid(grid)
```

The grid was adapted as expected, but in addition ...

```{code-cell}
markers = np.array(adaptation_helper.markers, copy=False)

print(markers)
```

... `adaptation_helper.markers` has not been cleared after adaptation but rather indicates the indices of those elements that have been created newly during the last call to `adapt()`.
In particular, we can use this to refine the same elements again:

```{code-cell}
adaptation_helper.mark()
adaptation_helper.pre_adapt()
adaptation_helper.adapt()
adaptation_helper.post_adapt(indicate_new_elements=True)
```

```{code-cell}
_ = visualize_grid(grid)
```

Download the code:
{download}`example__simple_grid_adaptation.md`
{nb-download}`example__simple_grid_adaptation.ipynb`
