// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)

#include <dune/xt/test/main.hxx> // <- this one has to come first (includes the config.h)!

#include <dune/common/fmatrix.hh>
#include <dune/common/fvector.hh>

#include <dune/gdt/operators/reconstruction/slopes.hh>

using namespace Dune;
using namespace Dune::GDT;


/// Identity transformation, allows to test the limiters on plain (non-characteristic) slopes.
template <size_t r>
struct IdentityEigenVectorWrapper
{
  using VectorType = FieldVector<double, r>;
  using MatrixType = FieldMatrix<double, r, r>;

  void apply_eigenvectors(const size_t /*dd*/, const VectorType& u, VectorType& ret) const
  {
    ret = u;
  }

  void apply_inverse_eigenvectors(const size_t /*dd*/, const VectorType& u, VectorType& ret) const
  {
    ret = u;
  }
};

using E = int; // <- only passed through, no entity required for these slopes
using Wrapper1 = IdentityEigenVectorWrapper<1>;
using Wrapper3 = IdentityEigenVectorWrapper<3>;
using V1 = typename Wrapper1::VectorType;
using V3 = typename Wrapper3::VectorType;

template <class SlopeType, int r>
FieldVector<double, r> limited_slope(const FieldVector<double, r>& u_left,
                                     const FieldVector<double, r>& u_entity,
                                     const FieldVector<double, r>& u_right)
{
  const SlopeType slope;
  const IdentityEigenVectorWrapper<r> eigenvectors;
  const FieldVector<FieldVector<double, r>, 3> stencil{u_left, u_entity, u_right};
  return slope.get(E(0), stencil, eigenvectors, 0);
}


GTEST_TEST(MinmodSlopeTest, limits_correctly)
{
  using SlopeType = MinmodSlope<E, Wrapper1>;
  EXPECT_DOUBLE_EQ(limited_slope<SlopeType>(V1(0.), V1(1.), V1(1.5))[0], 0.5);
  EXPECT_DOUBLE_EQ(limited_slope<SlopeType>(V1(1.5), V1(1.), V1(0.))[0], -0.5);
  EXPECT_DOUBLE_EQ(limited_slope<SlopeType>(V1(0.), V1(1.), V1(0.))[0], 0.); // extremum
  EXPECT_DOUBLE_EQ(limited_slope<SlopeType>(V1(1.), V1(1.), V1(5.))[0], 0.);
}

GTEST_TEST(McSlopeTest, limits_correctly)
{
  using SlopeType = McSlope<E, Wrapper1>;
  // minmod(2 (u - u_l), 2 (u_r - u), (u_r - u_l)/2)
  EXPECT_DOUBLE_EQ(limited_slope<SlopeType>(V1(0.), V1(1.), V1(4.))[0], 2.);
  EXPECT_DOUBLE_EQ(limited_slope<SlopeType>(V1(0.), V1(1.), V1(1.25))[0], 0.5);
  EXPECT_DOUBLE_EQ(limited_slope<SlopeType>(V1(0.), V1(1.), V1(0.))[0], 0.); // extremum
}

GTEST_TEST(SuperbeeSlopeTest, scalar_superbee_limits_correctly)
{
  // superbee(a, b) = maxmod(minmod(2a, b), minmod(a, 2b)); used to die from infinite recursion for r = 1 (and did
  // not compile for r > 1)
  using SlopeType = SuperbeeSlope<E, Wrapper1>;
  EXPECT_DOUBLE_EQ(SlopeType::superbee_scalar(1., 0.5), 1.);
  EXPECT_DOUBLE_EQ(SlopeType::superbee_scalar(0.5, 1.), 1.);
  EXPECT_DOUBLE_EQ(SlopeType::superbee_scalar(-1., -0.25), -0.5);
  EXPECT_DOUBLE_EQ(SlopeType::superbee_scalar(1., -1.), 0.);
  EXPECT_DOUBLE_EQ(SlopeType::superbee_scalar(0., 1.), 0.);
  EXPECT_DOUBLE_EQ(SlopeType::superbee_scalar(1., 1.), 1.);
  EXPECT_DOUBLE_EQ(SlopeType::superbee_scalar(1., 4.), 2.);
}

GTEST_TEST(SuperbeeSlopeTest, limits_correctly)
{
  using SlopeType = SuperbeeSlope<E, Wrapper1>;
  EXPECT_DOUBLE_EQ(limited_slope<SlopeType>(V1(0.), V1(1.), V1(1.5))[0], 1.);
  EXPECT_DOUBLE_EQ(limited_slope<SlopeType>(V1(1.5), V1(1.), V1(0.))[0], -1.);
  EXPECT_DOUBLE_EQ(limited_slope<SlopeType>(V1(0.), V1(1.), V1(0.))[0], 0.); // extremum
}

GTEST_TEST(SuperbeeSlopeTest, works_for_systems)
{
  // r > 1 used to fail to compile (no scalar superbee overload existed)
  using SlopeType = SuperbeeSlope<E, Wrapper3>;
  const auto slope = limited_slope<SlopeType, 3>({0., 1.5, 0.}, {1., 1., 1.}, {1.5, 0., 1.});
  EXPECT_DOUBLE_EQ(slope[0], 1.);
  EXPECT_DOUBLE_EQ(slope[1], -1.);
  EXPECT_DOUBLE_EQ(slope[2], 0.);
}
