// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)

#include <dune/xt/test/main.hxx> // <- this one has to come first (includes the config.h)!

#include <dune/xt/common/float_cmp.hh>
#include <dune/xt/functions/grid-function.hh>
#include <dune/xt/grid/grids.hh>
#include <dune/xt/grid/gridprovider/cube.hh>
#include <dune/xt/grid/type_traits.hh>

#include <dune/gdt/interpolations/default.hh>
#include <dune/gdt/operators/laplace-ipdg-flux-reconstruction.hh>
#include <dune/gdt/spaces/hdiv/raviart-thomas.hh>
#include <dune/gdt/spaces/l2/discontinuous-lagrange.hh>

using namespace Dune;
using namespace Dune::GDT;

using G = YASP_2D_EQUIDISTANT_OFFSET;
using GV = typename G::LeafGridView;
using E = XT::Grid::extract_entity_t<GV>;
using V = XT::LA::IstlDenseVector<double>;


/// For u_h(x) = x_0 (globally continuous, all jumps vanish) and K = I the reconstructed diffusive flux on inner
/// faces has to be t_h . n = -{K grad(u_h)} . n = -n_0, independently of the symmetry prefactor theta of the IPDG
/// variant: in the unified flux framework [Arnold, Brezzi, Cockburn, Marini, 2002] the numerical flux is
/// sigma_hat = {K grad(u_h)}_omega - penalty/h [u_h] n for SIPDG (theta=1), IIPDG (theta=0) and NIPDG (theta=-1)
/// alike; theta only enters the symmetry term {K grad(v)}[u]. The reconstruction used to multiply the consistency
/// part by theta, yielding a zero flux for theta=0 and a sign-flipped flux for theta=-1.
struct LaplaceIpdgFluxReconstructionThetaTest : public ::testing::Test
{
  void check_reconstructed_face_fluxes(const double symmetry_prefactor)
  {
    auto grid_provider = XT::Grid::make_cube_grid<G>(0., 1., 2u);
    auto grid_view = grid_provider.leaf_view();
    const auto dg_space = make_discontinuous_lagrange_space(grid_view, /*order=*/1);
    const auto rt_space = make_raviart_thomas_space(grid_view, /*order=*/0);
    const auto u_h = default_interpolation<V>(1, [](const auto& xx, const auto& /*mu*/) { return xx[0]; }, dg_space);
    LaplaceIpdgFluxReconstructionOperator<GV> reconstruction(grid_view,
                                                             rt_space,
                                                             symmetry_prefactor,
                                                             /*inner_penalty=*/8.,
                                                             /*dirichlet_penalty=*/14.,
                                                             /*diffusion=*/{1.});
    const auto t_h = reconstruction.apply(u_h);
    auto local_t_h = t_h.local_function();
    size_t num_checked_inner_faces = 0;
    size_t num_checked_boundary_faces = 0;
    for (auto&& element : elements(grid_view)) {
      local_t_h->bind(element);
      for (auto&& intersection : intersections(grid_view, element)) {
        // for RT_0 on axis-aligned cubes, t_h . n at the face center equals the face DoF
        const auto face_center = intersection.geometry().center();
        const auto value = local_t_h->evaluate(element.geometry().local(face_center));
        const auto normal = intersection.centerUnitOuterNormal();
        if (intersection.neighbor()) {
          // inner faces: their DoFs are determined by the inner-face equations only (no boundary penalty here)
          EXPECT_NEAR(value * normal, -normal[0], 1e-12)
              << "face center = " << face_center << ", symmetry_prefactor = " << symmetry_prefactor;
          ++num_checked_inner_faces;
        } else if (XT::Common::FloatCmp::eq(face_center[0], 0.)) {
          // Dirichlet faces on x_0 = 0, where u_h vanishes: the boundary penalty term penalty/h * u_h drops out and
          // the (theta-independent) consistency term alone determines the face DoF, t_h . n = -grad(u_h) . n = -n_0
          EXPECT_NEAR(value * normal, -normal[0], 1e-12)
              << "face center = " << face_center << ", symmetry_prefactor = " << symmetry_prefactor;
          ++num_checked_boundary_faces;
        }
      }
    }
    EXPECT_EQ(num_checked_inner_faces, 8); // 4 inner faces on the 2x2 grid, each visited from both sides
    EXPECT_EQ(num_checked_boundary_faces, 2); // 2 boundary faces on x_0 = 0
  } // ... check_reconstructed_face_fluxes(...)
}; // struct LaplaceIpdgFluxReconstructionThetaTest


TEST_F(LaplaceIpdgFluxReconstructionThetaTest, sipdg)
{
  check_reconstructed_face_fluxes(1.);
}

TEST_F(LaplaceIpdgFluxReconstructionThetaTest, iipdg)
{
  check_reconstructed_face_fluxes(0.); // <- the reconstructed flux used to vanish entirely
}

TEST_F(LaplaceIpdgFluxReconstructionThetaTest, nipdg)
{
  check_reconstructed_face_fluxes(-1.); // <- the reconstructed flux used to point in the wrong direction
}
