// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   René Fritze    (2026)

// This file complements algorithms_qr_5x5.tpl, algorithms_qr_5x3.tpl and algorithms_qr_block_2x3x3.tpl, all of which
// factorize matrices which are too small to be handed to LAPACK. The tests here cover the code paths of
// dune/xt/la/algorithms/qr.hh which are not reached by those tests, i.e.
// * the LAPACK backed factorization, reconstruction of Q and multiplication by Q,
// * the complex valued Householder reflections,
// * rank deficient matrices (for which the reduction of a column is skipped),
// * the solvers built on top of the factorization (for vector, matrix and blocked matrix right hand sides), and
// * all error conditions.

#include <dune/xt/test/main.hxx> // <- This one has to come first, includes config.h!

#include <cmath>
#include <complex>
#include <vector>

#include <dune/common/dynmatrix.hh>
#include <dune/common/dynvector.hh>
#include <dune/common/fmatrix.hh>
#include <dune/common/fvector.hh>

#include <dune/xt/common/exceptions.hh>
#include <dune/xt/common/float_cmp.hh>
#include <dune/xt/common/fmatrix.hh>
#include <dune/xt/common/fvector.hh>
#include <dune/xt/common/math.hh>
#include <dune/xt/common/matrix.hh>
#include <dune/xt/common/vector.hh>

#include <dune/xt/la/algorithms/qr.hh>
#include <dune/xt/la/container.hh>

using namespace Dune;
using namespace Dune::XT;
using namespace Dune::XT::LA;

namespace {


// Anything larger than internal::min_lapack_factor_size is factorized by LAPACK (if available), anything smaller by our
// own implementation.
constexpr size_t small_dim = 5;
constexpr size_t large_dim = 12;

template <class ScalarType>
ScalarType test_matrix_entry(const size_t ii, const size_t jj, const size_t dim)
{
  const auto real_part = 1. / (1. + std::abs(int(ii) - int(jj))) + 0.1 * double(int(ii) - int(jj)) / double(dim)
                         + (ii == jj ? double(dim) : 0.);
  if constexpr (Common::is_complex<ScalarType>::value)
    return ScalarType(real_part, 0.25 * double(int(ii) - int(jj)) / double(dim));
  else
    return ScalarType(real_part);
}

// A non-symmetric, strictly diagonally dominant (and thus invertible) matrix.
template <class MatrixType>
MatrixType test_matrix(const size_t dim)
{
  using M = Common::MatrixAbstraction<MatrixType>;
  auto ret = M::create(dim, dim, 0.);
  for (size_t ii = 0; ii < dim; ++ii)
    for (size_t jj = 0; jj < dim; ++jj)
      M::set_entry(ret, ii, jj, test_matrix_entry<typename M::ScalarType>(ii, jj, dim));
  return ret;
}

template <class VectorType>
VectorType prescribed_solution(const size_t dim)
{
  using V = Common::VectorAbstraction<VectorType>;
  auto ret = V::create(dim, 0.);
  for (size_t ii = 0; ii < dim; ++ii)
    V::set_entry(ret, ii, typename V::ScalarType((ii % 2 == 0 ? 1. : -1.) * (1. + double(ii))));
  return ret;
}

template <class MatrixType, class VectorType>
VectorType multiply(const MatrixType& A, const VectorType& x)
{
  using M = Common::MatrixAbstraction<MatrixType>;
  using V = Common::VectorAbstraction<VectorType>;
  auto ret = V::create(M::rows(A), 0.);
  for (size_t ii = 0; ii < M::rows(A); ++ii) {
    typename V::ScalarType ret_ii(0.);
    for (size_t jj = 0; jj < M::cols(A); ++jj)
      ret_ii += M::get_entry(A, ii, jj) * V::get_entry(x, jj);
    V::set_entry(ret, ii, ret_ii);
  }
  return ret;
}

// Checks that Q is orthogonal (unitary in the complex case) and that Q R equals the column permutation of A given by
// permutations, i.e. that A P = Q R.
template <class MatrixType, class QMatrixType, class IndexVectorType>
void expect_is_qr_factorization_of(const MatrixType& QR,
                                   const QMatrixType& Q,
                                   const IndexVectorType& permutations,
                                   const MatrixType& A,
                                   const size_t dim,
                                   const double tolerance = 1e-13)
{
  using M = Common::MatrixAbstraction<MatrixType>;
  using MQ = Common::MatrixAbstraction<QMatrixType>;
  using ScalarType = typename M::ScalarType;
  for (size_t ii = 0; ii < dim; ++ii)
    for (size_t jj = 0; jj < dim; ++jj) {
      ScalarType QT_Q_ij(0.);
      ScalarType Q_R_ij(0.);
      for (size_t kk = 0; kk < dim; ++kk) {
        QT_Q_ij += Common::conj(MQ::get_entry(Q, kk, ii)) * MQ::get_entry(Q, kk, jj);
        // R is the upper triangular part of QR
        if (kk <= jj)
          Q_R_ij += MQ::get_entry(Q, ii, kk) * M::get_entry(QR, kk, jj);
      }
      EXPECT_LT(std::abs(QT_Q_ij - ScalarType(ii == jj ? 1. : 0.)), tolerance) << "ii = " << ii << ", jj = " << jj;
      EXPECT_LT(std::abs(Q_R_ij - M::get_entry(A, ii, size_t(permutations[jj]))), tolerance)
          << "ii = " << ii << ", jj = " << jj;
    }
} // ... expect_is_qr_factorization_of(...)

template <class MatrixType, class VectorType>
void factorizes_correctly(const size_t dim, const double tolerance = 1e-13)
{
  using M = Common::MatrixAbstraction<MatrixType>;
  using V = Common::VectorAbstraction<VectorType>;
  using ScalarType = typename M::ScalarType;
  const auto matrix = test_matrix<MatrixType>(dim);
  auto QR = matrix;
  auto tau = V::create(dim, ScalarType(0.));
  std::vector<int> permutations(dim);
  qr(QR, tau, permutations);
  const auto Q = calculate_q_from_qr(QR, tau);
  expect_is_qr_factorization_of(QR, Q, permutations, matrix, dim, tolerance);

  // y = Q x and y = Q^T x have to coincide with the respective matrix vector products
  const auto x = prescribed_solution<VectorType>(dim);
  auto y = V::create(dim, ScalarType(0.));
  apply_q_from_qr<Common::Transpose::no>(QR, tau, x, y);
  const auto expected_y = multiply(Q, x);
  for (size_t ii = 0; ii < dim; ++ii)
    EXPECT_LT(std::abs(V::get_entry(y, ii) - V::get_entry(expected_y, ii)), tolerance) << "ii = " << ii;
  auto transposed_y = V::create(dim, ScalarType(0.));
  apply_q_from_qr<Common::Transpose::yes>(QR, tau, x, transposed_y);
  for (size_t ii = 0; ii < dim; ++ii) {
    ScalarType expected_ii(0.);
    for (size_t kk = 0; kk < dim; ++kk)
      expected_ii += Common::conj(Common::MatrixAbstraction<std::decay_t<decltype(Q)>>::get_entry(Q, kk, ii))
                     * V::get_entry(x, kk);
    EXPECT_LT(std::abs(V::get_entry(transposed_y, ii) - expected_ii), tolerance) << "ii = " << ii;
  }
} // ... factorizes_correctly(...)

template <class MatrixType, class VectorType>
void solves_correctly(const size_t dim, const double tolerance = 1e-12)
{
  using V = Common::VectorAbstraction<VectorType>;
  const auto matrix = test_matrix<MatrixType>(dim);
  const auto expected_solution = prescribed_solution<VectorType>(dim);
  const auto rhs = multiply(matrix, expected_solution);
  auto solution = V::create(dim, 0.);
  auto A = matrix;
  solve_by_qr_decomposition(A, solution, rhs);
  for (size_t ii = 0; ii < dim; ++ii)
    EXPECT_LT(std::abs(V::get_entry(solution, ii) - V::get_entry(expected_solution, ii)), tolerance)
        << "ii = " << ii << ", solution = " << V::get_entry(solution, ii);
} // ... solves_correctly(...)


} // namespace


