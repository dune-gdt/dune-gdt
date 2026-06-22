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

from dune.xt.cmake import parse_cache  # noqa: E402


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
    assert kv["ENABLE"] == "ON"
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
