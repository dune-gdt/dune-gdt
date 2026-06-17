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

from dune.xt.common.vtk.reader import _get_vtk_type  # noqa: E402


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
