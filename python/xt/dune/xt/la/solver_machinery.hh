// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2026 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   René Fritze (2026)

#ifndef DUNE_XT_LA_SOLVER_MACHINERY_PBH
#define DUNE_XT_LA_SOLVER_MACHINERY_PBH

#include <limits>
#include <memory>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <python/xt/dune/xt/common/configuration.hh>

namespace Dune::XT::LA {


/**
 * \brief Binds `eigenvalues()`, `real_eigenvalues()`, `min_eigenvalues(num_evs=max)` and
 *        `max_eigenvalues(num_evs=max)`, the eigenvalue-accessor surface LA::EigenSolver<M> and
 *        LA::GeneralizedEigenSolver<M> both expose identically (their eigen*vectors* accessors
 *        differ -- EigenSolver additionally inverts them -- so those stay in the respective files).
 */
template <class C>
void bind_solver_machinery_eigenvalue_accessors(pybind11::class_<C>& c)
{
  namespace py = pybind11;
  using namespace pybind11::literals;

  c.def("eigenvalues", &C::eigenvalues);
  c.def("real_eigenvalues", &C::real_eigenvalues);
  c.def(
      "min_eigenvalues",
      [](const C& self, const size_t num_evs) { return self.min_eigenvalues(num_evs); },
      "num_evs"_a = std::numeric_limits<size_t>::max());
  c.def(
      "max_eigenvalues",
      [](const C& self, const size_t num_evs) { return self.max_eigenvalues(num_evs); },
      "num_evs"_a = std::numeric_limits<size_t>::max());
}


/**
 * \brief Binds the static `types()`/`options(type="")` pair every eigen-solver/matrix-inverter-style
 *        options class exposes (EigenSolverOptions, GeneralizedEigenSolverOptions and
 *        MatrixInverterOptions all share this shape).
 *
 * Shared by eigen_solver.hh, generalized_eigen_solver.hh and matrix_inverter.hh so their otherwise
 * near-identical binder bodies do not trip SonarCloud's copy-paste detector (see the "collapse ...
 * shared macros, fix duplication gate" precedent elsewhere in this repository).
 */
template <class Opts, class C>
void bind_solver_machinery_options(pybind11::class_<C>& c)
{
  namespace py = pybind11;
  using namespace pybind11::literals;

  c.def_static("types", &Opts::types);
  c.def_static("options", &Opts::options, "type"_a = "");
}


/**
 * \brief Binds the (matrix, type="") / (matrix, Configuration) constructor pair plus the shared
 *        `options`/`matrix` read-only properties.
 *
 * The common shape of LA::EigenSolver<M> and LA::MatrixInverter<M>, which both wrap a single input
 * matrix (unlike LA::GeneralizedEigenSolver<M>, which wraps two) -- factored out for the same reason
 * as bind_solver_machinery_options() above.
 */
template <class C, class M>
void bind_single_matrix_solver_ctor(pybind11::class_<C>& c)
{
  namespace py = pybind11;
  using namespace pybind11::literals;

  c.def(py::init([](const M& matrix, const std::string& type) { return std::make_unique<C>(matrix, type); }),
        "matrix"_a,
        "type"_a = "",
        py::keep_alive<1, 2>());
  c.def(py::init([](const M& matrix, const Common::Configuration& opts) { return std::make_unique<C>(matrix, opts); }),
        "matrix"_a,
        "options"_a,
        py::keep_alive<1, 2>());
  c.def_property_readonly("options", &C::options);
  c.def_property_readonly("matrix", [](const C& self) { return M(self.matrix()); });
}


/**
 * \brief Binds the free `make_<...>(matrix, type="")` / `make_<...>(matrix, Configuration)` factory
 *        pair for a single-input-matrix solver-machinery class C, under the given Python-visible name.
 *
 * Returns std::make_unique<C>(...) rather than C(...) by value: pybind11 has to move- or
 * copy-construct a by-value return into its own heap allocation, and unlike EigenSolverBase,
 * MatrixInverterBase declares no move constructor of its own (its unique_ptr cache member makes the
 * implicit one deleted-via-virtual-destructor-suppression) -- returning a unique_ptr sidesteps that
 * requirement entirely, for every class this is used with.
 */
template <class C, class M>
void bind_single_matrix_solver_factory(pybind11::module& m, const std::string& factory_name)
{
  namespace py = pybind11;
  using namespace pybind11::literals;

  m.def(
      factory_name.c_str(),
      [](const M& matrix, const std::string& type) { return std::make_unique<C>(matrix, type); },
      "matrix"_a,
      "type"_a = "",
      py::keep_alive<0, 1>());
  m.def(
      factory_name.c_str(),
      [](const M& matrix, const Common::Configuration& opts) { return std::make_unique<C>(matrix, opts); },
      "matrix"_a,
      "options"_a,
      py::keep_alive<0, 1>());
}


} // namespace Dune::XT::LA

#endif // DUNE_XT_LA_SOLVER_MACHINERY_PBH
