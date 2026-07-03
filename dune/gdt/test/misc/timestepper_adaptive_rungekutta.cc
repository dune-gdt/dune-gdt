// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)

#include <dune/xt/test/main.hxx> // <- this one has to come first (includes the config.h)!

#include <cmath>

#include <dune/xt/grid/grids.hh>
#include <dune/xt/grid/gridprovider/cube.hh>

#include <dune/gdt/discretefunction/default.hh>
#include <dune/gdt/operators/identity.hh>
#include <dune/gdt/spaces/l2/finite-volume.hh>
#include <dune/gdt/tools/timestepper/adaptive-rungekutta.hh>

using namespace Dune;
using namespace Dune::GDT;

using G = YASP_1D_EQUIDISTANT_OFFSET;
using GV = typename G::LeafGridView;
using OperatorType = IdentityOperator<GV>;
using V = typename OperatorType::VectorType;
using DF = DiscreteFunction<V, GV>;


/// Solves u' = -u, u(0) = 1 (via the identity operator L(u) = u and the scalar factor r = -1) up to T = 1 and
/// compares against the exact solution exp(-1).
struct AdaptiveRungeKuttaTimeStepperTest : public ::testing::Test
{
  void SetUp() override
  {
    grid_provider_ = std::make_unique<XT::Grid::GridProvider<G>>(XT::Grid::make_cube_grid<G>(0., 1., 4u));
    space_ = std::make_unique<FiniteVolumeSpace<GV>>(grid_provider_->leaf_view());
    op_ = std::make_unique<OperatorType>(*space_);
  }

  double solve_exponential_decay(const double initial_dt, const double tol)
  {
    DF initial_values(*space_);
    for (size_t ii = 0; ii < space_->mapper().size(); ++ii)
      initial_values.dofs().vector()[ii] = 1.; // u(0) = 1
    AdaptiveRungeKuttaTimeStepper<OperatorType, DF> stepper(*op_, initial_values, /*r=*/-1., /*t_0=*/0., tol);
    stepper.solve(/*t_end=*/1., initial_dt, /*num_save_steps=*/size_t(-1), /*num_output_steps=*/0);
    return stepper.current_solution().dofs().vector()[0];
  }

  std::unique_ptr<XT::Grid::GridProvider<G>> grid_provider_;
  std::unique_ptr<FiniteVolumeSpace<GV>> space_;
  std::unique_ptr<OperatorType> op_;
}; // struct AdaptiveRungeKuttaTimeStepperTest


TEST_F(AdaptiveRungeKuttaTimeStepperTest, solves_exponential_decay)
{
  const double exact = std::exp(-1.);
  EXPECT_NEAR(solve_exponential_decay(/*initial_dt=*/1e-2, /*tol=*/1e-6), exact, 1e-4);
}

/// A too large initial dt forces the error estimator to reject and repeat steps, exercising the rollback of the
/// current solution to the previous time step.
TEST_F(AdaptiveRungeKuttaTimeStepperTest, recovers_from_rejected_steps)
{
  const double exact = std::exp(-1.);
  EXPECT_NEAR(solve_exponential_decay(/*initial_dt=*/10., /*tol=*/1e-6), exact, 1e-4);
}
