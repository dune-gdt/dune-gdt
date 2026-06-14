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

# Hatchling hooks that supply what cannot be expressed declaratively in pyproject.toml:
#
#   * CustomMetadataHook supplies the build-time-dynamic `dependencies` and `optional-dependencies`. Both pin dune.xt to
#     the exact same version as this package, which is recorded as __git_revision__ in the CMake-generated
#     dune/gdt/_version.py. We read that file directly instead of importing dune.gdt, which would pull in the compiled
#     extension modules; _version.py contains only plain assignments, so executing it in an empty namespace is safe.
#   * CustomBuildHook forces a platform (non-pure) wheel. The package ships pre-built .so extensions and is passed
#     through `auditwheel repair`, which rejects pure py3-none-any wheels. hatchling exposes no declarative option for
#     this, so we set the build-data flags here: pure_python=False (Root-Is-Purelib: false) and infer_tag=True (stamp
#     the building interpreter's platform tag, e.g. cp313-cp313-linux_x86_64, instead of py3-none-any).

from pathlib import Path

from hatchling.builders.hooks.plugin.interface import BuildHookInterface
from hatchling.metadata.plugin.interface import MetadataHookInterface


class CustomMetadataHook(MetadataHookInterface):
    def update(self, metadata):
        _version = {}
        exec((Path(self.root) / "dune" / "gdt" / "_version.py").read_text(), _version)
        version = _version["__git_revision__"]

        metadata["dependencies"] = [f"dune.xt=={version}"]

        optional_dependencies = {
            "visualisation": [f"dune-xt[visualisation]=={version}"],
            "docs": [f"dune-xt[docs]=={version}"],
            "examples": [f"dune-xt[examples]=={version}"],
            "infrastructure": [f"dune-xt[infrastructure]=={version}"],
            "parallel": [f"dune-xt[parallel]=={version}"],
        }
        optional_dependencies["all"] = [
            p for plist in optional_dependencies.values() for p in plist
        ]
        metadata["optional-dependencies"] = optional_dependencies


class CustomBuildHook(BuildHookInterface):
    def initialize(self, version, build_data):
        build_data["pure_python"] = False
        build_data["infer_tag"] = True
