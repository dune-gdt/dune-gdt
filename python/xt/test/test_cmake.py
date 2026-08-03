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

pytest.importorskip("pyparsing")

from dune.xt.cmake import is_cmake_true, parse_cache  # noqa: E402


def _write_cache(path, body):
    path.write_text(body)
    return str(path)


def test_parse_cache_basic_key_value_and_types(tmp_path):
    cache = _write_cache(
        tmp_path / "CMakeCache.txt",
        "FOO:STRING=bar\nENABLE:BOOL=ON\n",
    )
    kv, types = parse_cache(cache)
    assert kv["FOO"] == "bar"
    # BOOL entries are handed to the configs as booleans, not as the raw CMake spelling.
    assert kv["ENABLE"] is True
    assert types["FOO"] == "STRING"
    assert types["ENABLE"] == "BOOL"


def test_parse_cache_ignores_comments(tmp_path):
    cache = _write_cache(
        tmp_path / "CMakeCache.txt",
        "# this is a hash comment\n// this is a slash comment\nFOO:STRING=bar\n",
    )
    kv, _ = parse_cache(cache)
    assert kv == {"FOO": "bar"}


def test_parse_cache_dir_entry_existing(tmp_path):
    # A *_DIR entry pointing at an existing directory yields a derived boolean True.
    target = tmp_path / "somedir"
    target.mkdir()
    cache = _write_cache(
        tmp_path / "CMakeCache.txt",
        f"dune-istl_DIR:PATH={target}\n",
    )
    kv, _ = parse_cache(cache)
    assert kv["dune-istl_DIR"] == str(target)
    assert kv["dune-istl"] is True


def test_parse_cache_dir_entry_missing(tmp_path):
    cache = _write_cache(
        tmp_path / "CMakeCache.txt",
        f"dune-istl_DIR:PATH={tmp_path / 'does-not-exist'}\n",
    )
    kv, _ = parse_cache(cache)
    assert kv["dune-istl"] is False


@pytest.mark.parametrize("value", ("ON", "YES", "TRUE", "Y", "1", "2", "on", "true"))
def test_is_cmake_true_truthy_constants(value):
    assert is_cmake_true(value) is True


@pytest.mark.parametrize(
    "value",
    (
        "OFF",
        "NO",
        "FALSE",
        "N",
        "IGNORE",
        "NOTFOUND",
        "0",
        "",
        "  ",
        "Alberta-NOTFOUND",
    ),
)
def test_is_cmake_true_falsy_constants(value):
    assert is_cmake_true(value) is False


def test_is_cmake_true_passes_booleans_through():
    assert is_cmake_true(True) is True
    assert is_cmake_true(False) is False


def test_parse_cache_bool_entries_are_booleans(tmp_path):
    # A BOOL entry must not reach the configs as a string: both guard implementations
    # (codegen.is_found and grid_types._is_usable) would read the string "FALSE" as a yes.
    cache = _write_cache(
        tmp_path / "CMakeCache.txt",
        "YES_ON:BOOL=ON\nYES_TRUE:BOOL=TRUE\nNO_OFF:BOOL=OFF\nNO_FALSE:BOOL=FALSE\nNO_ZERO:BOOL=0\n",
    )
    kv, types = parse_cache(cache)
    assert kv["YES_ON"] is True
    assert kv["YES_TRUE"] is True
    assert kv["NO_OFF"] is False
    assert kv["NO_FALSE"] is False
    assert kv["NO_ZERO"] is False
    assert types["NO_OFF"] == "BOOL"


def test_parse_cache_alberta_guard_entry(tmp_path):
    # Regression test for issue #374: dxt_write_codegen_cache() appends ALBERTA_FOUND to the
    # snapshot (find_package(Alberta) sets a normal variable, which is not part of the cache).
    # It is only useful to the configs if it survives parsing as a boolean in both directions.
    kv, _ = parse_cache(
        _write_cache(tmp_path / "found.txt", "ALBERTA_FOUND:BOOL=TRUE\n")
    )
    assert kv["ALBERTA_FOUND"] is True
    kv, _ = parse_cache(
        _write_cache(tmp_path / "missing.txt", "ALBERTA_FOUND:BOOL=FALSE\n")
    )
    assert kv["ALBERTA_FOUND"] is False
