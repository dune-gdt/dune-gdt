#!/usr/bin/env python3
""" Put the supermodules's remote, status and commit
into .gitsuper
"""
import sys
import os
from pathlib import Path
import subprocess
from configparser import ConfigParser


root = Path(os.getcwd())
supermod = Path(root, '..')
modfile_fn = root / '.gitsuper'
os.chdir(supermod)
remote = subprocess.check_output(['git', 'remote', 'get-url', 'origin'], universal_newlines=True)
status = subprocess.check_output(['git', 'submodule', 'status'], universal_newlines=True)
commit = subprocess.check_output(['git', 'rev-parse', 'HEAD'], universal_newlines=True)
section = 'supermodule'
cf = ConfigParser(default_section=section)
cf.set(section, 'remote', remote)
cf.set(section, 'status', status)
cf.set(section, 'commit', commit)
cf.write(open(modfile_fn, 'wt'))
