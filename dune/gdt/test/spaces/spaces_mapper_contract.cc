// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2018)
//   René Fritze     (2018)

#ifndef DUNE_XT_COMMON_TEST_MAIN_CATCH_EXCEPTIONS
#  define DUNE_XT_COMMON_TEST_MAIN_CATCH_EXCEPTIONS 1
#endif

#include <dune/xt/test/main.hxx> // <- this one has to come first (includes the config.h)!

#include <algorithm>
#include <set>

#include <dune/common/dynvector.hh>

#include <dune/grid/common/rangegenerators.hh>

#include <dune/xt/grid/grids.hh>
#include <dune/xt/grid/gridprovider/cube.hh>

#include <dune/gdt/exceptions.hh>
#include <dune/gdt/spaces/h1/continuous-lagrange.hh>
#include <dune/gdt/spaces/l2/discontinuous-lagrange.hh>
#include <dune/gdt/spaces/l2/finite-volume.hh>
#include <dune/gdt/spaces/mapper/interfaces.hh>

using namespace Dune;
using namespace Dune::GDT;

using G = YASP_2D_EQUIDISTANT_OFFSET;


// Applies the mapper index-set contract from spaces/base.hh (mapper_(of_discontinuous_space_)maps_correctly): collect
// every global index reported by both call variants into a std::set, then assert the global numbering is consecutive
// (0 .. size()-1), covers the whole range and that mapper.size() agrees. This holds for continuous mappers (which share
// DoFs across elements, so the union of local index sets is smaller than the sum of local sizes but still spans
// {0, ..., size()-1}) as well as for discontinuous / finite-volume mappers (unique block of DoFs per element).
template <class SpaceType>
static void check_mapper_maps_correctly(const SpaceType& space)
{
  std::set<size_t> global_indices;
  std::set<size_t> map_to_global;
  for (auto&& element : Dune::elements(space.grid_view())) {
    for (auto&& global_index : space.mapper().global_indices(element))
      global_indices.insert(global_index);
    for (size_t ii = 0; ii < space.mapper().local_size(element); ++ii)
      map_to_global.insert(space.mapper().global_index(element, ii));
  }
  ASSERT_GT(global_indices.size(), 0u);
  EXPECT_EQ(0u, *global_indices.begin());
  EXPECT_EQ(global_indices.size() - 1, *global_indices.rbegin());
  EXPECT_EQ(0u, *map_to_global.begin());
  EXPECT_EQ(map_to_global.size() - 1, *map_to_global.rbegin());
  EXPECT_EQ(space.mapper().size(), global_indices.size());
  EXPECT_EQ(space.mapper().size(), map_to_global.size());
  for (const auto& global_index : global_indices)
    EXPECT_EQ(1u, global_indices.count(global_index));
  for (const auto& global_index : map_to_global)
    EXPECT_EQ(1u, map_to_global.count(global_index));
}


GTEST_TEST(spaces_mapper_contract, continuous_lagrange_maps_correctly)
{
  auto grid = XT::Grid::make_cube_grid<G>(0., 1., 3);
  auto grid_view = grid.leaf_view();
  const auto space = make_continuous_lagrange_space(grid_view, 1);
  check_mapper_maps_correctly(space);
}


GTEST_TEST(spaces_mapper_contract, discontinuous_lagrange_maps_correctly)
{
  auto grid = XT::Grid::make_cube_grid<G>(0., 1., 3);
  auto grid_view = grid.leaf_view();
  const auto space = make_discontinuous_lagrange_space(grid_view, 1);
  check_mapper_maps_correctly(space);
}


GTEST_TEST(spaces_mapper_contract, finite_volume_maps_correctly)
{
  auto grid = XT::Grid::make_cube_grid<G>(0., 1., 3);
  auto grid_view = grid.leaf_view();
  const auto space = make_finite_volume_space(grid_view);
  check_mapper_maps_correctly(space);
}


// The continuous mapper shares DoFs between neighbouring elements, hence its global size has to be strictly smaller
// than the discontinuous mapper's size on the same grid (which gives every element its own private block of DoFs).
GTEST_TEST(spaces_mapper_contract, continuous_shares_dofs_discontinuous_does_not)
{
  auto grid = XT::Grid::make_cube_grid<G>(0., 1., 3);
  auto grid_view = grid.leaf_view();
  const auto cg = make_continuous_lagrange_space(grid_view, 1);
  const auto dg = make_discontinuous_lagrange_space(grid_view, 1);
  const auto fv = make_finite_volume_space(grid_view);

  EXPECT_LT(cg.mapper().size(), dg.mapper().size());
  // one DoF per element for FV, Q1 has 4 local DoFs per element for DG
  EXPECT_EQ(fv.mapper().size() * 4u, dg.mapper().size());
}


