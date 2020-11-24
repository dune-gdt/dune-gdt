// This file is part of the dune-xt project:
//   https://github.com/dune-community/dune-xt
// Copyright 2009-2020 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:

#ifndef DUNE_XT_GRID_PROVIDER_COUPLING_HH
#define DUNE_XT_GRID_PROVIDER_COUPLING_HH

#include <memory>
#include <set>
#include <string>

#include <dune/xt/common/memory.hh>

namespace Dune::XT::Grid {


template <class CouplingGridViewImp>
class CouplingGridProvider
{
  static_assert(is_grid<typename CouplingGridViewImp::GridGlueType::MacroGridType>::value);
  static_assert(is_grid<typename CouplingGridViewImp::GridGlueType::LocalGridType>::value);

public:
  using ThisType = CouplingGridProvider<CouplingGridViewImp>;

  using CoulingGridViewType = CouplingGridViewImp;

  static const std::string static_id()
  {
    return "xt.grid.couplinggridprovider";
  }

  CouplingGridProvider(const CoulingGridViewType& view)
    : view_(view)
  {}

//  CouplingGridProvider(CoulingGridViewType view)
//    : view_(view)
//  {}

  CouplingGridProvider(const ThisType& other) = default;

  const CoulingGridViewType& coupling_view()
  {
    return view_.access();
  }

private:
  Dune::XT::Common::ConstStorageProvider<CoulingGridViewType> view_;
}; // class CouplingGridProvider


} // namespace Dune::XT::Grid


#endif // DUNE_XT_GRID_PROVIDER_COUPLING_HH
