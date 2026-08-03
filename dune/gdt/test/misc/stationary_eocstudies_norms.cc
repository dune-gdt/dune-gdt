// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   dune-gdt developers

#ifndef DUNE_XT_COMMON_TEST_MAIN_CATCH_EXCEPTIONS
#  define DUNE_XT_COMMON_TEST_MAIN_CATCH_EXCEPTIONS 1
#endif

#include <dune/xt/test/main.hxx> // <- this one has to come first (includes the config.h)!

#include <memory>

#include <dune/xt/common/exceptions.hh>
#include <dune/xt/grid/grids.hh>
#include <dune/xt/grid/gridprovider/cube.hh>

#include <dune/gdt/operators/identity.hh>
#include <dune/gdt/spaces/l2/finite-volume.hh>
#include <dune/gdt/test/stationary-eocstudies/base.hh>

using namespace Dune;
using namespace Dune::GDT;
using namespace Dune::GDT::Test;

namespace {


using G = YASP_2D_EQUIDISTANT_OFFSET;
using GV = typename G::LeafGridView;


/**
 * \brief Smallest possible concrete StationaryEocStudy, used to exercise the norm dispatch in
 *        StationaryEocStudy::compute() (base.hh) directly, independent of any concrete discretization.
 *
 * The residual operator is the identity, so solve() always yields the zero vector: both the current and the
 * reference solution are the zero function, which makes every norm of their difference trivially zero and thus
 * avoids the need for any precomputed/golden EOC values.
 */
class NormsStudy : public StationaryEocStudy<GV>
{
  using BaseType = StationaryEocStudy<GV>;

protected:
  using typename BaseType::GP;
  using typename BaseType::O;
  using typename BaseType::S;

public:
  NormsStudy()
    : BaseType(/*visualizer=*/[](const auto&, const auto&) {},
               /*num_refinements=*/0,
               /*num_additional_refinements_for_reference=*/0)
  {
  }

protected:
  GP make_initial_grid() override
  {
    return XT::Grid::make_cube_grid<G>(0., 1., 4u);
  }

  std::unique_ptr<S> make_space(const GP& current_grid) override
  {
    return std::make_unique<FiniteVolumeSpace<GV>>(current_grid.leaf_view());
  }

  std::unique_ptr<O> make_residual_operator(const S& space) override
  {
    return std::make_unique<IdentityOperator<GV>>(space);
  }
}; // class NormsStudy


} // namespace


TEST_F(NormsStudy, computes_the_known_norms)
{
  const auto data = this->compute(0, {"L_1", "L_2", "H_1_semi"}, {}, {});
  EXPECT_DOUBLE_EQ(0., data.at("norm").at("L_1"));
  EXPECT_DOUBLE_EQ(0., data.at("norm").at("L_2"));
  EXPECT_DOUBLE_EQ(0., data.at("norm").at("H_1_semi"));
}

TEST_F(NormsStudy, throws_for_an_unknown_norm)
{
  EXPECT_THROW(this->compute(0, {"not_a_known_norm"}, {}, {}), XT::Common::Exceptions::wrong_input_given);
}
