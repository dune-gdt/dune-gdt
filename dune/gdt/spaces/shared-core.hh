// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)

/**
 * \file  shared-core.hh
 * \brief Common base for spaces whose immutable state lives in a core shared between copies.
 **/
#ifndef DUNE_GDT_SPACES_SHARED_CORE_HH
#define DUNE_GDT_SPACES_SHARED_CORE_HH

#include <memory>
#include <utility>

#include <dune/gdt/spaces/interface.hh>

namespace Dune {
namespace GDT {
namespace internal {


/**
 * \brief Base for spaces whose grid view, finite element family and lazily created mapper and basis live in a Core
 *        that is shared between all copies of a space.
 *
 * Since mapper and basis reference the core's grid view and finite elements, the core's address has to be stable
 * anyway; sharing it makes copy() O(1) instead of a full mapper rebuild over the whole grid view (review A3/C7/D4),
 * and moving a space no longer leaves mapper and basis dangling. update_after_adapt() (via create_or_update_core())
 * updates the shared core in place, so all copies stay consistent with the adapted grid.
 */
template <class GV, size_t r, size_t rC, class R, class FiniteElementFamily, class MapperImpl, class BasisImpl>
class SpaceWithSharedCore : public SpaceInterface<GV, r, rC, R>
{
  using BaseType = SpaceInterface<GV, r, rC, R>;

public:
  using typename BaseType::GlobalBasisType;
  using typename BaseType::GridViewType;
  using typename BaseType::LocalFiniteElementFamilyType;
  using typename BaseType::MapperType;

protected:
  struct Core
  {
    template <class... FamilyArgs>
    Core(GridViewType grd_vw, const int order, FamilyArgs&&... family_args)
      : grid_view(std::move(grd_vw))
      , fe_order(order)
      , local_finite_elements(std::make_unique<const FiniteElementFamily>(std::forward<FamilyArgs>(family_args)...))
    {
    }

    const GridViewType grid_view;
    const int fe_order;
    std::unique_ptr<const FiniteElementFamily> local_finite_elements;
    std::unique_ptr<MapperImpl> mapper;
    std::unique_ptr<BasisImpl> basis;
  }; // struct Core

  template <class... FamilyArgs>
  SpaceWithSharedCore(GridViewType grd_vw,
                      const int order,
                      const std::string& logging_prefix,
                      const std::array<bool, 3>& logging_state,
                      FamilyArgs&&... family_args)
    : BaseType(logging_prefix, logging_state)
    , core_(std::make_shared<Core>(std::move(grd_vw), order, std::forward<FamilyArgs>(family_args)...))
  {
  }

  /// \note Shares the core (grid view, finite elements, mapper and basis) with other, \sa Core.
  SpaceWithSharedCore(const SpaceWithSharedCore&) = default;

  SpaceWithSharedCore(SpaceWithSharedCore&&) noexcept = default;

  /**
   * Creates mapper and basis on the first call, updates them (in the shared core, i.e. for all copies of the space)
   * on subsequent calls; extra_mapper_args are only used on creation. To be called from update_after_adapt().
   */
  template <class... ExtraMapperArgs>
  void create_or_update_core(ExtraMapperArgs&&... extra_mapper_args)
  {
    if (core_->mapper)
      core_->mapper->update_after_adapt();
    else
      core_->mapper = std::make_unique<MapperImpl>(core_->grid_view,
                                                   *core_->local_finite_elements,
                                                   core_->fe_order,
                                                   std::forward<ExtraMapperArgs>(extra_mapper_args)...);
    if (core_->basis)
      core_->basis->update_after_adapt();
    else
      core_->basis = std::make_unique<BasisImpl>(core_->grid_view, *core_->local_finite_elements, core_->fe_order);
    this->create_communicator();
  } // ... create_or_update_core(...)

  int fe_order() const
  {
    return core_->fe_order;
  }

public:
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

protected:
  std::shared_ptr<Core> core_;
}; // class SpaceWithSharedCore


} // namespace internal
} // namespace GDT
} // namespace Dune

#endif // DUNE_GDT_SPACES_SHARED_CORE_HH
