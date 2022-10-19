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
#   Felix Schindler (2016 - 2017)
#   René Fritze     (2019 - 2020)
#   Tobias Leibner  (2021)
# ~~~

from ._version import version_info, __version__
from .commands import get_include, get_cmake_dir

__all__ = (
    "version_info",
    "__version__",
    "get_include",
    "get_cmake_dir",
)
