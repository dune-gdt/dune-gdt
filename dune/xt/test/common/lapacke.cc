// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   René Fritze    (2026)

// Covers the wrappers in dune/xt/common/lapacke.hh which are not exercised anywhere else in the test suite: dgeev,
// dgeevx, dgeevx_work, dgeqp3, dgesvd, dlamch, dorgqr, dormqr, dpotrf, dptcon, dpocon and dtrcon.
//
// Every wrapper has two bodies, selected by HAVE_MKL || HAVE_LAPACKE: the LAPACKE call and a
// DUNE_THROW(dependency_missing). Which one is compiled is a property of the build, so each test asserts the
// behaviour of whichever branch is active -- available() tells them apart, exactly as the wrappers' documentation
// instructs callers to do.
//
// All matrices are laid out row-major and are small enough to have hand-checkable results.

#include <dune/xt/test/main.hxx> // <- This one has to come first, includes config.h!

#include <algorithm>
#include <cmath>
#include <vector>

#include <dune/xt/common/exceptions.hh>
#include <dune/xt/common/float_cmp.hh>
#include <dune/xt/common/lapacke.hh>

using namespace Dune::XT::Common;

namespace {


//! The symmetric positive definite A = [[2, 1], [1, 2]] with eigenvalues (and singular values) 1 and 3.
std::vector<double> spd_matrix()
{
  return {2., 1., 1., 2.};
}

//! Asserts that the LAPACK info code signals success.
void expect_lapack_success(const int info, const std::string& who)
{
  EXPECT_EQ(0, info) << who << " returned info = " << info;
}


} // namespace


GTEST_TEST(lapacke, available_matches_the_build_configuration)
{
#if HAVE_MKL || HAVE_LAPACKE
  EXPECT_TRUE(Lapacke::available());
#else
  EXPECT_FALSE(Lapacke::available());
#endif
  EXPECT_EQ(Lapacke::available(), Lapacke::available());
}


GTEST_TEST(lapacke, the_layout_constants_differ_or_throw)
{
  if (!Lapacke::available()) {
    EXPECT_THROW(Lapacke::row_major(), Exceptions::dependency_missing);
    EXPECT_THROW(Lapacke::col_major(), Exceptions::dependency_missing);
    return;
  }
  EXPECT_NE(Lapacke::row_major(), Lapacke::col_major());
}


GTEST_TEST(lapacke, dgeev)
{
  auto a = spd_matrix();
  std::vector<double> wr(2, 0.);
  std::vector<double> wi(2, 0.);
  std::vector<double> vl(4, 0.);
  std::vector<double> vr(4, 0.);
  if (!Lapacke::available()) {
    EXPECT_THROW(Lapacke::dgeev(0, 'V', 'V', 2, a.data(), 2, wr.data(), wi.data(), vl.data(), 2, vr.data(), 2),
                 Exceptions::dependency_missing);
    return;
  }
  expect_lapack_success(
      Lapacke::dgeev(Lapacke::row_major(), 'V', 'V', 2, a.data(), 2, wr.data(), wi.data(), vl.data(), 2, vr.data(), 2),
      "dgeev");
  // A is symmetric, so both eigenvalues are real; LAPACK does not promise an ordering, hence the sort.
  std::sort(wr.begin(), wr.end());
  EXPECT_TRUE(FloatCmp::eq(wr[0], 1.)) << "wr[0] = " << wr[0];
  EXPECT_TRUE(FloatCmp::eq(wr[1], 3.)) << "wr[1] = " << wr[1];
  EXPECT_TRUE(FloatCmp::eq(wi[0], 0.));
  EXPECT_TRUE(FloatCmp::eq(wi[1], 0.));
}


