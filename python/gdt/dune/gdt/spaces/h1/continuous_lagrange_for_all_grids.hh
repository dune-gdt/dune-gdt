// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2020)

#ifndef PYTHON_DUNE_GDT_SPACES_H1_CONTINUOUS_LAGRANGE_FOR_ALL_GRIDS_HH
#define PYTHON_DUNE_GDT_SPACES_H1_CONTINUOUS_LAGRANGE_FOR_ALL_GRIDS_HH


#include <python/xt/dune/xt/grid/grids.bindings.hh>

#include "continuous-lagrange.hh"


template <class GridTypes = Dune::XT::Grid::bindings::AvailableGridTypes>
struct ContinuousLagrangeSpace_for_all_grids
{
  using G = Dune::XT::Common::tuple_head_t<GridTypes>;
  using GV = typename G::LeafGridView;

  static void bind(pybind11::module& m)
  {
    using Dune::GDT::bindings::ContinuousLagrangeSpace;
    using Dune::XT::Grid::bindings::grid_name;

    ContinuousLagrangeSpace<GV>::bind(m, grid_name<G>::value());
    // add your extra dimensions here
    // ...
    ContinuousLagrangeSpace_for_all_grids<Dune::XT::Common::tuple_tail_t<GridTypes>>::bind(m);
  }
};

template <>
struct ContinuousLagrangeSpace_for_all_grids<Dune::XT::Common::tuple_null_type>
{
  static void bind(pybind11::module& /*m*/) {}
};


#endif // PYTHON_DUNE_GDT_SPACES_H1_CONTINUOUS_LAGRANGE_FOR_ALL_GRIDS_HH
