// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)

#ifndef DUNE_GDT_BENCHMARKS_BENCHMARK_COMMON_HH
#define DUNE_GDT_BENCHMARKS_BENCHMARK_COMMON_HH

#include <cstdlib>
#include <fstream>
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
 * \brief Render the nanobench results as JSON to ${DUNE_GDT_BENCHMARK_OUTPUT_DIR}/<name>.json
 *        (or ./<name>.json if the environment variable is unset).
 */
inline void write_report(const ankerl::nanobench::Bench& bench, const std::string& name)
{
  const char* dir = std::getenv("DUNE_GDT_BENCHMARK_OUTPUT_DIR");
  const std::string path = (dir && dir[0] != '\0' ? std::string(dir) + "/" : std::string()) + name + ".json";
  std::ofstream out(path);
  if (!out)
    throw std::runtime_error("failed to open benchmark report file for writing: " + path);
  ankerl::nanobench::render(ankerl::nanobench::templates::json(), bench, out);
}


} // namespace Benchmark
} // namespace GDT
} // namespace Dune

#endif // DUNE_GDT_BENCHMARKS_BENCHMARK_COMMON_HH
