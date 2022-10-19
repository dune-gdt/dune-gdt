# ~~~
# This file is part of the dune-gdt project:
#   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-gdt
# Copyright 2010-2021 dune-gdt developers and contributors. All rights reserved.
# License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
#      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
#          with "runtime exception" (http://www.dune-project.org/license.html)
# Authors:
#   Felix Schindler (2019 - 2020, 2022)
#   René Fritze     (2018 - 2019, 2022)
#   Tobias Leibner  (2019 - 2021)
#
# This file is part of the dune-xt project:
# ~~~

from dune.xt.test import grid_types as types


def test_load_all():
    import dune.xt.common  # noqa:F401
    import dune.xt.la  # noqa:F401
    import dune.xt.grid  # noqa:F401
    import dune.xt.functions  # noqa:F401


def test_empty():
    from dune.xt.common._common_empty import Dog, Pet, Terrier

    dog = Dog('Susi')
    pet = Pet('Bello')
    ter = Terrier()

    assert ter.getName() == 'Berti'
    assert pet.getName() == 'Bello'
    assert ter.bark() == 'woof!'
    assert dog.bark() == 'woof!'


def test_logging():
    import dune.xt.common.logging as lg
    lg.create(lg.log_max)
    lg.info('log info test')
    lg.error('log error test')
    lg.debug('log debug test')


def test_timings():
    from dune.xt.common.timings import instance
    timings = instance()
    timings.reset()
    timings.start("foo.bar")
    timings.stop()
    timings.output_simple()


if __name__ == '__main__':
    from dune.xt.test.base import runmodule
    runmodule(__file__)


def test_types():
    cache = {}
    rt = types.all_types(cache=cache, dims=(2, 3))
    assert len(rt) == 0
