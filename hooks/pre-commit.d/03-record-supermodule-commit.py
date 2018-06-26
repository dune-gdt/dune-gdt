#!/usr/bin/env python3
""" Put the supermodules's remote, status and commit
into .gitsuper
"""
import sys
import os
from pathlib import Path
import subprocess
from configparser import ConfigParser
from functools import partial

root = Path(os.getcwd())
supermod = Path(root, '..')
modfile_fn = root / '.gitsuper'
get_output = partial(subprocess.check_output, universal_newlines=True)

def for_module(cf, mod, section=None):
    os.chdir(mod)
    remote = get_output(['git', 'remote', 'get-url', 'origin']).strip()
    status = get_output(['git', 'submodule', 'status']).strip()
    commit = get_output(['git', 'rev-parse', 'HEAD']).strip()
    section = section or 'submodule.{}'.format(mod.name)
    cf.add_section(section)
    cf.set(section, 'remote', remote)
    if status:
        cf.set(section, 'status', status)
    cf.set(section, 'commit', commit)

cf = ConfigParser()
for_module(cf, supermod, 'supermodule')
os.chdir(supermod)
for s in [Path(s) for s in
              get_output(['git', 'submodule', '--quiet', 'foreach', 'pwd']).split()]:
    for_module(cf, s)
cf.write(open(modfile_fn, 'wt'))
