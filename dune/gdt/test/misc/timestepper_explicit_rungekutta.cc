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
#include <dune/gdt/tools/timestepper/explicit-rungekutta.hh>

using namespace Dune;
using namespace Dune::GDT;

using G = YASP_1D_EQUIDISTANT_OFFSET;
using GV = typename G::LeafGridView;
using OperatorType = IdentityOperator<GV>;
using V = typename OperatorType::VectorType;
using DF = DiscreteFunction<V, GV>;


/// Solves u' = -u, u(0) = 1 (via the identity operator L(u) = u and the scalar factor r = -1) up to T = 1 and
/// compares against the exact solution exp(-1).
struct ExplicitRungeKuttaTimeStepperTest : public ::testing::Test
{
  void SetUp() override
  {
    grid_provider_ = std::make_unique<XT::Grid::GridProvider<G>>(XT::Grid::make_cube_grid<G>(0., 1., 4u));
    space_ = std::make_unique<FiniteVolumeSpace<GV>>(grid_provider_->leaf_view());
    op_ = std::make_unique<OperatorType>(*space_);
  }

  template <TimeStepperMethods method>
  double solve_exponential_decay(const double dt)
  {
    DF initial_values(*space_);
    for (size_t ii = 0; ii < space_->mapper().size(); ++ii)
      initial_values.dofs().vector()[ii] = 1.; // u(0) = 1
    ExplicitRungeKuttaTimeStepper<OperatorType, DF, method> stepper(*op_, initial_values, /*r=*/-1.);
    stepper.solve(/*t_end=*/1., dt, /*num_save_steps=*/size_t(-1), /*num_output_steps=*/0);
    return stepper.current_solution().dofs().vector()[0];
  }

  std::unique_ptr<XT::Grid::GridProvider<G>> grid_provider_;
  std::unique_ptr<FiniteVolumeSpace<GV>> space_;
  std::unique_ptr<OperatorType> op_;
}; // struct ExplicitRungeKuttaTimeStepperTest


TEST_F(ExplicitRungeKuttaTimeStepperTest, methods_converge_with_their_order)
{
  const double exact = std::exp(-1.);
  EXPECT_NEAR(solve_exponential_decay<TimeStepperMethods::explicit_euler>(1e-3), exact, 2e-4);
  EXPECT_NEAR(solve_exponential_decay<TimeStepperMethods::explicit_rungekutta_second_order_ssp>(1e-2), exact, 1e-5);
  EXPECT_NEAR(solve_exponential_decay<TimeStepperMethods::explicit_rungekutta_third_order_ssp>(1e-2), exact, 1e-7);
  EXPECT_NEAR(solve_exponential_decay<TimeStepperMethods::explicit_rungekutta_classic_fourth_order>(1e-2), exact, 1e-9);
}

/// find_suitable_dt used to call step() with a single argument (step takes the time step length and the maximal
/// admissible time step length), so this did not even compile when instantiated.
TEST_F(ExplicitRungeKuttaTimeStepperTest, find_suitable_dt_compiles_and_accepts_stable_dt)
{
  DF initial_values(*space_);
  for (size_t ii = 0; ii < space_->mapper().size(); ++ii)
    initial_values.dofs().vector()[ii] = 1.;
  ExplicitRungeKuttaTimeStepper<OperatorType, DF, TimeStepperMethods::explicit_euler> stepper(
      *op_, initial_values, /*r=*/-1.);
  const auto result = stepper.find_suitable_dt(/*initial_dt=*/0.1);
  EXPECT_TRUE(result.first);
  EXPECT_DOUBLE_EQ(result.second, 0.1);
}
