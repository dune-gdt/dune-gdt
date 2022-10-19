#  -*- coding: utf-8 -*-

#
# ~~~
# This file is part of the dune-gdt project:
#   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-gdt
# Copyright 2010-2021 dune-gdt developers and contributors. All rights reserved.
# License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
#      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
#          with "runtime exception" (http://www.dune-project.org/license.html)
# Authors:
#   René Fritze    (2020)
#   Tobias Leibner (2021)
# ~~~

import pytest

asyncio = pytest.importorskip("asyncio")
m = pytest.importorskip("pybind11_tests.async_module")


@pytest.fixture
def event_loop():
    loop = asyncio.new_event_loop()
    yield loop
    loop.close()


async def get_await_result(x):
    return await x


def test_await(event_loop):
    assert 5 == event_loop.run_until_complete(get_await_result(m.SupportsAsync()))


def test_await_missing(event_loop):
    with pytest.raises(TypeError):
        event_loop.run_until_complete(get_await_result(m.DoesNotSupportAsync()))
