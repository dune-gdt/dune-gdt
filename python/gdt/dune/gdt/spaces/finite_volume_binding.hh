// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2020)

#ifndef PYTHON_DUNE_GDT_SPACES_FINITE_VOLUME_BINDING_HH
#define PYTHON_DUNE_GDT_SPACES_FINITE_VOLUME_BINDING_HH

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <dune/xt/grid/gridprovider/provider.hh>

#include <dune/gdt/spaces/interface.hh>

#include <python/xt/dune/xt/common/configuration.hh>
#include <python/xt/dune/xt/common/fvector.hh>
#include <python/xt/dune/xt/grid/grids.bindings.hh>
#include <python/xt/dune/xt/grid/traits.hh>
#include <python/gdt/dune/gdt/spaces/binding_helpers.hh>

namespace Dune {
namespace GDT {
namespace bindings {


/**
 * \brief Binds a piecewise constant space that is constructed from nothing but a grid view.
 *
 * GDT::FiniteVolumeSpace (dune/gdt/spaces/l2/finite-volume.hh, one DoF per element) and
 * GDT::FiniteVolumeSkeletonSpace (dune/gdt/spaces/skeleton/finite-volume.hh, one DoF per intersection) share their
 * whole binding surface - a single-argument constructor, a __repr__ and a module-level factory - and used to be bound
 * by two files that differed only in the name of the space. SpaceType is the template of the space to bind.
 *
 * \note Assumes that GV is the leaf view!
 */
template <template <class, size_t, size_t, class> class SpaceType, class GV, size_t r = 1, class R = double>
class FiniteVolumeSpaceBinding
{
  using G = typename GV::Grid;
  static const size_t d = G::dimension;

public:
  using type = SpaceType<GV, r, 1, R>;
  using base_type = GDT::SpaceInterface<GV, r, 1, R>;
  using bound_type = pybind11::class_<type, base_type>;

  static bound_type bind(pybind11::module& m, const std::string& grid_id, const std::string& class_id)
  {
    namespace py = pybind11;
    using namespace pybind11::literals;

    const auto ClassName = space_class_name<r, R>(class_id, grid_id);
    bound_type c(m, ClassName.c_str(), ClassName.c_str());
    c.def(py::init([](XT::Grid::GridProvider<G>& grid_provider) {
            return new type(grid_provider.leaf_view()); // Otherwise we get an error here!
          }),
          // A space holds the grid *view*, which points into the grid the provider owns: without this, a space
          // built from a temporary provider outlives its grid and every later use is a use-after-free (#378).
          py::keep_alive<1, 2>(),
          "grid_provider"_a);
    add_space_repr(c);

    const auto FactoryName = space_factory_name<R>(class_id);
    if (r == 1)
      m.def(
          FactoryName.c_str(),
          [](XT::Grid::GridProvider<G>& grid, const XT::Grid::bindings::Dimension<r>&) {
            return new type(grid.leaf_view()); // Otherwise we get an error here!
          },
          py::keep_alive<0, 1>(), // see the init above
          "grid"_a,
          "dim_range"_a = XT::Grid::bindings::Dimension<r>());
    else
      m.def(
          FactoryName.c_str(),
          [](XT::Grid::GridProvider<G>& grid, const XT::Grid::bindings::Dimension<r>&) {
            return new type(grid.leaf_view()); // Otherwise we get an error here!
          },
          py::keep_alive<0, 1>(), // see the init above
          "grid"_a,
          "dim_range"_a);

    return c;
  } // ... bind(...)
}; // class FiniteVolumeSpaceBinding


/**
 * \brief Binds FiniteVolumeSpaceBinding<SpaceType, ...> for every grid the build instantiates.
 *
 * bind_vector_valued has to stay false for a space that is only implemented for r == 1: the `if constexpr` below is
 * what keeps the vector-valued instantiation from being compiled at all in that case (it would hit the
 * "Untested for these dimensions!" static_assert on the space's primary template).
 */
template <template <class, size_t, size_t, class> class SpaceType,
          bool bind_vector_valued,
          class GridTypes = Dune::XT::Grid::bindings::AvailableGridTypes>
struct FiniteVolumeSpaceBinding_for_all_grids
{
  using G = Dune::XT::Common::tuple_head_t<GridTypes>;
  using GV = typename G::LeafGridView;
  static const constexpr size_t d = G::dimension;

  static void bind(pybind11::module& m, const std::string& class_id)
  {
    using Dune::XT::Grid::bindings::grid_name;

    FiniteVolumeSpaceBinding<SpaceType, GV>::bind(m, grid_name<G>::value(), class_id);
    if constexpr (bind_vector_valued)
      if (d > 1)
        FiniteVolumeSpaceBinding<SpaceType, GV, d>::bind(m, grid_name<G>::value(), class_id);
    // add your extra dimensions here
    // ...
    FiniteVolumeSpaceBinding_for_all_grids<SpaceType, bind_vector_valued, Dune::XT::Common::tuple_tail_t<GridTypes>>::
        bind(m, class_id);
  }
};

template <template <class, size_t, size_t, class> class SpaceType, bool bind_vector_valued>
struct FiniteVolumeSpaceBinding_for_all_grids<SpaceType, bind_vector_valued, Dune::XT::Common::tuple_null_type>
{
  static void bind(pybind11::module& /*m*/, const std::string& /*class_id*/) {}
};


} // namespace bindings
} // namespace GDT
} // namespace Dune

#endif // PYTHON_DUNE_GDT_SPACES_FINITE_VOLUME_BINDING_HH
