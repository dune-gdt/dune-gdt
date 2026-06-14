// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2017, 2019)
//   René Fritze     (2018 - 2020)
//   Tobias Leibner  (2020)

/// \file
/// \brief Provides adapters yielding a temporary grid view from a grid layer that may be a view or a part.

#ifndef DUNE_XT_GRID_VIEW_FROM_PART_HH
#define DUNE_XT_GRID_VIEW_FROM_PART_HH

#include <type_traits>

#include <dune/common/typetraits.hh>

#include <dune/xt/common/type_traits.hh>
#include <dune/xt/grid/type_traits.hh>

namespace Dune::XT::Grid {


/// \brief Provides const access to a grid view obtained from a grid layer, which may be a grid view or a grid part.
template <class GridLayerType>
class TemporaryConstView
{
  static_assert(is_layer<GridLayerType>::value);

  template <bool view = is_view<GridLayerType>::value, bool part = is_part<GridLayerType>::value, bool anything = true>
  struct const_storage
  {
    static_assert(AlwaysFalse<typename Common::dependent<bool>::_typename<view>::type>::value,
                  "This case is not covered (yet?)!");
  };

  template <bool anything>
  struct const_storage<true, false, anything>
  {
    using type = GridLayerType;

    explicit const_storage(const GridLayerType& grid_view)
      : value(grid_view)
    {
    }

    const type& value;
  };

  template <bool anything>
  struct const_storage<false, true, anything>
  {
    using type = typename GridLayerType::GridViewType;

    explicit const_storage(const GridLayerType& grid_part)
      : grid_part_(grid_part)
      , value(grid_part_)
    {
    }

    const GridLayerType& grid_part_;
    const type value;
  };

public:
  using type = typename const_storage<>::type;

  explicit TemporaryConstView(const GridLayerType& grid_layer)
    : const_storage_(grid_layer)
  {
  }

  const type& access() const
  {
    return const_storage_.value;
  }

private:
  const const_storage<> const_storage_;
}; // class TemporaryConstView


/// \brief Provides mutable access to a grid view obtained from a grid layer, which may be a grid view or a grid part.
template <class GridLayerType>
class TemporaryView : public TemporaryConstView<GridLayerType>
{
  template <bool view = is_view<GridLayerType>::value, bool part = is_part<GridLayerType>::value, bool anything = true>
  struct storage
  {
    static_assert(AlwaysFalse<typename Common::dependent<bool>::_typename<view>::type>::value,
                  "This case is not covered (yet?)!");
  };

  template <bool anything>
  struct storage<true, false, anything>
  {
    using type = GridLayerType;

    explicit storage(GridLayerType& grid_view)
      : value(grid_view)
    {
    }

    type& value;
  };

  template <bool anything>
  struct storage<false, true, anything>
  {
    using type = typename GridLayerType::GridViewType;

    explicit storage(GridLayerType& grid_part)
      : value(grid_part)
    {
    }

    type value;
  };

  using BaseType = TemporaryConstView<GridLayerType>;

public:
  using type = typename storage<>::type;

  explicit TemporaryView(GridLayerType& grid_layer)
    : BaseType(grid_layer)
    , storage_(grid_layer)
  {
  }

  using BaseType::access;

  type& access()
  {
    return storage_.value;
  }

private:
  storage<> storage_;
}; // class TemporaryView


/// \brief Creates a TemporaryConstView from a const grid layer.
template <class GridLayerType>
TemporaryConstView<GridLayerType> make_tmp_view(const GridLayerType& grid_layer)
{
  return TemporaryConstView<GridLayerType>(grid_layer);
}

/// \brief Creates a TemporaryView from a mutable grid layer.
template <class GridLayerType>
TemporaryView<GridLayerType> make_tmp_view(GridLayerType& grid_layer)
{
  return TemporaryView<GridLayerType>(grid_layer);
}


} // namespace Dune::XT::Grid

#endif // DUNE_XT_GRID_VIEW_FROM_PART_HH
