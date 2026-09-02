// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2017)
//   René Fritze     (2018 - 2019)
//   Tobias Leibner  (2018, 2020)

/// \file
/// \brief Internal eigen-solver implementation backed by NumPy.

#ifndef DUNE_XT_LA_EIGEN_SOLVER_INTERNAL_NUMPY_HH
#define DUNE_XT_LA_EIGEN_SOLVER_INTERNAL_NUMPY_HH

#include <utility>
#include <vector>

#include <dune/common/typetraits.hh>

#include <dune/xt/common/interpreter.hh>

#include <dune/xt/common/vector.hh>
#include <dune/xt/common/fmatrix.hh>
#include <dune/xt/common/type_traits.hh>
#include <dune/xt/la/exceptions.hh>

#include <python/xt/dune/xt/common/fvector.hh>
#include <python/xt/dune/xt/common/fmatrix.hh>

namespace Dune::XT::LA::internal {


bool numpy_eigensolver_available();


/// \brief RAII wrapper giving a Python context manager `with`-semantics from C++.
/// \note The GIL has to be held for the entire lifetime of an instance, including its destruction.
class ScopedPythonContext
{
public:
  explicit ScopedPythonContext(pybind11::object context)
    : context_(std::move(context))
  {
    context_.attr("__enter__")();
  }

  ScopedPythonContext(const ScopedPythonContext&) = delete;
  ScopedPythonContext& operator=(const ScopedPythonContext&) = delete;

  ~ScopedPythonContext()
  {
    try {
      context_.attr("__exit__")(pybind11::none(), pybind11::none(), pybind11::none());
    } catch (...) {
      // Nothing sensible to do from a destructor, and failing to restore the previous state must not mask whatever
      // exception is already propagating.
    }
  }

private:
  pybind11::object context_;
}; // class ScopedPythonContext


/**
 * \todo Extend this for all matrix types M where is_matrix<M>::value or Common::is_matrix<M>::value is true, which
 *       would require pybind11 type casting for these (should happen in <dune/xt/common/matrix/bindings/hh>).
 *
 * \attention This backend needs the GIL, so selecting it inside a parallel grid walk serializes that walk on a single
 *            thread. Worse, if the walk was entered from a binding that does *not* release the GIL (Operator::assemble
 *            and Solver::apply, among others, hold it) the calling thread waits on tbb::parallel_for while the workers
 *            wait for the GIL, and the process deadlocks. "numpy" is never selected by default -- it is not
 *            EigenSolverOptions<>::types()[0] in any configuration -- so this only bites callers who explicitly ask
 *            for it. Do not, from inside a parallel walk.
 */
template <class K, int SIZE>
void compute_eigenvalues_and_right_eigenvectors_of_a_fieldmatrix_using_numpy(
    const FieldMatrix<K, SIZE, SIZE>& matrix,
    std::vector<Common::complex_t<K>>& eigenvalues,
    FieldMatrix<Common::complex_t<K>, SIZE, SIZE>& right_eigenvectors)
{
  if (!numpy_eigensolver_available())
    DUNE_THROW(Exceptions::eigen_solver_failed_bc_it_was_not_set_up_correctly,
               "Do not call me if numpy_eigensolver_available() is false!");
  // Hold the GIL for the whole body: every pybind11 object below has to be created *and* destroyed under it, and when
  // we are called from Python this may well run on a thread of a parallel grid walk that has no thread state at all
  // (the bindings release the GIL around the walk). Declared first, so it outlives every object created after it.
  pybind11::gil_scoped_acquire acquire_gil;
  auto numpy = PybindXI::GlobalInterpreter().import_module("numpy");
  auto warnings = PybindXI::GlobalInterpreter().import_module("warnings");
  // Turning warnings and floating point errors into exceptions is how the casts below detect a failed decomposition,
  // but numpy.seterr and warnings.filterwarnings are process-global: called from an embedded interpreter that only
  // affects us, called from Python it silently reconfigures the caller's session. Both have a context manager
  // equivalent, so scope the change to this function instead.
  ScopedPythonContext caught_warnings(warnings.attr("catch_warnings")());
  warnings.attr("simplefilter")("error");
  ScopedPythonContext raising_errstate(numpy.attr("errstate")(pybind11::arg("all") = "raise"));
  auto eig = PybindXI::GlobalInterpreter().import_module("numpy.linalg").attr("eig");
  try {
    auto result =
        eig(matrix)
            .template cast<
                std::tuple<FieldVector<Common::complex_t<K>, SIZE>, FieldMatrix<Common::complex_t<K>, SIZE, SIZE>>>();
    eigenvalues = Common::convert_to<std::vector<Common::complex_t<K>>>(std::get<0>(result));
    right_eigenvectors = std::get<1>(result);
  } catch (const pybind11::cast_error&) {
    auto result =
        eig(matrix)
            .template cast<
                std::tuple<FieldVector<Common::real_t<K>, SIZE>, FieldMatrix<Common::real_t<K>, SIZE, SIZE>>>();
    eigenvalues = Common::convert_to<std::vector<Common::complex_t<K>>>(std::get<0>(result));
    right_eigenvectors = Common::convert_to<FieldMatrix<Common::complex_t<K>, SIZE, SIZE>>(std::get<1>(result));
  } catch (const std::runtime_error& ee) {
    DUNE_THROW(Exceptions::eigen_solver_failed,
               "Could not convert result!\n\nThis was the original error: " << ee.what());
  }
} // ... compute_eigenvalues_and_right_eigenvectors_of_a_fieldmatrix_using_numpy(...)


} // namespace Dune::XT::LA::internal

#endif // DUNE_XT_LA_EIGEN_SOLVER_INTERNAL_NUMPY_HH
