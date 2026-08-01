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

# Exercises the error paths of the dimension-split dispatch trampolines defined in
# dune.gdt.__init__ (see the "dimension-split binding trampolines" comment there): the
# dimension-probing helper (_dim_of), the per-dimension submodule loader (_split_submodule), and
# the dispatching factory closure built by _make_dispatch. The factories exercised elsewhere
# always pass a well-formed dimension-carrying argument for a submodule that is actually built,
# so these malformed-input branches need dedicated tests with deliberately bogus arguments.

from types import SimpleNamespace

import pytest

import dune.gdt


def test_dim_of_rejects_an_object_without_a_dimension_attribute():
    with pytest.raises(TypeError, match="cannot determine the grid dimension"):
        dune.gdt._dim_of(object())


def test_split_submodule_wraps_import_error():
    with pytest.raises(
        ImportError, match=r"cannot dispatch to dune\.gdt\.no_such_base_7d"
    ):
        dune.gdt._split_submodule("no_such_base", 7)


def test_factory_requires_the_dimension_carrying_argument():
    factory = dune.gdt._make_dispatch("no_such_base", "SomeFactory", dim_kwarg="grid")
    with pytest.raises(TypeError, match="missing the dimension-carrying argument"):
        factory()


def test_factory_accepts_the_dimension_carrying_argument_as_a_keyword():
    factory = dune.gdt._make_dispatch("no_such_base", "SomeFactory", dim_kwarg="grid")
    probe = SimpleNamespace(dimension=3)
    # the keyword is resolved into `probe` and handed to _dim_of/_split_submodule; the submodule
    # itself does not exist, so the call still fails, but only after the keyword-lookup branch
    # (as opposed to the positional-argument branch exercised by the real factories) has run
    with pytest.raises(
        ImportError, match=r"cannot dispatch to dune\.gdt\.no_such_base_3d"
    ):
        factory(grid=probe)


if __name__ == "__main__":
    from dune.xt.test.base import runmodule

    runmodule(__file__)