GTEST_TEST(QrExtraTest, factorizes_dense_matrices)
{
  factorizes_correctly<CommonDenseMatrix<double>, CommonDenseVector<double>>(small_dim);
  factorizes_correctly<CommonDenseMatrix<double>, CommonDenseVector<double>>(large_dim);
  factorizes_correctly<CommonDenseMatrix<double>, DynamicVector<double>>(large_dim);
  factorizes_correctly<DynamicMatrix<double>, DynamicVector<double>>(large_dim);
  factorizes_correctly<FieldMatrix<double, large_dim, large_dim>, FieldVector<double, large_dim>>(large_dim);
}

GTEST_TEST(QrExtraTest, factorizes_complex_matrices)
{
  factorizes_correctly<CommonDenseMatrix<std::complex<double>>, CommonDenseVector<std::complex<double>>>(small_dim);
  factorizes_correctly<CommonDenseMatrix<std::complex<double>>, CommonDenseVector<std::complex<double>>>(large_dim);
}

GTEST_TEST(QrExtraTest, factorizes_rank_deficient_matrices)
{
  // the reduction by a Householder matrix is skipped for columns which are zero below the diagonal
  const size_t dim = 4;
  using MatrixType = CommonDenseMatrix<double>;
  auto matrix = Common::MatrixAbstraction<MatrixType>::create(dim, dim, 0.);
  matrix.set_entry(0, 0, 2.);
  matrix.set_entry(1, 1, 1.);
  const auto expected_matrix = matrix;
  auto QR = matrix;
  std::vector<double> tau(dim, 0.);
  std::vector<int> permutations(dim);
  qr(QR, tau, permutations);
  const auto Q = calculate_q_from_qr(QR, tau);
  expect_is_qr_factorization_of(QR, Q, permutations, expected_matrix, dim);
}

