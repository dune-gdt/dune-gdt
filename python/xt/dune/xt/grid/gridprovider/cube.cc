// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2020)
//   Tobias Leibner  (2020)

#include "config.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <dune/xt/grid/gridprovider/cube.hh>

#include <python/xt/dune/xt/common/bindings.hh>
#include <python/xt/dune/xt/common/fvector.hh>
#include <python/xt/dune/xt/grid/traits.hh>

using namespace Dune;
using namespace Dune::XT::Grid::bindings;


template <class G, class element_type>
struct make_cube_grid
{
  static void bind(pybind11::module& m)
  {
    using namespace pybind11::literals;
    using D = typename G::ctype;
    static constexpr size_t d = G::dimension;

    m.def("make_cube_grid",
          [](const Dimension<d>&,
          const element_type&,
          const FieldVector<D, d>& lower_left,
          const FieldVector<D, d>& upper_right,
          const std::array<unsigned int, d>& num_elements,
          const unsigned int num_refinements,
          const std::array<unsigned int, d>& overlap_size/*,
          Common::MPI_Comm_Wrapper mpi_comm*/) {
      return XT::Grid::make_cube_grid<G>(lower_left, upper_right, num_elements, num_refinements, overlap_size /*, mpi_comm*/
      );},
    "dimension"_a,
    "element_type"_a,
    "lower_left"_a,
    "upper_right"_a,
    "num_elements"_a,
    "num_refinements"_a = 0,
    "overlap_size"_a = XT::Grid::cube_gridprovider_default_config().get<std::array<unsigned int, d>>("overlap_size")/*,
    "mpi_comm"_a = Common::MPI_Comm_Wrapper()*/);
  } // ... bind(...)
}; // struct make_cube_grid<...>

// Same factory as make_cube_grid<G, element_type>, but with an additional leading selector tag
// (e.g. Nonconforming) so that a second grid implementation sharing the (dimension, element_type)
// signature -- the unstructured ALU cube/simplex grids added in #320 WP1 -- can be reached without
// colliding with the structured/conforming default overload. pybind11 dispatches on the extra
// argument, so `make_cube_grid(Dim(2), Cube())` still yields YASP while
// `make_cube_grid(Dim(2), Cube(), Nonconforming())` yields the ALU cube grid.
template <class G, class element_type, class backend_type>
struct make_cube_grid_backend
{
  static void bind(pybind11::module& m)
  {
    using namespace pybind11::literals;
    using D = typename G::ctype;
    static constexpr size_t d = G::dimension;

    m.def(
        "make_cube_grid",
        [](const Dimension<d>&,
           const element_type&,
           const backend_type&,
           const FieldVector<D, d>& lower_left,
           const FieldVector<D, d>& upper_right,
           const std::array<unsigned int, d>& num_elements,
           const unsigned int num_refinements,
           const std::array<unsigned int, d>& overlap_size) {
          return XT::Grid::make_cube_grid<G>(lower_left, upper_right, num_elements, num_refinements, overlap_size);
        },
        "dimension"_a,
        "element_type"_a,
        "backend"_a,
        "lower_left"_a,
        "upper_right"_a,
        "num_elements"_a,
        "num_refinements"_a = 0,
        "overlap_size"_a =
            XT::Grid::cube_gridprovider_default_config().get<std::array<unsigned int, d>>("overlap_size"));
  } // ... bind(...)
}; // struct make_cube_grid_backend<...>

template <class G>
struct make_cube_grid<G, void>
{
  static void bind(pybind11::module& m)
  {
    using namespace pybind11::literals;
    using D = typename G::ctype;
    static constexpr size_t d = G::dimension;

    m.def("make_cube_grid",
            [](const Dimension<d>&,
            const FieldVector<D, d>& lower_left,
            const FieldVector<D, d>& upper_right,
            const std::array<unsigned int, d>& num_elements,
            const unsigned int num_refinements,
            const std::array<unsigned int, d>& overlap_size/*,
            Common::MPI_Comm_Wrapper mpi_comm*/) {
        return XT::Grid::make_cube_grid<G>(lower_left, upper_right, num_elements, num_refinements, overlap_size /*, mpi_comm*/
        );},
      "dimension"_a,
      "lower_left"_a,
      "upper_right"_a,
      "num_elements"_a,
      "num_refinements"_a = 0,
      "overlap_size"_a = XT::Grid::cube_gridprovider_default_config().get<std::array<unsigned int, d>>("overlap_size")/*,
      "mpi_comm"_a = Common::MPI_Comm_Wrapper()*/);
  } // ... bind(...)
}; // struct make_cube_grid<..., void>


PYBIND11_MODULE(_grid_gridprovider_cube, m)
{
  namespace py = pybind11;

  py::module::import("dune.xt.common");
  py::module::import("dune.xt.grid._grid_gridprovider_provider");
  py::module::import("dune.xt.grid._grid_traits");

  make_cube_grid<ONED_1D, void>::bind(m);
  // WP2 (#320): the structured 1d YaspGrid uses the (Dimension, Cube) overload so it can coexist
  // with ONED_1D (dimension-only overload) without an ambiguous 1d make_cube_grid signature.
  make_cube_grid<YASP_1D_EQUIDISTANT_OFFSET, Cube>::bind(m); // NOSONAR(cpp:S1117): m is an argument, not a decl
  make_cube_grid<YASP_2D_EQUIDISTANT_OFFSET, Cube>::bind(m);
  make_cube_grid<YASP_3D_EQUIDISTANT_OFFSET, Cube>::bind(m);
#if HAVE_DUNE_ALUGRID
  // default (structured cube / conforming simplex) overloads
  make_cube_grid<ALU_2D_SIMPLEX_CONFORMING, Simplex>::bind(m);
  make_cube_grid<ALU_3D_SIMPLEX_CONFORMING, Simplex>::bind(m);
  // Nonconforming() selects the unstructured, nonconforming ALU cube (hexahedral) grid, which shares
  // the (Dim(3), Cube) signature with the structured YASP cube. It is the only ALU variant beyond the
  // historical set that is separately bindable: the nonconforming ALU *simplex* grids share their
  // leaf-view type with the conforming ones (see grids.bindings.hh), and 2d has no cube ALUGrid.
  make_cube_grid_backend<ALU_3D_CUBE, Cube, Nonconforming>::bind(m);
#endif
}
