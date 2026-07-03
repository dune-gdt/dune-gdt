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
class ContinuousLagrangeSpace : public SpaceInterface<GV, r, 1, R>
{
  using ThisType = ContinuousLagrangeSpace;
  using BaseType = SpaceInterface<GV, r, 1, R>;

public:
  using BaseType::d;
  using typename BaseType::D;
  using typename BaseType::GlobalBasisType;
  using typename BaseType::GridViewType;
  using typename BaseType::LocalFiniteElementFamilyType;
  using typename BaseType::MapperType;

private:
  using MapperImplementation = ContinuousMapper<GridViewType, LocalFiniteElementFamilyType>;
  using GlobalBasisImplementation = DefaultGlobalBasis<GridViewType, r, 1, R>;

  /**
   * Holds everything the read-only interface of this space exposes. The core is shared between all copies of a space
   * (mapper and basis reference the core's grid view and finite elements, so its address has to be stable anyway),
   * which makes copy() O(1) instead of a full mapper rebuild over the whole grid view (review A3/C7/D4).
   */
  struct Core
  {
    Core(GridViewType grd_vw, const int order)
      : grid_view(std::move(grd_vw))
      , fe_order(order)
      , local_finite_elements(std::make_unique<LocalLagrangeFiniteElementFamily<D, d, R, r>>())
    {
    }

    const GridViewType grid_view;
    const int fe_order;
    std::unique_ptr<const LocalLagrangeFiniteElementFamily<D, d, R, r>> local_finite_elements;
    std::unique_ptr<MapperImplementation> mapper;
    std::unique_ptr<GlobalBasisImplementation> basis;
  }; // struct Core

public:
  ContinuousLagrangeSpace(GridViewType grd_vw,
                          const int order,
                          const std::string& logging_prefix = "",
                          const std::array<bool, 3>& logging_state = XT::Common::default_logger_state())
    : BaseType(logging_prefix.empty() ? "ContinuousLagrangeSpace" : logging_prefix, logging_state)
    , core_(std::make_shared<Core>(std::move(grd_vw), order))
  {
    LOG_(info) << "ContinuousLagrangeSpace(order=" << order << ")" << std::endl;
    this->update_after_adapt();
  }

  /// \note Shares the core (grid view, finite elements, mapper and basis) with other, \sa Core.
  ContinuousLagrangeSpace(const ThisType&) = default;

  ContinuousLagrangeSpace(ThisType&&) noexcept = default;

  ThisType& operator=(const ThisType&) = delete;

  ThisType& operator=(ThisType&&) = delete;

  BaseType* copy() const final
  {
    return new ThisType(*this);
  }

  const GridViewType& grid_view() const final
  {
    return core_->grid_view;
  }

  const MapperType& mapper() const final
  {
    assert(core_->mapper && "This must not happen!");
    return *core_->mapper;
  }

  const GlobalBasisType& basis() const final
  {
    assert(core_->basis && "This must not happen!");
    return *core_->basis;
  }

  const LocalFiniteElementFamilyType& finite_elements() const final
  {
    return *core_->local_finite_elements;
  }

  SpaceType type() const final
  {
    return SpaceType::continuous_lagrange;
  }

  int min_polorder() const final
  {
    return core_->fe_order;
  }

  int max_polorder() const final
  {
    return core_->fe_order;
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

  /// \note Updates the shared core, i.e. all copies of this space see the updated mapper and basis.
  void update_after_adapt() final
  {
    // check: the mapper does not work for non-conforming intersections
    if (d == 3 && core_->grid_view.indexSet().types(0).size() != 1)
      DUNE_THROW(Exceptions::space_error,
                 "in ContinuousLagrangeSpace: non-conforming intersections are not (yet) "
                 "supported, and more than one element type in 3d leads to non-conforming intersections!");
    // create/update mapper ...
    if (core_->mapper)
      core_->mapper->update_after_adapt();
    else
      core_->mapper =
          std::make_unique<MapperImplementation>(core_->grid_view, *core_->local_finite_elements, core_->fe_order);
    // ... and basis
    if (core_->basis)
      core_->basis->update_after_adapt();
    else
      core_->basis =
          std::make_unique<GlobalBasisImplementation>(core_->grid_view, *core_->local_finite_elements, core_->fe_order);
    this->create_communicator();
  } // ... update_after_adapt(...)

private:
  std::shared_ptr<Core> core_;
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