GTEST_TEST(QrExtraTest, solves_with_qr_decomposition)
{
  solves_correctly<CommonDenseMatrix<double>, CommonDenseVector<double>>(small_dim);
  solves_correctly<CommonDenseMatrix<double>, CommonDenseVector<double>>(large_dim);
  solves_correctly<DynamicMatrix<double>, DynamicVector<double>>(large_dim);
  solves_correctly<FieldMatrix<double, large_dim, large_dim>, FieldVector<double, large_dim>>(large_dim);
  solves_correctly<CommonSparseMatrixCsr<double>, CommonDenseVector<double>>(small_dim);
  solves_correctly<CommonSparseMatrixCsc<double>, CommonDenseVector<double>>(small_dim);
}

GTEST_TEST(QrExtraTest, solves_with_matrix_valued_right_hand_side)
{
  using MatrixType = CommonDenseMatrix<double>;
  using M = Common::MatrixAbstraction<MatrixType>;
  const size_t num_rhs = 3;
  const auto matrix = test_matrix<MatrixType>(large_dim);
  auto expected_solution = M::create(large_dim, num_rhs, 0.);
  for (size_t ii = 0; ii < large_dim; ++ii)
    for (size_t jj = 0; jj < num_rhs; ++jj)
      M::set_entry(expected_solution, ii, jj, (ii % 2 == 0 ? 1. : -1.) * (1. + double(ii + jj)));
  auto rhs = M::create(large_dim, num_rhs, 0.);
  for (size_t ii = 0; ii < large_dim; ++ii)
    for (size_t jj = 0; jj < num_rhs; ++jj) {
      double rhs_ij = 0.;
      for (size_t kk = 0; kk < large_dim; ++kk)
        rhs_ij += M::get_entry(matrix, ii, kk) * M::get_entry(expected_solution, kk, jj);
      M::set_entry(rhs, ii, jj, rhs_ij);
    }
  auto A = matrix;
  auto solution = M::create(large_dim, num_rhs, 0.);
  solve_by_qr_decomposition(A, solution, rhs);
  for (size_t ii = 0; ii < large_dim; ++ii)
    for (size_t jj = 0; jj < num_rhs; ++jj)
      EXPECT_LT(std::abs(M::get_entry(solution, ii, jj) - M::get_entry(expected_solution, ii, jj)), 1e-12)
          << "ii = " << ii << ", jj = " << jj;
}

GTEST_TEST(QrExtraTest, solves_blocked_systems)
{
  static constexpr size_t num_blocks = 2;
  static constexpr size_t block_dim = 3;
  using MatrixType = Common::BlockedFieldMatrix<double, num_blocks, block_dim, block_dim>;
  MatrixType matrix(0.);
  MatrixType expected_solution(0.);
  for (size_t bb = 0; bb < num_blocks; ++bb)
    for (size_t ii = 0; ii < block_dim; ++ii) {
      for (size_t jj = 0; jj < block_dim; ++jj)
        matrix.block(bb)[ii][jj] = test_matrix_entry<double>(ii, jj, block_dim) + double(bb);
      expected_solution.block(bb)[ii][0] = (ii % 2 == 0 ? 1. : -1.) * (1. + double(ii + bb));
    }
  MatrixType rhs(0.);
  for (size_t bb = 0; bb < num_blocks; ++bb)
    for (size_t ii = 0; ii < block_dim; ++ii)
      for (size_t kk = 0; kk < block_dim; ++kk)
        rhs.block(bb)[ii][0] += matrix.block(bb)[ii][kk] * expected_solution.block(bb)[kk][0];
  auto A = matrix;
  MatrixType solution(0.);
  solve_by_qr_decomposition(A, solution, rhs);
  for (size_t bb = 0; bb < num_blocks; ++bb)
    for (size_t ii = 0; ii < block_dim; ++ii)
      EXPECT_LT(std::abs(solution.block(bb)[ii][0] - expected_solution.block(bb)[ii][0]), 1e-13)
          << "bb = " << bb << ", ii = " << ii;
}

GTEST_TEST(QrExtraTest, throws_for_more_columns_than_rows)
{
  using MatrixType = CommonDenseMatrix<double>;
  const size_t num_rows = 3;
  const size_t num_cols = 5;
  auto matrix = Common::MatrixAbstraction<MatrixType>::create(num_rows, num_cols, 1.);
  std::vector<double> tau(num_cols, 0.);
  std::vector<int> permutations(num_cols);
  EXPECT_THROW(calculate_q_from_qr(matrix, tau), Dune::NotImplemented);
  auto rhs = Common::VectorAbstraction<CommonDenseVector<double>>::create(num_rows, 1.);
  auto solution = Common::VectorAbstraction<CommonDenseVector<double>>::create(num_cols, 0.);
  EXPECT_THROW(solve_qr_factorized(matrix, tau, permutations, solution, rhs), Dune::NotImplemented);
}
