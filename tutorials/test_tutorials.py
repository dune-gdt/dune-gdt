import os
import sys
from pathlib import Path

import pytest
from pytest_notebook.nb_regression import NBRegressionFixture
from pytest_notebook.plugin import gather_config_options

TUT_DIR = Path(os.path.dirname(__file__)).resolve().absolute()
_exclude_files = []
EXCLUDE = [TUT_DIR / t for t in _exclude_files]
TUTORIALS = [t for t in TUT_DIR.glob("converted_example_*ipynb") if t not in EXCLUDE]


def runmodule(filename):
    import pytest

    sys.exit(pytest.main(sys.argv[1:] + [filename]))


class NBLaxFixture(NBRegressionFixture):
    """Same functionality as base class, but result comparison for regressions is skipped"""

    def check(self, path):
        return super().check(path=path, raise_errors=False)


@pytest.fixture(scope="function")
def nb_lax(pytestconfig):
    kwargs, other_args = gather_config_options(pytestconfig)
    return NBLaxFixture(**kwargs)


@pytest.mark.parametrize("filename", TUTORIALS, ids=[t.name for t in TUTORIALS])
def test_check(filename, nb_lax):
    nb_lax.check(filename)


if __name__ == "__main__":
    runmodule(filename=__file__)
