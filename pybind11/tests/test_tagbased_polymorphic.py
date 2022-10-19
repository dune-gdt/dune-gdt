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
#   René Fritze    (2019 - 2020)
#   Tobias Leibner (2021)
# ~~~

from pybind11_tests import tagbased_polymorphic as m


def test_downcast():
    zoo = m.create_zoo()
    assert [type(animal) for animal in zoo] == [
        m.Labrador,
        m.Dog,
        m.Chihuahua,
        m.Cat,
        m.Panther,
    ]
    assert [animal.name for animal in zoo] == [
        "Fido",
        "Ginger",
        "Hertzl",
        "Tiger",
        "Leo",
    ]
    zoo[1].sound = "woooooo"
    assert [dog.bark() for dog in zoo[:3]] == [
        "Labrador Fido goes WOOF!",
        "Dog Ginger goes woooooo",
        "Chihuahua Hertzl goes iyiyiyiyiyi and runs in circles",
    ]
    assert [cat.purr() for cat in zoo[3:]] == ["mrowr", "mrrrRRRRRR"]
    zoo[0].excitement -= 1000
    assert zoo[0].excitement == 14000
