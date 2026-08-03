# ~~~
# This file is part of the dune-xt project:
#   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
# Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
# License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
#      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
#          with "runtime exception" (http://www.dune-project.org/license.html)
# Authors:
#   Felix Schindler (2020)
#   René Fritze     (2018 - 2019)
#   Tim Keil        (2018)
#   Tobias Leibner  (2019 - 2020)
# ~~~

try:
    from dune.xt.test._test_grid_types import *  # noqa: F403
except ImportError as e:
    import logging
    import os

    if os.environ.get("DXT_PYTHON_DEBUG", False):
        raise e
    # Logged with the original exception (not just a fixed string) so a genuinely broken build --
    # as opposed to one that never compiled this binding at all -- is diagnosable from CI output
    # instead of silently shrinking the grid-type matrix that depends on it (#393).
    logging.error("dune-xt-grid bindings not available: %s", e)

from collections import namedtuple

from dune.xt.codegen import is_found

arguments = {
    "alu": namedtuple("alu_args", "dim element_type refinement"),
    "yasp": namedtuple("yasp_args", "dim"),
    "ug": namedtuple("ug_args", "dim"),
    "alberta": namedtuple("alberta_args", "dim"),
}
templates = {
    "alu": "Dune::ALUGrid<{dim},{dim},Dune::{element_type},Dune::{refinement}>",
    "yasp": "Dune::YaspGrid<{dim},Dune::EquidistantOffsetCoordinates<double,{dim}>>",
    "ug": "Dune::UGGrid<{dim}>",
    "alberta": "Dune::AlbertaGrid<{dim},{dim}>",
}
# Keys looked up in the CMake cache snapshot the codegen reads. The dune-* ones are the boolean
# companions parse_cache derives from the modules' *_DIR cache paths; ALBERTA_FOUND is not a cache
# variable at all and is appended to the snapshot by dxt_write_codegen_cache() (DuneXTTesting.cmake)
# from find_package(Alberta)'s normal variable -- keep the two names in sync.
guards = {
    "alu": "dune-alugrid",
    "yasp": "dune-grid",
    "ug": "dune-uggrid",
    "alberta": "ALBERTA_FOUND",
}


def _is_usable(grid, cache):
    return is_found(cache, guards[grid])


def all_args(dims):
    two_and_three = [f for f in dims if 1 < f < 4]
    return {
        "alu": [
            arguments["alu"](d, "simplex", e)
            for e in ("nonconforming", "conforming")
            for d in two_and_three
        ]
        + [arguments["alu"](d, "cube", "nonconforming") for d in two_and_three],
        "yasp": [arguments["yasp"](d) for d in dims if d > 0],
        "ug": [arguments["ug"](d) for d in two_and_three],
        "alberta": [arguments["alberta"](d) for d in two_and_three],
    }


def all_types(cache, dims, gridnames=None):
    gridnames = gridnames or templates.keys()
    return [
        templates[grid].format(**arg._asdict())
        for grid in gridnames
        if _is_usable(grid, cache)
        for arg in all_args(dims)[grid]
    ]
