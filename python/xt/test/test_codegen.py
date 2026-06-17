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

from dune.xt.codegen import (
    have_eigen,
    have_istl,
    is_found,
    la_backends,
    typeid_to_typedef_name,
)


def test_typeid_to_typedef_name_replaces_all_illegal_chars():
    dirty = "a-b>c<d:e f,g+h.i"
    assert typeid_to_typedef_name(dirty) == "a_b_c_d_e_f_g_h_i"


def test_typeid_to_typedef_name_custom_replacement():
    assert typeid_to_typedef_name("a-b", replacement="X") == "aXb"


def test_typeid_to_typedef_name_clean_input_unchanged():
    assert typeid_to_typedef_name("Already_clean99") == "Already_clean99"


def test_is_found_present_and_valid():
    assert is_found({"FOO": "/usr/include/foo"}, "FOO") is True


def test_is_found_notfound_is_case_insensitive():
    assert is_found({"FOO": "FOO-NOTFOUND"}, "FOO") is False
    assert is_found({"FOO": "foo-NotFound"}, "FOO") is False


def test_is_found_missing_key():
    assert is_found({}, "FOO") is False


def test_have_eigen_and_istl():
    found = {"EIGEN3_INCLUDE_DIR": "/usr/include/eigen3", "dune-istl_DIR": "/opt/istl"}
    missing = {
        "EIGEN3_INCLUDE_DIR": "EIGEN3_INCLUDE_DIR-NOTFOUND",
        "dune-istl_DIR": "dune-istl_DIR-NOTFOUND",
    }
    assert have_eigen(found) is True
    assert have_istl(found) is True
    assert have_eigen(missing) is False
    assert have_istl(missing) is False


def test_la_backends_both_available():
    cache = {
        "EIGEN3_INCLUDE_DIR": "/usr/include/eigen3",
        "dune-istl_DIR": "/opt/istl",
    }
    assert la_backends(cache) == ["eigen_sparse", "istl_sparse"]


def test_la_backends_skips_missing_istl():
    # Regression test for the `if have_istl:` bug (a function object is always
    # truthy): with istl not found, `istl_sparse` must NOT be reported.
    cache = {
        "EIGEN3_INCLUDE_DIR": "/usr/include/eigen3",
        "dune-istl_DIR": "dune-istl_DIR-NOTFOUND",
    }
    assert la_backends(cache) == ["eigen_sparse"]


def test_la_backends_empty_cache():
    assert la_backends({}) == []
