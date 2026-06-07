// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)

#ifndef DUNE_GDT_BENCHMARKS_BENCHMARK_COMMON_HH
#define DUNE_GDT_BENCHMARKS_BENCHMARK_COMMON_HH

#include <cstdlib>
#include <ctime>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

// The benchmarks are standalone executables (one translation unit each), so it is safe to emit
// the nanobench implementation here in the single header every benchmark includes.
#define ANKERL_NANOBENCH_IMPLEMENT
#include <nanobench.h>

namespace Dune {
namespace GDT {
namespace Benchmark {


/**
 * \brief Create a nanobench Bench tuned for heavy (multi-millisecond to multi-second) PDE solves.
 *
 * We keep the iteration count low because a single run of an e2e solve already dominates the
 * measurement; nanobench would otherwise try to accumulate many iterations per epoch.
 */
inline ankerl::nanobench::Bench make_bench(const std::string& title)
{
  ankerl::nanobench::Bench bench;
  bench.title(title).warmup(0).epochs(3).minEpochIterations(1);
  return bench;
}


/**
 * \brief Return the current UTC time as an ISO-8601 timestamp, e.g. "2026-06-07T09:58:00Z".
 */
inline std::string utc_timestamp()
{
  const std::time_t now = std::time(nullptr);
  std::tm tm_utc{};
#if defined(_WIN32)
  gmtime_s(&tm_utc, &now);
#else
  gmtime_r(&now, &tm_utc);
#endif
  char buf[32];
  std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm_utc);
  return std::string(buf);
}


/**
 * \brief Render the nanobench results as JSON to ${DUNE_GDT_BENCHMARK_OUTPUT_DIR}/<name>.json
 *        (or ./<name>.json if the environment variable is unset).
 *
 * The nanobench JSON document (a top-level object with a "results" array) is augmented with a
 * "meta" object so that downstream consumers (e.g. the documentation plots) have reliable date
 * metadata to plot runtime over time. The capture date is the UTC wall-clock time at which the
 * report is written; the optional git revision is taken from ${DUNE_GDT_BENCHMARK_GIT_REVISION}.
 */
inline void write_report(const ankerl::nanobench::Bench& bench, const std::string& name)
{
  const char* dir = std::getenv("DUNE_GDT_BENCHMARK_OUTPUT_DIR");
  const std::string path = (dir && dir[0] != '\0' ? std::string(dir) + "/" : std::string()) + name + ".json";

  // Render nanobench's JSON to a string so that we can inject the metadata block right after the
  // opening brace. The nanobench json template always starts the document with '{'.
  std::ostringstream rendered;
  ankerl::nanobench::render(ankerl::nanobench::templates::json(), bench, rendered);
  std::string json = rendered.str();
  const auto brace = json.find('{');
  if (brace == std::string::npos)
    throw std::runtime_error("unexpected nanobench JSON output (no opening brace) for: " + name);

  std::ostringstream meta;
  meta << "\n \"meta\": {\n  \"date\": \"" << utc_timestamp() << "\"";
  if (const char* rev = std::getenv("DUNE_GDT_BENCHMARK_GIT_REVISION"); rev && rev[0] != '\0')
    meta << ",\n  \"git_revision\": \"" << rev << "\"";
  meta << "\n },";
  json.insert(brace + 1, meta.str());

  std::ofstream out(path);
  if (!out)
    throw std::runtime_error("failed to open benchmark report file for writing: " + path);
  out << json;
}


} // namespace Benchmark
} // namespace GDT
} // namespace Dune

#endif // DUNE_GDT_BENCHMARKS_BENCHMARK_COMMON_HH
