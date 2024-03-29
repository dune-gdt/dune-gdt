// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2017, 2019)
//   René Fritze     (2016, 2018 - 2019)
//   Tobias Leibner  (2018, 2020)

#include <dune/xt/test/main.hxx>
#include <dune/xt/common/logging.hh>

#include <dune/xt/grid/gridprovider.hh>
#include <dune/xt/grid/grids.hh>
#include <dune/xt/grid/information.hh>
#include <dune/xt/grid/search.hh>
#include <dune/xt/grid/view/periodic.hh>

struct InLevelSearch : public testing::Test
{
  using GridType = TESTGRIDTYPE;
  using GridProviderType = Dune::XT::Grid::GridProvider<GridType>;

  InLevelSearch()
    : grid_provider_(Dune::XT::Grid::make_cube_grid<TESTGRIDTYPE>())
  {
  }

  void check() const
  {
    const auto view = grid_provider_.leaf_view();
    const auto periodic_view = Dune::XT::Grid::make_periodic_grid_layer(view);
    const auto dimensions = Dune::XT::Grid::dimensions(view);
    Dune::XT::Grid::EntityInlevelSearch<decltype(view)> search(view);
    Dune::XT::Grid::EntityInlevelSearch<decltype(periodic_view)> periodic_search(periodic_view);
    const auto center = dimensions.view_center();
    const auto result = search(std::vector<std::remove_const<decltype(center)>::type>{center});
    const auto periodic_result = periodic_search(std::vector<std::remove_const<decltype(center)>::type>{center});
    EXPECT_GE(result.size(), 1);
    EXPECT_GE(periodic_result.size(), 1);
  }

  GridProviderType grid_provider_;
}; // class ConstGridProviderBase


TEST_F(InLevelSearch, check)
{
  this->check();
}
