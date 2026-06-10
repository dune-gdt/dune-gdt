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
/// compares against the exact solution exp(-1). Aside from the accuracy check, this instantiates
/// AdaptiveRungeKuttaTimeStepper at all, which used to fail to compile (its solve() override did not match any
/// virtual base method and it stored DiscreteFunctions by value, whose copy constructor is deleted).
struct AdaptiveRungeKuttaTimeStepperTest : public ::testing::Test
{
  void SetUp() override
  {
    grid_provider_ = std::make_unique<XT::Grid::GridProvider<G>>(XT::Grid::make_cube_grid<G>(0., 1., 4u));
    space_ = std::make_unique<FiniteVolumeSpace<GV>>(grid_provider_->leaf_view());
    op_ = std::make_unique<OperatorType>(*space_);
  }

  template <TimeStepperMethods method, class... Args>
  void check_solves_exponential_decay(Args&&... args)
  {
    DF initial_values(*space_);
    for (size_t ii = 0; ii < space_->mapper().size(); ++ii)
      initial_values.dofs().vector()[ii] = 1.; // u(0) = 1
    AdaptiveRungeKuttaTimeStepper<OperatorType, DF, method> stepper(
        *op_, initial_values, /*r=*/-1., /*t_0=*/0., /*tol=*/1e-8, 0.2, 5, std::forward<Args>(args)...);
    stepper.solve(/*t_end=*/1., /*initial_dt=*/0.1, /*num_save_steps=*/size_t(-1), /*num_output_steps=*/0);
    EXPECT_NEAR(stepper.current_time(), 1., 1e-10);
    for (size_t ii = 0; ii < space_->mapper().size(); ++ii)
      EXPECT_NEAR(stepper.current_solution().dofs().vector()[ii], std::exp(-1.), 1e-5);
  }

  std::unique_ptr<XT::Grid::GridProvider<G>> grid_provider_;
  std::unique_ptr<FiniteVolumeSpace<GV>> space_;
  std::unique_ptr<OperatorType> op_;
}; // struct AdaptiveRungeKuttaTimeStepperTest


TEST_F(AdaptiveRungeKuttaTimeStepperTest, dormand_prince)
{
  check_solves_exponential_decay<TimeStepperMethods::dormand_prince>();
}

TEST_F(AdaptiveRungeKuttaTimeStepperTest, bogacki_shampine)
{
  check_solves_exponential_decay<TimeStepperMethods::bogacki_shampine>();
}

TEST_F(AdaptiveRungeKuttaTimeStepperTest, non_fsal_user_provided_tableau)
{
  // Fehlberg 4(5): c_last = 1/2 != 1 and the last row of A differs from b_1, so the first-same-as-last reuse of the
  // previous step's last stage is invalid for this method and must not be applied by the time stepper
  using M = Dune::DynamicMatrix<double>;
  using X = Dune::DynamicVector<double>;
  const M A{{0., 0., 0., 0., 0., 0.},
            {1. / 4., 0., 0., 0., 0., 0.},
            {3. / 32., 9. / 32., 0., 0., 0., 0.},
            {1932. / 2197., -7200. / 2197., 7296. / 2197., 0., 0., 0.},
            {439. / 216., -8., 3680. / 513., -845. / 4104., 0., 0.},
            {-8. / 27., 2., -3544. / 2565., 1859. / 4104., -11. / 40., 0.}};
  const X b_1{16. / 135., 0., 6656. / 12825., 28561. / 56430., -9. / 50., 2. / 55.};
  const X b_2{25. / 216., 0., 1408. / 2565., 2197. / 4104., -1. / 5., 0.};
  const X c{0., 1. / 4., 3. / 8., 12. / 13., 1., 1. / 2.};
  check_solves_exponential_decay<TimeStepperMethods::adaptive_rungekutta_other>(A, b_1, b_2, c);
}
