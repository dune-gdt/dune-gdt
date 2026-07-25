# ~~~
# This file is part of the dune-xt project:
#   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
# Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
# License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
#      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
#          with "runtime exception" (http://www.dune-project.org/license.html)
# Authors:
#   René Fritze    (2018 - 2019)
#   Tim Keil       (2018)
#   Tobias Leibner (2019 - 2020)
# ~~~

import itertools

from dune.xt.codegen import is_found


def _make_alu(dimDomain):
    tmpl = "Dune::ALUGrid<{d},{d},Dune::{g},Dune::{r}>"
    alu_dims = [d for d in dimDomain if d != 1]
    grids = []
    for d in alu_dims:
        grids.append((tmpl.format(d=d, g="cube", r="nonconforming"), d))
    for d, r in itertools.product(alu_dims, ("nonconforming", "conforming")):
        grids.append((tmpl.format(d=d, g="simplex", r=r), d))
    return grids


def _if_active(val, guard, cache):
    return val if is_found(cache, guard) else []


def type_and_dim(cache, dimDomain):
    grid_alu = _if_active(_make_alu(dimDomain), "dune-alugrid", cache)
    grid_yasp = [
        (
            "Dune::YaspGrid<{d},Dune::EquidistantOffsetCoordinates<double,{d}>>".format(
                d=d
            ),
            d,
        )
        for d in dimDomain
    ]
    grid_ug = _if_active(
        [(f"Dune::UGGrid<{d}>", d) for d in dimDomain if d != 1], "dune-uggrid", cache
    )
    # ALBERTA_FOUND is not a cache variable: dxt_write_codegen_cache() appends it to the snapshot
    # from find_package(Alberta)'s normal variable, under this name (see DuneXTTesting.cmake).
    grid_alberta = _if_active(
        [("Dune::AlbertaGrid<{d},{d}>".format(d=d), d) for d in dimDomain if d != 1],
        "ALBERTA_FOUND",
        cache,
    )
    grid_oneD = [("Dune::OneDGrid", d) for d in dimDomain if d == 1]
    return grid_alberta + grid_alu + grid_yasp + grid_ug + grid_oneD


def pretty_print(grid_name, dim):
    if "Alberta" in grid_name:
        return f"{dim}d_simplex_albertagrid"
    elif "ALU" in grid_name and "simplex" in grid_name and "nonconforming" in grid_name:
        return f"{dim}d_simplex_alunonconformgrid"
    elif "ALU" in grid_name and "simplex" in grid_name and "conforming" in grid_name:
        return f"{dim}d_simplex_aluconformgrid"
    elif "ALU" in grid_name and "cube" in grid_name and "nonconforming" in grid_name:
        return f"{dim}d_cube_alunonconformgrid"
    elif "Yasp" in grid_name:
        return f"{dim}d_cube_yaspgrid"
    elif "UG" in grid_name:
        return f"{dim}d_simplex_uggrid"
    elif "OneD" in grid_name:
        return f"{dim}d_cube_onedgrid"
    else:
        raise RuntimeError(f"unknown type: {grid_name}")
