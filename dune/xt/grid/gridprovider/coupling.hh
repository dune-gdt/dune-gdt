// This file is part of the dune-gdt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-gdt
// Copyright 2010-2021 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Tim Keil       (2020 - 2021)
//   Tobias Leibner (2021)
//
// This file is part of the dune-xt project:

#ifndef DUNE_XT_GRID_PROVIDER_COUPLING_HH
#define DUNE_XT_GRID_PROVIDER_COUPLING_HH

#include <memory>
#include <set>
#include <string>

#include <dune/xt/common/memory.hh>
#include <dune/xt/grid/type_traits.hh>

namespace Dune::XT::Grid {


template <class CouplingGridViewImp>
class CouplingGridProvider
{
  static_assert(is_grid<typename CouplingGridViewImp::GridGlueType::MacroGridType>::value);
  static_assert(is_grid<typename CouplingGridViewImp::GridGlueType::LocalGridType>::value);

public:
  using ThisType = CouplingGridProvider<CouplingGridViewImp>;

  using CouplingGridViewType = CouplingGridViewImp;

  static std::string static_id()
  {
    return "xt.grid.couplinggridprovider";
  }

  CouplingGridProvider(const CouplingGridViewType view)
    : view_(view)
  {}

  CouplingGridProvider(const ThisType& other) = default;

  const CouplingGridViewType& coupling_view() const
  {
    return view_;
  }

private:
  const CouplingGridViewType view_;
}; // class CouplingGridProvider


} // namespace Dune::XT::Grid


#endif // DUNE_XT_GRID_PROVIDER_COUPLING_HH