// The remaining surface of the finite volume mapper: it hands out the grid view it was created on and P0 local
// coefficients, resizes an undersized indices vector in global_indices, and rejects out-of-range local indices.
GTEST_TEST(spaces_mapper_contract, finite_volume_mapper_accessors_and_bounds)
{
  auto grid = XT::Grid::make_cube_grid<G>(0., 1., 3);
  auto grid_view = grid.leaf_view();
  const auto space = make_finite_volume_space(grid_view);
  const auto element = *Dune::elements(grid_view).begin();

  EXPECT_EQ(grid_view.size(0), space.mapper().grid_view().size(0));
  EXPECT_EQ(1u, space.mapper().local_coefficients(element.type()).size());
  DynamicVector<size_t> undersized_indices;
  space.mapper().global_indices(element, undersized_indices);
  ASSERT_GE(undersized_indices.size(), 1u);
  EXPECT_EQ(space.mapper().global_index(element, 0), undersized_indices[0]);
  EXPECT_THROW(space.mapper().global_index(element, space.mapper().local_size(element)), Exceptions::mapper_error);
}


// A vector-valued DG space with dimension-wise global numbering blocks its DoFs by range dimension: all DoFs of
// component s occupy the contiguous global range [s * size / r, (s + 1) * size / r). This runs the dimension-wise
// (else) branches of the DiscontinuousMapper -- construction, update_after_adapt, the single-index global_index and
// the resizing global_indices -- alongside the usual index-set contract, and the mapper rejects out-of-range local
// indices just like the scalar one.
GTEST_TEST(spaces_mapper_contract, dimensionwise_discontinuous_mapper_blocks_by_component)
{
  auto grid = XT::Grid::make_cube_grid<G>(0., 1., 3);
  auto grid_view = grid.leaf_view();
  constexpr size_t r = 2;
  const DiscontinuousLagrangeSpace<decltype(grid_view), r> space(grid_view, /*order=*/1, /*dimwise=*/true);
  check_mapper_maps_correctly(space);

  const size_t dofs_per_component = space.mapper().size() / r;
  ASSERT_EQ(space.mapper().size(), dofs_per_component * r);
  for (auto&& element : Dune::elements(grid_view)) {
    const size_t local_size = space.mapper().local_size(element);
    ASSERT_EQ(0u, local_size % r);
    const size_t local_per_component = local_size / r;
    DynamicVector<size_t> undersized_indices;
    space.mapper().global_indices(element, undersized_indices);
    ASSERT_GE(undersized_indices.size(), local_size);
    for (size_t local_index = 0; local_index < local_size; ++local_index) {
      const size_t component = local_index / local_per_component;
      const size_t global_index = space.mapper().global_index(element, local_index);
      EXPECT_EQ(global_index, undersized_indices[local_index]);
      EXPECT_GE(global_index, component * dofs_per_component);
      EXPECT_LT(global_index, (component + 1) * dofs_per_component);
    }
    EXPECT_THROW(space.mapper().global_index(element, local_size), Exceptions::mapper_error);
  }
}


// A mapper that does not override update_after_adapt() inherits the interface's does-not-support-adaptation report;
// the stub forwards everything else to a real FV mapper so the interface default has a working mapper underneath.
GTEST_TEST(spaces_mapper_contract, mapper_interface_defaults)
{
  auto grid = XT::Grid::make_cube_grid<G>(0., 1., 3);
  auto grid_view = grid.leaf_view();
  const auto space = make_finite_volume_space(grid_view);

  using GV = decltype(grid_view);
  struct MinimalMapper : MapperInterface<GV>
  {
    MinimalMapper(const MapperInterface<GV>& wrapped)
      : wrapped_(wrapped)
    {
    }
    const GV& grid_view() const override
    {
      return wrapped_.grid_view();
    }
    const LocalFiniteElementCoefficientsInterface<typename MapperInterface<GV>::D, MapperInterface<GV>::d>&
    local_coefficients(const GeometryType& geometry_type) const override
    {
      return wrapped_.local_coefficients(geometry_type);
    }
    size_t size() const override
    {
      return wrapped_.size();
    }
    size_t max_local_size() const override
    {
      return wrapped_.max_local_size();
    }
    size_t local_size(const typename MapperInterface<GV>::ElementType& element) const override
    {
      return wrapped_.local_size(element);
    }
    size_t global_index(const typename MapperInterface<GV>::ElementType& element,
                        const size_t local_index) const override
    {
      return wrapped_.global_index(element, local_index);
    }
    using MapperInterface<GV>::global_indices;
    void global_indices(const typename MapperInterface<GV>::ElementType& element,
                        DynamicVector<size_t>& indices) const override
    {
      wrapped_.global_indices(element, indices);
    }
    const MapperInterface<GV>& wrapped_;
  };

  MinimalMapper minimal_mapper(space.mapper());
  const auto element = *Dune::elements(grid_view).begin();
  // the DynamicVector-returning convenience of the interface runs on top of the stub's two-argument override
  EXPECT_EQ(space.mapper().global_indices(element), minimal_mapper.global_indices(element));
  EXPECT_THROW(minimal_mapper.update_after_adapt(), Exceptions::mapper_error);
}
