// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2019)

#ifndef DUNE_GDT_TEST_STATIONARY_HEAT_EQUATION_ESV2007_HH
#define DUNE_GDT_TEST_STATIONARY_HEAT_EQUATION_ESV2007_HH

#include <dune/xt/common/type_traits.hh>
#include <dune/xt/la/container/eye-matrix.hh>
#include <dune/xt/grid/boundaryinfo/alldirichlet.hh>
#include <dune/xt/grid/gridprovider/cube.hh>
#include <dune/xt/grid/type_traits.hh>
#include <dune/xt/grid/grids.hh>
#include <dune/xt/functions/ESV2007.hh>
#include <dune/xt/functions/constant.hh>
#include <dune/xt/functions/grid-function.hh>

#include <dune/gdt/data/stationary_heat_equation.hh>
#include <dune/gdt/interpolations/default.hh>
#include <dune/gdt/test/stationary-eocstudies/diffusion-ipdg.hh>

namespace Dune {
namespace GDT {
namespace Test {


// The gtest-free problem definition now lives in dune/gdt/data/stationary_heat_equation.hh
// and is shared with the benchmarks; keep the historical name available in this namespace.
using Data::ESV2007DiffusionProblem;


/**
 * \brief To reproduce the results in [1], Section 8.1
 *
 * [1]: Ern, Stephansen, Vohralik, 2007, Improved energy norm a posteriori error estimation based on flux reconstruction
 *      for discontinuous Galerkin methods, Preprint R07050, Laboratoire Jacques-Louis Lions & HAL Preprint
 */
template <class G>
class ESV2007DiffusionTest : public StationaryDiffusionIpdgEocStudy<G>
{
  using BaseType = StationaryDiffusionIpdgEocStudy<G>;

  using BaseType::d;
  using typename BaseType::E;
  using typename BaseType::FF;
  using typename BaseType::FT;
  using typename BaseType::GP;
  using typename BaseType::GV;
  using typename BaseType::I;
  using typename BaseType::V;

public:
  ESV2007DiffusionTest()
    : BaseType(
          /*symmetry_prefactor=*/1,
          /*inner_penalty=*/8,
          /*dirichlet_penalty=*/14,
          /*intersection_diameter=*/[](const auto& intersection) { return intersection.geometry().volume(); })
    , problem(DXTC_TEST_CONFIG_GET("setup.force_order", 2))
  {
  }

protected:
  std::vector<std::string> norms() const override final
  {
    return {"H_1_semi", "eta_NC", "eta_R", "eta_DF"};
  }

  void compute_reference_solution() override final
  {
    auto& self = *this;
    if (self.reference_solution_on_reference_grid_)
      return;
    self.reference_grid_ = std::make_unique<GP>(make_initial_grid());
    for (size_t ref = 0; ref < self.num_refinements_ + self.num_additional_refinements_for_reference_; ++ref)
      self.reference_grid_->global_refine(DGFGridInfo<G>::refineStepsForHalf());
    auto backup_space_type = self.space_type_;
    self.space_type_ = "dg_p" + DXTC_TEST_CONFIG_GET("setup.reference_solution_order", "3");
    self.reference_space_ = self.make_space(*self.reference_grid_);
    self.space_type_ = backup_space_type;
    self.reference_solution_on_reference_grid_ = std::make_unique<V>(
        default_interpolation<V>(XT::Functions::ESV2007::Testcase1ExactSolution<d, 1>(), *self.reference_space_)
            .dofs()
            .vector());
    // visualize
    self.visualize_(
        make_discrete_function(*self.reference_space_, *self.reference_solution_on_reference_grid_),
        "reference_solution_on_refinement_"
            + XT::Common::to_string(self.num_refinements_ + self.num_additional_refinements_for_reference_));
  } // ... compute_reference_solution(...)

  const XT::Grid::BoundaryInfo<I>& boundary_info() const override final
  {
    return problem.boundary_info;
  }

  const FT& diffusion() const override final
  {
    return problem.diffusion;
  }

  const FF& force() const override final
  {
    return problem.force;
  }

  const FT& weight_function() const override final
  {
    return this->diffusion();
  }

  GP make_initial_grid() override final
  {
    return problem.make_initial_grid();
  }

  ESV2007DiffusionProblem<GV> problem;
}; // struct ESV2007DiffusionTest


} // namespace Test
} // namespace GDT
} // namespace Dune

#endif // DUNE_GDT_TEST_STATIONARY_HEAT_EQUATION_ESV2007_HH
