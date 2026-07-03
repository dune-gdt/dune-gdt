// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2019)

/**
 * \file  continuous-flattop.hh
 * \brief Continuous H^1-conforming space based on flat-top finite elements.
 **/
#ifndef DUNE_GDT_SPACES_H1_CONTINUOUS_FLATTOP_HH
#define DUNE_GDT_SPACES_H1_CONTINUOUS_FLATTOP_HH

#include <memory>
// #include <vector>

// #include <dune/common/typetraits.hh>

// #include <dune/geometry/type.hh>

#include <dune/grid/common/gridview.hh>

// #include <dune/xt/common/exceptions.hh>
// #include <dune/xt/grid/type_traits.hh>

#include <dune/gdt/local/finite-elements/flattop.hh>
#include <dune/gdt/spaces/basis/default.hh>
#include <dune/gdt/spaces/mapper/continuous.hh>
#include <dune/gdt/spaces/interface.hh>
#include <dune/gdt/spaces/shared-core.hh>

namespace Dune {
namespace GDT {


/**
 * \sa make_local_lagrange_finite_element
 */
template <class GV, size_t r = 1, class R = double>
class ContinuousFlatTopSpace
  : public internal::SpaceWithSharedCore<
        GV,
        r,
        1,
        R,
        LocalFlatTopFiniteElementFamily<typename GV::ctype, GV::dimension, R, r>,
        ContinuousMapper<GV, LocalFiniteElementFamilyInterface<typename GV::ctype, GV::dimension, R, r, 1>>,
        DefaultGlobalBasis<GV, r, 1, R>>
{
  using ThisType = ContinuousFlatTopSpace;
  using BaseType = internal::SpaceWithSharedCore<
      GV,
      r,
      1,
      R,
      LocalFlatTopFiniteElementFamily<typename GV::ctype, GV::dimension, R, r>,
      ContinuousMapper<GV, LocalFiniteElementFamilyInterface<typename GV::ctype, GV::dimension, R, r, 1>>,
      DefaultGlobalBasis<GV, r, 1, R>>;

public:
  using BaseType::d;
  using typename BaseType::D;
  using typename BaseType::GridViewType;

  ContinuousFlatTopSpace(GridViewType grd_vw, const int fe_order, const D& overlap = 0.5)
    : BaseType(std::move(grd_vw), fe_order, "ContinuousFlatTopSpace", XT::Common::default_logger_state(), overlap)
  {
    this->update_after_adapt();
  }

  ContinuousFlatTopSpace(const ThisType&) = default;

  ContinuousFlatTopSpace(ThisType&&) = default;

  ThisType& operator=(const ThisType&) = delete;
  ThisType& operator=(ThisType&&) = delete;

  BaseType* copy() const override final
  {
    return new ThisType(*this);
  }

  SpaceType type() const override final
  {
    return SpaceType::continuous_lagrange;
  }

  int min_polorder() const override final
  {
    return this->fe_order();
  }

  int max_polorder() const override final
  {
    return this->fe_order() + 1;
  }

  bool continuous(const int diff_order) const override final
  {
    return diff_order == 0;
  }

  bool continuous_normal_components() const override final
  {
    return true;
  }

  bool is_lagrangian() const override final
  {
    return true;
  }

  void update_after_adapt() override final
  {
    // check: the mapper does not work for non-conforming intersections
    if (d == 3 && this->grid_view().indexSet().types(0).size() != 1)
      DUNE_THROW(Exceptions::space_error,
                 "in ContinuousFlatTopSpace: non-conforming intersections are not (yet) "
                 "supported, and more than one element type in 3d leads to non-conforming intersections!");
    this->create_or_update_core();
  }
}; // class ContinuousFlatTopSpace


/**
 * \sa ContinuousFlatTopSpace
 */
template <size_t r, class GV, class R = double>
ContinuousFlatTopSpace<GV, r, R>
make_continuous_flattop_space(GV grid_view, const int order, const double& overlap = 0.5)
{
  return ContinuousFlatTopSpace<GV, r, R>(grid_view, order, overlap);
}


/**
 * \sa ContinuousFlatTopSpace
 */
template <class GV, class R = double>
ContinuousFlatTopSpace<GV, 1, R>
make_continuous_flattop_space(GV grid_view, const int order, const double& overlap = 0.5)
{
  return ContinuousFlatTopSpace<GV, 1, R>(grid_view, order, overlap);
}


} // namespace GDT
} // namespace Dune

#endif // DUNE_GDT_SPACES_H1_CONTINUOUS_FLATTOP_HH
