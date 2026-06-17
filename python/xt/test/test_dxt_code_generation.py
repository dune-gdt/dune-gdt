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

pytest.importorskip("jinja2")
pytest.importorskip("pyparsing")

dxt = pytest.importorskip("dxt_code_generation")


def _cache(dir_, body="FOO:STRING=bar\n"):
    (dir_ / "CMakeCache.txt").write_text(body)
    return str(dir_)


def test_generate_plain_config(tmp_path):
    bindir = tmp_path / "build"
    bindir.mkdir()
    _cache(bindir)
    config_fn = tmp_path / "config.py"
    config_fn.write_text("greeting = 'hi'\n")
    tpl_fn = tmp_path / "tpl.in"
    tpl_fn.write_text("G={{ config['greeting'] }} C={{ cache['FOO'] }}\n")
    out_fn = tmp_path / "out" / "result.hh"

    dxt.generate(str(config_fn), str(tpl_fn), str(bindir), str(out_fn), str(bindir))

    assert out_fn.read_text().strip() == "G=hi C=bar"


def test_generate_multi_out(tmp_path):
    bindir = tmp_path / "build"
    bindir.mkdir()
    _cache(bindir)
    config_fn = tmp_path / "config.py"
    config_fn.write_text("multi_out = {'one': {'val': 1}, 'two': {'val': 2}}\n")
    tpl_fn = tmp_path / "tpl.in"
    tpl_fn.write_text("V={{ config['val'] }}\n")
    out_fn = tmp_path / "result.hh"

    dxt.generate(str(config_fn), str(tpl_fn), str(bindir), str(out_fn), str(bindir))

    assert (tmp_path / "result.hh.one").read_text().strip() == "V=1"
    assert (tmp_path / "result.hh.two").read_text().strip() == "V=2"


def test_generate_falls_back_to_backup_cache(tmp_path):
    primary = tmp_path / "primary"  # exists but has no CMakeCache.txt
    primary.mkdir()
    backup = tmp_path / "backup"
    backup.mkdir()
    _cache(backup, "FOO:STRING=frombackup\n")
    config_fn = tmp_path / "config.py"
    config_fn.write_text("greeting = 'x'\n")
    tpl_fn = tmp_path / "tpl.in"
    tpl_fn.write_text("{{ cache['FOO'] }}\n")
    out_fn = tmp_path / "out.hh"

    dxt.generate(str(config_fn), str(tpl_fn), str(primary), str(out_fn), str(backup))

    assert out_fn.read_text().strip() == "frombackup"
