// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)

// This one has to come first (includes the config.h)!
#include <dune/xt/test/main.hxx>

#include <algorithm>
#include <cmath>
#include <type_traits>
#include <vector>

#include <dune/common/dynmatrix.hh>
#include <dune/common/fmatrix.hh>
#include <dune/common/fvector.hh>

#include <dune/xt/common/fmatrix.hh>
#include <dune/xt/common/matrix.hh>
#include <dune/xt/la/algorithms/qr.hh>
#include <dune/xt/la/container/eye-matrix.hh>

using namespace Dune;
using namespace Dune::XT;
using namespace Dune::XT::LA;


// Checks AP = QR and the orthogonality of Q for a matrix of full column rank.
//
// The pre-existing 5x3 test passes a rank-2 matrix (its third column is a linear combination of the first two), so
// it never noticed that the Householder QR used to apply only num_cols - 1 reflections: for full-rank matrices with
// num_rows > num_cols the subdiagonal entries of the last (pivoted) column were left un-eliminated, producing
// Q and R with Q R != A P.
template <class MatrixType>
void check_qr_decomposition(const MatrixType& matrix, const size_t num_rows, const size_t num_cols)
{
  using M = Common::MatrixAbstraction<MatrixType>;
  auto QR = matrix;
  Dune::DynamicVector<double> tau(num_cols, 0.);
  std::vector<int> permutations(num_cols);
  qr(QR, tau, permutations);
  // extract R (upper triangular part)
  auto R = QR;
  for (size_t ii = 0; ii < num_rows; ++ii)
    for (size_t jj = 0; jj < std::min(ii, num_cols); ++jj)
      M::set_entry(R, ii, jj, 0.);
  const auto Q = calculate_q_from_qr(QR, tau);
  using QM = Common::MatrixAbstraction<std::decay_t<decltype(Q)>>;
  // Q has to be orthogonal
  for (size_t ii = 0; ii < num_rows; ++ii)
    for (size_t jj = 0; jj < num_rows; ++jj) {
      double entry_QtQ = 0.;
      for (size_t kk = 0; kk < num_rows; ++kk)
        entry_QtQ += QM::get_entry(Q, kk, ii) * QM::get_entry(Q, kk, jj);
      EXPECT_NEAR(entry_QtQ, ii == jj ? 1. : 0., 1e-13) << "ii = " << ii << ", jj = " << jj;
    }
  // Q R has to equal A P (P permutes the columns of A)
  for (size_t ii = 0; ii < num_rows; ++ii)
    for (size_t jj = 0; jj < num_cols; ++jj) {
      double entry_QR = 0.;
      for (size_t kk = 0; kk < num_rows; ++kk)
        entry_QR += QM::get_entry(Q, ii, kk) * (kk < num_rows ? M::get_entry(R, kk, jj) : 0.);
      const double entry_AP = M::get_entry(matrix, ii, static_cast<size_t>(permutations[jj]));
      EXPECT_NEAR(entry_QR, entry_AP, 1e-13) << "ii = " << ii << ", jj = " << jj;
    }
} // ... check_qr_decomposition(...)


GTEST_TEST(QrTallFullRank, decomposes_5x3_full_rank_matrix)
{
  const FieldMatrix<double, 5, 3> matrix{{2., 1., 0.}, {1., 3., 1.}, {0., 1., 4.}, {1., 0., 1.}, {3., 1., 2.}};
  check_qr_decomposition(matrix, 5, 3);
}

GTEST_TEST(QrTallFullRank, decomposes_3x2_full_rank_matrix)
{
  // for this matrix the missing last reflection left A(2,1) un-eliminated before the fix
  const FieldMatrix<double, 3, 2> matrix{{1., 0.}, {0., 1.}, {0., 1.}};
  check_qr_decomposition(matrix, 3, 2);
}

GTEST_TEST(QrTallFullRank, decomposes_4x1_matrix)
{
  // a single-column matrix received no reflection at all before the fix
  const FieldMatrix<double, 4, 1> matrix{{1.}, {2.}, {2.}, {4.}};
  check_qr_decomposition(matrix, 4, 1);
  // additionally check R(0,0) = +-||A||_2 = +-5
  auto QR = matrix;
  Dune::DynamicVector<double> tau(1, 0.);
  std::vector<int> permutations(1);
  qr(QR, tau, permutations);
  EXPECT_NEAR(std::abs(QR[0][0]), 5., 1e-13);
}

