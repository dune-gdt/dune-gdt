# Benchmarks

These plots track the runtime of the standalone [nanobench](https://github.com/martinus/nanobench)
benchmarks in [`benchmarks/`](https://github.com/dune-gdt/dune-gdt/tree/main/benchmarks). The
`Benchmarks` workflow builds them in `Release` on every push to `main`, runs them, and publishes the
per-benchmark JSON reports onto the orphan `benchmarks` branch. The documentation build fetches
those reports and renders, for each benchmark series, the median runtime over the report date so
that performance regressions become visible.

The table shows the most recent measurement for every series; each plot below it shows that series'
history. Error bars correspond to nanobench's median-absolute-percent-error; hover over a point to
see the commit it was measured at.

```{benchmark-plots}
```
