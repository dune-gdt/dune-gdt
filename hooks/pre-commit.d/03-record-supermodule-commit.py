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

get_output = partial(subprocess.check_output, universal_newlines=True)
root = Path(get_output(['git', 'rev-parse', '--show-toplevel']).strip())
supermod = Path(root, '..')
modfile_fn = root / '.gitsuper'


def get_env(mod):
    env = os.environ.copy()
    env['GIT_WORKING_TREE'] = str(mod)
    env['GIT_DIR'] = str(mod / '.git')
    del env['GIT_INDEX_FILE']
    return env


def for_module(cf, mod, section=None):
    mod = mod.resolve()
    logging.error('entering {}'.format(mod))
    os.chdir(mod)
    env = get_env(mod)
    remote = get_output(['git', 'remote', 'get-url', 'origin'], env=env).strip()
    commit = get_output(['git', 'rev-parse', 'HEAD'], env=env).strip()
    section = section or 'submodule.{}'.format(mod.name)
    cf.add_section(section)
    cf.set(section, 'remote', remote)
    try:
        status = get_output(['git', 'submodule', 'status'], env=env).strip()
        cf.set(section, 'status', status)
    except subprocess.CalledProcessError:
        pass
    cf.set(section, 'commit', commit)

cf = ConfigParser()
for_module(cf, supermod, 'supermodule')
os.chdir(supermod)
for s in [Path(s) for s in
              get_output(['git', 'submodule', '--quiet', 'foreach', 'pwd'],
                         env=get_env(supermod)).split()]:
    for_module(cf, s)
cf.write(open(modfile_fn, 'wt'))

os.chdir(root)
subprocess.check_call(['git', 'add', str(modfile_fn)])
