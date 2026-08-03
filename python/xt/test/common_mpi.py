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
#   Tobias Leibner  (2019 - 2020)
# ~~~

import pytest

# hatch_build.py's CustomMetadataHook only adds mpi4py to the project's runtime dependencies for
# MPI-enabled builds (see __have_mpi__ in the generated _version.py), and none of the CI presets
# build with MPI enabled (see #393) -- so this module-level skip, not a per-test one, makes plain
# why both tests below are unconditionally skipped in every CI run today rather than repeating a
# try/except ImportError in each of them.
pytest.importorskip("mpi4py")
from mpi4py import MPI  # noqa: E402


def test_mpi4py():
    mpi_comm = MPI.COMM_WORLD
    from dune.xt.common import CollectiveCommunication

    comm_def = CollectiveCommunication()
    comm = CollectiveCommunication(mpi_comm)
    assert type(comm) is type(comm_def)
    assert comm.size > 0
    assert comm.rank < comm.size
    assert comm.sum(1) == comm.size


def test_wrapper():
    mpi_comm = MPI.COMM_WORLD  # noqa: F841


if __name__ == "__main__":
    from dune.xt.common.test import runmodule

    runmodule(__file__)
