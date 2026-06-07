"""A minimal Sphinx extension that renders nanobench JSON reports as Plotly plots.

The extension provides a single directive, ``benchmark-plots``, which reads the per-run nanobench
JSON reports produced by the ``Benchmarks`` workflow (and published onto the orphan ``benchmarks``
branch) and renders, into the static HTML page:

* a summary table of the most recent timings, and
* one interactive Plotly plot per benchmark *series* (a ``title``/``name`` pair, i.e. one
  ``bench.run(...)`` call), showing the median runtime over the report date.

Data location
-------------
The directive reads the directory configured via the ``benchmark_data_dir`` Sphinx config value,
which defaults to the ``DUNE_GDT_BENCHMARK_DATA`` environment variable and, failing that, to
``<confdir>/_benchmark_data``. That directory is expected to contain the report layout used on the
``benchmarks`` branch::

    <data-dir>/
      20260607-095800-abc1234/   # one directory per run (<UTCstamp>-<shortsha>)
        burgers__1d__explicit__fv.json
        stokes__taylorhood.json
        ...
      20260608-101500-def5678/
        ...
      latest/                    # ignored here (a copy of the most recent run)

Each JSON file is a nanobench document (a top-level object with a ``results`` array) augmented with
a ``meta`` block carrying ``date`` (and optionally ``git_revision``); see
``benchmarks/benchmark_common.hh``. The run-directory name is used as a fallback when a report
predates the metadata or lacks it.

Robustness
----------
When no data is available (the ``benchmarks`` branch has not produced reports yet, or Plotly is not
installed) the directive renders an informational note instead of failing the build. This keeps the
``-W`` (warnings-as-errors) docs build green.
"""

from __future__ import annotations

import json
import os
from datetime import datetime, timezone
from pathlib import Path

from docutils import nodes
from docutils.parsers.rst import Directive
from sphinx.util import logging

logger = logging.getLogger(__name__)

# nanobench renders elapsed time in seconds per operation; pick a human-friendly unit for the axis.
_TIME_UNITS = [(1.0, "s"), (1e-3, "ms"), (1e-6, "µs"), (1e-9, "ns")]


def _resolve_data_dir(env) -> Path | None:
    """Return the configured benchmark data directory, or ``None`` if it does not exist."""
    configured = env.config.benchmark_data_dir
    if not configured:
        configured = os.environ.get("DUNE_GDT_BENCHMARK_DATA")
    if configured:
        path = Path(configured).expanduser()
    else:
        path = Path(env.app.confdir) / "_benchmark_data"
    return path if path.is_dir() else None


def _parse_date(report: dict, run_name: str) -> datetime | None:
    """Determine the date of a report, preferring the embedded metadata over the directory name."""
    meta_date = (report.get("meta") or {}).get("date")
    if meta_date:
        try:
            parsed = datetime.fromisoformat(str(meta_date).replace("Z", "+00:00"))
            # normalize to UTC-aware so that mixing reports with and without an explicit offset
            # does not raise when sorting the series chronologically
            if parsed.tzinfo is None:
                return parsed.replace(tzinfo=timezone.utc)
            return parsed.astimezone(timezone.utc)
        except ValueError:
            logger.info("benchmark-plots: could not parse meta date %r", meta_date)
    # fall back to the "<YYYYmmdd>-<HHMMSS>-<sha>" run directory name
    parts = run_name.split("-")
    if len(parts) >= 2:
        try:
            return datetime.strptime(f"{parts[0]}-{parts[1]}", "%Y%m%d-%H%M%S").replace(
                tzinfo=timezone.utc
            )
        except ValueError:
            pass
    return None


def _revision(report: dict, run_name: str) -> str:
    rev = (report.get("meta") or {}).get("git_revision")
    if rev:
        return str(rev)[:12]
    parts = run_name.split("-")
    return parts[2] if len(parts) >= 3 else ""


def _collect_series(data_dir: Path) -> dict[tuple[str, str], list[dict]]:
    """Collect ``{(title, name): [point, ...]}`` across all run directories, sorted by date.

    Each point is ``{"date", "elapsed", "error", "rev", "unit"}`` where ``elapsed`` is the median
    runtime in seconds.
    """
    series: dict[tuple[str, str], list[dict]] = {}
    run_dirs = sorted(
        d for d in data_dir.iterdir() if d.is_dir() and d.name != "latest"
    )
    for run in run_dirs:
        for jf in sorted(run.glob("*.json")):
            try:
                report = json.loads(jf.read_text())
            except (OSError, json.JSONDecodeError) as exc:
                logger.warning(
                    "benchmark-plots: skipping unreadable report %s: %s", jf, exc
                )
                continue
            date = _parse_date(report, run.name)
            if date is None:
                logger.info("benchmark-plots: no date for %s, skipping", jf)
                continue
            rev = _revision(report, run.name)
            for result in report.get("results", []):
                title = result.get("title") or jf.stem
                name = result.get("name", "")
                elapsed = result.get("median(elapsed)")
                if elapsed is None:
                    continue
                key = (title, name)
                series.setdefault(key, []).append(
                    {
                        "date": date,
                        "elapsed": float(elapsed),
                        "error": result.get("medianAbsolutePercentError(elapsed)"),
                        "rev": rev,
                        "unit": result.get("unit", "op"),
                    }
                )
    for points in series.values():
        points.sort(key=lambda p: p["date"])
    return series


