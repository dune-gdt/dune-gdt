// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2017)
//   Rene Milk       (2016 - 2018)

#include <dune/xt/common/test/main.hxx> // <- This one has to come first!

#include <dune/xt/grid/type_traits.hh>

#include <dune/gdt/test/grids.hh>
#include <dune/gdt/test/projections/l2-local.hh>
#include <dune/gdt/test/projections/l2.hh>
#include <dune/gdt/spaces/cg.hh>
#include <dune/gdt/spaces/dg.hh>
#include <dune/gdt/spaces/fv.hh>
#include <dune/gdt/spaces/rt/default.hh>


using namespace Dune::GDT::Test;

// clang-format off
{% for SpaceType,Name in config.spaces_with_names %}

typedef L2LocalProjectionOperatorTest<{{SpaceType}}>
  L2LocalProjectionOperatorTest_{{Name}};
typedef L2LocalProjectionLocalizableOperatorTest<{{SpaceType}}>
  L2LocalProjectionLocalizableOperatorTest_{{Name}};

{% if 'FvSpace' in SpaceType %}
  const double {{Name}}_tolerance = 1.45e-1;
{% elif 'RaviartThomasSpace' in SpaceType %}
    const auto {{Name}}_tolerance = rt_tolerance<L2LocalProjectionOperatorTest_{{Name}}>();
{% elif 'ContinuousLagrangeSpace' in SpaceType %}
    const auto {{Name}}_tolerance = cg_tolerance<L2LocalProjectionOperatorTest_{{Name}}>();
{% else %}
  const auto {{Name}}_tolerance = Dune::XT::Grid::is_alugrid<Dune::XT::Grid::extract_grid_t<typename {{SpaceType}}::GridLayerType>>::value
      ? L2LocalProjectionOperatorTest_{{Name}}::alugrid_tolerance
      : L2LocalProjectionOperatorTest_{{Name}}::default_tolerance;
{% endif %}

TEST_F(L2LocalProjectionOperatorTest_{{Name}}, constructible_by_ctor)
{
  this->constructible_by_ctor();
}
TEST_F(L2LocalProjectionOperatorTest_{{Name}}, constructible_by_factory)
{
  this->constructible_by_factory();
}
TEST_F(L2LocalProjectionOperatorTest_{{Name}}, produces_correct_results)
{
  this->produces_correct_results({{Name}}_tolerance);
}

TEST_F(L2LocalProjectionLocalizableOperatorTest_{{Name}}, constructible_by_ctor)
{
  this->constructible_by_ctor();
}
TEST_F(L2LocalProjectionLocalizableOperatorTest_{{Name}}, constructible_by_factory)
{
  this->constructible_by_factory();
}
TEST_F(L2LocalProjectionLocalizableOperatorTest_{{Name}}, produces_correct_results)
{
  this->produces_correct_results({{Name}}_tolerance);
  this->produces_correct_results({{Name}}_tolerance);
}


{% endfor %}
// clang-format on
