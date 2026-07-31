// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   René Fritze    (2026)

// This file complements algorithms_solve_sym_tridiag_posdef.tpl, which only solves a single five dimensional system
// given as a full matrix. The tests here additionally cover the overload which takes the diagonal and the subdiagonal
// of the matrix, as well as systems of other dimensions.

#include <dune/xt/test/main.hxx> // <- This one has to come first, includes config.h!

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

#include <dune/xt/la/algorithms/solve_sym_tridiag_posdef.hh>
#include <dune/xt/la/container.hh>

using namespace Dune;
using namespace Dune::XT;
using namespace Dune::XT::LA;

namespace {


// The solution of A x = (1, ..., 1)^T, where A is the tridiagonal matrix with 2 on the diagonal and -1 on the sub- and
// superdiagonal.
std::vector<double> expected_solution(const size_t size)
{
  std::vector<double> ret(size);
  for (size_t ii = 0; ii < size; ++ii)
    ret[ii] = 0.5 * double(ii + 1) * double(size - ii);
  return ret;
}

template <class MatrixType>
MatrixType tridiagonal_matrix(const size_t size)
{
  using M = Common::MatrixAbstraction<MatrixType>;
  auto ret = M::create(size, size, 0., tridiagonal_pattern(size, size));
  for (size_t ii = 0; ii < size; ++ii) {
    M::set_entry(ret, ii, ii, 2.);
    if (ii > 0) {
      M::set_entry(ret, ii, ii - 1, -1.);
      M::set_entry(ret, ii - 1, ii, -1.);
    }
  }
  return ret;
}

template <class MatrixType, class VectorType>
void solves_correctly(const size_t size)
{
  using V = Common::VectorAbstraction<VectorType>;
  const auto matrix = tridiagonal_matrix<MatrixType>(size);
  const auto rhs = V::create(size, 1.);
  auto solution = V::create(size, 0.);
  solve_sym_tridiag_posdef(matrix, solution, rhs);
  const auto expected = expected_solution(size);
  for (size_t ii = 0; ii < size; ++ii)
    EXPECT_TRUE(Common::FloatCmp::eq(V::get_entry(solution, ii), expected[ii])) << "size = " << size << ", ii = " << ii;
}


} // namespace


GTEST_TEST(SolveSymTridiagPosdefExtraTest, solves_systems_given_by_a_matrix)
{
  solves_correctly<CommonDenseMatrix<double>, CommonDenseVector<double>>(3);
  solves_correctly<CommonDenseMatrix<double>, CommonDenseVector<double>>(12);
  solves_correctly<DynamicMatrix<double>, DynamicVector<double>>(12);
  solves_correctly<FieldMatrix<double, 12, 12>, FieldVector<double, 12>>(12);
  solves_correctly<CommonSparseMatrixCsr<double>, CommonDenseVector<double>>(12);
  solves_correctly<IstlRowMajorSparseMatrix<double>, IstlDenseVector<double>>(12);
}

GTEST_TEST(SolveSymTridiagPosdefExtraTest, solves_systems_given_by_diagonal_and_subdiagonal)
{
  for (const size_t size : {size_t(3), size_t(12)}) {
    std::vector<double> diag(size, 2.);
    std::vector<double> subdiag(size - 1, -1.);
    DynamicVector<double> rhs(size, 1.);
    solve_sym_tridiag_posdef(diag, subdiag, rhs);
    const auto expected = expected_solution(size);
    for (size_t ii = 0; ii < size; ++ii)
      EXPECT_TRUE(Common::FloatCmp::eq(rhs[ii], expected[ii])) << "size = " << size << ", ii = " << ii;
  }
}
