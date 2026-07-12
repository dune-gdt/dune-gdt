# ~~~
# This file is part of the dune-gdt project:
#   https://github.com/dune-gdt/dune-gdt
# Copyright 2010-2026 dune-gdt developers and contributors. All rights reserved.
# License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
#      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
#          with "runtime exception" (http://www.dune-project.org/license.html)
# Authors:
#   René Fritze (2026)
# ~~~

import os

from hypothesis import HealthCheck, settings
from hypothesis.errors import InvalidArgument

# Every example crosses into compiled dune code (grid construction, assembly); wall time per
# example is dominated by C++, not by data generation, and varies wildly between machines and
# ccache states. Hypothesis' 200ms deadline is meant to catch pathological *generation*, so it
# is disabled here; the example counts keep total runtime bounded instead.
settings.register_profile(
    "dune-ci",
    max_examples=25,
    deadline=None,
    suppress_health_check=[HealthCheck.too_slow],
)
settings.register_profile(
    "dune-dev",
    max_examples=10,
    deadline=None,
    suppress_health_check=[HealthCheck.too_slow],
)
try:
    settings.load_profile(os.environ.get("HYPOTHESIS_PROFILE", "dune-ci"))
except InvalidArgument:
    # HYPOTHESIS_PROFILE names a profile this conftest does not know (e.g. one registered by a
    # developer's own tooling that is not on this run's plugin path); fall back instead of
    # failing collection of the whole suite
    settings.load_profile("dune-ci")
