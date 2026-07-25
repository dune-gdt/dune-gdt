// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   René Fritze    (2026)

// This file complements algorithms_cholesky.tpl: while the latter checks the factorization of a single small
// tridiagonal matrix for all container types, the tests here cover the code paths of dune/xt/la/algorithms/cholesky.hh
// which are not reached by that test, i.e.
// * the LAPACK backed factorization (which is only used for dense matrices larger than min_lapack_factor_size),
// * the storage layout specific implementations (row-wise, column-wise, csr and csc),
// * solving with a given Cholesky factor,
// * the LDL^T implementations which are not backed by LAPACK, and
// * all error conditions.

#include <dune/xt/test/main.hxx> // <- This one has to come first, includes config.h!

#include <cmath>
#include <vector>

#include <dune/common/dynmatrix.hh>
#include <dune/common/dynvector.hh>
#include <dune/common/fmatrix.hh>
#include <dune/common/fvector.hh>

#include <dune/xt/common/exceptions.hh>
#include <dune/xt/common/float_cmp.hh>
#include <dune/xt/common/fmatrix.hh>
#include <dune/xt/common/fvector.hh>
#include <dune/xt/common/matrix.hh>
#include <dune/xt/common/vector.hh>

#include <dune/xt/la/algorithms/cholesky.hh>
#include <dune/xt/la/container.hh>

using namespace Dune;
using namespace Dune::XT;
using namespace Dune::XT::LA;

namespace {


// Anything larger than internal::min_lapack_factor_size is factorized by LAPACK (if available), anything smaller by our
// own implementations. We test both regimes with each of the dimensions below.
constexpr size_t small_dim = 5;
constexpr size_t large_dim = 12;

// A symmetric and strictly diagonally dominant (and thus positive definite) matrix without zero entries.
template <class MatrixType>
MatrixType spd_matrix(const size_t dim)
{
  using M = Common::MatrixAbstraction<MatrixType>;
  auto ret = M::create(dim, dim, 0.);
  for (size_t ii = 0; ii < dim; ++ii)
    for (size_t jj = 0; jj < dim; ++jj)
      M::set_entry(ret, ii, jj, 1. / (1. + std::abs(int(ii) - int(jj))));
  for (size_t ii = 0; ii < dim; ++ii)
    M::add_to_entry(ret, ii, ii, double(dim));
  return ret;
}

// The same matrix with a negative entry on the last diagonal element, i.e. not positive definite anymore.
template <class MatrixType>
MatrixType indefinite_matrix(const size_t dim)
{
  using M = Common::MatrixAbstraction<MatrixType>;
  auto ret = spd_matrix<MatrixType>(dim);
  M::set_entry(ret, dim - 1, dim - 1, -1.);
  return ret;
}

// Checks that the lower triangular part of L is the Cholesky factor of A, i.e. that (L L^T)_ij == A_ij for j <= i. The
// strictly upper triangular part of L is not checked, as it is left untouched by all implementations.
template <class MatrixType>
void expect_is_cholesky_factor_of(const MatrixType& L, const MatrixType& A, const size_t dim)
{
  using M = Common::MatrixAbstraction<MatrixType>;
  for (size_t ii = 0; ii < dim; ++ii) {
    for (size_t jj = 0; jj <= ii; ++jj) {
      typename M::ScalarType L_LT_ij(0.);
      for (size_t kk = 0; kk <= jj; ++kk)
        L_LT_ij += M::get_entry(L, ii, kk) * M::get_entry(L, jj, kk);
      EXPECT_TRUE(Common::FloatCmp::eq(L_LT_ij, M::get_entry(A, ii, jj), 1e-13, 1e-13))
          << "ii = " << ii << ", jj = " << jj << ", (L L^T)_ij = " << L_LT_ij << ", A_ij = " << M::get_entry(A, ii, jj);
    }
    EXPECT_GT(M::get_entry(L, ii, ii), 0.);
  }
} // ... expect_is_cholesky_factor_of(...)

template <class MatrixType>
void factorizes_correctly(const size_t dim)
{
  const auto matrix = spd_matrix<MatrixType>(dim);
  auto L = matrix;
  cholesky(L);
  expect_is_cholesky_factor_of(L, matrix, dim);
}

// The solution of A x = b for the matrix returned by spd_matrix is not available in closed form, so we prescribe x and
// compute the corresponding right hand side instead.
template <class MatrixType, class VectorType>
void solves_correctly(const size_t dim)
{
  using M = Common::MatrixAbstraction<MatrixType>;
  using V = Common::VectorAbstraction<VectorType>;
  const auto matrix = spd_matrix<MatrixType>(dim);
  auto expected_solution = V::create(dim, 0.);
  for (size_t ii = 0; ii < dim; ++ii)
    V::set_entry(expected_solution, ii, (ii % 2 == 0 ? 1. : -1.) * (1. + double(ii)));
  auto rhs = V::create(dim, 0.);
  for (size_t ii = 0; ii < dim; ++ii) {
    typename V::ScalarType rhs_ii(0.);
    for (size_t jj = 0; jj < dim; ++jj)
      rhs_ii += M::get_entry(matrix, ii, jj) * V::get_entry(expected_solution, jj);
    V::set_entry(rhs, ii, rhs_ii);
  }
  auto L = matrix;
  cholesky(L);
  solve_cholesky_factorized(L, rhs);
  for (size_t ii = 0; ii < dim; ++ii)
    EXPECT_TRUE(Common::FloatCmp::eq(V::get_entry(rhs, ii), V::get_entry(expected_solution, ii), 1e-12, 1e-12))
        << "ii = " << ii << ", x_ii = " << V::get_entry(rhs, ii) << ", expected "
        << V::get_entry(expected_solution, ii);
} // ... solves_correctly(...)

// The expected LDL^T factorization of the tridiagonal matrix with 2 on the diagonal and -1 on the sub- and
// superdiagonal, see e.g. algorithms_cholesky.tpl for the five dimensional case.
std::vector<double> expected_ldlt_diag(const size_t dim)
{
  std::vector<double> ret(dim);
  for (size_t ii = 0; ii < dim; ++ii)
    ret[ii] = double(ii + 2) / double(ii + 1);
  return ret;
}

std::vector<double> expected_ldlt_subdiag(const size_t dim)
{
  std::vector<double> ret(dim - 1);
  for (size_t ii = 0; ii < dim - 1; ++ii)
    ret[ii] = -double(ii + 1) / double(ii + 2);
  return ret;
}

// The solution of A x = (1, ..., 1)^T for the same matrix.
std::vector<double> expected_tridiag_solution(const size_t dim)
{
  std::vector<double> ret(dim);
  for (size_t ii = 0; ii < dim; ++ii)
    ret[ii] = 0.5 * double(ii + 1) * double(dim - ii);
  return ret;
}


} // namespace


