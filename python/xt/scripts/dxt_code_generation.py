#!/usr/bin/env python3
#
# ~~~
# This file is part of the dune-xt project:
#   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
# Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
# License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
#      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
#          with "runtime exception" (http://www.dune-project.org/license.html)
# Authors:
#   Felix Schindler (2017)
#   René Fritze     (2017 - 2019)
#   Tobias Leibner  (2019 - 2020)
# ~~~

import logging
import os
import sys
from runpy import run_path

from jinja2 import Template

from dune.xt.cmake import parse_cache


def generate(config_fn, tpl_fn, cmake_binary_dir, out_fn, backup_bindir, logger=None):
    logger = logger or logging.getLogger("Codegen")
    cache_path = os.path.join(cmake_binary_dir, "CMakeCache.txt")
    try:
        cache, _ = parse_cache(cache_path)
    except FileNotFoundError as fe:
        logger.critical(f"using fallback cache instead of {cache_path}: {str(fe)}")
        cache, _ = parse_cache(os.path.join(backup_bindir, "CMakeCache.txt"))
    sys.path.append(os.path.dirname(config_fn))
    config = run_path(config_fn, init_globals=locals(), run_name="__dxt_codegen__")

    dir_base = os.path.dirname(out_fn)
    if dir_base and not os.path.isdir(dir_base):
        os.makedirs(dir_base)
    # Validate the (CLI-provided) template path before reading it, so a faulty
    # argument cannot be used to read an unexpected file (SonarQube S8707).
    tpl_path = os.path.realpath(tpl_fn)
    if not os.path.isfile(tpl_path):
        raise FileNotFoundError(f"template file not found: {tpl_fn}")
    with open(tpl_path) as tpl:
        template = Template(tpl.read())

    try:
        for postfix, cfg in config["multi_out"].items():
            fn = f"{out_fn}.{postfix}"
            with open(fn, "w") as out:
                out.write(template.render(config=cfg, cache=cache))
    except KeyError:
        with open(out_fn, "w") as out:
            out.write(template.render(config=config, cache=cache))


def main(argv=None):
    argv = sys.argv if argv is None else argv
    generate(argv[1], argv[2], argv[3], argv[4], argv[5])


if __name__ == "__main__":
    main()
