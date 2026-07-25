// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2026 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   René Fritze (2026)

// Tests of the free functions in dune/xt/la/generalized-eigen-solver/internal/lapacke.hh, which
// GeneralizedEigenSolver only ever reaches through const references to its two matrices. That fixes the
// MatrixDataProvider specialization to the copying one (a const matrix is never "contiguous and mutable"), so the
// in-place branch -- and with it the row-major dispatch to LAPACKE -- is only reachable by calling the free functions
// with mutable matrices, as done below. The same holds for the size checks of the *_impl function, which the solver's
// own check_size() shields.

#include <dune/xt/test/main.hxx> // <- has to come first (includes the config.h)!

#include <complex>
#include <vector>

#include <dune/xt/common/matrix.hh>
#include <dune/xt/la/container/common/matrix/dense.hh>
#include <dune/xt/la/container/common/matrix/sparse.hh>
#include <dune/xt/la/container/eigen.hh>
#include <dune/xt/la/exceptions.hh>
#include <dune/xt/la/generalized-eigen-solver.hh>
#include <dune/xt/la/generalized-eigen-solver/internal/lapacke.hh>

using namespace Dune;
using namespace Dune::XT;
using namespace Dune::XT::LA;


/// \brief Creates a rows x cols matrix from a row-wise list of entries.
template <class MatrixType>
MatrixType make_matrix(const size_t rows, const size_t cols, const std::vector<double>& entries)
{
  using M = Common::MatrixAbstraction<MatrixType>;
  EXPECT_EQ(rows * cols, entries.size());
  auto matrix = M::create(rows, cols, 0.);
  for (size_t ii = 0; ii < rows; ++ii)
    for (size_t jj = 0; jj < cols; ++jj)
      M::set_entry(matrix, ii, jj, entries[ii * cols + jj]);
  return matrix;
}


/**
 * \brief lhs = [3 1; 1 3], rhs = [2 0; 0 2] has the exactly representable eigenvalues 1 and 2.
 *
 * Called once with mutable matrices (which lets the implementation hand lapack the matrix memory directly, in
 * whatever storage layout the matrix type uses) and once with const ones (which makes it serialize into a
 * column-major buffer first). Both have to agree; note that the mutable variant is destructive, as dsygv overwrites
 * both of its inputs.
 */
template <class MatrixType>
void check_both_data_provider_paths()
{
  const std::vector<std::complex<double>> expected{{1., 0.}, {2., 0.}};
  {
    auto lhs = make_matrix<MatrixType>(2, 2, {3., 1., 1., 3.});
    auto rhs = make_matrix<MatrixType>(2, 2, {2., 0., 0., 2.});
    const auto eigenvalues = internal::compute_generalized_eigenvalues_using_lapack(lhs, rhs);
    ASSERT_EQ(expected.size(), eigenvalues.size());
    for (size_t ii = 0; ii < expected.size(); ++ii)
      EXPECT_TRUE(Common::FloatCmp::eq(expected[ii], eigenvalues[ii], {1e-14, 1e-14})) << eigenvalues[ii];
  }
  {
    const auto lhs = make_matrix<MatrixType>(2, 2, {3., 1., 1., 3.});
    const auto rhs = make_matrix<MatrixType>(2, 2, {2., 0., 0., 2.});
    const auto eigenvalues = internal::compute_generalized_eigenvalues_using_lapack(lhs, rhs);
    ASSERT_EQ(expected.size(), eigenvalues.size());
    for (size_t ii = 0; ii < expected.size(); ++ii)
      EXPECT_TRUE(Common::FloatCmp::eq(expected[ii], eigenvalues[ii], {1e-14, 1e-14})) << eigenvalues[ii];
  }
} // ... check_both_data_provider_paths(...)


/// \brief Each of the three size requirements of the implementation has to be reported separately.
template <class MatrixType>
void check_size_requirements()
{
  const auto square = make_matrix<MatrixType>(2, 2, {1., 0., 0., 1.});
  const auto non_square = make_matrix<MatrixType>(2, 3, {1., 0., 0., 0., 1., 0.});
  const auto larger_square = make_matrix<MatrixType>(3, 3, {1., 0., 0., 0., 1., 0., 0., 0., 1.});
  EXPECT_THROW(internal::compute_generalized_eigenvalues_using_lapack(non_square, square),
               Exceptions::generalized_eigen_solver_failed_bc_data_did_not_fulfill_requirements);
  EXPECT_THROW(internal::compute_generalized_eigenvalues_using_lapack(square, larger_square),
               Exceptions::generalized_eigen_solver_failed_bc_data_did_not_fulfill_requirements);
  EXPECT_THROW(internal::compute_generalized_eigenvalues_using_lapack(square, non_square),
               Exceptions::generalized_eigen_solver_failed_bc_data_did_not_fulfill_requirements);
} // ... check_size_requirements(...)


