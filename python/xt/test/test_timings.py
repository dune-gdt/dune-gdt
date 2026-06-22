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

# dune.xt.common.timings is a thin facade: it guarded_imports the compiled `_common_timings` binding into its namespace
# and instantiates the global timings singleton at import time. There is no pure-Python logic to exercise, so the only
# meaningful check is a light smoke test that the binding loads and re-exports the expected names.
#
# It is skipped when the facade cannot be imported -- both a bare source checkout (binding absent, ModuleNotFoundError)
# and a build that fails to load the binding (guarded_import re-raises a plain ImportError). pytest.importorskip only
# auto-skips on ModuleNotFoundError for the requested module, so exc_type=ImportError is needed to skip on the latter.

import pytest

timings = pytest.importorskip("dune.xt.common.timings", exc_type=ImportError)


def test_timings_reexports_instance():
    # the module calls instance() at import time, so the name must have been injected from the binding
    assert hasattr(timings, "instance")
    assert callable(timings.instance)
