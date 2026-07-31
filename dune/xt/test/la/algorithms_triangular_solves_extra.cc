// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   René Fritze    (2026)

// This file complements algorithms_triangular_solves_2x2.tpl and algorithms_triangular_solves_3x3.tpl, which only check
// the solutions of two very small systems. The tests here cover the code paths of
// dune/xt/la/algorithms/triangular_solves.hh which are not reached by those tests, i.e.
// * systems which are large enough to be of interest for the BLAS backend,
// * the CommonSparseOrDenseMatrix specialization of the solver, and
// * all error conditions (singular matrices, non-square matrices and calling the compressed sparse implementations for
//   matrices which are not stored in a compressed sparse format).

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

#include <dune/xt/la/algorithms/triangular_solves.hh>
#include <dune/xt/la/container.hh>

using namespace Dune;
using namespace Dune::XT;
using namespace Dune::XT::LA;

namespace {


constexpr size_t dim = 12;

// A triangular matrix with a strictly positive diagonal and without zeros in the triangular part.
template <class MatrixType>
MatrixType triangular_matrix(const size_t size, const Common::MatrixPattern pattern_type)
{
  using M = Common::MatrixAbstraction<MatrixType>;
  const bool lower = (pattern_type == Common::MatrixPattern::lower_triangular);
  auto ret = M::create(size, size, 0., triangular_pattern(size, size, pattern_type));
  for (size_t ii = 0; ii < size; ++ii)
    for (size_t jj = 0; jj < size; ++jj)
      if ((lower && jj <= ii) || (!lower && jj >= ii))
        M::set_entry(ret, ii, jj, ii == jj ? 2. + double(ii) : 1. / (1. + std::abs(int(ii) - int(jj))));
  return ret;
}

template <class VectorType>
VectorType prescribed_solution(const size_t size)
{
  using V = Common::VectorAbstraction<VectorType>;
  auto ret = V::create(size, 0.);
  for (size_t ii = 0; ii < size; ++ii)
    V::set_entry(ret, ii, (ii % 2 == 0 ? 1. : -1.) * (1. + double(ii)));
  return ret;
}

// Computes A x or A^T x, entry by entry, to obtain a right hand side for a prescribed solution.
template <class MatrixType, class VectorType>
VectorType multiply(const MatrixType& A, const VectorType& x, const bool transposed)
{
  using M = Common::MatrixAbstraction<MatrixType>;
  using V = Common::VectorAbstraction<VectorType>;
  const size_t size = M::rows(A);
  auto ret = V::create(size, 0.);
  for (size_t ii = 0; ii < size; ++ii) {
    typename V::ScalarType ret_ii(0.);
    for (size_t jj = 0; jj < size; ++jj)
      ret_ii += M::get_entry(A, transposed ? jj : ii, transposed ? ii : jj) * V::get_entry(x, jj);
    V::set_entry(ret, ii, ret_ii);
  }
  return ret;
}

template <class VectorType>
void expect_solution_eq(const VectorType& actual, const VectorType& expected, const size_t size)
{
  using V = Common::VectorAbstraction<VectorType>;
  for (size_t ii = 0; ii < size; ++ii)
    EXPECT_TRUE(Common::FloatCmp::eq(V::get_entry(actual, ii), V::get_entry(expected, ii), 1e-12, 1e-12))
        << "ii = " << ii << ", actual = " << V::get_entry(actual, ii) << ", expected = " << V::get_entry(expected, ii);
}

// Checks all four triangular solves for the given matrix and vector type.
template <class MatrixType, class VectorType>
void solves_correctly(const size_t size)
{
  using V = Common::VectorAbstraction<VectorType>;
  const auto lower = triangular_matrix<MatrixType>(size, Common::MatrixPattern::lower_triangular);
  const auto upper = triangular_matrix<MatrixType>(size, Common::MatrixPattern::upper_triangular);
  const auto expected_solution = prescribed_solution<VectorType>(size);
  auto solution = V::create(size, 0.);

  solve_lower_triangular(lower, solution, multiply(lower, expected_solution, false));
  expect_solution_eq(solution, expected_solution, size);
  solve_lower_triangular_transposed(lower, solution, multiply(lower, expected_solution, true));
  expect_solution_eq(solution, expected_solution, size);
  solve_upper_triangular(upper, solution, multiply(upper, expected_solution, false));
  expect_solution_eq(solution, expected_solution, size);
  solve_upper_triangular_transposed(upper, solution, multiply(upper, expected_solution, true));
  expect_solution_eq(solution, expected_solution, size);
} // ... solves_correctly(...)

// Checks that all four triangular solves report a matrix with a zero on the diagonal as singular.
template <class MatrixType, class VectorType>
void throws_for_singular_matrices(const size_t size)
{
  using M = Common::MatrixAbstraction<MatrixType>;
  using V = Common::VectorAbstraction<VectorType>;
  auto lower = triangular_matrix<MatrixType>(size, Common::MatrixPattern::lower_triangular);
  auto upper = triangular_matrix<MatrixType>(size, Common::MatrixPattern::upper_triangular);
  // the zero has to be in the interior, as the first and last row are not necessarily visited before the
  // implementations divide by the respective other diagonal entry
  M::set_entry(lower, size / 2, size / 2, 0.);
  M::set_entry(upper, size / 2, size / 2, 0.);
  const auto rhs = V::create(size, 1.);
  auto solution = V::create(size, 0.);
  EXPECT_THROW(solve_lower_triangular(lower, solution, rhs), Dune::MathError);
  EXPECT_THROW(solve_lower_triangular_transposed(lower, solution, rhs), Dune::MathError);
  EXPECT_THROW(solve_upper_triangular(upper, solution, rhs), Dune::MathError);
  EXPECT_THROW(solve_upper_triangular_transposed(upper, solution, rhs), Dune::MathError);
} // ... throws_for_singular_matrices(...)


} // namespace


