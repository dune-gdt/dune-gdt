// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2026 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   René Fritze (2026)

#ifndef DUNE_XT_GRID_GRIDPROVIDER_UNSTRUCTURED_HH
#define DUNE_XT_GRID_GRIDPROVIDER_UNSTRUCTURED_HH

#include <dune/geometry/type.hh>

#include <dune/grid/common/gridfactory.hh>

#include <dune/xt/common/fvector.hh>
#include <dune/xt/grid/gridprovider/provider.hh>

namespace Dune::XT::Grid {


/// \brief A grid with mixed cube/simplex elements (2d or 3d), built via the GridFactory.
///
/// Only grid managers that can hold more than one geometry type (i.e. UGGrid) can be instantiated
/// with this; the factory body is therefore only ever compiled for such grids. This is the single
/// source of truth for the mixed-element mesh used both by the C++ space tests
/// (dune/gdt/test/spaces/base.hh) and by the python make_mixed_grid binding.
template <class G>
GridProvider<G> make_mixed_grid(const unsigned int num_refinements = 0)
{
  using D = typename G::ctype;
  static constexpr size_t d = G::dimension;
  static_assert(d == 2 || d == 3, "mixed-element grids are only implemented in 2d and 3d");
  GridFactory<G> factory;
  if constexpr (d == 2) {
    for (auto&& vertex : {Common::FieldVector<D, d>({-1., -1.5}),
                          Common::FieldVector<D, d>({-1., -1.25}),
                          Common::FieldVector<D, d>({-1., -1.}),
                          Common::FieldVector<D, d>({-1.5, -1.5}),
                          Common::FieldVector<D, d>({-1.5, -1.25}),
                          Common::FieldVector<D, d>({-1.5, -1.}),
                          Common::FieldVector<D, d>({-1.75, -1.25})}) {
      factory.insertVertex(vertex);
    }
    factory.insertElement(GeometryTypes::cube(2), {3, 0, 4, 1});
    factory.insertElement(GeometryTypes::cube(2), {4, 1, 5, 2});
    factory.insertElement(GeometryTypes::simplex(2), {4, 6, 3});
    factory.insertElement(GeometryTypes::simplex(2), {4, 5, 6});
  } else {
    for (auto&& vertex : {Common::FieldVector<D, d>({-1., -1.5, -1.}),
                          Common::FieldVector<D, d>({-1., -1.25, -1.}),
                          Common::FieldVector<D, d>({-1., -1., -1.}),
                          Common::FieldVector<D, d>({-1.5, -1.5, -1.}),
                          Common::FieldVector<D, d>({-1.5, -1.25, -1.}),
                          Common::FieldVector<D, d>({-1.5, -1., -1.}),
                          Common::FieldVector<D, d>({-1., -1.5, -1.5}),
                          Common::FieldVector<D, d>({-1., -1.25, -1.5}),
                          Common::FieldVector<D, d>({-1., -1., -1.5}),
                          Common::FieldVector<D, d>({-1.5, -1.5, -1.5}),
                          Common::FieldVector<D, d>({-1.5, -1.25, -1.5}),
                          Common::FieldVector<D, d>({-1.5, -1., -1.5}),
                          Common::FieldVector<D, d>({-1.75, -1.25, -1.})}) {
      factory.insertVertex(vertex);
    }
    factory.insertElement(GeometryTypes::cube(3), {3, 0, 4, 1, 9, 6, 10, 7});
    factory.insertElement(GeometryTypes::cube(3), {4, 1, 5, 2, 10, 7, 11, 8});
    factory.insertElement(GeometryTypes::simplex(3), {4, 12, 3, 10});
    factory.insertElement(GeometryTypes::simplex(3), {4, 5, 12, 10});
  }
  GridProvider<G> grid(factory.createGrid());
  grid.global_refine(num_refinements);
  return grid;
} // ... make_mixed_grid(...)


/// \brief A 3d grid consisting of a single prism, built via the GridFactory.
///
/// As with make_mixed_grid, only UGGrid supports prism elements, so the body is only ever compiled
/// for such grids. Single source of truth shared by dune/gdt/test/spaces/base.hh and the python
/// make_prism_grid binding.
template <class G>
GridProvider<G> make_prism_grid(const unsigned int num_refinements = 0)
{
  using D = typename G::ctype;
  static constexpr size_t d = G::dimension;
  static_assert(d == 3, "prism grids are only meaningful in 3d");
  GridFactory<G> factory;
  for (auto&& vertex : {Common::FieldVector<D, d>({-1., -1.5, -1.5}),
                        Common::FieldVector<D, d>({-1., -1., -1.5}),
                        Common::FieldVector<D, d>({-1.5, -1.5, -1.5}),
                        Common::FieldVector<D, d>({-1., -1.5, -1.}),
                        Common::FieldVector<D, d>({-1., -1., -1.}),
                        Common::FieldVector<D, d>({-1.5, -1.5, -1.})}) {
    factory.insertVertex(vertex);
  }
  factory.insertElement(GeometryTypes::prism, {0, 1, 2, 3, 4, 5});
  GridProvider<G> grid(factory.createGrid());
  grid.global_refine(num_refinements);
  return grid;
} // ... make_prism_grid(...)


} // namespace Dune::XT::Grid

#endif // DUNE_XT_GRID_GRIDPROVIDER_UNSTRUCTURED_HH
