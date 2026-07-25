# ~~~
# This file is part of the dune-xt project:
#   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
# Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
# License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
#      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
#          with "runtime exception" (http://www.dune-project.org/license.html)
# Authors:
#   Felix Schindler (2017, 2019)
#   René Fritze     (2017 - 2019)
#   Tobias Leibner  (2018 - 2020)
# ~~~

import os
import pprint

# The constants CMake's own `if()` treats as false; everything else (a non-zero number, any other
# word) is true. See https://cmake.org/cmake/help/latest/command/if.html#constant.
_CMAKE_FALSE_CONSTANTS = frozenset(
    ("", "0", "OFF", "NO", "FALSE", "N", "IGNORE", "NOTFOUND")
)


def is_cmake_true(value):
    """Evaluate a CMake constant the way `if()` would."""
    if isinstance(value, bool):
        return value
    value = value.strip().upper()
    return not (value in _CMAKE_FALSE_CONSTANTS or value.endswith("-NOTFOUND"))


def parse_cache(filepath):
    import pyparsing as p

    EQ = p.Literal("=").suppress()
    DP = p.Literal(":").suppress()
    token = p.Word(p.alphanums + "/_- .")
    line = p.Group(token("key") + DP + token("type") + EQ + p.restOfLine("value"))
    line.ignore(p.pythonStyleComment)
    line.ignore(p.dblSlashComment)
    grammar = p.OneOrMore(line)

    kv = {}
    types = {}
    for key, type, value in grammar.parseFile(filepath, parseAll=True):
        value = value.strip()
        types[key] = type
        if type == "BOOL":
            # Hand BOOL entries to the configs as actual booleans. The guards read them either by
            # truthiness (grid_types._is_usable) or by looking for "notfound" in the string
            # (codegen.is_found), and both of those would take the string "FALSE" for a yes.
            kv[key] = is_cmake_true(value)
            continue
        kv[key] = value
        if key.endswith("_DIR"):
            kv[key[:-4]] = os.path.isdir(value)
    return kv, types


if __name__ == "__main__":
    import sys

    # Read the cache path from the command line (default to the CMakeCache.txt in
    # the current build directory). Avoids reading from a hardcoded, publicly
    # writable location such as /tmp (SonarCloud python:S5443).
    cache_file = sys.argv[1] if len(sys.argv) > 1 else "CMakeCache.txt"
    kv, types = parse_cache(cache_file)
    pprint.pprint(kv)
