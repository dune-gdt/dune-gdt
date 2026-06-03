from importlib import metadata

import pytest
from packaging.version import Version


@pytest.mark.parametrize("module", ["dune.gdt", "dune.xt"])
def test_version(module):
    version_str = metadata.version(module)
    assert version_str is not None
    assert version_str != "unknown"
    assert Version(version_str) > Version("2022.0.0")
