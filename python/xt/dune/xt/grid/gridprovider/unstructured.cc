// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2026 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   René Fritze (2026)

#include "config.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <dune/geometry/type.hh>
#include <dune/grid/common/gridfactory.hh>

#include <dune/xt/common/fvector.hh>
#include <dune/xt/grid/gridprovider/provider.hh>
#include <dune/xt/grid/grids.hh>

#include <python/xt/dune/xt/common/bindings.hh>
#include <python/xt/dune/xt/common/fvector.hh>
#include <python/xt/dune/xt/grid/traits.hh>

using namespace Dune;
using namespace Dune::XT::Grid::bindings;


// The mixed-element and prism factories below mirror the C++ test fixtures make_mixed_grid /
// make_prism_grid (dune/gdt/test/spaces/base.hh), which are currently the only place those element
// types are exercised anywhere. Only UGGrid can hold a mesh with more than one geometry type (or a
// prism), so these factories are instantiated for UG grids exclusively.
#if HAVE_DUNE_UGGRID || HAVE_UG


template <class G>
struct make_prism_grid
{
  static constexpr size_t d = G::dimension;
  static_assert(d == 3, "prism grids are only meaningful in 3d");

  static void bind(pybind11::module& m)
  {
    using namespace pybind11::literals;
    using D = typename G::ctype;

    m.def(
        "make_prism_grid",
        [](const Dimension<d>&, const unsigned int num_refinements) {
          GridFactory<G> factory;
          for (auto&& vertex : {XT::Common::FieldVector<D, d>({-1., -1.5, -1.5}),
                                XT::Common::FieldVector<D, d>({-1., -1., -1.5}),
                                XT::Common::FieldVector<D, d>({-1.5, -1.5, -1.5}),
                                XT::Common::FieldVector<D, d>({-1., -1.5, -1.}),
                                XT::Common::FieldVector<D, d>({-1., -1., -1.}),
                                XT::Common::FieldVector<D, d>({-1.5, -1.5, -1.})}) {
            factory.insertVertex(vertex);
          }
          factory.insertElement(GeometryTypes::prism, {0, 1, 2, 3, 4, 5});
          XT::Grid::GridProvider<G> grid(factory.createGrid());
          grid.global_refine(num_refinements);
          return grid;
        },
        "dimension"_a,
        "num_refinements"_a = 0);
  } // ... bind(...)
}; // struct make_prism_grid


template <class G>
struct make_mixed_grid
{
  static constexpr size_t d = G::dimension;
  static_assert(d == 2 || d == 3, "mixed-element grids are only implemented in 2d and 3d");

  static void bind(pybind11::module& m)
  {
    using namespace pybind11::literals;
    using D = typename G::ctype;

    m.def(
        "make_mixed_grid",
        [](const Dimension<d>&, const unsigned int num_refinements) {
          GridFactory<G> factory;
          if constexpr (d == 2) {
            for (auto&& vertex : {XT::Common::FieldVector<D, d>({-1., -1.5}),
                                  XT::Common::FieldVector<D, d>({-1., -1.25}),
                                  XT::Common::FieldVector<D, d>({-1., -1.}),
                                  XT::Common::FieldVector<D, d>({-1.5, -1.5}),
                                  XT::Common::FieldVector<D, d>({-1.5, -1.25}),
                                  XT::Common::FieldVector<D, d>({-1.5, -1.}),
                                  XT::Common::FieldVector<D, d>({-1.75, -1.25})}) {
              factory.insertVertex(vertex);
            }
            factory.insertElement(GeometryTypes::cube(2), {3, 0, 4, 1});
            factory.insertElement(GeometryTypes::cube(2), {4, 1, 5, 2});
            factory.insertElement(GeometryTypes::simplex(2), {4, 6, 3});
            factory.insertElement(GeometryTypes::simplex(2), {4, 5, 6});
          } else {
            for (auto&& vertex : {XT::Common::FieldVector<D, d>({-1., -1.5, -1.}),
                                  XT::Common::FieldVector<D, d>({-1., -1.25, -1.}),
                                  XT::Common::FieldVector<D, d>({-1., -1., -1.}),
                                  XT::Common::FieldVector<D, d>({-1.5, -1.5, -1.}),
                                  XT::Common::FieldVector<D, d>({-1.5, -1.25, -1.}),
                                  XT::Common::FieldVector<D, d>({-1.5, -1., -1.}),
                                  XT::Common::FieldVector<D, d>({-1., -1.5, -1.5}),
                                  XT::Common::FieldVector<D, d>({-1., -1.25, -1.5}),
                                  XT::Common::FieldVector<D, d>({-1., -1., -1.5}),
                                  XT::Common::FieldVector<D, d>({-1.5, -1.5, -1.5}),
                                  XT::Common::FieldVector<D, d>({-1.5, -1.25, -1.5}),
                                  XT::Common::FieldVector<D, d>({-1.5, -1., -1.5}),
                                  XT::Common::FieldVector<D, d>({-1.75, -1.25, -1.})}) {
              factory.insertVertex(vertex);
            }
            factory.insertElement(GeometryTypes::cube(3), {3, 0, 4, 1, 9, 6, 10, 7});
            factory.insertElement(GeometryTypes::cube(3), {4, 1, 5, 2, 10, 7, 11, 8});
            factory.insertElement(GeometryTypes::simplex(3), {4, 12, 3, 10});
            factory.insertElement(GeometryTypes::simplex(3), {4, 5, 12, 10});
          }
          XT::Grid::GridProvider<G> grid(factory.createGrid());
          grid.global_refine(num_refinements);
          return grid;
        },
        "dimension"_a,
        "num_refinements"_a = 0);
  } // ... bind(...)
}; // struct make_mixed_grid


#endif // HAVE_DUNE_UGGRID || HAVE_UG


PYBIND11_MODULE(_grid_gridprovider_unstructured, m)
{
  namespace py = pybind11;

  py::module::import("dune.xt.common");
  py::module::import("dune.xt.grid._grid_gridprovider_provider");
  py::module::import("dune.xt.grid._grid_traits");

#if HAVE_DUNE_UGGRID || HAVE_UG
  make_mixed_grid<UG_2D>::bind(m);
  make_mixed_grid<UG_3D>::bind(m);
  make_prism_grid<UG_3D>::bind(m);
#endif
}