def _scale_for(max_elapsed: float) -> tuple[float, str]:
    """Pick a (divisor, label) so that runtimes read in a friendly unit."""
    for factor, label in _TIME_UNITS:
        if max_elapsed >= factor:
            return factor, label
    return _TIME_UNITS[-1]


def _note(text: str) -> list[nodes.Node]:
    note = nodes.note()
    note += nodes.paragraph(text=text)
    return [note]


def _summary_table(series: dict[tuple[str, str], list[dict]]) -> nodes.Node:
    """A docutils table of the most recent timing for every series."""
    table = nodes.table()
    tgroup = nodes.tgroup(cols=4)
    table += tgroup
    for width in (4, 2, 1, 2):
        tgroup += nodes.colspec(colwidth=width)
    header = nodes.thead()
    tgroup += header
    header_row = nodes.row()
    header += header_row
    for label in ("Benchmark", "Median runtime", "Error", "Date"):
        entry = nodes.entry()
        entry += nodes.paragraph(text=label)
        header_row += entry
    body = nodes.tbody()
    tgroup += body
    for (title, name), points in sorted(series.items()):
        latest = points[-1]
        factor, unit = _scale_for(latest["elapsed"])
        err = latest["error"]
        cells = [
            f"{title} / {name}" if name else title,
            f"{latest['elapsed'] / factor:.3f} {unit}",
            f"{err:.1f} %" if err is not None else "—",
            latest["date"].strftime("%Y-%m-%d"),
        ]
        row = nodes.row()
        body += row
        for cell in cells:
            entry = nodes.entry()
            entry += nodes.paragraph(text=cell)
            row += entry
    return table


def _plot_html(title: str, name: str, points: list[dict], include_js: bool) -> str:
    import plotly.graph_objects as go

    factor, unit = _scale_for(max(p["elapsed"] for p in points))
    dates = [p["date"] for p in points]
    values = [p["elapsed"] / factor for p in points]
    # nanobench reports the median-absolute-percent-error; turn it into an absolute error band
    errors = [
        (p["error"] / 100.0) * (p["elapsed"] / factor)
        if p["error"] is not None
        else 0.0
        for p in points
    ]
    revs = [p["rev"] for p in points]
    fig = go.Figure(
        go.Scatter(
            x=dates,
            y=values,
            error_y=dict(type="data", array=errors, visible=True),
            mode="lines+markers",
            customdata=revs,
            hovertemplate=(
                "%{x|%Y-%m-%d %H:%M} UTC<br>%{y:.3f} "
                + unit
                + "<br>%{customdata}<extra></extra>"
            ),
        )
    )
    heading = f"{title} / {name}" if name else title
    fig.update_layout(
        title=heading,
        xaxis_title="date",
        yaxis_title=f"median runtime [{unit}]",
        margin=dict(l=60, r=20, t=50, b=40),
        height=380,
    )
    return fig.to_html(
        full_html=False,
        include_plotlyjs="cdn" if include_js else False,
        default_width="100%",
    )


class BenchmarkPlotsDirective(Directive):
    has_content = False

    def run(self) -> list[nodes.Node]:
        env = self.state.document.settings.env
        data_dir = _resolve_data_dir(env)
        if data_dir is None:
            return _note(
                "No benchmark data available. Reports are published onto the `benchmarks` branch "
                "by the Benchmarks workflow; once present they will be plotted here."
            )
        series = _collect_series(data_dir)
        if not series:
            return _note(f"No benchmark reports found under {data_dir}.")

        try:
            import plotly.graph_objects  # noqa: F401
        except ImportError:
            logger.info(
                "benchmark-plots: plotly is not installed; rendering the table only"
            )
            return [
                _summary_table(series),
                *_note(
                    "Plotly is not installed; rendering the benchmark summary table only."
                ),
            ]

        result: list[nodes.Node] = [_summary_table(series)]
        for index, ((title, name), points) in enumerate(sorted(series.items())):
            html = _plot_html(title, name, points, include_js=(index == 0))
            result.append(nodes.raw("", html, format="html"))
        return result


def setup(app):
    app.add_config_value("benchmark_data_dir", None, "env")
    app.add_directive("benchmark-plots", BenchmarkPlotsDirective)
    return {
        "version": "0.1",
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
