// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2018)
//   René Fritze     (2018)

/**
 * \file  continuous-lagrange.hh
 * \brief Continuous H^1-conforming Lagrange finite element space.
 **/
#ifndef DUNE_GDT_SPACES_H1_CONTINUOUS_LAGRANGE_HH
#define DUNE_GDT_SPACES_H1_CONTINUOUS_LAGRANGE_HH

#include <memory>
#include <vector>

#include <dune/common/typetraits.hh>

#include <dune/geometry/type.hh>

#include <dune/grid/common/gridview.hh>

#include <dune/xt/common/exceptions.hh>
#include <dune/xt/grid/type_traits.hh>

#include <dune/gdt/local/finite-elements/lagrange.hh>
#include <dune/gdt/spaces/basis/default.hh>
#include <dune/gdt/spaces/mapper/continuous.hh>
#include <dune/gdt/spaces/interface.hh>
#include <dune/gdt/spaces/shared-core.hh>

namespace Dune {
namespace GDT {


/**
 * The following dimensions/orders/elements are tested to work:
 *
 * - 1d: orders 1, 2 work
 * - 2d: orders 1, 2 work on simplices, cubes and mixed simplices and cubes
 * - 3d: orders 1, 2 work on simplices, cubes, prisms
 *
 * The following dimensions/orders/elements are tested to fail:
 *
 * - 3d: pyramids (jacobians seem to be incorrect)
 * - 3d: mixed simplices and cubes (intersections are non-conforming)
 *
 * \sa make_local_lagrange_finite_element
 */
template <class GV, size_t r = 1, class R = double>
class ContinuousLagrangeSpace
  : public internal::SpaceWithSharedCore<
        GV,
        r,
        1,
        R,
        LocalLagrangeFiniteElementFamily<typename GV::ctype, GV::dimension, R, r>,
        ContinuousMapper<GV, LocalFiniteElementFamilyInterface<typename GV::ctype, GV::dimension, R, r, 1>>,
        DefaultGlobalBasis<GV, r, 1, R>>
{
  using ThisType = ContinuousLagrangeSpace;
  using BaseType = internal::SpaceWithSharedCore<
      GV,
      r,
      1,
      R,
      LocalLagrangeFiniteElementFamily<typename GV::ctype, GV::dimension, R, r>,
      ContinuousMapper<GV, LocalFiniteElementFamilyInterface<typename GV::ctype, GV::dimension, R, r, 1>>,
      DefaultGlobalBasis<GV, r, 1, R>>;

public:
  using BaseType::d;
  using typename BaseType::GridViewType;

  ContinuousLagrangeSpace(GridViewType grd_vw,
                          const int order,
                          const std::string& logging_prefix = "",
                          const std::array<bool, 3>& logging_state = XT::Common::default_logger_state())
    : BaseType(
          std::move(grd_vw), order, logging_prefix.empty() ? "ContinuousLagrangeSpace" : logging_prefix, logging_state)
  {
    LOG_(info) << "ContinuousLagrangeSpace(order=" << order << ")" << std::endl;
    this->update_after_adapt();
  }

  ContinuousLagrangeSpace(const ThisType&) = default;

  ContinuousLagrangeSpace(ThisType&&) noexcept = default;

  ThisType& operator=(const ThisType&) = delete;

  ThisType& operator=(ThisType&&) = delete;

  BaseType* copy() const final
  {
    return new ThisType(*this);
  }

  SpaceType type() const final
  {
    return SpaceType::continuous_lagrange;
  }

  int min_polorder() const final
  {
    return this->fe_order();
  }

  int max_polorder() const final
  {
    return this->fe_order();
  }

  bool continuous(const int diff_order) const final
  {
    return diff_order == 0;
  }

  bool continuous_normal_components() const final
  {
    return true;
  }

  bool is_lagrangian() const final
  {
    return true;
  }

  void update_after_adapt() final
  {
    // check: the mapper does not work for non-conforming intersections
    if (d == 3 && this->grid_view().indexSet().types(0).size() != 1)
      DUNE_THROW(Exceptions::space_error,
                 "in ContinuousLagrangeSpace: non-conforming intersections are not (yet) "
                 "supported, and more than one element type in 3d leads to non-conforming intersections!");
    this->create_or_update_core();
  }
}; // class ContinuousLagrangeSpace


/**
 * \sa ContinuousLagrangeSpace
 */
template <size_t r, class GV, class R = double>
ContinuousLagrangeSpace<GV, r, R> make_continuous_lagrange_space(GV grid_view, const int order)
{
  return ContinuousLagrangeSpace<GV, r, R>(grid_view, order);
}


/**
 * \sa ContinuousLagrangeSpace
 */
template <class GV, class R = double>
ContinuousLagrangeSpace<GV, 1, R> make_continuous_lagrange_space(GV grid_view, const int order)
{
  return ContinuousLagrangeSpace<GV, 1, R>(grid_view, order);
}


} // namespace GDT
} // namespace Dune

#endif // DUNE_GDT_SPACES_H1_CONTINUOUS_LAGRANGE_HH