// Systems of dimension two are special cased in the solver, all other dimensions share the same code paths.
GTEST_TEST(TriangularSolvesExtraTest, solves_two_dimensional_systems)
{
  solves_correctly<CommonDenseMatrix<double>, CommonDenseVector<double>>(2);
  solves_correctly<DynamicMatrix<double>, DynamicVector<double>>(2);
  solves_correctly<FieldMatrix<double, 2, 2>, FieldVector<double, 2>>(2);
  solves_correctly<CommonSparseMatrixCsr<double>, CommonDenseVector<double>>(2);
  solves_correctly<CommonSparseMatrixCsc<double>, CommonDenseVector<double>>(2);
}

GTEST_TEST(TriangularSolvesExtraTest, solves_dense_systems)
{
  solves_correctly<CommonDenseMatrix<double>, CommonDenseVector<double>>(dim);
  solves_correctly<CommonDenseMatrix<double>, DynamicVector<double>>(dim);
  solves_correctly<DynamicMatrix<double>, DynamicVector<double>>(dim);
  solves_correctly<FieldMatrix<double, dim, dim>, FieldVector<double, dim>>(dim);
  solves_correctly<Common::FieldMatrix<double, dim, dim>, Common::FieldVector<double, dim>>(dim);
}

GTEST_TEST(TriangularSolvesExtraTest, solves_sparse_systems)
{
  solves_correctly<CommonSparseMatrixCsr<double>, CommonDenseVector<double>>(dim);
  solves_correctly<CommonSparseMatrixCsc<double>, CommonDenseVector<double>>(dim);
  solves_correctly<CommonSparseMatrixCsr<double>, CommonSparseVector<double>>(dim);
  solves_correctly<CommonSparseMatrixCsc<double>, CommonSparseVector<double>>(dim);
  solves_correctly<IstlRowMajorSparseMatrix<double>, IstlDenseVector<double>>(dim);
}

