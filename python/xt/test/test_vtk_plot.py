# ~~~
# This file is part of the dune-xt project:
#   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
# Copyright 2009-2026 dune-xt developers and contributors. All rights reserved.
# License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
#      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
#          with "runtime exception" (http://www.dune-project.org/license.html)
# Authors:
#   René Fritze (2026)
# ~~~
"""#393: k3d is now in the ctest `test` dependency group, so `dune.xt.common.vtk.plot` -- entirely
unreachable before, since HAVE_K3D was permanently False under ctest -- is exercisable. This drives
its `plot()` entry point (and the `_transform_to_k3d` helper it uses per-timestep) against
synthetic .vtu/.pvd files, the same way test_vtk_reader.py drives the reader half of the module.
"""

import pytest

vtk = pytest.importorskip("vtk")
pytest.importorskip("lxml")
pytest.importorskip("xmljson")
pytest.importorskip("k3d")
pytest.importorskip("ipywidgets")

plot_mod = pytest.importorskip("dune.xt.common.vtk.plot", exc_type=ImportError)


@pytest.fixture(autouse=True)
def _display_builtin(monkeypatch):
    """k3d's Plot.display() (k3d/plot/plot_display.py) calls a bare `display(...)` name. A real
    Jupyter frontend gets this for free -- IPython.core.interactiveshell.InteractiveShell installs
    it into `builtins` during init -- but under ctest there is no kernel to do that, so plot()
    would otherwise raise `NameError: name 'display' is not defined`. Provide it directly instead
    of standing up a whole InteractiveShell, which would mutate process-global state
    (sys.displayhook/excepthook, builtin traps) for the rest of the test session.
    """
    import builtins

    from IPython.display import display

    monkeypatch.setattr(builtins, "display", display, raising=False)


def _write_vtu(path, values):
    """Write a minimal two-triangle UnstructuredGrid with a "Data" point array as an ASCII .vtu."""
    points = vtk.vtkPoints()
    for coord in [(0, 0, 0), (1, 0, 0), (0, 1, 0), (1, 1, 0)]:
        points.InsertNextPoint(*coord)
    data = vtk.vtkDoubleArray()
    data.SetName("Data")
    for value in values:
        data.InsertNextValue(value)
    grid = vtk.vtkUnstructuredGrid()
    grid.SetPoints(points)
    grid.GetPointData().AddArray(data)
    for ids in [(0, 1, 2), (1, 3, 2)]:
        triangle = vtk.vtkTriangle()
        for local, global_ in enumerate(ids):
            triangle.GetPointIds().SetId(local, global_)
        grid.InsertNextCell(triangle.GetCellType(), triangle.GetPointIds())
    writer = vtk.vtkXMLUnstructuredGridWriter()
    writer.SetFileName(str(path))
    writer.SetDataModeToAscii()
    writer.SetInputData(grid)
    writer.Write()
    return str(path)


def _write_collection(tmp_path, datasets):
    pvd = tmp_path / "collection.pvd"
    lines = [
        '<?xml version="1.0"?>',
        '<VTKFile type="Collection" version="0.1">',
        "<Collection>",
    ]
    lines += [f'<DataSet timestep="{ts}" file="{f}"/>' for ts, f in datasets]
    lines += ["</Collection>", "</VTKFile>", ""]
    pvd.write_text("\n".join(lines), encoding="utf-8")
    return str(pvd)


def _read_single_poly(path):
    from dune.xt.common.vtk.reader import read_vtkfile

    return read_vtkfile(path)[0][1]


def test_transform_to_k3d_shapes_and_color_range(tmp_path):
    path = _write_vtu(tmp_path / "grid.vtu", [0.0, 1.0, 2.0, 3.0])
    poly = _read_single_poly(path)
    timestep, attribute, cmin, cmax, vertices, indices = plot_mod._transform_to_k3d(
        0.5, poly, "Data"
    )
    assert timestep == 0.5
    assert attribute.shape == (4,)
    assert (cmin, cmax) == (0.0, 3.0)
    assert vertices.shape == (4, 3)
    # two triangles, three vertex indices each
    assert indices.shape == (2, 3)


def test_transform_to_k3d_without_color_attribute(tmp_path):
    path = _write_vtu(tmp_path / "grid.vtu", [0.0, 1.0, 2.0, 3.0])
    poly = _read_single_poly(path)
    _, attribute, cmin, cmax, _, _ = plot_mod._transform_to_k3d(0.0, poly, None)
    assert len(attribute) == 0
    assert (cmin, cmax) == (0, -1)


def test_plot_single_file_returns_vtk_plot(tmp_path):
    path = _write_vtu(tmp_path / "grid.vtu", [0.0, 1.0, 2.0, 3.0])
    result = plot_mod.plot(path, color_attribute_name="Data")
    assert isinstance(result, plot_mod.VTKPlot)
    assert result.idx == 0
    # the non-interactive default locks the camera controls
    assert result.camera_no_pan is True
    assert result.camera_no_rotate is True
    assert result.camera_no_zoom is True


def test_plot_interactive_leaves_camera_controls_enabled(tmp_path):
    path = _write_vtu(tmp_path / "grid.vtu", [0.0, 1.0, 2.0, 3.0])
    result = plot_mod.plot(path, color_attribute_name="Data", interactive=True)
    assert result.camera_no_pan is False
    assert result.camera_no_rotate is False
    assert result.camera_no_zoom is False


def test_plot_collection_creates_timestep_controls_and_goto_idx(tmp_path):
    first = _write_vtu(tmp_path / "a.vtu", [0.0, 1.0, 2.0, 3.0])
    second = _write_vtu(tmp_path / "b.vtu", [10.0, 11.0, 12.0, 13.0])
    pvd = _write_collection(tmp_path, [(0, first), (1, second)])

    result = plot_mod.plot(pvd, color_attribute_name="Data")
    assert result.timestep == 0
    result.inc()
    assert result.idx == 1
    assert result.timestep == 1
    result.dec()
    assert result.idx == 0
    assert result.timestep == 0


def test_goto_idx_out_of_range_warns_and_is_a_noop(tmp_path):
    path = _write_vtu(tmp_path / "grid.vtu", [0.0, 1.0, 2.0, 3.0])
    result = plot_mod.plot(path, color_attribute_name="Data")
    with pytest.warns(RuntimeWarning, match="outside data range"):
        result._goto_idx(5)
    # the index (and therefore the displayed timestep) is unchanged
    assert result.idx == 0


def test_plot_with_matplotlib_colormap(tmp_path):
    from matplotlib import colormaps

    path = _write_vtu(tmp_path / "grid.vtu", [0.0, 1.0, 2.0, 3.0])
    result = plot_mod.plot(
        path, color_attribute_name="Data", color_map=colormaps["plasma"]
    )
    assert isinstance(result, plot_mod.VTKPlot)
