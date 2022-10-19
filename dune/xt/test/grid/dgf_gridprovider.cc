// This file is part of the dune-gdt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-gdt
// Copyright 2010-2021 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2016 - 2017)
//   René Fritze     (2018 - 2019)
//   Tobias Leibner  (2020 - 2021)
//
// This file is part of the dune-xt project:

#include <dune/xt/test/main.hxx>

#include "provider.hh"


struct DgfGridProvider : public GridProviderBase<TESTGRIDTYPE>
{
  using BaseType = GridProviderBase<TESTGRIDTYPE>;

  void check_layers()
  {
    BaseType::check_layers(Dune::XT::Grid::make_dgf_grid<TESTGRIDTYPE>());
  }

  void check_visualize()
  {
    BaseType::check_visualize(Dune::XT::Grid::make_dgf_grid<TESTGRIDTYPE>());
  }
};

TEST_F(DgfGridProvider, layers)
{
  this->check_layers();
}
TEST_F(DgfGridProvider, visualize)
{
  this->check_visualize();
}
