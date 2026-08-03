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
import sys
import types

import pytest

# The wrapper sits next to this test dir under wrapper/, ships as a console script (not importable as a package) and
# pulls in dune.testtools, which is not installable from an index. We stub dune.testtools and load the module from its
# file path so call() can be exercised without the real dependency or executing a child process.
_WRAPPER = (
    pathlib.Path(__file__).resolve().parent.parent / "wrapper" / "dune_xt_execute.py"
)


class _RecordingSubprocess:
    """Stand-in for the subprocess module that records the last call() invocation."""

    def __init__(self, returncode=0):
        self.returncode = returncode
        self.calls = []

    def call(self, command, env=None):
        self.calls.append((list(command), env))
        return self.returncode


def _load_module(monkeypatch, parse_result=None, returncode=0):
    """Load dune_xt_execute with dune.testtools stubbed out and subprocess recorded.

    The module runs get_args()/sys.exit() at import time, so we feed it a minimal argv and catch the SystemExit; the
    returned module then exposes call() for direct testing.
    """
    parse_result = {} if parse_result is None else parse_result

    parser_mod = types.ModuleType("dune.testtools.parser")
    parser_mod.parse_ini_file = lambda inifile: parse_result
    argparser_mod = types.ModuleType("dune.testtools.wrapper.argumentparser")
    argparser_mod.get_args = lambda: {"exec": "the_exe", "ini": "the.ini"}

    monkeypatch.setitem(
        sys.modules, "dune.testtools", types.ModuleType("dune.testtools")
    )
    monkeypatch.setitem(sys.modules, "dune.testtools.parser", parser_mod)
    monkeypatch.setitem(
        sys.modules,
        "dune.testtools.wrapper",
        types.ModuleType("dune.testtools.wrapper"),
    )
    monkeypatch.setitem(
        sys.modules, "dune.testtools.wrapper.argumentparser", argparser_mod
    )

    recording = _RecordingSubprocess(returncode=returncode)
    monkeypatch.setitem(sys.modules, "subprocess", recording)
    monkeypatch.setattr(sys, "argv", ["dune_xt_execute.py"])

    spec = importlib.util.spec_from_file_location(
        "dune_xt_execute_under_test", _WRAPPER
    )
    mod = importlib.util.module_from_spec(spec)
    # Module-level code ends in sys.exit(call(...)); swallow it and keep the loaded module.
    with pytest.raises(SystemExit):
        spec.loader.exec_module(mod)
    return mod, recording


def test_call_without_inifile(monkeypatch):
    mod, recording = _load_module(monkeypatch)
    recording.calls.clear()
    rc = mod.call("my_exe")
    assert rc == 0
    ((command, env),) = recording.calls
    assert command == ["my_exe"]
    assert env is mod.os.environ


def test_call_with_plain_inifile(monkeypatch):
    mod, recording = _load_module(monkeypatch, parse_result={})
    recording.calls.clear()
    mod.call("my_exe", "params.ini")
    ((command, _),) = recording.calls
    assert command == ["my_exe", "params.ini"]


def test_call_with_inifile_optionkey(monkeypatch):
    mod, recording = _load_module(
        monkeypatch, parse_result={"__inifile_optionkey": "-ini"}
    )
    recording.calls.clear()
    mod.call("my_exe", "params.ini")
    ((command, _),) = recording.calls
    # the option key is inserted before the inifile path
    assert command == ["my_exe", "-ini", "params.ini"]


def test_call_appends_additional_args(monkeypatch):
    mod, recording = _load_module(monkeypatch, parse_result={})
    recording.calls.clear()
    mod.call("my_exe", "params.ini", "--gtest_output=xml:out.xml", "--extra")
    ((command, _),) = recording.calls
    assert command == ["my_exe", "params.ini", "--gtest_output=xml:out.xml", "--extra"]


def test_call_propagates_returncode(monkeypatch):
    mod, recording = _load_module(monkeypatch, returncode=42)
    recording.calls.clear()
    assert mod.call("my_exe") == 42
