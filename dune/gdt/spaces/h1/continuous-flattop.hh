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

namespace Dune {
namespace GDT {


/**
 * \sa make_local_lagrange_finite_element
 */
template <class GV, size_t r = 1, class R = double>
class ContinuousFlatTopSpace : public SpaceInterface<GV, r, 1, R>
{
  using ThisType = ContinuousFlatTopSpace;
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
    Core(GridViewType grd_vw, const int order, const D& ovrlp)
      : grid_view(std::move(grd_vw))
      , fe_order(order)
      , overlap(ovrlp)
      , local_finite_elements(std::make_unique<LocalFlatTopFiniteElementFamily<D, d, R, r>>(overlap))
    {
    }

    const GridViewType grid_view;
    const int fe_order;
    const D overlap;
    std::unique_ptr<const LocalFlatTopFiniteElementFamily<D, d, R, r>> local_finite_elements;
    std::unique_ptr<MapperImplementation> mapper;
    std::unique_ptr<GlobalBasisImplementation> basis;
  }; // struct Core

public:
  ContinuousFlatTopSpace(GridViewType grd_vw, const int fe_order, const D& overlap = 0.5)
    : core_(std::make_shared<Core>(std::move(grd_vw), fe_order, overlap))
  {
    this->update_after_adapt();
  }

  /// \note Shares the core (grid view, finite elements, mapper and basis) with other, \sa Core.
  ContinuousFlatTopSpace(const ThisType&) = default;

  ContinuousFlatTopSpace(ThisType&&) = default;

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
    return SpaceType::continuous_lagrange;
  }

  int min_polorder() const override final
  {
    return core_->fe_order;
  }

  int max_polorder() const override final
  {
    return core_->fe_order + 1;
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

  /// \note Updates the shared core, i.e. all copies of this space see the updated mapper and basis.
  void update_after_adapt() override final
  {
    // check: the mapper does not work for non-conforming intersections
    if (d == 3 && core_->grid_view.indexSet().types(0).size() != 1)
      DUNE_THROW(Exceptions::space_error,
                 "in ContinuousFlatTopSpace: non-conforming intersections are not (yet) "
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
