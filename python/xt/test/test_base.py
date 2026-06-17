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

import importlib

import pytest

from dune.xt.test.base import load_all_submodule, runmodule


def _make_package(tmp_path, monkeypatch, name, submodules):
    """Create an importable package on disk and return the imported module.

    `submodules` maps submodule filename -> file body.
    """
    pkg = tmp_path / name
    pkg.mkdir()
    (pkg / "__init__.py").write_text("")
    for fname, body in submodules.items():
        (pkg / fname).write_text(body)
    monkeypatch.syspath_prepend(str(tmp_path))
    importlib.invalidate_caches()
    return importlib.import_module(name)


def test_load_all_submodule_success(tmp_path, monkeypatch):
    mod = _make_package(
        tmp_path,
        monkeypatch,
        "good_pkg",
        {"a.py": "x = 1\n", "b.py": "y = 2\n"},
    )
    # all submodules import cleanly -> no exception
    assert load_all_submodule(mod) is None


def test_load_all_submodule_raises_on_failure(tmp_path, monkeypatch):
    mod = _make_package(
        tmp_path,
        monkeypatch,
        "bad_pkg",
        {"ok.py": "x = 1\n", "broken.py": "raise ImportError('boom')\n"},
    )
    with pytest.raises(ImportError):
        load_all_submodule(mod)


def test_load_all_submodule_skips_playground(tmp_path, monkeypatch):
    # A broken module under a `playground` name is ignored, so no error is raised.
    mod = _make_package(
        tmp_path,
        monkeypatch,
        "pg_pkg",
        {"playground.py": "raise ImportError('ignored')\n", "ok.py": "x = 1\n"},
    )
    assert load_all_submodule(mod) is None


def test_runmodule_exits_with_pytest_return_code(monkeypatch):
    calls = {}

    def fake_main(argv):
        calls["argv"] = argv
        return 0

    monkeypatch.setattr("pytest.main", fake_main)
    with pytest.raises(SystemExit) as excinfo:
        runmodule("some_file.py")
    assert excinfo.value.code == 0
    assert calls["argv"][-1] == "some_file.py"