GTEST_TEST(lapacke, dgeevx)
{
  auto a = spd_matrix();
  std::vector<double> wr(2, 0.);
  std::vector<double> wi(2, 0.);
  std::vector<double> vl(4, 0.);
  std::vector<double> vr(4, 0.);
  std::vector<double> scale(2, 0.);
  std::vector<double> rconde(2, 0.);
  std::vector<double> rcondv(2, 0.);
  int ilo = 0;
  int ihi = 0;
  double abnrm = 0.;
  if (!Lapacke::available()) {
    EXPECT_THROW(Lapacke::dgeevx(0,
                                 'B',
                                 'V',
                                 'V',
                                 'B',
                                 2,
                                 a.data(),
                                 2,
                                 wr.data(),
                                 wi.data(),
                                 vl.data(),
                                 2,
                                 vr.data(),
                                 2,
                                 &ilo,
                                 &ihi,
                                 scale.data(),
                                 &abnrm,
                                 rconde.data(),
                                 rcondv.data()),
                 Exceptions::dependency_missing);
    return;
  }
  // sense = 'B' requires both eigenvector jobs to be 'V'.
  expect_lapack_success(Lapacke::dgeevx(Lapacke::row_major(),
                                        'B',
                                        'V',
                                        'V',
                                        'B',
                                        2,
                                        a.data(),
                                        2,
                                        wr.data(),
                                        wi.data(),
                                        vl.data(),
                                        2,
                                        vr.data(),
                                        2,
                                        &ilo,
                                        &ihi,
                                        scale.data(),
                                        &abnrm,
                                        rconde.data(),
                                        rcondv.data()),
                        "dgeevx");
  std::sort(wr.begin(), wr.end());
  EXPECT_TRUE(FloatCmp::eq(wr[0], 1.)) << "wr[0] = " << wr[0];
  EXPECT_TRUE(FloatCmp::eq(wr[1], 3.)) << "wr[1] = " << wr[1];
  // abnrm is the one-norm of the balanced matrix, which for our A is 3.
  EXPECT_TRUE(FloatCmp::eq(abnrm, 3.)) << "abnrm = " << abnrm;
  // A is symmetric, so all its eigenvalues and eigenvectors are perfectly conditioned.
  for (size_t ii = 0; ii < 2; ++ii) {
    EXPECT_GT(rconde[ii], 0.) << "ii = " << ii;
    EXPECT_GT(rcondv[ii], 0.) << "ii = " << ii;
  }
}


GTEST_TEST(lapacke, dgeevx_work)
{
  // The _work variant leaves the workspace allocation to the caller; we obtain its size from the usual lwork = -1
  // query. Column major is used here so that the query is passed straight through to LAPACK.
  auto a = spd_matrix();
  std::vector<double> wr(2, 0.);
  std::vector<double> wi(2, 0.);
  std::vector<double> vl(4, 0.);
  std::vector<double> vr(4, 0.);
  std::vector<double> scale(2, 0.);
  std::vector<double> rconde(2, 0.);
  std::vector<double> rcondv(2, 0.);
  std::vector<int> iwork(4, 0);
  int ilo = 0;
  int ihi = 0;
  double abnrm = 0.;
  std::vector<double> work(1, 0.);
  if (!Lapacke::available()) {
    EXPECT_THROW(Lapacke::dgeevx_work(0,
                                      'B',
                                      'V',
                                      'V',
                                      'B',
                                      2,
                                      a.data(),
                                      2,
                                      wr.data(),
                                      wi.data(),
                                      vl.data(),
                                      2,
                                      vr.data(),
                                      2,
                                      &ilo,
                                      &ihi,
                                      scale.data(),
                                      &abnrm,
                                      rconde.data(),
                                      rcondv.data(),
                                      work.data(),
                                      -1,
                                      iwork.data()),
                 Exceptions::dependency_missing);
    return;
  }
  expect_lapack_success(Lapacke::dgeevx_work(Lapacke::col_major(),
                                             'B',
                                             'V',
                                             'V',
                                             'B',
                                             2,
                                             a.data(),
                                             2,
                                             wr.data(),
                                             wi.data(),
                                             vl.data(),
                                             2,
                                             vr.data(),
                                             2,
                                             &ilo,
                                             &ihi,
                                             scale.data(),
                                             &abnrm,
                                             rconde.data(),
                                             rcondv.data(),
                                             work.data(),
                                             -1,
                                             iwork.data()),
                        "dgeevx_work (workspace query)");
  const auto lwork = static_cast<int>(work[0]);
  ASSERT_GT(lwork, 0);
  work.assign(static_cast<size_t>(lwork), 0.);
  expect_lapack_success(Lapacke::dgeevx_work(Lapacke::col_major(),
                                             'B',
                                             'V',
                                             'V',
                                             'B',
                                             2,
                                             a.data(),
                                             2,
                                             wr.data(),
                                             wi.data(),
                                             vl.data(),
                                             2,
                                             vr.data(),
                                             2,
                                             &ilo,
                                             &ihi,
                                             scale.data(),
                                             &abnrm,
                                             rconde.data(),
                                             rcondv.data(),
                                             work.data(),
                                             lwork,
                                             iwork.data()),
                        "dgeevx_work");
  std::sort(wr.begin(), wr.end());
  EXPECT_TRUE(FloatCmp::eq(wr[0], 1.)) << "wr[0] = " << wr[0];
  EXPECT_TRUE(FloatCmp::eq(wr[1], 3.)) << "wr[1] = " << wr[1];
}


