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
class DiscontinuousLagrangeSpace : public SpaceInterface<GV, r, 1, R>
{
  using ThisType = DiscontinuousLagrangeSpace;
  using BaseType = SpaceInterface<GV, r, 1, R>;

public:
  using BaseType::d;
  using typename BaseType::D;
  using typename BaseType::GlobalBasisType;
  using typename BaseType::GridViewType;
  using typename BaseType::LocalFiniteElementFamilyType;
  using typename BaseType::MapperType;

private:
  using MapperImplementation = DiscontinuousMapper<GridViewType, LocalFiniteElementFamilyType>;
  using GlobalBasisImplementation = DefaultGlobalBasis<GridViewType, r, 1, R>;

  /**
   * Holds everything the read-only interface of this space exposes. The core is shared between all copies of a space
   * (mapper and basis reference the core's grid view and finite elements, so its address has to be stable anyway),
   * which makes copy() O(1) instead of a full mapper rebuild over the whole grid view (review A3/C7/D4).
   */
  struct Core
  {
    Core(GridViewType grd_vw, const int ord)
      : grid_view(std::move(grd_vw))
      , order(ord)
      , local_finite_elements(std::make_unique<const LocalLagrangeFiniteElementFamily<D, d, R, r>>())
    {
    }

    const GridViewType grid_view;
    const int order;
    std::unique_ptr<const LocalLagrangeFiniteElementFamily<D, d, R, r>> local_finite_elements;
    std::unique_ptr<MapperImplementation> mapper;
    std::unique_ptr<GlobalBasisImplementation> basis;
  }; // struct Core

public:
  DiscontinuousLagrangeSpace(GridViewType grd_vw, const int order = 1, const bool dimws_glbl_mppng = false)
    : BaseType()
    , dimwise_global_mapping((r == 1) ? false : dimws_glbl_mppng) // does not make sense in the scalar case
    , core_(std::make_shared<Core>(std::move(grd_vw), order))
  {
    this->update_after_adapt();
  }

  /// \note Shares the core (grid view, finite elements, mapper and basis) with other, \sa Core.
  DiscontinuousLagrangeSpace(const ThisType&) = default;

  DiscontinuousLagrangeSpace(ThisType&&) noexcept = default;

  ThisType& operator=(const ThisType&) = delete;

  ThisType& operator=(ThisType&&) = delete;

  BaseType* copy() const override final
  {
    return new ThisType(*this);
  }

  const GridViewType& grid_view() const override final
  {
    return core_->grid_view;
  }

  const MapperType& mapper() const override final
  {
    assert(core_->mapper && "This must not happen!");
    return *core_->mapper;
  }

  const GlobalBasisType& basis() const override final
  {
    assert(core_->basis && "This must not happen!");
    return *core_->basis;
  }

  const LocalFiniteElementFamilyType& finite_elements() const override final
  {
    return *core_->local_finite_elements;
  }

  SpaceType type() const override final
  {
    return SpaceType::discontinuous_lagrange;
  }

  int min_polorder() const override final
  {
    return core_->order;
  }

  int max_polorder() const override final
  {
    return core_->order;
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

  /// \note Updates the shared core, i.e. all copies of this space see the updated mapper and basis.
  void update_after_adapt() override final
  {
    // create/update mapper ...
    if (core_->mapper)
      core_->mapper->update_after_adapt();
    else
      core_->mapper = std::make_unique<MapperImplementation>(
          core_->grid_view, *core_->local_finite_elements, core_->order, dimwise_global_mapping);
    // ... and basis
    if (core_->basis)
      core_->basis->update_after_adapt();
    else
      core_->basis =
          std::make_unique<GlobalBasisImplementation>(core_->grid_view, *core_->local_finite_elements, core_->order);
    this->create_communicator();
  } // ... update_after_adapt(...)

public:
  const bool dimwise_global_mapping;

private:
  std::shared_ptr<Core> core_;
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
