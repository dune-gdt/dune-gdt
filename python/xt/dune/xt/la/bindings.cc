// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2016 - 2019)
//   René Fritze     (2018 - 2019)
//   Tim Keil        (2018)
//   Tobias Leibner  (2018, 2020 - 2021)

#include "config.h"

#include <complex>
#include <string>
#include <vector>

#include <dune/common/parallel/mpihelper.hh>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/complex.h>

#include <python/xt/dune/xt/common/bindings.hh>

#include <python/xt/dune/xt/la/container/container-interface.hh>
#include <python/xt/dune/xt/la/container/vector-interface.hh>
#include <python/xt/dune/xt/la/container/pattern.hh>
#include <python/xt/dune/xt/la/container/matrix-interface.hh>
#include <python/xt/dune/xt/la/solver.hh>
#include <python/xt/dune/xt/la/eigen_solver.hh>
#include <python/xt/dune/xt/la/generalized_eigen_solver.hh>
#include <python/xt/dune/xt/la/matrix_inverter.hh>

#include <dune/xt/common/timedlogging.hh>

#include <dune/xt/la/container.hh>
#include <dune/xt/common/timedlogging.hh>


PYBIND11_MODULE(_la, m)
{
  namespace py = pybind11;
  using namespace pybind11::literals;
  namespace LA = Dune::XT::LA;

  py::module::import("dune.xt.common");

  LA::bind_Backends(m);

  auto common_dense_vector_double = LA::bind_Vector<LA::CommonDenseVector<double>>(m);
  LA::bind_Vector<LA::CommonDenseVector<size_t>>(m);
  auto common_dense_vector_complex = LA::bind_Vector<LA::CommonDenseVector<std::complex<double>>>(m);
#if HAVE_DUNE_ISTL
  auto istl_dense_vector_double = LA::bind_Vector<LA::IstlDenseVector<double>>(m);
  auto istl_dense_vector_complex = LA::bind_Vector<LA::IstlDenseVector<std::complex<double>>>(m);
#endif
#if HAVE_EIGEN
  auto eigen_dense_vector_double = LA::bind_Vector<LA::EigenDenseVector<double>>(m);
  auto eigen_dense_vector_complex = LA::bind_Vector<LA::EigenDenseVector<std::complex<double>>>(m);
#endif

  LA::bind_SparsityPatternDefault(m);

#define BIND_MATRIX(C, s, c) auto c = LA::bind_Matrix<C, s>(m);

  BIND_MATRIX(LA::CommonDenseMatrix<double>, false, common_dense_matrix_double);
  BIND_MATRIX(LA::CommonDenseMatrix<std::complex<double>>, false, common_dense_matrix_complex);
  // The generic linear Solver (see solver.hh) has no CommonSparseMatrix specialization yet (a
  // pre-existing C++-side gap, not a bindings gap -- see the WP5 PR description), so these sparse
  // Common containers are bound as containers only: full vector-space algebra, I/O, matrix-vector
  // products, and (via matrix_inverter.hh below) direct inversion, but no bind_Solver call.
  BIND_MATRIX(LA::CommonSparseMatrixCsr<double>, true, common_sparse_csr_matrix_double);
  BIND_MATRIX(LA::CommonSparseMatrixCsc<double>, true, common_sparse_csc_matrix_double);
  BIND_MATRIX(LA::CommonSparseMatrixCsr<std::complex<double>>, true, common_sparse_csr_matrix_complex);
  BIND_MATRIX(LA::CommonSparseMatrixCsc<std::complex<double>>, true, common_sparse_csc_matrix_complex);
#if HAVE_DUNE_ISTL
  BIND_MATRIX(LA::IstlRowMajorSparseMatrix<double>, true, istl_row_major_sparse_matrix_double);
  BIND_MATRIX(LA::IstlRowMajorSparseMatrix<std::complex<double>>, true, istl_row_major_sparse_matrix_complex);
#endif
#if HAVE_EIGEN
  BIND_MATRIX(LA::EigenDenseMatrix<double>, false, eigen_dense_matrix_double);
  BIND_MATRIX(LA::EigenDenseMatrix<std::complex<double>>, false, eigen_dense_matrix_complex);
  BIND_MATRIX(LA::EigenRowMajorSparseMatrix<double>, true, eigen_row_major_sparse_matrix_double);
  BIND_MATRIX(LA::EigenRowMajorSparseMatrix<std::complex<double>>, true, eigen_row_major_sparse_matrix_complex);
#endif
#undef BIND_MATRIX
  LA::addbind_Matrix_Vector_interaction(common_dense_matrix_double, common_dense_vector_double);
  LA::addbind_Matrix_Vector_interaction(common_dense_matrix_complex, common_dense_vector_complex);
  LA::addbind_Matrix_Vector_interaction(common_sparse_csr_matrix_double, common_dense_vector_double);
  LA::addbind_Matrix_Vector_interaction(common_sparse_csc_matrix_double, common_dense_vector_double);
  LA::addbind_Matrix_Vector_interaction(common_sparse_csr_matrix_complex, common_dense_vector_complex);
  LA::addbind_Matrix_Vector_interaction(common_sparse_csc_matrix_complex, common_dense_vector_complex);
#if HAVE_DUNE_ISTL
  LA::addbind_Matrix_Vector_interaction(istl_row_major_sparse_matrix_double, istl_dense_vector_double);
  LA::addbind_Matrix_Vector_interaction(istl_row_major_sparse_matrix_complex, istl_dense_vector_complex);
#endif
#if HAVE_EIGEN
  LA::addbind_Matrix_Vector_interaction(eigen_dense_matrix_double, eigen_dense_vector_double);
  LA::addbind_Matrix_Vector_interaction(eigen_dense_matrix_complex, eigen_dense_vector_complex);
  LA::addbind_Matrix_Vector_interaction(eigen_row_major_sparse_matrix_double, eigen_dense_vector_double);
  LA::addbind_Matrix_Vector_interaction(eigen_row_major_sparse_matrix_complex, eigen_dense_vector_complex);
#endif

  // The plain linear Solver (Ax = b, used throughout the FEM/operator solving pipeline) stays
  // double-only real-valued: extending it to complex fields is out of scope for WP5, which closes
  // the *bindings* gap for the already complex-capable eigen-solver/matrix-inverter machinery below,
  // not the Solver itself.
  LA::bind_Solver<LA::CommonDenseMatrix<double>>(m);
#if HAVE_DUNE_ISTL
  LA::bind_Solver<LA::IstlRowMajorSparseMatrix<double>>(m);
#endif
#if HAVE_EIGEN
  LA::bind_Solver<LA::EigenDenseMatrix<double>>(m);
  LA::bind_Solver<LA::EigenRowMajorSparseMatrix<double>>(m);
#endif

  // Eigen-solver, generalized eigen-solver and matrix-inverter (WP5, #320): bound exactly for the
  // matrix/field combinations the C++ .tpl test suite already exercises (see
  // dune/xt/test/la/{eigensolver,generalized-eigensolver,matrixinverter}_for_*.py), so every bound
  // combination below has existing, passing compiled test coverage to fall back on.
  LA::bind_EigenSolver<LA::CommonDenseMatrix<double>>(m);
  LA::bind_EigenSolver<LA::CommonSparseMatrixCsr<double>>(m);
  LA::bind_GeneralizedEigenSolver<LA::CommonDenseMatrix<double>>(m);
  LA::bind_GeneralizedEigenSolver<LA::CommonSparseMatrixCsr<double>>(m);
  LA::bind_MatrixInverter<LA::CommonDenseMatrix<double>>(m);
  LA::bind_MatrixInverter<LA::CommonDenseMatrix<std::complex<double>>>(m);
  LA::bind_MatrixInverter<LA::CommonSparseMatrixCsr<double>>(m);
  LA::bind_MatrixInverter<LA::CommonSparseMatrixCsr<std::complex<double>>>(m);
#if HAVE_EIGEN
  LA::bind_EigenSolver<LA::EigenDenseMatrix<double>>(m);
  LA::bind_GeneralizedEigenSolver<LA::EigenDenseMatrix<double>>(m);
  LA::bind_MatrixInverter<LA::EigenDenseMatrix<double>>(m);
  LA::bind_MatrixInverter<LA::EigenDenseMatrix<std::complex<double>>>(m);
#endif
} // PYBIND11_MODULE(...)