GTEST_TEST(CholeskyExtraTest, factorizes_dense_matrices)
{
  factorizes_correctly<CommonDenseMatrix<double>>(small_dim);
  factorizes_correctly<CommonDenseMatrix<double>>(large_dim);
  factorizes_correctly<DynamicMatrix<double>>(small_dim);
  factorizes_correctly<DynamicMatrix<double>>(large_dim);
  factorizes_correctly<FieldMatrix<double, small_dim, small_dim>>(small_dim);
  factorizes_correctly<FieldMatrix<double, large_dim, large_dim>>(large_dim);
  factorizes_correctly<Common::FieldMatrix<double, large_dim, large_dim>>(large_dim);
}

GTEST_TEST(CholeskyExtraTest, factorizes_sparse_matrices)
{
  factorizes_correctly<CommonSparseMatrixCsr<double>>(small_dim);
  factorizes_correctly<CommonSparseMatrixCsr<double>>(large_dim);
  factorizes_correctly<CommonSparseMatrixCsc<double>>(small_dim);
  factorizes_correctly<CommonSparseMatrixCsc<double>>(large_dim);
#if HAVE_DUNE_ISTL
  factorizes_correctly<IstlRowMajorSparseMatrix<double>>(small_dim);
  factorizes_correctly<IstlRowMajorSparseMatrix<double>>(large_dim);
#endif
}

GTEST_TEST(CholeskyExtraTest, storage_layout_specific_implementations_agree)
{
  const auto matrix = spd_matrix<CommonDenseMatrix<double>>(small_dim);
  auto rowwise = matrix;
  internal::cholesky_rowwise<false>(rowwise);
  expect_is_cholesky_factor_of(rowwise, matrix, small_dim);
  auto rowwise_sparse = matrix;
  internal::cholesky_rowwise<true>(rowwise_sparse);
  expect_is_cholesky_factor_of(rowwise_sparse, matrix, small_dim);
  auto colwise = matrix;
  internal::cholesky_colwise<false>(colwise);
  expect_is_cholesky_factor_of(colwise, matrix, small_dim);
  auto colwise_sparse = matrix;
  internal::cholesky_colwise<true>(colwise_sparse);
  expect_is_cholesky_factor_of(colwise_sparse, matrix, small_dim);
  for (size_t ii = 0; ii < small_dim; ++ii)
    for (size_t jj = 0; jj <= ii; ++jj) {
      EXPECT_DOUBLE_EQ(rowwise.get_entry(ii, jj), colwise.get_entry(ii, jj));
      EXPECT_DOUBLE_EQ(rowwise.get_entry(ii, jj), rowwise_sparse.get_entry(ii, jj));
      EXPECT_DOUBLE_EQ(rowwise.get_entry(ii, jj), colwise_sparse.get_entry(ii, jj));
    }
}

