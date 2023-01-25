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
    from matplotlib.colors import Colormap

    DEFAULT_COLOR_MAP = get_cmap('viridis')

    class NumpyPlot(k3dPlot):

        def __init__(self,
                     vertices,
                     indices,
                     data,
                     color_attribute_name='Data',
                     color_map=k3d.basic_color_maps.CoolWarm,
                     interactive=False,
                     *args,
                     **kwargs):
            super().__init__(*args, **kwargs)

            self.idx = 0

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

    def grid_plot(gridprovider, color_attribute_name, interactive=False, color_map=DEFAULT_COLOR_MAP):
        ''' Generate a k3d Plot and associated controls for VTK data from file

        :param gridprovider: a GridProvider object
        :param color_attribute_name: which data array from vtk to use for plot coloring
        :param color_map: a Matplotlib Colormap object or a K3D array((step, r, g, b))
        :return: the generated Plot object
        '''
        if isinstance(color_map, Colormap):
            color_map = [(x, *color_map(x)[:3]) for x in np.linspace(0, 1, 256)]

        vertices = np.array(gridprovider.centers(codim=2), copy=False).astype(np.float32)
        if gridprovider.dimension == 2:
            # k3d does not support 2D meshes, so pad Z direction with zeros
            vertices = np.hstack((vertices, np.zeros((vertices.shape[0], 1), dtype=np.float32)))

        indices = gridprovider.subentity_indices(codim=2)
        data = np.linspace(0, 1, vertices.shape[0], dtype=np.float32)

        # TODO fix hardcoded bounds
        combined_bounds = np.array([0, 0, 0, 1, 1, 1], dtype=np.float32)

        plot = NumpyPlot(vertices,
                         indices,
                         data,
                         color_attribute_name=color_attribute_name,
                         grid_auto_fit=False,
                         camera_auto_fit=False,
                         color_map=color_map,
                         grid=combined_bounds,
                         interactive=interactive)
        plot.grid_visible = True
        plot.menu_visibility = not interactive
        plot.camera = plot.get_auto_camera(yaw=0, pitch=0, bounds=combined_bounds, factor=0.5)
        plot.menu_visibility = True
        return plot
