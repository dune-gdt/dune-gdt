// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2020)

/**
 * \file  finite-volume.hh
 * \brief Finite volume skeleton space (one constant DoF per intersection) and its factory functions.
 **/
#ifndef DUNE_GDT_SPACES_SKELETON_FINITE_VOLUME_HH
#define DUNE_GDT_SPACES_SKELETON_FINITE_VOLUME_HH

#include <dune/grid/common/rangegenerators.hh>

#include <dune/xt/common/type_traits.hh>
#include <dune/xt/grid/type_traits.hh>

#include <dune/gdt/spaces/basis/finite-volume.hh>
#include <dune/gdt/spaces/mapper/finite-volume-skeleton.hh>
#include <dune/gdt/spaces/interface.hh>


namespace Dune {
namespace GDT {


template <class GV, size_t r = 1, size_t rC = 1, class R = double>
class FiniteVolumeSkeletonSpace
{
  static_assert(Dune::AlwaysFalse<GV>::value, "Untested for these dimensions!");
};


/**
 * \brief Finite volume skeleton space of scalar (r == rC == 1), piecewise constant discrete functions living on the
 *        grid intersections.
 */
template <class GV, class R>
class FiniteVolumeSkeletonSpace<GV, 1, 1, R> : public SpaceInterface<GV, 1, 1, R>
{
  using ThisType = FiniteVolumeSkeletonSpace;
  using BaseType = SpaceInterface<GV, 1, 1, R>;

public:
  using BaseType::d;
  using typename BaseType::D;
  using typename BaseType::ElementType;
  using typename BaseType::G;
  using typename BaseType::GlobalBasisType;
  using typename BaseType::GridViewType;
  using typename BaseType::LocalFiniteElementFamilyType;
  using typename BaseType::MapperType;

private:
  using MapperImplementation = FiniteVolumeSkeletonMapper<GridViewType, 1, 1>;
  using GlobalBasisImplementation = FiniteVolumeGlobalBasis<GridViewType, 1, R>;

  /**
   * Holds everything the read-only interface of this space exposes. The core is shared between all copies of a space
   * (mapper and basis reference the core's grid view, so its address has to be stable anyway), which makes copy()
   * O(1) instead of a full mapper rebuild over the whole grid view (review A3/C7/D4).
   */
  struct Core
  {
    Core(GridViewType grd_vw)
      : grid_view(std::move(grd_vw))
      , local_finite_elements(std::make_unique<const LocalLagrangeFiniteElementFamily<D, d, R, 1>>())
      , mapper(grid_view)
      , basis(grid_view)
    {
    }

    const GridViewType grid_view;
    std::unique_ptr<const LocalLagrangeFiniteElementFamily<D, d, R, 1>> local_finite_elements;
    MapperImplementation mapper;
    GlobalBasisImplementation basis;
  }; // struct Core

public:
  FiniteVolumeSkeletonSpace(GridViewType grd_vw)
    : BaseType()
    , core_(std::make_shared<Core>(std::move(grd_vw)))
  {
    this->update_after_adapt();
  }

  /// \note Shares the core (grid view, finite elements, mapper and basis) with other, \sa Core.
  FiniteVolumeSkeletonSpace(const ThisType&) = default;

  FiniteVolumeSkeletonSpace(ThisType&&) noexcept = default;

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
    return core_->mapper;
  }

  const GlobalBasisType& basis() const override final
  {
    return core_->basis;
  }

  const LocalFiniteElementFamilyType& finite_elements() const override final
  {
    DUNE_THROW(Exceptions::space_error, "Not implemented yet for skeleton space!");
    return *core_->local_finite_elements;
  }

  SpaceType type() const override final
  {
    return SpaceType::finite_volume_skeleton;
  }

  int min_polorder() const override final
  {
    return 0;
  }

  int max_polorder() const override final
  {
    return 0;
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

  void restrict_to(
      const ElementType& /*element*/,
      PersistentContainer<G, std::pair<DynamicVector<R>, DynamicVector<R>>>& /*persistent_data*/) const override final
  {
    DUNE_THROW(Exceptions::space_error, "Not implemented yet for skeleton space!");
  }

  /// \note Updates the shared core, i.e. all copies of this space see the updated mapper and basis.
  void update_after_adapt() override final
  {
    // update mapper and basis
    core_->mapper.update_after_adapt();
    core_->basis.update_after_adapt();
    this->create_communicator();
  }

  using BaseType::prolong_onto;

  void prolong_onto(const ElementType& /*element*/,
                    const PersistentContainer<G, std::pair<DynamicVector<R>, DynamicVector<R>>>& /*persistent_data*/,
                    DynamicVector<R>& /*element_data*/) const override final
  {
    DUNE_THROW(Exceptions::space_error, "Not implemented yet for skeleton space!");
  }

private:
  std::shared_ptr<Core> core_;
}; // class FiniteVolumeSkeletonSpace< ..., 1, 1 >


/**
 * \brief Creates a FiniteVolumeSkeletonSpace with given range dimensions and range field type over a grid view.
 */
template <size_t r, size_t rC, class R, class GV>
FiniteVolumeSkeletonSpace<GV, r, rC, R> make_finite_volume_skeleton_space(const GV& grid_view)
{
  return FiniteVolumeSkeletonSpace<GV, r, rC, R>(grid_view);
}

/**
 * \brief Creates a FiniteVolumeSkeletonSpace with given range dimensions and double range field over a grid view.
 */
template <size_t r, size_t rC, class GV>
FiniteVolumeSkeletonSpace<GV, r, rC, double> make_finite_volume_skeleton_space(const GV& grid_view)
{
  return FiniteVolumeSkeletonSpace<GV, r, rC, double>(grid_view);
}

/**
 * \brief Creates a scalar-columns FiniteVolumeSkeletonSpace with given range dimension and double range field over a
 *        grid view.
 */
template <size_t r, class GV>
FiniteVolumeSkeletonSpace<GV, r, 1, double> make_finite_volume_skeleton_space(const GV& grid_view)
{
  return FiniteVolumeSkeletonSpace<GV, r, 1, double>(grid_view);
}

/**
 * \brief Creates a scalar FiniteVolumeSkeletonSpace with double range field over a grid view.
 */
template <class GV>
FiniteVolumeSkeletonSpace<GV, 1, 1, double> make_finite_volume_skeleton_space(const GV& grid_view)
{
  return FiniteVolumeSkeletonSpace<GV, 1, 1, double>(grid_view);
}


} // namespace GDT
} // namespace Dune

#endif // DUNE_GDT_SPACES_SKELETON_FINITE_VOLUME_HH
