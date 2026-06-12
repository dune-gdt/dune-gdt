// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)

#include <dune/xt/test/main.hxx> // <- this one has to come first (includes the config.h)!

#include <dune/xt/grid/grids.hh>
#include <dune/xt/grid/gridprovider/cube.hh>
#include <dune/xt/la/container/common.hh>

#include <dune/gdt/discretefunction/default.hh>
#include <dune/gdt/spaces/l2/finite-volume.hh>
#include <dune/gdt/tools/timestepper/interface.hh>

using namespace Dune;
using namespace Dune::GDT;

using G = YASP_1D_EQUIDISTANT_OFFSET;
using GV = typename G::LeafGridView;
using V = XT::LA::CommonDenseVector<double>;
using DF = DiscreteFunction<V, GV>;


struct DummyTimeStepper : public TimeStepperInterface<DF>
{
  DummyTimeStepper(DF& initial_values)
    : TimeStepperInterface<DF>(0., initial_values)
  {
  }

  double step(const double dt, const double /*max_dt*/) override final
  {
    current_time() += dt;
    return dt;
  }
};


/// solution_closest_to_time used to dereference the map's end() iterator for any query time at or beyond the last
/// stored time point (e.g. the natural query t = t_end) and could never return the closest *earlier* time point
/// (it compared lower_bound and upper_bound, which coincide for any t between two stored time points).
GTEST_TEST(TimeStepperInterface, solution_closest_to_time_returns_the_closest_time_point)
{
  auto grid_provider = XT::Grid::make_cube_grid<G>(0., 1., 4u);
  auto grid_view = grid_provider.leaf_view();
  const FiniteVolumeSpace<GV> space(grid_view);
  DF initial_values(space);
  DummyTimeStepper stepper(initial_values);

  EXPECT_THROW(stepper.solution_closest_to_time(0.), Dune::InvalidStateException);

  for (const double t : {0., 1., 2.}) {
    DF u(space);
    u.dofs().vector()[0] = t;
    stepper.solution().emplace_hint(stepper.solution().end(), t, u.copy_as_discrete_function());
  }

  EXPECT_DOUBLE_EQ(stepper.solution_closest_to_time(-1.).first, 0.);
  EXPECT_DOUBLE_EQ(stepper.solution_closest_to_time(0.).first, 0.);
  EXPECT_DOUBLE_EQ(stepper.solution_closest_to_time(0.4).first, 0.); // used to wrongly return 1.
  EXPECT_DOUBLE_EQ(stepper.solution_closest_to_time(0.6).first, 1.);
  EXPECT_DOUBLE_EQ(stepper.solution_closest_to_time(1.).first, 1.);
  EXPECT_DOUBLE_EQ(stepper.solution_closest_to_time(1.9).first, 2.);
  EXPECT_DOUBLE_EQ(stepper.solution_closest_to_time(2.).first, 2.); // used to dereference end()
  EXPECT_DOUBLE_EQ(stepper.solution_closest_to_time(5.).first, 2.); // used to dereference end()
  // the returned discrete function belongs to the returned time point
  EXPECT_DOUBLE_EQ(stepper.solution_closest_to_time(1.9).second->dofs().vector()[0], 2.);
}
