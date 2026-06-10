// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)

// This one has to come first (includes the config.h)!
#include <dune/xt/test/main.hxx>

#include <dune/common/dynvector.hh>

#include <dune/xt/common/exceptions.hh>
#include <dune/xt/la/algorithms/cholesky.hh>

using namespace Dune;
using namespace Dune::XT;
using namespace Dune::XT::LA;

using VectorType = Dune::DynamicVector<double>;


// solves the tridiagonal system with constant diagonal 2 and constant subdiagonal 1 of the given size and checks the
// solution against a manufactured right-hand side
void factorize_and_solve_tridiag_2_1(const size_t size)
{
  VectorType diag(size, 2.);
  VectorType subdiag(size - 1, 1.);
  internal::tridiagonal_ldlt(diag, subdiag);
  // manufactured solution x_i = i + 1, rhs = A x
  VectorType expected(size, 0.);
  VectorType rhs(size, 0.);
  for (size_t ii = 0; ii < size; ++ii) {
    expected[ii] = double(ii) + 1.;
    rhs[ii] = 2. * (double(ii) + 1.) + (ii > 0 ? double(ii) : 0.) + (ii < size - 1 ? double(ii) + 2. : 0.);
  }
  internal::solve_tridiag_ldlt(diag, subdiag, rhs);
  for (size_t ii = 0; ii < size; ++ii)
    EXPECT_NEAR(rhs[ii], expected[ii], 1e-13) << "size = " << size << ", ii = " << ii;
} // ... factorize_and_solve_tridiag_2_1(...)


GTEST_TEST(TridiagonalLdlt, throws_for_singular_matrix_with_singular_last_pivot)
{
  // [[1, 1], [1, 1]] is singular positive semidefinite: the LDL^T pivots are d = (1, 0), and only the last one
  // vanishes. The fallback factorization used to validate all pivots but the last, so the subsequent solve silently
  // divided by zero instead of throwing (LAPACK's dpttrf reports this case as an error).
  VectorType diag{1., 1.};
  VectorType subdiag{1.};
  EXPECT_THROW(internal::tridiagonal_ldlt(diag, subdiag), Dune::MathError);
  // same through the public interface (which dispatches to LAPACK if available)
  VectorType diag2{1., 1.};
  VectorType subdiag2{1.};
  EXPECT_THROW(tridiagonal_ldlt(diag2, subdiag2), Dune::MathError);
}

GTEST_TEST(TridiagonalLdlt, solve_works_for_consecutive_systems_of_different_sizes)
{
  // the solver used to cache its unit-lower-bidiagonal factor matrix in a thread_local that was sized on the first
  // call only, so a subsequent solve with a different size on the same thread accessed it out of bounds
  factorize_and_solve_tridiag_2_1(3);
  factorize_and_solve_tridiag_2_1(5); // <- used to access entries outside of the cached 3x3 sparsity pattern
  factorize_and_solve_tridiag_2_1(2); // <- used to iterate 5 rows over vectors of length 2
}
