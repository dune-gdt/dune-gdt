// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2017 - 2020)
//   René Fritze     (2017 - 2020)
//   Tobias Leibner  (2020)

#ifndef DUNE_XT_GRID_GRIDS_BINDINGS_HH
#define DUNE_XT_GRID_GRIDS_BINDINGS_HH

#include <tuple>
#include <type_traits>

#include <dune/xt/common/string.hh>
#include <dune/xt/common/tuple.hh>
#include <dune/xt/grid/grids.hh>


namespace Dune::XT::Grid::bindings {


namespace internal {


// Whether Tuple already contains the exact type T.
template <class T, class Tuple>
struct tuple_contains : std::false_type
{};

template <class T, class Head, class... Tail>
struct tuple_contains<T, std::tuple<Head, Tail...>>
  : std::conditional_t<std::is_same<T, Head>::value, std::true_type, tuple_contains<T, std::tuple<Tail...>>>
{};

// Accumulate the first occurrence of every type, dropping later duplicates.
template <class Acc, class Tuple>
struct unique_tuple_helper
{
  using type = Acc;
};

template <class... Accs, class Head, class... Tail>
struct unique_tuple_helper<std::tuple<Accs...>, std::tuple<Head, Tail...>>
{
  using next = std::
      conditional_t<tuple_contains<Head, std::tuple<Accs...>>::value, std::tuple<Accs...>, std::tuple<Accs..., Head>>;
  using type = typename unique_tuple_helper<next, std::tuple<Tail...>>::type;
};


} // namespace internal


/// \brief Drop duplicate types from a grid-type tuple.
///
/// pybind11 registers each C++ type exactly once per interpreter, so a grid tuple that lists two
/// aliases resolving to the *same* type (e.g. dune-alugrid maps ALUGrid<2,2,cube,nonconforming>
/// onto the 2d simplex grid -- there is no distinct 2d cube ALUGrid) would double-register its
/// GridProvider/BoundaryInfo classes and abort module import. Deduplicating here keeps the binding
/// grid lists robust against such aliasing regardless of the dune-alugrid configuration.
template <class Tuple>
using unique_grid_tuple_t = typename internal::unique_tuple_helper<std::tuple<>, Tuple>::type;


template <class G>
struct grid_name
{
  static std::string value()
  {
    static_assert(AlwaysFalse<G>::value, "missing specialization of grid_name");
    return "";
  }
};


template <class G>
struct grid_name<const G>
{
  static std::string value()
  {
    return grid_name<G>::value();
  }
};


template <>
struct grid_name<Dune::OneDGrid>
{
  static std::string value()
  {
    return "1d_simplex_onedgrid";
  }
};


template <int dim>
struct grid_name<YaspGrid<dim, EquidistantOffsetCoordinates<double, dim>>>
{
  static std::string value()
  {
    return Common::to_string(dim) + "d_cube_yaspgrid";
  }
};

#if HAVE_DUNE_ALUGRID


template <int dim, class Comm>
struct grid_name<ALUGrid<dim, dim, simplex, conforming, Comm>>
{
  static std::string value()
  {
    return Common::to_string(dim) + "d_simplex_aluconformgrid";
  }
};

template <int dim, class Comm>
struct grid_name<ALUGrid<dim, dim, cube, nonconforming, Comm>>
{
  static std::string value()
  {
    return Common::to_string(dim) + "d_cube_alunonconformgrid";
  }
};

template <int dim, class Comm>
struct grid_name<ALUGrid<dim, dim, simplex, nonconforming, Comm>>
{
  static std::string value()
  {
    return Common::to_string(dim) + "d_simplex_alunonconformgrid";
  }
};

template <int dim, class Comm>
struct grid_name<ALUGrid<dim, dim, cube, conforming, Comm>>
{
  static std::string value()
  {
    return Common::to_string(dim) + "d_cube_aluconformgrid";
  }
};

template <int dim, class Comm>
struct grid_name<ALU3dGrid<dim, dim, ALU3dGridElementType::tetra, Comm>>
{
  static std::string value()
  {
    return Common::to_string(dim) + "d_tetrahedral_alugrid";
  }
};

template <int dim, class Comm>
struct grid_name<ALU3dGrid<dim, dim, ALU3dGridElementType::hexa, Comm>>
{
  static std::string value()
  {
    return Common::to_string(dim) + "d_hexahedral_alugrid";
  }
};


#endif // HAVE_DUNE_ALUGRID
#if HAVE_ALBERTA


template <int dim>
struct grid_name<Dune::AlbertaGrid<dim, dim>>
{
  static std::string value()
  {
    return Common::to_string(dim) + "d_simplex_albertagrid";
  }
};


#endif // HAVE_ALBERTA
#if HAVE_DUNE_UGGRID || HAVE_UG


template <int dim>
struct grid_name<UGGrid<dim>>
{
  static std::string value()
  { // the "simplex" is due to the fact that our grid provider currently only creates ug grids with simplices
    return Common::to_string(dim) + "d_simplex_uggrid";
  }
};


#endif // HAVE_DUNE_UGGRID || HAVE_UG


/// \attention grid_name<G>::value must be unique for all grid variants below, and every make_...grid
///            factory that a bound grid needs (see gridprovider/cube.cc) has to be able to construct it.
///
/// WP2 (#320) adds YASP_1D_EQUIDISTANT_OFFSET next to ONED_1D: a structured 1d grid with the same
/// equidistant-offset coordinates as the 2d/3d YaspGrids (periodic/overlap features, and the C++
/// SimplicialGrids list tests it). The two 1d grids are disambiguated in the make_cube_grid factory
/// by their element-type tag -- ONED_1D keeps the dimension-only overload, YASP_1D binds the
/// (Dimension, Cube) overload (see python/xt/dune/xt/grid/gridprovider/cube.cc).
///
/// #320 WP1 adds the ALU cube (hexahedral) grid in 3d -- the only ALU variant beyond the historical
/// "one cube, one simplex per dim" set that is actually bindable: the bindings register
/// spaces/operators/boundaryinfo keyed on the *leaf grid view* type (only named via grid_name<G>),
/// and dune-alugrid expresses that view through the legacy ALU3dGrid<tetra|hexa> / ALU2dGrid,
/// dropping the conforming/nonconforming refinement policy from the view type. Hence the conforming
/// and nonconforming ALU *simplex* grids share one leaf-view type and cannot both be registered (the
/// second aborts module import with a pybind11 "type already registered"), and there is no distinct
/// 2d cube ALUGrid at all. Only the 3d hexahedral cube grid has a leaf view (ALU3dGrid<hexa>)
/// distinct from the tetrahedral simplex and structured YASP views, so only it is added here.
/// unique_grid_tuple_t guards the grid tuples against any exact-type aliasing on top.
///
/// WP3 (#320) adds UGGrid, the only bound grid manager that supports mixed-element and prism meshes,
/// as the backend for make_mixed_grid / make_prism_grid (see gridprovider/unstructured.cc); it is
/// added below so that GridProvider, spaces and operators are instantiated for it across the binding TUs.

using Available1dGridTypes = std::tuple<ONED_1D, YASP_1D_EQUIDISTANT_OFFSET>;

using Available2dGridTypes = unique_grid_tuple_t<std::tuple<YASP_2D_EQUIDISTANT_OFFSET
#if HAVE_DUNE_ALUGRID
                                                            ,
                                                            ALU_2D_SIMPLEX_CONFORMING
#endif
#if HAVE_DUNE_UGGRID || HAVE_UG
                                                            ,
                                                            UG_2D
#endif
                                                            >>;

using Available3dGridTypes = unique_grid_tuple_t<std::tuple<YASP_3D_EQUIDISTANT_OFFSET
#if HAVE_DUNE_ALUGRID
                                                            ,
                                                            ALU_3D_SIMPLEX_CONFORMING,
                                                            ALU_3D_CUBE
#endif
#if HAVE_DUNE_UGGRID || HAVE_UG
                                                            ,
                                                            UG_3D
#endif
                                                            >>;

using AvailableGridTypes =
    unique_grid_tuple_t<Common::tuple_cat_t<Available1dGridTypes, Available2dGridTypes, Available3dGridTypes>>;


} // namespace Dune::XT::Grid::bindings

#endif // DUNE_XT_GRID_GRIDS_BINDINGS_HH
