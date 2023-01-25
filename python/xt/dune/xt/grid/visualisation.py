import numpy as np

from dune.xt.common.config import config

if config.HAVE_K3D:

    def grid_plot(gridprovider, interactive=False, color_map=None):
        ''' Generate a k3d Plot and associated controls for a gridprovider

        :param gridprovider: a GridProvider object
        :param color_map: a Matplotlib Colormap object or a K3D array((step, r, g, b))
        :return: the generated Plot object
        '''
        from matplotlib.colors import Colormap

        from dune.xt.common.gui.k3d import DEFAULT_COLOR_MAP, NumpyPlot

        color_map = color_map or DEFAULT_COLOR_MAP
        if isinstance(color_map, Colormap):
            color_map = [(x, *color_map(x)[:3]) for x in np.linspace(0, 1, 256)]

        vertices = np.array(gridprovider.centers(codim=2), copy=False).astype(np.float32)
        if gridprovider.dimension == 2:
            # k3d does not support 2D meshes, so pad Z direction with zeros
            vertices = np.hstack((vertices, np.zeros((vertices.shape[0], 1), dtype=np.float32)))

        indices = gridprovider.subentity_indices(codim=gridprovider.dimension)
        data = np.linspace(0, 1, vertices.shape[0], dtype=np.float32)

        return NumpyPlot(vertices,
                         indices,
                         data,
                         grid_auto_fit=False,
                         camera_auto_fit=False,
                         color_map=color_map,
                         interactive=interactive)
