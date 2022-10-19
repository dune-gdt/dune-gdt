#!/usr/bin/env python3
#  -*- coding: utf-8 -*-

#
# ~~~
# This file is part of the dune-gdt project:
#   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-gdt
# Copyright 2010-2021 dune-gdt developers and contributors. All rights reserved.
# License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
#      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
#          with "runtime exception" (http://www.dune-project.org/license.html)
# Authors:
#   Tobias Leibner (2021)
# ~~~

import re

import ghapi.all

from rich import print
from rich.syntax import Syntax

ENTRY = re.compile(
    r"""
    Suggested \s changelog \s entry:
    .*
    ```rst
    \s*
    (.*?)
    \s*
    ```
""",
    re.DOTALL | re.VERBOSE,
)

print()

api = ghapi.all.GhApi(owner="pybind", repo="pybind11")

issues_pages = ghapi.page.paged(api.issues.list_for_repo, labels="needs changelog", state="closed")
issues = (issue for page in issues_pages for issue in page)
missing = []

for issue in issues:
    changelog = ENTRY.findall(issue.body)
    if changelog:
        (msg,) = changelog
        if not msg.startswith("* "):
            msg = "* " + msg
        if not msg.endswith("."):
            msg += "."

        msg += f"\n  `#{issue.number} <{issue.html_url}>`_"

        print(Syntax(msg, "rst", theme="ansi_light", word_wrap=True))
        print()

    else:
        missing.append(issue)

if missing:
    print()
    print("[blue]" + "-" * 30)
    print()

    for issue in missing:
        print(f"[red bold]Missing:[/red bold][red] {issue.title}")
        print(f"[red]  {issue.html_url}\n")

    print("[bold]Template:\n")
    msg = "## Suggested changelog entry:\n\n```rst\n\n```"
    print(Syntax(msg, "md", theme="ansi_light"))

print()
