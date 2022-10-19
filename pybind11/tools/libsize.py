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
#   Felix Schindler (2017)
#   René Fritze     (2020)
#   Tobias Leibner  (2021)
# ~~~

from __future__ import print_function, division
import os
import sys

# Internal build script for generating debugging test .so size.
# Usage:
#     python libsize.py file.so save.txt -- displays the size of file.so and, if save.txt exists, compares it to the
#                                           size in it, then overwrites save.txt with the new size for future runs.

if len(sys.argv) != 3:
    sys.exit("Invalid arguments: usage: python libsize.py file.so save.txt")

lib = sys.argv[1]
save = sys.argv[2]

if not os.path.exists(lib):
    sys.exit("Error: requested file ({}) does not exist".format(lib))

libsize = os.path.getsize(lib)

print("------", os.path.basename(lib), "file size:", libsize, end="")

if os.path.exists(save):
    with open(save) as sf:
        oldsize = int(sf.readline())

    if oldsize > 0:
        change = libsize - oldsize
        if change == 0:
            print(" (no change)")
        else:
            print(" (change of {:+} bytes = {:+.2%})".format(change, change / oldsize))
else:
    print()

with open(save, "w") as sf:
    sf.write(str(libsize))