GTEST_TEST(lapacke, dgeqp3_dorgqr_and_dormqr)
{
  // A = [[1, 2], [3, 4]] is factorized as A * P = Q * R; dorgqr then expands the reflectors into Q and dormqr
  // applies the very same reflectors to a right hand side without forming Q.
  const std::vector<double> original{1., 2., 3., 4.};
  auto a = original;
  std::vector<int> jpvt(2, 0); // all-zero: dgeqp3 is free to pivot as it sees fit
  std::vector<double> tau(2, 0.);
  if (!Lapacke::available()) {
    EXPECT_THROW(Lapacke::dgeqp3(0, 2, 2, a.data(), 2, jpvt.data(), tau.data()), Exceptions::dependency_missing);
    EXPECT_THROW(Lapacke::dorgqr(0, 2, 2, 2, a.data(), 2, tau.data()), Exceptions::dependency_missing);
    std::vector<double> c{1., 1.};
    EXPECT_THROW(Lapacke::dormqr(0, 'L', 'N', 2, 1, 2, a.data(), 2, tau.data(), c.data(), 1),
                 Exceptions::dependency_missing);
    return;
  }
  const auto layout = Lapacke::row_major();
  expect_lapack_success(Lapacke::dgeqp3(layout, 2, 2, a.data(), 2, jpvt.data(), tau.data()), "dgeqp3");
  // dgeqp3 returns one-based column indices, each column exactly once.
  std::vector<int> sorted_jpvt = jpvt;
  std::sort(sorted_jpvt.begin(), sorted_jpvt.end());
  EXPECT_EQ(1, sorted_jpvt[0]);
  EXPECT_EQ(2, sorted_jpvt[1]);

  // Apply the reflectors to the identity's columns one at a time; the result has to have unit length, since Q is
  // orthogonal. This uses the factorization in place, before dorgqr overwrites it below.
  for (const auto& e : {std::vector<double>{1., 0.}, std::vector<double>{0., 1.}}) {
    auto c = e;
    expect_lapack_success(Lapacke::dormqr(layout, 'L', 'N', 2, 1, 2, a.data(), 2, tau.data(), c.data(), 1), "dormqr");
    EXPECT_TRUE(FloatCmp::eq(c[0] * c[0] + c[1] * c[1], 1.)) << "|Q e|^2 = " << c[0] * c[0] + c[1] * c[1];
  }

  auto q = a;
  expect_lapack_success(Lapacke::dorgqr(layout, 2, 2, 2, q.data(), 2, tau.data()), "dorgqr");
  // Q^T Q = I.
  EXPECT_TRUE(FloatCmp::eq(q[0] * q[0] + q[2] * q[2], 1.));
  EXPECT_TRUE(FloatCmp::eq(q[1] * q[1] + q[3] * q[3], 1.));
  EXPECT_TRUE(FloatCmp::eq(q[0] * q[1] + q[2] * q[3], 0.));
}


GTEST_TEST(lapacke, dgesvd)
{
  auto a = spd_matrix();
  std::vector<double> s(2, 0.);
  std::vector<double> u(4, 0.);
  std::vector<double> vt(4, 0.);
  std::vector<double> superb(1, 0.);
  if (!Lapacke::available()) {
    EXPECT_THROW(Lapacke::dgesvd(0, 'A', 'A', 2, 2, a.data(), 2, s.data(), u.data(), 2, vt.data(), 2, superb.data()),
                 Exceptions::dependency_missing);
    return;
  }
  expect_lapack_success(
      Lapacke::dgesvd(
          Lapacke::row_major(), 'A', 'A', 2, 2, a.data(), 2, s.data(), u.data(), 2, vt.data(), 2, superb.data()),
      "dgesvd");
  // A is symmetric positive definite, so its singular values coincide with its eigenvalues, in descending order.
  EXPECT_TRUE(FloatCmp::eq(s[0], 3.)) << "s[0] = " << s[0];
  EXPECT_TRUE(FloatCmp::eq(s[1], 1.)) << "s[1] = " << s[1];
}