// CommonSparseOrDenseMatrix decides on construction whether to store the matrix in a sparse or in a dense format, both
// of which are handled by a dedicated specialization of the triangular solver.
GTEST_TEST(TriangularSolvesExtraTest, solves_sparse_or_dense_systems)
{
  // a triangular pattern is dense enough for the dense storage to be chosen
  solves_correctly<CommonSparseOrDenseMatrixCsr<double>, CommonDenseVector<double>>(dim);
  solves_correctly<CommonSparseOrDenseMatrixCsc<double>, CommonDenseVector<double>>(dim);
  // ... while a bidiagonal pattern of a large enough matrix is sparse enough for the sparse storage
  using SparseOrDenseType = CommonSparseOrDenseMatrixCsr<double>;
  const size_t large_dim = 64;
  SparseOrDenseType lower(
      large_dim, large_dim, diagonal_pattern(large_dim, large_dim) + diagonal_pattern(large_dim, large_dim, -1));
  for (size_t ii = 0; ii < large_dim; ++ii) {
    lower.set_entry(ii, ii, 2. + double(ii));
    if (ii > 0)
      lower.set_entry(ii, ii - 1, -1.);
  }
  EXPECT_TRUE(lower.sparse());
  const auto expected_solution = prescribed_solution<CommonDenseVector<double>>(large_dim);
  auto solution = Common::VectorAbstraction<CommonDenseVector<double>>::create(large_dim, 0.);
  solve_lower_triangular(lower, solution, multiply(lower, expected_solution, false));
  expect_solution_eq(solution, expected_solution, large_dim);
}

GTEST_TEST(TriangularSolvesExtraTest, throws_for_singular_matrices)
{
  throws_for_singular_matrices<CommonDenseMatrix<double>, CommonDenseVector<double>>(dim);
  throws_for_singular_matrices<DynamicMatrix<double>, DynamicVector<double>>(dim);
  throws_for_singular_matrices<CommonSparseMatrixCsr<double>, CommonDenseVector<double>>(dim);
  throws_for_singular_matrices<CommonSparseMatrixCsc<double>, CommonDenseVector<double>>(dim);
  throws_for_singular_matrices<IstlRowMajorSparseMatrix<double>, IstlDenseVector<double>>(dim);
}

GTEST_TEST(TriangularSolvesExtraTest, throws_for_non_square_matrices)
{
  auto matrix = Common::MatrixAbstraction<CommonDenseMatrix<double>>::create(3, 4, 1.);
  auto rhs = Common::VectorAbstraction<CommonDenseVector<double>>::create(3, 1.);
  auto solution = Common::VectorAbstraction<CommonDenseVector<double>>::create(3, 0.);
  EXPECT_THROW(solve_lower_triangular(matrix, solution, rhs), Dune::InvalidStateException);
  EXPECT_THROW(solve_lower_triangular_transposed(matrix, solution, rhs), Dune::InvalidStateException);
  EXPECT_THROW(solve_upper_triangular(matrix, solution, rhs), Dune::InvalidStateException);
  EXPECT_THROW(solve_upper_triangular_transposed(matrix, solution, rhs), Dune::InvalidStateException);
}

GTEST_TEST(TriangularSolvesExtraTest, compressed_sparse_solves_throw_for_other_storage_layouts)
{
  const auto matrix = triangular_matrix<CommonDenseMatrix<double>>(dim, Common::MatrixPattern::lower_triangular);
  std::vector<double> x(dim, 0.);
  std::vector<double> rhs(dim, 1.);
  EXPECT_THROW(internal::forward_solve_csr(matrix, x, rhs), Dune::InvalidStateException);
  EXPECT_THROW(internal::forward_solve_csc(matrix, x, rhs), Dune::InvalidStateException);
  EXPECT_THROW(internal::backward_solve_csr(matrix, x, rhs), Dune::InvalidStateException);
  EXPECT_THROW(internal::backward_solve_csc(matrix, x, rhs), Dune::InvalidStateException);
}
