#!/usr/bin/env python3
#
# ~~~
# This file is part of the dune-xt project:
#   https://github.com/dune-community/dune-xt
# Copyright 2009-2020 dune-xt developers and contributors. All rights reserved.
# License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
#      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
#          with "runtime exception" (http://www.dune-project.org/license.html)
# Authors:
#   René Fritze    (2019)
#   Tobias Leibner (2019 - 2020)
# ~~~

import os
from os.path import expanduser
from shlex import quote
from pathlib import Path

home = expanduser("~")

prefixes = os.environ.get(
    "ENV_PREFIXES", "BUILD SYSTEM GITLAB CODECOV CI encrypt TOKEN TESTS"
).split(" ")
blacklist = [
    "TRAVIS_COMMIT_MESSAGE",
    "CI_COMMIT_MESSAGE",
    "CI_COMMIT_DESCRIPTION",
]
env_file = Path(os.environ.get("DOCKER_ENVFILE", os.path.join(home, "env")))
env_file.parent.mkdir(parents=True, exist_ok=True)
with env_file.open("wt") as env:
    for k, v in os.environ.items():
        if any(k.startswith(pref) and k not in blacklist for pref in prefixes):
            env.write(f'{k}="{quote(v)}"\n')
