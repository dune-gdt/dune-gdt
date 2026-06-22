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

from dune.xt.test import grid_types


def test_all_args_dimension_filtering():
    args = grid_types.all_args((1, 2, 3, 4))
    # yasp keeps every positive dim; the others are restricted to 1 < d < 4.
    assert sorted(a.dim for a in args["yasp"]) == [1, 2, 3, 4]
    assert sorted(a.dim for a in args["ug"]) == [2, 3]
    assert sorted(a.dim for a in args["alberta"]) == [2, 3]


def test_all_args_alu_combinations():
    args = grid_types.all_args((2, 3))
    alu = args["alu"]
    # 2 simplex refinements x 2 dims + 1 cube refinement x 2 dims = 6
    assert len(alu) == 6
    simplex = {(a.dim, a.refinement) for a in alu if a.element_type == "simplex"}
    assert simplex == {
        (2, "nonconforming"),
        (3, "nonconforming"),
        (2, "conforming"),
        (3, "conforming"),
    }
    cube = {(a.dim, a.refinement) for a in alu if a.element_type == "cube"}
    assert cube == {(2, "nonconforming"), (3, "nonconforming")}


def test_is_usable():
    cache = {"dune-grid": True, "dune-alugrid": False}
    assert grid_types._is_usable("yasp", cache) is True
    assert grid_types._is_usable("alu", cache) is False
    # missing guard key -> not usable
    assert grid_types._is_usable("ug", cache) is False


def test_all_types_empty_cache():
    assert grid_types.all_types(cache={}, dims=(2, 3)) == []


def test_all_types_yasp_enabled():
    cache = {"dune-grid": True}
    types = grid_types.all_types(cache=cache, dims=(2,))
    assert types == ["Dune::YaspGrid<2,Dune::EquidistantOffsetCoordinates<double,2>>"]


def test_all_types_respects_gridnames_filter():
    cache = {"dune-grid": True, "dune-alugrid": True}
    # restrict to yasp only even though alu is also usable
    types = grid_types.all_types(cache=cache, dims=(2,), gridnames=["yasp"])
    assert all("YaspGrid" in t for t in types)
    assert types == ["Dune::YaspGrid<2,Dune::EquidistantOffsetCoordinates<double,2>>"]


def test_all_types_alu_formatting():
    cache = {"dune-alugrid": True}
    types = grid_types.all_types(cache=cache, dims=(2,), gridnames=["alu"])
    assert "Dune::ALUGrid<2,2,Dune::simplex,Dune::nonconforming>" in types
    assert "Dune::ALUGrid<2,2,Dune::cube,Dune::nonconforming>" in types
