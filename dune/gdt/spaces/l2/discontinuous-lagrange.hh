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
 * \file  discontinuous-lagrange.hh
 * \brief Discontinuous L^2 Lagrange finite element space.
 **/
#ifndef DUNE_GDT_SPACES_L2_DISCONTINUOUS_LAGRANGE_HH
#define DUNE_GDT_SPACES_L2_DISCONTINUOUS_LAGRANGE_HH

#include <memory>
#include <vector>

#include <dune/common/typetraits.hh>

#include <dune/geometry/referenceelements.hh>
#include <dune/geometry/type.hh>

#include <dune/grid/common/capabilities.hh>
#include <dune/grid/common/rangegenerators.hh>

#include <dune/xt/common/exceptions.hh>
#include <dune/xt/common/numeric_cast.hh>

#include <dune/gdt/local/finite-elements/lagrange.hh>
#include <dune/gdt/spaces/basis/default.hh>
#include <dune/gdt/spaces/mapper/discontinuous.hh>
#include <dune/gdt/spaces/interface.hh>
#include <dune/gdt/spaces/shared-core.hh>

namespace Dune {
namespace GDT {


/**
 * The following dimensions/orders/elements are tested to work:
 *
 * - 1d: orders 0, ..., 18
 * - 2d: orders 0, ..., 10 work on simplices, cubes and mixed simplices and cubes
 * - 2d: orders 11, ..., 15 also work on simplices
 * - 3d: orders 0, ..., 7 work on simplices, cubes, prisms and mixed simplices and cubes
 * - 3d: orders 8, ..., 14 also work on simplices
 * - 3d: orders 8, 9 also work on prisms
 *
 * The following dimensions/orders/elements are tested to fail (basis matrix fails to invert):
 *
 * - 1d: orders > 18
 * - 2d: orders > 15 on simplices
 * - 2d: orders > 10 on cubes
 * - 3d: orders > 14 on simplices
 * - 3d: orders > 7 on cubes
 * - 3d: orders > 9 on prisms
 *
 * \sa make_local_lagrange_finite_element
 * \sa make_discontinuous_lagrange_space
 */
template <class GV, size_t r = 1, class R = double>
class DiscontinuousLagrangeSpace
  : public internal::SpaceWithSharedCore<
        GV,
        r,
        1,
        R,
        LocalLagrangeFiniteElementFamily<typename GV::ctype, GV::dimension, R, r>,
        DiscontinuousMapper<GV, LocalFiniteElementFamilyInterface<typename GV::ctype, GV::dimension, R, r, 1>>,
        DefaultGlobalBasis<GV, r, 1, R>>
{
  using ThisType = DiscontinuousLagrangeSpace;
  using BaseType = internal::SpaceWithSharedCore<
      GV,
      r,
      1,
      R,
      LocalLagrangeFiniteElementFamily<typename GV::ctype, GV::dimension, R, r>,
      DiscontinuousMapper<GV, LocalFiniteElementFamilyInterface<typename GV::ctype, GV::dimension, R, r, 1>>,
      DefaultGlobalBasis<GV, r, 1, R>>;

public:
  using typename BaseType::GridViewType;

  DiscontinuousLagrangeSpace(GridViewType grd_vw, const int order = 1, const bool dimws_glbl_mppng = false)
    : BaseType(std::move(grd_vw), order, "DiscontinuousLagrangeSpace", XT::Common::default_logger_state())
    , dimwise_global_mapping((r == 1) ? false : dimws_glbl_mppng) // does not make sense in the scalar case
  {
    this->update_after_adapt();
  }

  DiscontinuousLagrangeSpace(const ThisType&) = default;

  DiscontinuousLagrangeSpace(ThisType&&) noexcept = default;

  ThisType& operator=(const ThisType&) = delete;

  ThisType& operator=(ThisType&&) = delete;

  BaseType* copy() const override final
  {
    return new ThisType(*this);
  }

  SpaceType type() const override final
  {
    return SpaceType::discontinuous_lagrange;
  }

  int min_polorder() const override final
  {
    return this->fe_order();
  }

  int max_polorder() const override final
  {
    return this->fe_order();
  }

  bool continuous(const int /*diff_order*/) const override final
  {
    return false;
  }

  bool continuous_normal_components() const override final
  {
    return false;
  }

  bool is_lagrangian() const override final
  {
    return true;
  }

  void update_after_adapt() override final
  {
    this->create_or_update_core(dimwise_global_mapping);
  }

public:
  const bool dimwise_global_mapping;
}; // class DiscontinuousLagrangeSpace


/**
 * \sa DiscontinuousLagrangeSpace
 */
template <size_t r, class GV, class R = double>
DiscontinuousLagrangeSpace<GV, r, R>
make_discontinuous_lagrange_space(GV grid_view, const int order, const bool dimwise_global_mapping = false)
{
  return DiscontinuousLagrangeSpace<GV, r, R>(grid_view, order, dimwise_global_mapping);
}


/**
 * \sa DiscontinuousLagrangeSpace
 */
template <class GV, class R = double>
DiscontinuousLagrangeSpace<GV, 1, R>
make_discontinuous_lagrange_space(GV grid_view, const int order, const bool dimwise_global_mapping = false)
{
  return DiscontinuousLagrangeSpace<GV, 1, R>(grid_view, order, dimwise_global_mapping);
}


} // namespace GDT
} // namespace Dune

#endif // DUNE_GDT_SPACES_L2_DISCONTINUOUS_LAGRANGE_HH