GTEST_TEST(CholeskyExtraTest, solves_with_given_cholesky_factor)
{
  solves_correctly<CommonDenseMatrix<double>, CommonDenseVector<double>>(small_dim);
  solves_correctly<CommonDenseMatrix<double>, CommonDenseVector<double>>(large_dim);
  solves_correctly<CommonDenseMatrix<double>, DynamicVector<double>>(large_dim);
  solves_correctly<DynamicMatrix<double>, DynamicVector<double>>(small_dim);
  solves_correctly<DynamicMatrix<double>, DynamicVector<double>>(large_dim);
  // the two dimensional case is special cased in the triangular solves
  solves_correctly<CommonDenseMatrix<double>, CommonDenseVector<double>>(2);
}

GTEST_TEST(CholeskyExtraTest, throws_on_non_square_matrix)
{
  auto matrix = Common::MatrixAbstraction<CommonDenseMatrix<double>>::create(3, 4, 1.);
  EXPECT_THROW(cholesky(matrix), Dune::InvalidStateException);
}

GTEST_TEST(CholeskyExtraTest, throws_on_matrix_which_is_not_positive_definite)
{
  auto small_dense = indefinite_matrix<CommonDenseMatrix<double>>(small_dim);
  EXPECT_THROW(cholesky(small_dense), Dune::MathError);
  auto large_dense = indefinite_matrix<CommonDenseMatrix<double>>(large_dim);
  EXPECT_THROW(cholesky(large_dense), Dune::MathError);
  auto dynamic_matrix = indefinite_matrix<DynamicMatrix<double>>(large_dim);
  EXPECT_THROW(cholesky(dynamic_matrix), Dune::MathError);
  auto csr = indefinite_matrix<CommonSparseMatrixCsr<double>>(small_dim);
  EXPECT_THROW(cholesky(csr), Dune::MathError);
  auto csc = indefinite_matrix<CommonSparseMatrixCsc<double>>(small_dim);
  EXPECT_THROW(cholesky(csc), Dune::MathError);
#if HAVE_DUNE_ISTL
  auto istl = indefinite_matrix<IstlRowMajorSparseMatrix<double>>(small_dim);
  EXPECT_THROW(cholesky(istl), Dune::MathError);
#endif
}

GTEST_TEST(CholeskyExtraTest, csr_and_csc_factorizations_throw_for_other_storage_layouts)
{
  auto dense = spd_matrix<CommonDenseMatrix<double>>(small_dim);
  EXPECT_THROW(internal::cholesky_csr(dense), Dune::InvalidStateException);
  EXPECT_THROW(internal::cholesky_csc(dense), Dune::InvalidStateException);
  auto csr = spd_matrix<CommonSparseMatrixCsr<double>>(small_dim);
  EXPECT_THROW(internal::cholesky_csc(csr), Dune::InvalidStateException);
  auto csc = spd_matrix<CommonSparseMatrixCsc<double>>(small_dim);
  EXPECT_THROW(internal::cholesky_csr(csc), Dune::InvalidStateException);
}

GTEST_TEST(CholeskyExtraTest, tridiagonal_ldlt_factorizes_correctly)
{
  for (const size_t dim : {small_dim, large_dim}) {
    std::vector<double> diag(dim, 2.);
    std::vector<double> subdiag(dim - 1, -1.);
    tridiagonal_ldlt(diag, subdiag);
    const auto expected_diag = expected_ldlt_diag(dim);
    const auto expected_subdiag = expected_ldlt_subdiag(dim);
    for (size_t ii = 0; ii < dim; ++ii)
      EXPECT_TRUE(Common::FloatCmp::eq(diag[ii], expected_diag[ii])) << "dim = " << dim << ", ii = " << ii;
    for (size_t ii = 0; ii < dim - 1; ++ii)
      EXPECT_TRUE(Common::FloatCmp::eq(subdiag[ii], expected_subdiag[ii])) << "dim = " << dim << ", ii = " << ii;
  }
}

