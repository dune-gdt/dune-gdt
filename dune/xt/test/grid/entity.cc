// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   René Fritze (2026)

#include <dune/xt/test/main.hxx>

#include <cmath>

#include <dune/grid/common/rangegenerators.hh>

#include <dune/xt/common/float_cmp.hh>
#include <dune/xt/common/logstreams.hh>

#include <dune/xt/grid/entity.hh>
#include <dune/xt/grid/grids.hh>
#include <dune/xt/grid/gridprovider/cube.hh>
#include <dune/xt/grid/type_traits.hh>

using namespace Dune::XT;
using namespace Dune::XT::Common;
using namespace Dune::XT::Grid;

template <class T>
struct EntityTest : public ::testing::Test
{
  static constexpr size_t d = T::value;
  using GridType = Dune::YaspGrid<d, Dune::EquidistantOffsetCoordinates<double, d>>;
  using GridViewType = typename GridType::LeafGridView;
  using ElementType = extract_entity_t<GridViewType>;

  static constexpr size_t num_elements_per_dim = 2;
  const GridProvider<GridType> grid_prv;

  EntityTest()
    : grid_prv(make_cube_grid<GridType>(0., 1., num_elements_per_dim))
  {
  }

  // On a unit cube subdivided into n^d cells, each cell is a cube of edge h = 1/n,
  // so its diameter (longest corner-to-corner distance) is h * sqrt(d).
  void check_diameter()
  {
    const auto gv = grid_prv.grid().leafGridView();
    const double h = 1. / double(num_elements_per_dim);
    const double expected = h * std::sqrt(double(d));
    size_t checked = 0;
    for (auto&& element : Dune::elements(gv)) {
      EXPECT_TRUE(FloatCmp::eq(diameter(element), expected));
      EXPECT_DOUBLE_EQ(diameter(element), entity_diameter(element));
      ++checked;
    }
    EXPECT_EQ(checked, gv.size(0));
  }

  void check_reference_element()
  {
    const auto gv = grid_prv.grid().leafGridView();
    for (auto&& element : Dune::elements(gv)) {
      const auto ref_from_entity = reference_element(element);
      const auto ref_from_geometry = reference_element(element.geometry());
      // the element itself is the single codim-0 subentity
      EXPECT_EQ(1, ref_from_entity.size(0));
      EXPECT_EQ(ref_from_entity.size(0), ref_from_geometry.size(0));
      // a cube element in d dimensions has 2^d corners (codim-d subentities)
      EXPECT_EQ(size_t(1) << d, size_t(ref_from_entity.size(int(d))));
    }
  }

  void check_print()
  {
    const auto gv = grid_prv.grid().leafGridView();
    for (auto&& element : Dune::elements(gv)) {
      print_entity(element, "element", dev_null);
      dev_null << element; // operator<<
      // also exercise the empty-name branch
      print_entity(element, "", dev_null);
      break; // one element is enough to exercise the output code path
    }
  }
};

using GridDims = ::testing::Types<Int<1>, Int<2>, Int<3>>;
TYPED_TEST_SUITE(EntityTest, GridDims);

TYPED_TEST(EntityTest, diameter)
{
  this->check_diameter();
}
TYPED_TEST(EntityTest, reference_element)
{
  this->check_reference_element();
}
TYPED_TEST(EntityTest, print)
{
  this->check_print();
}
