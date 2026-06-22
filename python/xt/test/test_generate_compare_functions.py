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

import pathlib
import sys

import pytest

# generate_compare_functions ships as a wheel script (installed to bin/, not importable as a module), so it is not on
# sys.path. In the build tree it sits next to this test dir under scripts/; add that to sys.path so the test can import
# it, and skip cleanly when run outside a build tree (e.g. from a bare source checkout).
_scripts_dir = pathlib.Path(__file__).resolve().parent.parent / "scripts"
if not (_scripts_dir / "generate_compare_functions.py").is_file():
    pytest.skip(
        "generate_compare_functions.py unavailable (run from the build tree)",
        allow_module_level=True,
    )
sys.path.insert(0, str(_scripts_dir))

import generate_compare_functions as g  # noqa: E402


def test_comparators_list():
    assert g.cmps == ["eq", "ne", "gt", "lt", "ge", "le"]


def test_render_common_substitutes_id():
    rendered = g.render_common("eq")
    assert " eq(" in rendered
    assert "${id}" not in rendered


def test_render_test_substitutes_id_and_upper():
    rendered = g.render_test("eq")
    assert "DXTC_EXPECT_FLOAT_EQ" in rendered
    assert "${id}" not in rendered and "${ID}" not in rendered


def test_main_writes_both_headers_for_all_comparators(tmp_path):
    bindir = tmp_path / "scripts"
    bindir.mkdir()
    common_dir = tmp_path / "dune" / "xt" / "common"
    (common_dir / "test").mkdir(parents=True)

    g.main(bindir=str(bindir))

    common = (common_dir / "float_cmp_generated.hxx").read_text()
    test = (common_dir / "test" / "float_cmp_generated.hxx").read_text()
    for name in g.cmps:
        assert f" {name}(" in common
        assert f"DXTC_EXPECT_FLOAT_{name.upper()}" in test
