#!/usr/bin/env python
#
# ~~~
# This file is part of the dune-gdt project:
#   https://github.com/dune-community/dune-gdt
# Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
# License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
#      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
#          with "runtime exception" (http://www.dune-project.org/license.html)
# Authors:
#   Felix Schindler (2017)
#   Rene Fritze     (2016, 2018)
# ~~~

# Static package metadata lives in pyproject.toml ([project] table). This file only carries what cannot be expressed
# declaratively:
#   * the platform (non-pure) wheel forcing via BinaryDistribution/InstallPlatlib,
#   * the version and the dune.xt dependency/extras, which are pinned to the exact same version and are read from the
#     CMake-generated dune/gdt/_version.py (see dune/gdt/_version.py.in), and
#   * package discovery and the `dune` namespace package.

from pathlib import Path

from setuptools import find_packages, setup
from setuptools.command.install import install
from setuptools.dist import Distribution

HERE = Path(__file__).parent

# Read the CMake-generated version module directly. We must not `import dune.gdt`, which pulls in the compiled extension
# modules; _version.py contains only plain assignments, so executing it in an empty namespace is safe.
_version = {}
exec((HERE / "dune" / "gdt" / "_version.py").read_text(), _version)
version = _version["__git_revision__"]

install_requires = [f"dune.xt=={version}"]

extras_require = {
    "visualisation": [f"dune-xt[visualisation]=={version}"],
    "docs": [f"dune-xt[docs]=={version}"],
    "examples": [f"dune-xt[examples]=={version}"],
    "infrastructure": [f"dune-xt[infrastructure]=={version}"],
    "parallel": [f"dune-xt[parallel]=={version}"],
}
extras_require["all"] = [p for plist in extras_require.values() for p in plist]


class BinaryDistribution(Distribution):
    """Distribution which always forces a binary package with platform name"""

    def is_pure(self):
        return False

    def has_ext_modules(self):
        return True


class InstallPlatlib(install):
    def finalize_options(self):
        install.finalize_options(self)
        self.install_lib = self.install_platlib


setup(
    version=version,
    namespace_packages=["dune"],
    packages=find_packages(),
    package_data={"": ["*.so"]},
    include_package_data=True,
    cmdclass={
        "install": InstallPlatlib,
    },
    distclass=BinaryDistribution,
    install_requires=install_requires,
    extras_require=extras_require,
)