// internal::tridiagonal_ldlt is only used if neither MKL nor LAPACKE are available, so we call it directly to have it
// covered in any case.
GTEST_TEST(CholeskyExtraTest, internal_tridiagonal_ldlt_factorizes_correctly)
{
  const size_t dim = small_dim;
  std::vector<double> diag(dim, 2.);
  std::vector<double> subdiag(dim - 1, -1.);
  internal::tridiagonal_ldlt(diag, subdiag);
  const auto expected_diag = expected_ldlt_diag(dim);
  const auto expected_subdiag = expected_ldlt_subdiag(dim);
  for (size_t ii = 0; ii < dim; ++ii)
    EXPECT_TRUE(Common::FloatCmp::eq(diag[ii], expected_diag[ii])) << "ii = " << ii;
  for (size_t ii = 0; ii < dim - 1; ++ii)
    EXPECT_TRUE(Common::FloatCmp::eq(subdiag[ii], expected_subdiag[ii])) << "ii = " << ii;
}

GTEST_TEST(CholeskyExtraTest, internal_tridiagonal_ldlt_throws_if_not_positive_definite)
{
  std::vector<double> diag{1., -1., 1.};
  std::vector<double> subdiag{2., 0.};
  EXPECT_THROW(internal::tridiagonal_ldlt(diag, subdiag), Dune::MathError);
}

GTEST_TEST(CholeskyExtraTest, tridiagonal_ldlt_throws_for_mismatching_sizes)
{
  std::vector<double> diag(small_dim, 2.);
  std::vector<double> too_long_subdiag(small_dim, -1.);
  EXPECT_THROW(tridiagonal_ldlt(diag, too_long_subdiag), Dune::InvalidStateException);
  std::vector<double> too_short_subdiag(small_dim - 2, -1.);
  EXPECT_THROW(tridiagonal_ldlt(diag, too_short_subdiag), Dune::InvalidStateException);
}

// internal::solve_tridiag_ldlt is only used for right hand sides which are not stored contiguously, so we call it
// directly to have it covered for all configurations. Note that both dimensions are solved within the same test to
// ensure that the (thread local) helper matrix is adapted to the size of the system.
GTEST_TEST(CholeskyExtraTest, internal_solve_tridiag_ldlt_solves_correctly)
{
  for (const size_t dim : {small_dim, large_dim}) {
    std::vector<double> diag(dim, 2.);
    std::vector<double> subdiag(dim - 1, -1.);
    internal::tridiagonal_ldlt(diag, subdiag);
    DynamicVector<double> vector_rhs(dim, 1.);
    internal::solve_tridiag_ldlt(diag, subdiag, vector_rhs);
    const auto expected_solution = expected_tridiag_solution(dim);
    for (size_t ii = 0; ii < dim; ++ii)
      EXPECT_TRUE(Common::FloatCmp::eq(vector_rhs[ii], expected_solution[ii])) << "dim = " << dim << ", ii = " << ii;
    DynamicMatrix<double> matrix_rhs(dim, dim, 1.);
    internal::solve_tridiag_ldlt(diag, subdiag, matrix_rhs);
    for (size_t jj = 0; jj < dim; ++jj)
      for (size_t ii = 0; ii < dim; ++ii)
        EXPECT_TRUE(Common::FloatCmp::eq(matrix_rhs[ii][jj], expected_solution[ii]))
            << "dim = " << dim << ", ii = " << ii << ", jj = " << jj;
  }
}

GTEST_TEST(CholeskyExtraTest, solve_tridiagonal_ldlt_factorized_handles_matrix_right_hand_sides)
{
  const size_t dim = large_dim;
  std::vector<double> diag(dim, 2.);
  std::vector<double> subdiag(dim - 1, -1.);
  tridiagonal_ldlt(diag, subdiag);
  const auto expected_solution = expected_tridiag_solution(dim);
  // a dense row major right hand side (handled by LAPACK, if available)
  auto dense_rhs = Common::MatrixAbstraction<CommonDenseMatrix<double>>::create(dim, dim, 1.);
  solve_tridiagonal_ldlt_factorized(diag, subdiag, dense_rhs);
  // a right hand side which is not stored contiguously
  DynamicMatrix<double> dynamic_rhs(dim, dim, 1.);
  solve_tridiagonal_ldlt_factorized(diag, subdiag, dynamic_rhs);
  for (size_t jj = 0; jj < dim; ++jj)
    for (size_t ii = 0; ii < dim; ++ii) {
      EXPECT_TRUE(Common::FloatCmp::eq(dense_rhs.get_entry(ii, jj), expected_solution[ii]))
          << "ii = " << ii << ", jj = " << jj;
      EXPECT_TRUE(Common::FloatCmp::eq(dynamic_rhs[ii][jj], expected_solution[ii])) << "ii = " << ii << ", jj = " << jj;
    }
}
