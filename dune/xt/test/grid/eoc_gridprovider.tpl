// This file is part of the dune-gdt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-gdt
// Copyright 2010-2021 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2012, 2014, 2016 - 2017, 2019)
//   René Fritze     (2012 - 2013, 2015 - 2016, 2018 - 2019)
//   Tobias Leibner  (2014, 2016, 2018, 2020 - 2021)
//
// This file is part of the dune-xt project:

#include <dune/xt/test/main.hxx>

#include <dune/xt/grid/gridprovider/eoc.hh>
#include <dune/xt/grid/grids.hh>


{% for name, type in config.all_grids %}

GTEST_TEST(EocProvider_{{name}}, layers)
{
  using Level [[maybe_unused]] = Dune::XT::Grid::LevelBasedEOCGridProvider<{{type}}>;
  using Leaf [[maybe_unused]] = Dune::XT::Grid::LeafBasedEOCGridProvider<{{type}}>;
}

{% endfor %}
