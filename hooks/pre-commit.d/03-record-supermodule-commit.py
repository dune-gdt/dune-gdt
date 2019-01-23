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
import logging


get_output = partial(subprocess.check_output, universal_newlines=True)
root = Path(get_output(['git', 'rev-parse', '--show-toplevel']).strip())
supermod = Path(root, '..')
supermod_name = supermod.resolve().parts[-1]
modfile_fn = root / '.gitsuper'


def get_env(mod):
    env = os.environ.copy()
    env['GIT_WORKING_TREE'] = str(mod)
    env['GIT_DIR'] = str(mod / '.git')
    del env['GIT_INDEX_FILE']
    return env


def _save_config(super, mod, cf):
    target_dir = Path(os.path.expandvars('$HOME')) / '.local' / 'share' / 'dxt' / 'gitsuper'
    if not target_dir.is_dir():
        logging.error('Not a directory {}, skipping gitsuper committing'.format(target_dir))
        return
    target = target_dir / mod
    os.chdir(target_dir)
    env = get_env(target_dir)
    with open(target, 'wt') as out:
        cf.write(out)
    subprocess.check_output(['git', 'add', str(target)], env=env)
    subprocess.check_output(['git', 'commit', '-m', '"Updated {} from {}"'.format(mod, super)], env=env)


def for_module(cf, mod, section=None):
    mod = mod.resolve()
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
    return cf


cf = ConfigParser()
for_module(cf, supermod, 'supermodule')
os.chdir(supermod)
for s in [Path(s) for s in
              get_output(['git', 'submodule', '--quiet', 'foreach', 'pwd'],
                         env=get_env(supermod)).split()]:
    for_module(cf, s)
_save_config(supermod_name, root.parts[-1], cf)

