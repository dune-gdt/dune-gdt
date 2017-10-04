#!/usr/bin/env python3
""" Recursively walk the current working directory looking for broken
symlinks. If any broken symlinks are found print out a report and exit
with status 1. If none are found exit with status 0.

"""
import sys
import os

def _target(path):
    """Resolve relative symlinks"""
    try:
        from pathlib import Path
        return Path(path).resolve()
    except ImportError:
        pass
    target_path = os.readlink(path)
    if not os.path.isabs(target_path):
        target_path = os.path.join(os.path.dirname(path),target_path)
    return str(target_path)

broken = []
for root, dirs, files in os.walk('.'):
    if root.startswith('./.git'):
        # Ignore the .git directory.
        continue
    for filename in files:
        path = os.path.join(root,filename)
        if os.path.islink(path):
            target_path = _target(path)
            if not os.path.exists(target_path):
                broken.append('{} --> {}'.format(path, target_path))
        else:
            # If it's not a symlink we're not interested.
            continue

if len(broken) == 0:
    sys.exit(0)

print('\nbroken symlink(s) found: {}'.format('\n'.join(broken)), file=sys.stderr)
sys.exit(1)