/// \brief dsygv needs a positive definite right hand side and reports anything else through its info return value.
template <class MatrixType>
void check_backend_failure_is_reported()
{
  const auto lhs = make_matrix<MatrixType>(2, 2, {1., 1., 1., 1.});
  const auto zero = make_matrix<MatrixType>(2, 2, {0., 0., 0., 0.});
  const auto indefinite = make_matrix<MatrixType>(2, 2, {1., 0., 0., -1.});
  EXPECT_THROW(internal::compute_generalized_eigenvalues_using_lapack(lhs, zero),
               Exceptions::generalized_eigen_solver_failed);
  EXPECT_THROW(internal::compute_generalized_eigenvalues_using_lapack(lhs, indefinite),
               Exceptions::generalized_eigen_solver_failed);
} // ... check_backend_failure_is_reported(...)


/**
 * \brief Solving problems of growing size in one thread has to keep working.
 *
 * The implementation keeps the eigenvalue buffer in a thread_local, which used to be sized on the first call only
 * and made lapack write past its end for every larger problem afterwards. That overwrote unrelated heap memory
 * without changing any of the values checked here, so this is a regression check that bites in a sanitizer build;
 * the growing-then-shrinking sequence is what a single test binary looks like to the shared buffer.
 *
 * Run for const and for mutable inputs alike: those select different MatrixDataProvider specializations, hence
 * different instantiations of the implementation, each with its own function-local buffer.
 */
template <class MatrixType>
void check_varying_problem_sizes()
{
  for (const size_t size : {1, 2, 3, 4, 5, 6, 2, 1}) {
    std::vector<double> lhs_entries(size * size, 0.);
    std::vector<double> rhs_entries(size * size, 0.);
    for (size_t ii = 0; ii < size; ++ii) {
      lhs_entries[ii * size + ii] = double(ii + 1);
      rhs_entries[ii * size + ii] = 1.;
    }
    const auto lhs = make_matrix<MatrixType>(size, size, lhs_entries);
    const auto rhs = make_matrix<MatrixType>(size, size, rhs_entries);
    // dsygv overwrites both of its inputs, so the mutable variant needs matrices of its own
    auto mutable_lhs = make_matrix<MatrixType>(size, size, lhs_entries);
    auto mutable_rhs = make_matrix<MatrixType>(size, size, rhs_entries);
    for (const auto& eigenvalues : {internal::compute_generalized_eigenvalues_using_lapack(lhs, rhs),
                                    internal::compute_generalized_eigenvalues_using_lapack(mutable_lhs, mutable_rhs)}) {
      ASSERT_EQ(size, eigenvalues.size()) << "size: " << size;
      for (size_t ii = 0; ii < size; ++ii)
        EXPECT_TRUE(Common::FloatCmp::eq(std::complex<double>(double(ii + 1), 0.), eigenvalues[ii], {1e-14, 1e-14}))
            << "size: " << size << ", eigenvalue: " << eigenvalues[ii];
    }
  }
} // ... check_varying_problem_sizes(...)


GTEST_TEST(GeneralizedEigenSolverInternals, common_dense_matrix)
{
  using MatrixType = CommonDenseMatrix<double>; // <- row-major and contiguous
  check_both_data_provider_paths<MatrixType>();
  check_size_requirements<MatrixType>();
  check_backend_failure_is_reported<MatrixType>();
  check_varying_problem_sizes<MatrixType>();
}

GTEST_TEST(GeneralizedEigenSolverInternals, common_sparse_matrix)
{
  using MatrixType = CommonSparseMatrix<double>; // <- never contiguous, always serialized
  check_both_data_provider_paths<MatrixType>();
  check_size_requirements<MatrixType>();
  check_backend_failure_is_reported<MatrixType>();
  check_varying_problem_sizes<MatrixType>();
}

GTEST_TEST(GeneralizedEigenSolverInternals, field_matrix)
{
  using MatrixType = FieldMatrix<double, 2, 2>; // <- row-major, contiguous and of static size
  check_both_data_provider_paths<MatrixType>();
  check_backend_failure_is_reported<MatrixType>();
}

#if HAVE_EIGEN
GTEST_TEST(GeneralizedEigenSolverInternals, eigen_dense_matrix)
{
  using MatrixType = EigenDenseMatrix<double>; // <- column-major and contiguous
  check_both_data_provider_paths<MatrixType>();
  check_size_requirements<MatrixType>();
  check_backend_failure_is_reported<MatrixType>();
  check_varying_problem_sizes<MatrixType>();
}
#endif // HAVE_EIGEN
