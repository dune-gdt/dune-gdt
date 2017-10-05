#!/usr/bin/env python3
""" Recursively walk the current working directory looking for broken
symlinks. If any broken symlinks are found print out a report and exit
with status 1. If none are found exit with status 0.

"""
import sys
import os
from pathlib import Path

def _resolve(path):
    try:
        return path.resolve(False)
    except TypeError:
        # fallback for py < 3.6
        try:
            return path.resolve()
        except FileNotFoundError:
            return Path(os.readlink(str(path)))

broken = []
for root, dirs, files in os.walk('.'):
    if root.startswith('./.git'):
        # Ignore the .git directory.
        continue
    for filename in files:
        path = Path(os.path.join(root,filename))
        if path.is_symlink():
            target = _resolve(path)
            # exists already checks if the pointed to file is there
            if not path.exists():
                broken.append('{} --> {}'.format(path, target))
        else:
            # If it's not a symlink we're not interested.
            continue

if len(broken) == 0:
    sys.exit(0)

print('\nbroken symlink(s) found: {}'.format('\n'.join(broken)), file=sys.stderr)
sys.exit(1)
