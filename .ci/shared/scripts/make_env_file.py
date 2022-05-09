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
home = expanduser("~")

prefixes = os.environ.get('ENV_PREFIXES', 'BUILD SYSTEM GITLAB CODECOV CI encrypt TOKEN TESTS').split(' ')
blacklist = [
    'TRAVIS_COMMIT_MESSAGE',
    'CI_COMMIT_MESSAGE',
    'CI_COMMIT_DESCRIPTION',
]
env_file = os.environ.get('DOCKER_ENVFILE', os.path.join(home, 'env'))
with open(env_file, 'wt') as env:
    for k, v in os.environ.items():
        for pref in prefixes:
            if k.startswith(pref) and k not in blacklist:
                env.write('{}="{}"\n'.format(k, quote(v)))
