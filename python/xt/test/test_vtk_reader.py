# ~~~
# This file is part of the dune-xt project:
#   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
# Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
# License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
#      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
#          with "runtime exception" (http://www.dune-project.org/license.html)
# Authors:
#   René Fritze (2024)
# ~~~

import pytest

# reader.py imports the whole VTK reader stack at module import time.
vtk = pytest.importorskip("vtk")
pytest.importorskip("lxml")
pytest.importorskip("xmljson")

from dune.xt.common.vtk.reader import (  # noqa: E402
    _get_vtk_type,
    _read_collection,
    _read_single,
    read_vtkfile,
)


def _write(tmp_path, body):
    p = tmp_path / "file.vtu"
    p.write_bytes(body.encode("utf-8"))
    return str(p)


def _write_vtu(path):
    """Write a minimal two-triangle UnstructuredGrid as an ASCII .vtu and return its path."""
    points = vtk.vtkPoints()
    for coord in [(0, 0, 0), (1, 0, 0), (0, 1, 0), (1, 1, 0)]:
        points.InsertNextPoint(*coord)
    grid = vtk.vtkUnstructuredGrid()
    grid.SetPoints(points)
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
    """Write a Collection .pvd referencing `datasets` (list of (timestep, file)) and return its path."""
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


def test_get_vtk_type_unstructured(tmp_path):
    path = _write(
        tmp_path,
        '<?xml version="1.0"?>\n<VTKFile type="UnstructuredGrid"><Piece/></VTKFile>\n',
    )
    assert _get_vtk_type(path) == "UnstructuredGrid"


def test_get_vtk_type_collection(tmp_path):
    path = _write(
        tmp_path,
        '<?xml version="1.0"?>\n<VTKFile type="Collection"></VTKFile>\n',
    )
    assert _get_vtk_type(path) == "Collection"


def test_get_vtk_type_no_vtkfile_element(tmp_path):
    path = _write(tmp_path, '<?xml version="1.0"?>\n<Other></Other>\n')
    assert _get_vtk_type(path) is None


def test_read_single_unstructured_grid(tmp_path):
    path = _write_vtu(tmp_path / "grid.vtu")
    poly = _read_single(path)
    # the geometry filter turns the two triangles of the unstructured grid into surface polygons
    assert poly.GetNumberOfPoints() == 4
    assert poly.GetNumberOfCells() == 2


def test_read_single_honours_explicit_vtk_type(tmp_path):
    path = _write_vtu(tmp_path / "grid.vtu")
    # passing the type explicitly skips the _get_vtk_type peek but yields the same result
    poly = _read_single(path, vtk_type="UnstructuredGrid")
    assert poly.GetNumberOfPoints() == 4


def test_read_single_unsupported_type_raises(tmp_path):
    path = _write_vtu(tmp_path / "grid.vtu")
    with pytest.raises(NotImplementedError, match="ImageData"):
        _read_single(path, vtk_type="ImageData")


def test_read_vtkfile_single(tmp_path):
    path = _write_vtu(tmp_path / "grid.vtu")
    result = read_vtkfile(path)
    # a single file is reported as one timestep at t=0.0
    assert len(result) == 1
    timestep, poly = result[0]
    assert timestep == pytest.approx(0.0)
    assert poly.GetNumberOfPoints() == 4


def test_read_vtkfile_collection_is_sorted_by_timestep(tmp_path):
    first = _write_vtu(tmp_path / "a.vtu")
    second = _write_vtu(tmp_path / "b.vtu")
    # deliberately list the datasets out of order to exercise the sort in _read_collection
    pvd = _write_collection(tmp_path, [(1, second), (0, first)])
    result = read_vtkfile(pvd)
    assert [timestep for timestep, _ in result] == [0, 1]
    for _, poly in result:
        assert poly.GetNumberOfPoints() == 4


def test_read_collection_directly(tmp_path):
    from dune.xt.common.vtk.reader import _get_collection_data

    first = _write_vtu(tmp_path / "a.vtu")
    second = _write_vtu(tmp_path / "b.vtu")
    pvd = _write_collection(tmp_path, [(0, first), (1, second)])
    _, xml = _get_collection_data(pvd)
    result = _read_collection(xml)
    assert [timestep for timestep, _ in result] == [0, 1]
    assert all(poly.GetNumberOfCells() == 2 for _, poly in result)
