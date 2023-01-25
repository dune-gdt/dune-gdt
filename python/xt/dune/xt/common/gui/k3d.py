# ~~~
# This file is part of the dune-xt project:
#   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
# Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
# License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
#      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
#          with "runtime exception" (http://www.dune-project.org/license.html)
# Authors:
#   Felix Schindler (2020)
#   René Fritze     (2019)
# ~~~

from dune.xt.common.config import config

if config.HAVE_K3D:
    import k3d
    import numpy as np
    from k3d.plot import Plot as k3dPlot
    from matplotlib.cm import get_cmap

    DEFAULT_COLOR_MAP = get_cmap('viridis')

    class NumpyPlot(k3dPlot):

        def __init__(self,
                     vertices,
                     indices,
                     data,
                     color_map=k3d.basic_color_maps.CoolWarm,
                     interactive=False,
                     *args,
                     **kwargs):
            super().__init__(*args, **kwargs)

            # TODO fix hardcoded bounds
            combined_bounds = np.array([0, 0, 0, 1, 1, 1], dtype=np.float32)

            if 'transform' in kwargs.keys():
                raise RuntimeError('supplying transforms is currently not supported for time series VTK Data')

            self.mesh = k3d.mesh(vertices=vertices,
                                 indices=indices,
                                 color=0x0000FF,
                                 opacity=1.0,
                                 attribute=data,
                                 color_range=(np.nanmin(data), np.nanmax(data)),
                                 color_map=np.array(color_map, np.float32),
                                 wireframe=True,
                                 compression_level=0)
            self += self.mesh

            self.camera_no_pan = not interactive
            self.camera_no_rotate = not interactive
            self.camera_no_zoom = not interactive
            self.grid_visible = True
            self.menu_visibility = not interactive
            self.camera = self.get_auto_camera(yaw=0, pitch=0, bounds=combined_bounds, factor=0.5)
            self.menu_visibility = True
