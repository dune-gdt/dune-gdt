// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)

// This one has to come first (includes the config.h)!
#include <dune/xt/test/main.hxx>

#include <dune/xt/common/fmatrix.hh>
#include <dune/xt/common/matrix.hh>

#include <dune/xt/la/algorithms/cholesky.hh>
#include <dune/xt/la/container.hh>
#include <dune/xt/test/la/container.hh>

using namespace Dune;
using namespace Dune::XT;
using namespace Dune::XT::LA;


// The sparse code paths used to skip writing factor entries that are exactly zero ("only_set_nonzero"). If the
// corresponding entry of A is part of the sparsity pattern and nonzero, the stale value of A survived in the factor
// and silently poisoned the remaining factorization (or made it throw on a perfectly SPD matrix).
template <class MatrixType>
void check_cholesky_factor_with_exact_zero()
{
  using M = Common::MatrixAbstraction<MatrixType>;
  // A = [[1,1,1],[1,2,1],[1,1,5]] is SPD with L = [[1,0,0],[1,1,0],[1,0,2]]; note the exact zero L_21 at a position
  // where A holds a nonzero value
  auto A = create<MatrixType>({{1, 1, 1}, {1, 2, 1}, {1, 1, 5}}, dense_pattern(3, 3));
  cholesky(A);
  EXPECT_NEAR(M::get_entry(A, 0, 0), 1., 1e-14);
  EXPECT_NEAR(M::get_entry(A, 1, 0), 1., 1e-14);
  EXPECT_NEAR(M::get_entry(A, 1, 1), 1., 1e-14);
  EXPECT_NEAR(M::get_entry(A, 2, 0), 1., 1e-14);
  EXPECT_NEAR(M::get_entry(A, 2, 1), 0., 1e-14); // <- used to keep the stale value 1 on the sparse paths
  EXPECT_NEAR(M::get_entry(A, 2, 2), 2., 1e-14); // <- used to be sqrt(3) on the sparse paths

  // A2 = [[1,1,1],[1,2,1],[1,1,2]] is SPD with L = [[1,0,0],[1,1,0],[1,0,1]]; with the stale L_21 = 1 the pivot
  // L_22^2 = 2 - 1 - 1 = 0 made the factorization throw on this SPD matrix
  auto A2 = create<MatrixType>({{1, 1, 1}, {1, 2, 1}, {1, 1, 2}}, dense_pattern(3, 3));
  EXPECT_NO_THROW(cholesky(A2));
  EXPECT_NEAR(M::get_entry(A2, 2, 1), 0., 1e-14);
  EXPECT_NEAR(M::get_entry(A2, 2, 2), 1., 1e-14);
} // ... check_cholesky_factor_with_exact_zero(...)


GTEST_TEST(CholeskyWithExactZeroInFactor, dense_field_matrix)
{
  check_cholesky_factor_with_exact_zero<FieldMatrix<double, 3, 3>>();
}

GTEST_TEST(CholeskyWithExactZeroInFactor, common_sparse_matrix_csr)
{
  check_cholesky_factor_with_exact_zero<CommonSparseMatrixCsr<double>>();
}

GTEST_TEST(CholeskyWithExactZeroInFactor, common_sparse_matrix_csc)
{
  check_cholesky_factor_with_exact_zero<CommonSparseMatrixCsc<double>>();
}

GTEST_TEST(CholeskyWithExactZeroInFactor, istl_row_major_sparse_matrix)
{
  check_cholesky_factor_with_exact_zero<IstlRowMajorSparseMatrix<double>>();
}
