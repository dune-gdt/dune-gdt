// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2026 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   René Fritze (2026)

#ifndef DUNE_XT_LA_EIGEN_SOLVER_PBH
#define DUNE_XT_LA_EIGEN_SOLVER_PBH

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <python/xt/dune/xt/common/configuration.hh>
#include <python/xt/dune/xt/la/container.bindings.hh>
#include <python/xt/dune/xt/la/solver_machinery.hh>

#include <dune/xt/la/container.hh>
#include <dune/xt/la/type_traits.hh>
#include <dune/xt/la/eigen-solver.hh>

namespace Dune::XT::LA {


/**
 * \brief Binds LA::EigenSolver<M> (dune/xt/la/eigen-solver.hh) for a matrix type M that already
 *        has an EigenSolver specialization with C++ test coverage (see
 *        dune/xt/test/la/eigensolver_for_*.py for the exact matrix/field combinations verified).
 *
 * Accessors returning a container (matrix(), eigenvectors(), ...) return by value: EigenSolverBase
 * stores its inputs/outputs by const reference internally, and returning copies to Python sidesteps
 * any question of whose lifetime a returned reference would depend on. The constructor/types/options
 * boilerplate below is shared with matrix_inverter.hh via solver_machinery.hh (both wrap a single
 * input matrix), the one part of the accessor surface that actually differs stays here.
 */
template <class M>
auto bind_EigenSolver(pybind11::module& m)
{
  using C = EigenSolver<M>;
  using Opts = EigenSolverOptions<M>;
  using RealMatrixType = typename C::RealMatrixType;
  using ComplexMatrixType = typename C::ComplexMatrixType;

  namespace py = pybind11;
  using namespace pybind11::literals;

  const auto ClassName = Common::to_camel_case(bindings::container_name<M>::value() + "_eigen_solver");

  py::class_<C> c(m, ClassName.c_str(), ClassName.c_str());

  bind_solver_machinery_options<Opts>(c);
  bind_single_matrix_solver_ctor<C, M>(c);
  bind_solver_machinery_eigenvalue_accessors(c);

  c.def("eigenvectors", [](const C& self) { return ComplexMatrixType(self.eigenvectors()); });
  c.def("eigenvectors_inverse", [](const C& self) { return ComplexMatrixType(self.eigenvectors_inverse()); });
  c.def("real_eigenvectors", [](const C& self) { return RealMatrixType(self.real_eigenvectors()); });
  c.def("real_eigenvectors_inverse", [](const C& self) { return M(self.real_eigenvectors_inverse()); });

  bind_single_matrix_solver_factory<C, M>(m, "make_eigen_solver");

  return c;
} // ... bind_EigenSolver(...)


} // namespace Dune::XT::LA

#endif // DUNE_XT_LA_EIGEN_SOLVER_PBH
