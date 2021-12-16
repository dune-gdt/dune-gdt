# This file is part of the pyMOR project (https://www.pymor.org).
# Copyright 2013-2021 pyMOR developers and contributors. All rights reserved.
# License: BSD 2-Clause License (https://opensource.org/licenses/BSD-2-Clause)
import os
import sys
from pathlib import Path
import io
import importlib.machinery
import importlib.util
from contextlib import contextmanager
from docutils.core import publish_doctree
from docutils.parsers.rst import Directive
from docutils.parsers.rst.directives import flag, register_directive
import pytest


TUT_DIR = Path(os.path.dirname(__file__)).resolve() / 'source'
_exclude_files = ['tutorial_external_solver.rst']
EXCLUDE = [TUT_DIR / t for t in _exclude_files]
TUTORIALS = [t for t in TUT_DIR.glob('tutorial_*rst') if t not in EXCLUDE]


def runmodule(filename):
    import pytest

    sys.exit(pytest.main(sys.argv[1:] + [filename]))


@contextmanager
def change_to_directory(name):
    """Change current working directory to `name` for the scope of the context."""
    old_cwd = os.getcwd()
    try:
        yield os.chdir(name)
    finally:
        os.chdir(old_cwd)


def _test_demo(demo):
    import sys
    sys._called_from_test = True

    def nop(*args, **kwargs):
        pass

    try:
        from matplotlib import pyplot
        if sys.version_info[:2] > (3, 7) or (
                sys.version_info[0] == 3 and sys.version_info[1] == 6):
            pyplot.ion()
        else:
            # the ion switch results in interpreter segfaults during multiple
            # demo tests on 3.7 -> fall back on old monkeying solution
            pyplot.show = nop
    except ImportError:
        pass
    try:
        import petsc4py
        # the default X handlers can interfere with process termination
        petsc4py.PETSc.Sys.popSignalHandler()
        petsc4py.PETSc.Sys.popErrorHandler()
    except ImportError:
        pass

    # reset default RandomState
    import pymor.tools.random
    pymor.tools.random._default_random_state = None

    result = None
    try:
        result = demo()
    except (QtMissing, GmshMissing, MeshioMissing, TorchMissing) as e:
        if os.environ.get('DOCKER_PYMOR', False):
            # these are all installed in our CI env so them missing is a grave error
            raise e
        else:
            miss = str(type(e)).replace('Missing', '')
            pytest.xfail(f'{miss} not installed')
    finally:
        from pymor.parallel.default import _cleanup
        _cleanup()
        try:
            from matplotlib import pyplot
            pyplot.close('all')
        except ImportError:
            pass

    return result

class CodeCell(Directive):

    required_arguments = 0
    optional_arguments = 0
    final_argument_whitespace = True
    option_spec = {'hide-output': flag,
                   'hide-code': flag,
                   'raises': flag}
    has_content = True

    def run(self):
        self.assert_has_content()
        if 'raises' in self.options:
            text = 'try:\n    ' + '\n    '.join(
                self.content) + '\nexcept:\n    import traceback; traceback.print_exc()'
        else:
            text = '\n'.join(self.content)
        print('# %%')
        print(text)
        print()
        return []


@pytest.fixture(params=TUTORIALS, ids=[t.name for t in TUTORIALS])
def tutorial_code(request):
    filename = request.param
    with change_to_directory(TUT_DIR):
        code = io.StringIO()
        register_directive('jupyter-execute', CodeCell)
        with open(filename, 'rt') as f:
            original = sys.stdout
            sys.stdout = code
            publish_doctree(f.read(), settings_overrides={'report_level': 42})
            sys.stdout = original
    code.seek(0)
    source_fn = Path(f'{str(filename).replace(".rst", "_rst")}_extracted.py')
    with open(source_fn, 'wt') as source:
        # filter line magics
        source.write(''.join([line for line in code.readlines() if not line.startswith('%')]))
    return request.param, source_fn


def test_tutorial(tutorial_code):
    filename, source_module_path = tutorial_code

    # make sure (picture) resources can be loaded as in sphinx-build
    with change_to_directory(TUT_DIR):
        def _run():
            loader = importlib.machinery.SourceFileLoader(source_module_path.stem, str(source_module_path))
            spec = importlib.util.spec_from_loader(loader.name, loader)
            mod = importlib.util.module_from_spec(spec)
            loader.exec_module(mod)
        try:
            # wrap module execution in hacks to auto-close Qt-Apps, etc.
            _test_demo(_run)
        except Exception as e:
            print(f'Failed: {source_module_path}')
            raise e


if __name__ == "__main__":
    runmodule(filename=__file__)
