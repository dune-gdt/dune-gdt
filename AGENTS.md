# AGENTS.md

Orientation for automated coding agents working in this repository. Humans: start with
[README.md](README.md) and [CONTRIBUTING.md](CONTRIBUTING.md) — this file does not replace
either, it collects the things that are otherwise only discoverable by reading CMake and the
CI workflows.

## What this is

dune-gdt is a [DUNE](https://www.dune-project.org/) module providing a generic discretization
toolbox for grid-based numerical methods: local operators, local evaluations, local
assemblers, discrete function spaces.

It also carries **dune-xt** in-tree (`dune/xt`, `python/xt`). That code is developed and
maintained here, not in a separate upstream project — treat it like the rest of the
repository.

## Layout

| Path | Contents |
| --- | --- |
| `dune/gdt/` | GDT headers (`operators/`, `spaces/`, `local/`, `interpolations/`, …) and `test/` |
| `dune/xt/` | dune-xt headers, split into `common/`, `grid/`, `functions/`, `la/`, plus `test/` |
| `python/gdt/`, `python/xt/` | pybind11 bindings, `pyproject.toml`, `uv.lock`, `test/` |
| `cmake/modules/` | the build's own machinery — `DuneXTTesting.cmake` in particular |
| `docs/source/` | Sphinx + myst-nb sources; the tutorials/examples are executed notebooks |
| `examples/`, `benchmarks/` | standalone demos and nanobench benchmarks |
| `.ci/` | wheel, coverage and disk-space helper scripts used by the workflows |
| `.vcpkg-overlays/` | vcpkg overlay ports and triplets |

Headers mirror the core modules: an extension of `dune/common/fvector.hh` lives at
`dune/xt/common/fvector.hh` and includes it. Test sources mirror header paths — tests for
`dune/xt/common/foo/bar.hh` belong in `dune/xt/test/common/foo_bar.cc`.

## Building

**CMake presets plus vcpkg manifest mode. Not `dunecontrol`, and there is no Makefile.**
`dunecontrol` survives only in `deps/Dockerfile` and `.vcsetup/`, both of which are stale and
unused by CI — do not follow them.

[`uv`](https://docs.astral.sh/uv/) is a hard requirement even for a pure C++ build:
configuring fails with a `FATAL_ERROR` without it, because the templated test suites are
expanded through `uv run` at configure time. The interpreter is pinned by `.python-version`
(3.13) and resolved via `uv python find` unless `DXT_USE_UV_PYTHON=OFF`.

Presets live in `CMakePresets.json`; the build tree is always `build/<preset>`. The everyday
Linux ones are `debug`, `release` and `release_coverage` (gcc/Ninja), `clang-debug` /
`clang-release`, and `clang22-debug` / `clang22-release_coverage` (what CI's matrix runs).
All of them use C++20 and set `CMAKE_EXPORT_COMPILE_COMMANDS=ON`.

Cap the compile concurrency — an unbounded `--parallel` will OOM a 16 GB machine. These are
the same limits CI derives in the `config` job of `.github/workflows/non_docker_build.yml`:

```bash
JOBS=$(( $(nproc) - 1 ))          # ordinary TUs peak ~2 GB each
BINDING_JOBS=$(( $(nproc) / 2 ))  # pybind11 TUs peak well above 4 GB each
CTEST_JOBS=$(( $(nproc) * 2 ))    # tests are short and IO-light, so oversubscribe

cmake --preset=debug                                                            # configure
cmake --build --preset=debug --parallel -- -j "${JOBS}"                         # the library
cmake --build --preset=debug --target test_binaries --parallel -- -j "${JOBS}"   # ~520 binaries
cmake --build --preset=debug --target bindings --parallel -- -j "${BINDING_JOBS}"
xvfb-run -a ctest --preset=debug --parallel "${CTEST_JOBS}"                      # run the suite
```

The bindings get their own tighter cap because their translation units instantiate
deeply-nested DUNE templates; building them at the shared `nproc - 1` OOM-killed CI's 16 GB
runner. The `-- -j` form assumes the Ninja generator, which every Linux preset uses.

Also budget for the cold configure: it builds **every** vcpkg dependency from source, tens of
minutes (~27 min on CI) before a single line of this project compiles.

Other targets worth knowing: `check` / `recheck` (build and run tests per subdir),
`dxt_headercheck`, `benchmarks` / `run_benchmarks`, `tidy` (clang-tidy),
`license` (license-header rewrite), `coverage_cpp`, `coverage_cpp_llvm`, `coverage_python`.

## Tests

Test presets filter on the ctest label `dune-gdt-test`; `ctest --preset=<preset>` is the
entry point for both languages.

The **Python suites are ctest entries, not a separate build target**: `xt_test_python`,
`gdt_test_python` and `docs_test_python`, each shelling out to
`uv run --frozen --group test python -m pytest` (see `cmake/modules/DuneXTTesting.cmake`).
`--frozen` means the lockfiles are used as-is; regenerate them with
`python python/update_lockfiles.py` from a clean checkout — no build directory needed — never
by hand.

C++ tests are gtest binaries, plus templated `.tpl` suites and `.mini` meta-ini suites that
are expanded at configure time. Adding a test source under an existing `test/` directory is
enough; the per-subdir glob picks it up on the next configure.

## Style and pre-commit

`pre-commit install` is required before you start making changes (see `CONTRIBUTING.md`).
The hooks are pinned by frozen SHAs and cover clang-format, ruff + ruff-format, yamlfmt,
cmake-format/cmake-lint, actionlint, shellcheck and JSON-schema checks for the workflow and
dependabot files. Run `pre-commit run --files <paths>` before committing.

* **clang-format decides all C++ formatting** — Mozilla base, 120 columns, 2-space indent.
  There is no flexibility here; do not hand-format.
* `CONTRIBUTING.md` is the authority for everything formatting cannot express: `CamelCase`
  types vs. `stl_standard` methods, trailing underscores on members, the `DUNE_XT_COMMON_*`
  include-guard pattern, `numeric_cast` for integer conversions, the include grouping order,
  and the one-or-two-blank-lines rule. Read it before writing C++ here.
* Every C++, CMake and Python source starts with the dual BSD-2-Clause / GPL-2.0+
  (with runtime exception) header plus an `Authors:` list. Copy it from a neighbouring file
  when adding a source file; the `license` target rewrites them via the `.pylicense-*.py`
  configs. Recently added Markdown files do not carry it.
* `.yamlfmt` sets `include_document_start`, which is why every YAML file begins with `---`.

## CI

Everything runs on `ubuntu-26.04`. The **single required status check is the `ci-gate` job**
in `.github/workflows/non_docker_build.yml`, which aggregates the build matrix, the wheel
build and the docs build. `check_markdown_links.yml` and `sanity_checks.yml` are advisory.

A `changes` path filter lets a docs-only change skip the C++ matrix and the wheel build; the
docs job then pulls the last good wheels from `main`.

The docs build installs the built wheels into a `uv venv` and runs
`python -m sphinx --keep-going -j 1 -b html docs/source <out>`, executing every notebook. It
fails the job if any `reports/*.err.log` was written. The `docs` CMake target is **not** an
equivalent — it depends on a `RUN_IN_ENV_SCRIPT` that is no longer set, so it does not work.

## Working without a full build

A full configure plus build is usually not viable inside a sandbox, and that is fine — most
review-sized changes can be validated without one:

* `pre-commit run --files <paths>` and `ruff check` need no build at all.
* If a tree has already been configured, `build/<preset>/compile_commands.json` gives you the
  real compile flags for any translation unit.
* Header and test layout questions are answered by the conventions above rather than by
  compiling.

Say plainly in the PR description which checks you actually ran and which you could not.
