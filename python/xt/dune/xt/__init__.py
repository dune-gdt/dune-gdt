# ~~~
# This file is part of the dune-gdt project:
#   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-gdt
# Copyright 2010-2021 dune-gdt developers and contributors. All rights reserved.
# License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
#      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
#          with "runtime exception" (http://www.dune-project.org/license.html)
# Authors:
#   Felix Schindler (2013 - 2017, 2019 - 2021)
#   René Fritze     (2012, 2015 - 2016, 2018 - 2020, 2022)
#   Tim Keil        (2019)
#   Tobias Leibner  (2019 - 2021)
#   f_albr01        (2010)
#
# This file is part of the dune-xt project:
# ~~~

# Note: This import makes sure that metis is imported before python bindings from dune-xt. Importing dune-xt python
# bindings at any point before metis will cause a segfault due to misscommunication between libmetis5 and
# libscotchmetis.

try:
    import metis
except ImportError:
    pass
except RuntimeError:     # metis actually fires a RuntimeError if the module is installed but the system package not
    pass

from importlib import import_module


def guarded_import(globs, base_name, mod_name):
    # see https://stackoverflow.com/questions/43059267/how-to-do-from-module-import-using-importlib
    try:
        mod = import_module('.{}'.format(mod_name), base_name)
    except ImportError as e:
        import logging
        logging.getLogger('dune.xt').fatal(f'cannot load {mod_name} into {base_name}:\n{e}')
        raise e
    if "__all__" in mod.__dict__:
        names = mod.__dict__["__all__"]
    else:
        names = [x for x in mod.__dict__ if not x.startswith("_")]
    # check for duplicity
    for nm in names:
        if nm in globs:
            raise ImportError(f'{base_name}: name \'{nm}\' already exists (when importing from \'{mod_name}\'!)')
    # and import
    globs.update({k: getattr(mod, k) for k in names})


from ._version import __version__