GTEST_TEST(lapacke, dlamch)
{
  if (!Lapacke::available()) {
    EXPECT_THROW(Lapacke::dlamch('E'), Exceptions::dependency_missing);
    return;
  }
  const auto eps = Lapacke::dlamch('E');
  EXPECT_GT(eps, 0.);
  EXPECT_LT(eps, 1e-10) << "the relative machine precision should be tiny, but is " << eps;
  const auto safe_min = Lapacke::dlamch('S');
  EXPECT_GT(safe_min, 0.);
  EXPECT_LT(safe_min, 1.);
}


GTEST_TEST(lapacke, dpotrf_and_dpocon)
{
  auto a = spd_matrix();
  double rcond = -1.;
  if (!Lapacke::available()) {
    EXPECT_THROW(Lapacke::dpotrf(0, 'L', 2, a.data(), 2), Exceptions::dependency_missing);
    EXPECT_THROW(Lapacke::dpocon(0, 'L', 2, a.data(), 2, 3., &rcond), Exceptions::dependency_missing);
    return;
  }
  const auto layout = Lapacke::row_major();
  expect_lapack_success(Lapacke::dpotrf(layout, 'L', 2, a.data(), 2), "dpotrf");
  // A = L L^T with L = [[sqrt(2), 0], [1/sqrt(2), sqrt(3/2)]].
  EXPECT_TRUE(FloatCmp::eq(a[0], std::sqrt(2.))) << "a[0] = " << a[0];
  EXPECT_TRUE(FloatCmp::eq(a[2], 1. / std::sqrt(2.))) << "a[2] = " << a[2];
  EXPECT_TRUE(FloatCmp::eq(a[3], std::sqrt(1.5))) << "a[3] = " << a[3];

  // The one-norm of A is 3, so dpocon has to report a reciprocal condition number in (0, 1].
  expect_lapack_success(Lapacke::dpocon(layout, 'L', 2, a.data(), 2, 3., &rcond), "dpocon");
  EXPECT_GT(rcond, 0.) << "rcond = " << rcond;
  EXPECT_LE(rcond, 1.) << "rcond = " << rcond;

  // A non positive definite matrix makes dpotrf report the offending leading minor instead of throwing.
  std::vector<double> indefinite{1., 2., 2., 1.};
  EXPECT_GT(Lapacke::dpotrf(layout, 'L', 2, indefinite.data(), 2), 0);
}


GTEST_TEST(lapacke, dptcon)
{
  // The symmetric positive definite tridiagonal system given by A = [[2, 1], [1, 2]]: dpttrf computes its L D L^T
  // factorization, which dptcon then uses to estimate the condition number.
  std::vector<double> d{2., 2.};
  std::vector<double> e{1.};
  double rcond = -1.;
  if (!Lapacke::available()) {
    EXPECT_THROW(Lapacke::dptcon(2, d.data(), e.data(), 3., &rcond), Exceptions::dependency_missing);
    return;
  }
  expect_lapack_success(Lapacke::dpttrf(2, d.data(), e.data()), "dpttrf");
  expect_lapack_success(Lapacke::dptcon(2, d.data(), e.data(), 3., &rcond), "dptcon");
  EXPECT_GT(rcond, 0.) << "rcond = " << rcond;
  EXPECT_LE(rcond, 1.) << "rcond = " << rcond;
}


GTEST_TEST(lapacke, dtrcon)
{
  // dtrcon only *estimates* the reciprocal condition number, so the assertions below stay qualitative: a well
  // conditioned triangular matrix has to come out clearly better than a nearly singular one.
  const std::vector<double> well_conditioned{2., 0., 1., 2.};
  const std::vector<double> nearly_singular{1., 0., 1., 1e-12};
  double rcond = -1.;
  if (!Lapacke::available()) {
    EXPECT_THROW(Lapacke::dtrcon(0, '1', 'L', 'N', 2, well_conditioned.data(), 2, &rcond),
                 Exceptions::dependency_missing);
    return;
  }
  const auto layout = Lapacke::row_major();
  expect_lapack_success(Lapacke::dtrcon(layout, '1', 'L', 'N', 2, well_conditioned.data(), 2, &rcond), "dtrcon");
  EXPECT_GT(rcond, 0.1) << "rcond = " << rcond;
  EXPECT_LE(rcond, 1.) << "rcond = " << rcond;

  double bad_rcond = -1.;
  expect_lapack_success(Lapacke::dtrcon(layout, '1', 'L', 'N', 2, nearly_singular.data(), 2, &bad_rcond), "dtrcon");
  EXPECT_GE(bad_rcond, 0.);
  EXPECT_LT(bad_rcond, rcond) << "bad_rcond = " << bad_rcond << ", rcond = " << rcond;
}
