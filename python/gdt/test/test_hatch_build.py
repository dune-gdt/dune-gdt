# ~~~
# This file is part of the dune-xt project:
#   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
# Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
# License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
#      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
#          with "runtime exception" (http://www.dune-project.org/license.html)
# Authors:
#   René Fritze (2024)
# ~~~

import importlib.util
import pathlib

import pytest

# The hooks subclass the hatchling plugin interfaces, so without hatchling there is nothing to test.
pytest.importorskip("hatchling")

# hatch_build.py lives at the package root (next to pyproject.toml), not inside an importable package, and both the xt
# and gdt copies are named `hatch_build`. Load it from its file path under a unique module name to avoid clashes.
_HATCH_BUILD = pathlib.Path(__file__).resolve().parent.parent / "hatch_build.py"


def _load_hatch_build():
    spec = importlib.util.spec_from_file_location("gdt_hatch_build", _HATCH_BUILD)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def _write_version(root, git_revision):
    version_file = root / "dune" / "gdt" / "_version.py"
    version_file.parent.mkdir(parents=True)
    version_file.write_text(
        f'__version__ = "1.2.3"\n__git_revision__ = "{git_revision}"\n',
        encoding="utf-8",
    )


def test_metadata_hook_pins_dune_xt_to_same_version(tmp_path):
    _write_version(tmp_path, "1.2.3-0-gdeadbeef")
    hook = _load_hatch_build().CustomMetadataHook(str(tmp_path), {})
    metadata = {}
    hook.update(metadata)
    assert metadata["dependencies"] == ["dune.xt==1.2.3-0-gdeadbeef"]


def test_metadata_hook_optional_dependencies(tmp_path):
    version = "1.2.3-0-gdeadbeef"
    _write_version(tmp_path, version)
    hook = _load_hatch_build().CustomMetadataHook(str(tmp_path), {})
    metadata = {}
    hook.update(metadata)

    optional = metadata["optional-dependencies"]
    assert optional["visualisation"] == [f"dune.xt[visualisation]=={version}"]
    assert optional["docs"] == [f"dune.xt[docs]=={version}"]
    assert optional["examples"] == [f"dune.xt[examples]=={version}"]
    assert optional["infrastructure"] == [f"dune.xt[infrastructure]=={version}"]
    assert optional["parallel"] == [f"dune.xt[parallel]=={version}"]
    # `all` aggregates every other extra (and nothing else).
    assert optional["all"] == [
        f"dune.xt[visualisation]=={version}",
        f"dune.xt[docs]=={version}",
        f"dune.xt[examples]=={version}",
        f"dune.xt[infrastructure]=={version}",
        f"dune.xt[parallel]=={version}",
    ]


def test_build_hook_forces_platform_wheel(tmp_path):
    mod = _load_hatch_build()
    hook = mod.CustomBuildHook(str(tmp_path), {}, None, None, str(tmp_path), "wheel")
    build_data = {}
    hook.initialize("standard", build_data)
    assert build_data["pure_python"] is False
    assert build_data["infer_tag"] is True
