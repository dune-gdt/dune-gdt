"""Unit tests for the ``benchmark_plots`` Sphinx extension (see source/_ext/benchmark_plots.py).

These exercise the report parsing / aggregation independently of a full Sphinx build, so they run
fast and without the dune-gdt bindings.
"""

import json
import sys
from datetime import timezone
from pathlib import Path

import pytest

EXT_DIR = Path(__file__).resolve().parent / "source" / "_ext"
sys.path.insert(0, str(EXT_DIR))

import benchmark_plots as bp  # noqa: E402


def _write_report(directory: Path, filename: str, payload: dict) -> None:
    directory.mkdir(parents=True, exist_ok=True)
    (directory / filename).write_text(json.dumps(payload))


def _make_data(root: Path) -> None:
    _write_report(
        root / "20260601-100000-aaaa111",
        "burgers__1d__explicit__fv.json",
        {
            "meta": {"date": "2026-06-01T10:00:00Z", "git_revision": "aaaa111"},
            "results": [
                {
                    "title": "burgers__1d__explicit__fv",
                    "name": "explicit_euler__upwind",
                    "unit": "op",
                    "median(elapsed)": 0.0123,
                    "medianAbsolutePercentError(elapsed)": 2.5,
                }
            ],
        },
    )
    # second run, two series in one file, and no meta block (exercises directory-name fallback)
    _write_report(
        root / "20260605-100000-bbbb222",
        "burgers__1d__explicit__fv.json",
        {
            "meta": {"date": "2026-06-05T10:00:00Z", "git_revision": "bbbb222"},
            "results": [
                {
                    "title": "burgers__1d__explicit__fv",
                    "name": "explicit_euler__upwind",
                    "median(elapsed)": 0.0131,
                    "medianAbsolutePercentError(elapsed)": 1.8,
                }
            ],
        },
    )
    _write_report(
        root / "20260605-100000-bbbb222",
        "stokes__taylorhood.json",
        {
            "results": [
                {
                    "title": "stokes__taylorhood",
                    "name": "solve",
                    "median(elapsed)": 1.2,
                },
            ]
        },
    )
    # a "latest" copy that must be ignored by the aggregation
    _write_report(
        root / "latest",
        "burgers__1d__explicit__fv.json",
        {"meta": {"date": "2026-06-05T10:00:00Z"}, "results": []},
    )


def test_collect_series_groups_and_sorts(tmp_path):
    _make_data(tmp_path)
    series = bp._collect_series(tmp_path)

    burgers = ("burgers__1d__explicit__fv", "explicit_euler__upwind")
    assert burgers in series
    points = series[burgers]
    assert [p["elapsed"] for p in points] == [0.0123, 0.0131]  # sorted by date
    assert [p["rev"] for p in points] == ["aaaa111", "bbbb222"]

    # the "latest" directory must not contribute an extra run
    assert all(p["date"].year == 2026 for p in points)
    assert ("stokes__taylorhood", "solve") in series


def test_parse_date_prefers_meta_then_dirname():
    meta_date = bp._parse_date({"meta": {"date": "2026-06-05T10:00:00Z"}}, "ignored")
    assert meta_date.tzinfo is not None
    assert meta_date.astimezone(timezone.utc).hour == 10

    fallback = bp._parse_date({}, "20260605-100000-bbbb222")
    assert fallback.year == 2026 and fallback.month == 6 and fallback.day == 5

    assert bp._parse_date({}, "not-a-date") is None


def test_revision_falls_back_to_dirname():
    assert bp._revision({"meta": {"git_revision": "deadbeef"}}, "x") == "deadbeef"
    assert bp._revision({}, "20260605-100000-bbbb222") == "bbbb222"
    assert bp._revision({}, "nofields") == ""


def test_scale_for_picks_human_unit():
    assert bp._scale_for(1.5)[1] == "s"
    assert bp._scale_for(0.012)[1] == "ms"
    assert bp._scale_for(5e-6)[1] == "µs"


def test_plot_html_renders_plotly(tmp_path):
    pytest.importorskip("plotly")
    _make_data(tmp_path)
    series = bp._collect_series(tmp_path)
    key = ("burgers__1d__explicit__fv", "explicit_euler__upwind")
    html = bp._plot_html(*key, series[key], include_js=True)
    assert "plotly" in html.lower()
    # the embedded data must reference the measured commits
    assert "aaaa111" in html and "bbbb222" in html


if __name__ == "__main__":
    sys.exit(pytest.main(sys.argv[1:] + [__file__]))