GTEST_TEST(QrTallFullRank, decomposes_dynamic_matrix)
{
  Dune::DynamicMatrix<double> matrix(5, 3, 0.);
  const FieldMatrix<double, 5, 3> values{{2., 1., 0.}, {1., 3., 1.}, {0., 1., 4.}, {1., 0., 1.}, {3., 1., 2.}};
  for (size_t ii = 0; ii < 5; ++ii)
    for (size_t jj = 0; jj < 3; ++jj)
      matrix[ii][jj] = values[ii][jj];
  check_qr_decomposition(matrix, 5, 3);
}

GTEST_TEST(QrTallFullRank, apply_q_from_qr_roundtrip)
{
  const FieldMatrix<double, 5, 3> matrix{{2., 1., 0.}, {1., 3., 1.}, {0., 1., 4.}, {1., 0., 1.}, {3., 1., 2.}};
  auto QR = matrix;
  Dune::DynamicVector<double> tau(3, 0.);
  std::vector<int> permutations(3);
  qr(QR, tau, permutations);
  const FieldVector<double, 5> x{1., -2., 3., 0.5, -1.};
  FieldVector<double, 5> Qx(0.), QtQx(0.);
  apply_q_from_qr<Common::Transpose::no>(QR, tau, x, Qx);
  apply_q_from_qr<Common::Transpose::yes>(QR, tau, Qx, QtQx);
  for (size_t ii = 0; ii < 5; ++ii)
    EXPECT_NEAR(QtQx[ii], x[ii], 1e-13) << "ii = " << ii;
}

GTEST_TEST(QrTallFullRank, decomposes_12x2_matrix_and_applies_q)
{
  // 12 rows > min_lapack_factor_size = 10, so this exercises the LAPACK code paths (geqp3/dormqr) where available;
  // the dormqr call used to pass the number of rows as the number of elementary reflectors (reading past the end of
  // tau for tall matrices) and the wrong leading dimension of QR
  FieldMatrix<double, 12, 2> matrix;
  for (size_t ii = 0; ii < 12; ++ii) {
    matrix[ii][0] = 1. + 0.5 * ii;
    matrix[ii][1] = std::pow(-1., int(ii)) + 0.1 * ii * ii;
  }
  check_qr_decomposition(matrix, 12, 2);
  auto QR = matrix;
  Dune::DynamicVector<double> tau(2, 0.);
  std::vector<int> permutations(2);
  qr(QR, tau, permutations);
  FieldVector<double, 12> x(0.), Qx(0.), QtQx(0.);
  for (size_t ii = 0; ii < 12; ++ii)
    x[ii] = std::sin(double(ii));
  apply_q_from_qr<Common::Transpose::no>(QR, tau, x, Qx);
  apply_q_from_qr<Common::Transpose::yes>(QR, tau, Qx, QtQx);
  for (size_t ii = 0; ii < 12; ++ii)
    EXPECT_NEAR(QtQx[ii], x[ii], 1e-12) << "ii = " << ii;
}

GTEST_TEST(MultiplyHouseholderFromRight, matches_hand_computed_example)
{
  // A (I - tau v v^T) for A = [[1, 2], [3, 4]], tau = 1, v = (1, 1):
  // A v = (3, 7), so the result is A - (A v) v^T = [[-2, -1], [-4, -3]].
  // The implementation used to accumulate A v with the wrong index, yielding [[-3, -2], [-3, -2]].
  FieldMatrix<double, 2, 2> A{{1., 2.}, {3., 4.}};
  const FieldVector<double, 2> v{1., 1.};
  internal::multiply_householder_from_right(A, 1., v, 0, 2, 0, 2);
  EXPECT_NEAR(A[0][0], -2., 1e-15);
  EXPECT_NEAR(A[0][1], -1., 1e-15);
  EXPECT_NEAR(A[1][0], -4., 1e-15);
  EXPECT_NEAR(A[1][1], -3., 1e-15);
}
