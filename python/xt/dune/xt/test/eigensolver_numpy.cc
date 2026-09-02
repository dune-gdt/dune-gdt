// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)

// Test-only bindings for the NumPy eigen-solver backend.
//
// EigenSolver<FieldMatrix<...>> is the only specialization offering the "numpy" backend, and it is not part of the
// public bindings (python/xt/dune/xt/la/bindings.cc binds the CommonDense/CommonSparse/EigenDense variants, none of
// which has a numpy branch). Without these bindings there is no way to assert from Python that the backend works when
// dune-xt runs inside a live interpreter, which is exactly the case that used to segfault.

#include "config.h"

#include <string>
#include <vector>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <dune/xt/common/fmatrix.hh>
#include <dune/xt/la/eigen-solver.hh>

PYBIND11_MODULE(_test_eigensolver_numpy, m)
{
  namespace py = pybind11;
  using namespace pybind11::literals;
  using namespace Dune::XT;

  using MatrixType = Dune::FieldMatrix<double, 2, 2>;

  m.def("numpy_eigensolver_available", []() { return LA::internal::numpy_eigensolver_available(); });

  m.def("interpreter_is_running", []() { return Dune::PybindXI::interpreter_is_running(); });

  m.def("types", []() { return LA::EigenSolverOptions<MatrixType>::types(); });

  // The GIL is released for the duration of the solve, which is what the operator bindings do around a grid walk and
  // what made the embedded-interpreter probe crash. The backend has to take the GIL back for itself.
  m.def(
      "eigenvalues",
      [](const std::vector<std::vector<double>>& matrix, const std::string& type) {
        MatrixType mat(0.);
        for (size_t ii = 0; ii < 2; ++ii)
          for (size_t jj = 0; jj < 2; ++jj)
            mat[ii][jj] = matrix.at(ii).at(jj);
        // assert_eigendecomposition (on by default) verifies A = V diag(lambda) V^-1, so this also covers the
        // eigenvector convention, not just the eigenvalues.
        return LA::make_eigen_solver(mat, LA::EigenSolverOptions<MatrixType>::options(type)).real_eigenvalues();
      },
      "matrix"_a,
      "type"_a,
      py::call_guard<py::gil_scoped_release>());
}
