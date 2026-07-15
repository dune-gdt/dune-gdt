// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2026 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   René Fritze (2026)

#ifndef DUNE_XT_LA_GENERALIZED_EIGEN_SOLVER_PBH
#define DUNE_XT_LA_GENERALIZED_EIGEN_SOLVER_PBH

#include <memory>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <python/xt/dune/xt/common/configuration.hh>
#include <python/xt/dune/xt/la/container.bindings.hh>
#include <python/xt/dune/xt/la/solver_machinery.hh>

#include <dune/xt/la/container.hh>
#include <dune/xt/la/type_traits.hh>
#include <dune/xt/la/generalized-eigen-solver.hh>

namespace Dune::XT::LA {


/**
 * \brief Binds LA::GeneralizedEigenSolver<M> (dune/xt/la/generalized-eigen-solver.hh), solving
 *        the generalized eigenvalue problem lhs * v = lambda * rhs * v, for a matrix type M with
 *        C++ test coverage (see dune/xt/test/la/generalized-eigensolver_for_*.py).
 *
 * The two-matrix constructor below is genuinely specific to this class (LA::EigenSolver<M> and
 * LA::MatrixInverter<M> wrap a single matrix, see solver_machinery.hh), but the types/options and
 * eigenvalue-accessor boilerplate is shared with eigen_solver.hh via the same header.
 */
template <class M>
auto bind_GeneralizedEigenSolver(pybind11::module& m)
{
  using C = GeneralizedEigenSolver<M>;
  using Opts = GeneralizedEigenSolverOptions<M>;
  using RealMatrixType = typename C::RealMatrixType;
  using ComplexMatrixType = typename C::ComplexMatrixType;

  namespace py = pybind11;
  using namespace pybind11::literals;

  const auto ClassName = Common::to_camel_case(bindings::container_name<M>::value() + "_generalized_eigen_solver");

  py::class_<C> c(m, ClassName.c_str(), ClassName.c_str());

  bind_solver_machinery_options<Opts>(c);

  c.def(
      py::init([](const M& lhs, const M& rhs, const std::string& type) { return std::make_unique<C>(lhs, rhs, type); }),
      "lhs_matrix"_a,
      "rhs_matrix"_a,
      "type"_a = "",
      py::keep_alive<1, 2>(),
      py::keep_alive<1, 3>());
  c.def(py::init([](const M& lhs, const M& rhs, const Common::Configuration& opts) {
          return std::make_unique<C>(lhs, rhs, opts);
        }),
        "lhs_matrix"_a,
        "rhs_matrix"_a,
        "options"_a,
        py::keep_alive<1, 2>(),
        py::keep_alive<1, 3>());

  c.def_property_readonly("options", &C::options);
  c.def_property_readonly("lhs_matrix", [](const C& self) { return M(self.lhs_matrix()); });
  c.def_property_readonly("rhs_matrix", [](const C& self) { return M(self.rhs_matrix()); });
  bind_solver_machinery_eigenvalue_accessors(c);

  c.def("eigenvectors", [](const C& self) { return ComplexMatrixType(self.eigenvectors()); });
  c.def("real_eigenvectors", [](const C& self) { return RealMatrixType(self.real_eigenvectors()); });

  // std::make_unique<C>(...) rather than C(...) by value: see the analogous comment on
  // bind_single_matrix_solver_factory() in solver_machinery.hh.
  m.def(
      "make_generalized_eigen_solver",
      [](const M& lhs, const M& rhs, const std::string& type) { return std::make_unique<C>(lhs, rhs, type); },
      "lhs_matrix"_a,
      "rhs_matrix"_a,
      "type"_a = "",
      py::keep_alive<0, 1>(),
      py::keep_alive<0, 2>());
  m.def(
      "make_generalized_eigen_solver",
      [](const M& lhs, const M& rhs, const Common::Configuration& opts) { return std::make_unique<C>(lhs, rhs, opts); },
      "lhs_matrix"_a,
      "rhs_matrix"_a,
      "options"_a,
      py::keep_alive<0, 1>(),
      py::keep_alive<0, 2>());

  return c;
} // ... bind_GeneralizedEigenSolver(...)


} // namespace Dune::XT::LA

#endif // DUNE_XT_LA_GENERALIZED_EIGEN_SOLVER_PBH
