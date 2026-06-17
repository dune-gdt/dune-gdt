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

import re

import pytest

from dune.xt.common.config import Config, _can_import


def test_can_import_stdlib_module():
    assert _can_import("sys") is True


def test_can_import_missing_module():
    assert _can_import("definitely_not_a_real_module_xyz") is False


def test_can_import_list_all_present():
    assert _can_import(["sys", "os"]) is True


def test_can_import_list_one_missing():
    assert _can_import(["sys", "definitely_not_a_real_module_xyz"]) is False


def test_python_version_format():
    assert re.fullmatch(r"\d+\.\d+\.\d+", Config().PYTHON_VERSION)


def test_have_k3d_is_bool_and_cached():
    cfg = Config()
    have = cfg.HAVE_K3D
    assert isinstance(have, bool)
    # __getattr__ caches the resolved value on the instance.
    assert "HAVE_K3D" in cfg.__dict__
    assert cfg.HAVE_K3D is have


def test_version_attr_consistent_with_have():
    cfg = Config()
    if cfg.HAVE_K3D:
        assert cfg.K3D_VERSION not in (None, False)
    else:
        assert cfg.K3D_VERSION is None


def test_unknown_attribute_raises():
    cfg = Config()
    with pytest.raises(AttributeError):
        getattr(cfg, "NOT_A_VALID_ATTR")
    # right shape, but not a known package
    with pytest.raises(AttributeError):
        getattr(cfg, "HAVE_NOSUCHPACKAGE")


def test_dir_exposes_package_keys():
    keys = dir(Config())
    assert "HAVE_K3D" in keys
    assert "K3D_VERSION" in keys
