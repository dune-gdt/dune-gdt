#!/usr/bin/env python3
"""Aggregate an lcov-format .info file into the per-area table used in issue #314.

Prints a markdown table with one row per source area (dune/xt/la, dune/gdt/local, ...)
listing lines / hits / misses / partials and two coverage numbers:

* "codecov %" counts a partial line (a hit line whose BRDA branch records are not all
  taken) as not covered -- this is how Codecov's headline treats partials today, so this
  column is comparable to the numbers in issue #314.
* "line %" counts every executed line as covered regardless of branch data -- this is
  what an lcov upload without branch records (lcov's default) would report.

For a line-only .info (no BRDA records) the two columns coincide. stdlib only.

Usage: lcov_area_summary.py <coverage.info> [--root <sourcedir>]
"""

import argparse
import sys
from collections import defaultdict

# Same rows, same order as the table in issue #314 (longest prefix wins; anything
# unmatched lands in "everything else").
AREAS = [
    "dune/xt/la",
    "dune/gdt/local",
    "dune/gdt/test",
    "dune/xt/common",
    "dune/xt/functions",
    "dune/xt/test",
    "dune/gdt/operators",
    "dune/gdt/spaces",
    "dune/xt/grid",
    "dune/gdt/tools",
    "dune/gdt/interpolations",
]
OTHER = "everything else"


def parse_info(path):
    """Return {file: {line: [exec_count, branches_total, branches_taken]}}."""
    files = defaultdict(lambda: defaultdict(lambda: [0, 0, 0]))
    current = None
    with open(path) as fh:
        for raw in fh:
            record = raw.strip()
            if record.startswith("SF:"):
                current = record[3:]
            elif record == "end_of_record":
                current = None
            elif current is None:
                continue
            elif record.startswith("DA:"):
                line, count = record[3:].split(",")[:2]
                files[current][int(line)][0] += int(count)
            elif record.startswith("BRDA:"):
                line, _block, _branch, taken = record[5:].split(",")
                entry = files[current][int(line)]
                entry[1] += 1
                if taken != "-" and int(taken) > 0:
                    entry[2] += 1
    return files


def area_of(path, root):
    if root and path.startswith(root):
        path = path[len(root):].lstrip("/")
    for area in sorted(AREAS, key=len, reverse=True):
        if path.startswith(area + "/") or path == area:
            return area
    return OTHER


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("info")
    parser.add_argument("--root", default="", help="source dir prefix to strip from SF: paths")
    args = parser.parse_args()

    stats = {area: [0, 0, 0, 0] for area in AREAS + [OTHER]}  # lines, hits, misses, partials
    for path, lines in parse_info(args.info).items():
        acc = stats[area_of(path, args.root)]
        for count, br_total, br_taken in lines.values():
            acc[0] += 1
            if count == 0:
                acc[2] += 1
            elif br_total > 0 and br_taken < br_total:
                acc[3] += 1
            else:
                acc[1] += 1

    def fmt(name, lines, hits, misses, partials):
        codecov = f"{100 * hits / lines:.2f}%" if lines else "n/a"
        line_only = f"{100 * (hits + partials) / lines:.2f}%" if lines else "n/a"
        return f"| {name} | {lines} | {hits} | {misses} | {partials} | {codecov} | {line_only} |"

    print(f"### {args.info}")
    print()
    print("| Area | Lines | Hits | Misses | Partials | codecov % | line % |")
    print("|---|---:|---:|---:|---:|---:|---:|")
    totals = [0, 0, 0, 0]
    for area in AREAS + [OTHER]:
        row = stats[area]
        if row[0] == 0 and area != OTHER:
            continue
        totals = [a + b for a, b in zip(totals, row)]
        print(fmt(f"`{area}`", *row))
    print(fmt("**total**", *totals))
    return 0


if __name__ == "__main__":
    sys.exit(main())
