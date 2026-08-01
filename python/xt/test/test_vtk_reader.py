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
pytest.importorskip("vtk")
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


def test_read_single_unstructured_grid(tmp_path, write_vtu):
    path = write_vtu(tmp_path / "grid.vtu")
    poly = _read_single(path)
    # the geometry filter turns the two triangles of the unstructured grid into surface polygons
    assert poly.GetNumberOfPoints() == 4
    assert poly.GetNumberOfCells() == 2


def test_read_single_honours_explicit_vtk_type(tmp_path, write_vtu):
    path = write_vtu(tmp_path / "grid.vtu")
    # passing the type explicitly skips the _get_vtk_type peek but yields the same result
    poly = _read_single(path, vtk_type="UnstructuredGrid")
    assert poly.GetNumberOfPoints() == 4


def test_read_single_unsupported_type_raises(tmp_path, write_vtu):
    path = write_vtu(tmp_path / "grid.vtu")
    with pytest.raises(NotImplementedError, match="ImageData"):
        _read_single(path, vtk_type="ImageData")


def test_read_vtkfile_single(tmp_path, write_vtu):
    path = write_vtu(tmp_path / "grid.vtu")
    result = read_vtkfile(path)
    # a single file is reported as one timestep at t=0.0
    assert len(result) == 1
    timestep, poly = result[0]
    assert timestep == pytest.approx(0.0)
    assert poly.GetNumberOfPoints() == 4


def test_read_vtkfile_collection_is_sorted_by_timestep(
    tmp_path, write_vtu, write_vtk_collection
):
    first = write_vtu(tmp_path / "a.vtu")
    second = write_vtu(tmp_path / "b.vtu")
    # deliberately list the datasets out of order to exercise the sort in _read_collection
    pvd = write_vtk_collection(tmp_path, [(1, second), (0, first)])
    result = read_vtkfile(pvd)
    assert [timestep for timestep, _ in result] == [0, 1]
    for _, poly in result:
        assert poly.GetNumberOfPoints() == 4


def test_read_collection_directly(tmp_path, write_vtu, write_vtk_collection):
    from dune.xt.common.vtk.reader import _get_collection_data

    first = write_vtu(tmp_path / "a.vtu")
    second = write_vtu(tmp_path / "b.vtu")
    pvd = write_vtk_collection(tmp_path, [(0, first), (1, second)])
    _, xml = _get_collection_data(pvd)
    result = _read_collection(xml)
    assert [timestep for timestep, _ in result] == [0, 1]
    assert all(poly.GetNumberOfCells() == 2 for _, poly in result)
