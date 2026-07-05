// This file is part of the dune-gdt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-gdt
// Copyright 2010-2021 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2017 - 2020)
//   René Fritze     (2018)
//   Tobias Leibner  (2019 - 2020)

/**
 * \file  raviart-thomas.hh
 * \brief H(div)-conforming Raviart-Thomas finite element space.
 **/
#ifndef DUNE_GDT_SPACES_HDIV_RAVIART_THOMAS_HH
#define DUNE_GDT_SPACES_HDIV_RAVIART_THOMAS_HH

#include <memory>
#include <vector>

#include <dune/common/typetraits.hh>

#include <dune/geometry/referenceelements.hh>
#include <dune/geometry/type.hh>

#include <dune/grid/common/capabilities.hh>
#include <dune/grid/common/rangegenerators.hh>

#include <dune/xt/common/exceptions.hh>
#include <dune/xt/common/numeric_cast.hh>

#include <dune/gdt/local/finite-elements/raviart-thomas.hh>
#include <dune/gdt/spaces/basis/raviart-thomas.hh>
#include <dune/gdt/spaces/mapper/continuous.hh>
#include <dune/gdt/spaces/mapper/finite-volume.hh>
#include <dune/gdt/spaces/interface.hh>

namespace Dune {
namespace GDT {


/**
 * The following dimensions/orders/elements are tested to work:
 *
 * - 1d: order 0 works
 * - 2d: order 0 works on simplices, cubes and mixed simplices and cubes
 * - 3d: order 0 work on simplices, cubes
 *
 * The following dimensions/orders/elements are tested to fail:
 *
 * - 3d: mixed simplices and cubes (the mapper cannot handle non-conforming intersections/the switches are not corect)
 */
template <class GV, class R = double>
class RaviartThomasSpace : public SpaceInterface<GV, GV::dimension, 1, R>
{
  using ThisType = RaviartThomasSpace;
  using BaseType = SpaceInterface<GV, GV::dimension, 1, R>;

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
  using MapperImplementation = ContinuousMapper<GV, LocalFiniteElementFamilyType>;
  using GlobalBasisImplementation = RaviartThomasGlobalBasis<GV, R>;

  /**
   * Holds everything the read-only interface of this space exposes. The core is shared between all copies of a space
   * (mapper and basis reference the core's grid view, finite elements and FE data, so its address has to be stable
   * anyway), which makes copy() O(1) instead of a full rebuild over the whole grid view (review A3/C7/D4).
   */
  struct Core
  {
    Core(GridViewType grd_vw,
         const int ord,
         const std::string& logging_prefix,
         const std::array<bool, 3>& logging_state)
      : grid_view(std::move(grd_vw))
      , order(ord)
      , local_finite_elements()
      , element_indices(grid_view)
      , fe_data()
      , mapper(grid_view, local_finite_elements, order)
      , basis(grid_view,
              order,
              local_finite_elements,
              element_indices,
              fe_data,
              logging_prefix.empty() ? "" : logging_prefix + "_basis",
              logging_state)
    {
    }

    const GridViewType grid_view;
    const int order;
    const LocalRaviartThomasFiniteElementFamily<D, d, R> local_finite_elements;
    FiniteVolumeMapper<GV> element_indices;
    std::vector<DynamicVector<R>> fe_data;
    MapperImplementation mapper;
    GlobalBasisImplementation basis;
  }; // struct Core

public:
  RaviartThomasSpace(GridViewType grd_vw,
                     const int order,
                     const std::string& logging_prefix = "",
                     const std::array<bool, 3>& logging_state = XT::Common::default_logger_state())
    : BaseType(logging_prefix.empty() ? "RtSpace" : logging_prefix, logging_state)
    , core_(std::make_shared<Core>(std::move(grd_vw), order, logging_prefix, logging_state))
  {
    LOG_(info) << "RaviartThomasSpace(order=" << order << ")" << std::endl;
    DUNE_THROW_IF(core_->order != 0, Exceptions::space_error, "Higher orders are not testet yet!");
    this->update_after_adapt();
    LOG_(debug) << "   element_indices.size() = " << core_->element_indices.size()
                << "\n   fe_data.size() = " << core_->fe_data.size() << std::endl;
  }

  /// \note Shares the core (grid view, finite elements, mapper and basis) with other, \sa Core.
  RaviartThomasSpace(const ThisType&) = default;

  RaviartThomasSpace(ThisType&&) noexcept = default;

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
    return core_->local_finite_elements;
  }

  SpaceType type() const override final
  {
    return SpaceType::raviart_thomas;
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
    return true;
  }

  bool is_lagrangian() const override final
  {
    return false;
  }

protected:
  /// \note Updates the shared core, i.e. all copies of this space see the updated mapper and basis.
  void update_after_adapt() override final
  {
    LOG_(info) << "update_after_adapt()" << std::endl;
    core_->element_indices.update_after_adapt();
    // check: the mapper does not work for non-conforming intersections
    if (d == 3 && core_->grid_view.indexSet().types(0).size() != 1)
      DUNE_THROW(Exceptions::space_error,
                 "in RaviartThomasSpace: non-conforming intersections are not (yet) "
                 "supported, and more than one element type in 3d leads to non-conforming intersections!");
    // compute scaling to ensure basis*integrationElementNormal = 1
    std::map<GeometryType, DynamicVector<R>> geometry_to_scaling_factors_map;
    for (const auto& geometry_type : core_->grid_view.indexSet().types(0)) {
      const auto& finite_element = core_->local_finite_elements.get(geometry_type, core_->order);
      const auto& reference_element = ReferenceElements<D, d>::general(geometry_type);
      const auto num_intersections = reference_element.size(1);
      geometry_to_scaling_factors_map.insert(std::make_pair(geometry_type, DynamicVector<R>(num_intersections, R(1.))));
      auto& scaling_factors = geometry_to_scaling_factors_map.at(geometry_type);
      const auto intersection_to_local_DoF_indices_map = finite_element.coefficients().local_key_indices(1);
      for (auto&& intersection_index : XT::Common::value_range(num_intersections)) {
        const auto& normal = reference_element.integrationOuterNormal(intersection_index);
        const auto& intersection_center = reference_element.position(intersection_index, 1);
        const auto shape_functions_evaluations = finite_element.basis().evaluate(intersection_center);
        for (auto&& local_DoF_index : intersection_to_local_DoF_indices_map[intersection_index])
          scaling_factors[intersection_index] /= shape_functions_evaluations[local_DoF_index] * normal;
      }
    }
    // compute switches (as signs of the scaling factor) to ensure continuity of the normal component (therefore we need
    // unique indices for codim 0  entities, which cannot be achieved by the grid layers index set for mixed grids)
    core_->fe_data.resize(core_->element_indices.size());
    for (auto&& entity : elements(core_->grid_view)) {
      const auto geometry_type = entity.type();
      const auto& finite_element = core_->local_finite_elements.get(geometry_type, core_->order);
      const auto& coeffs = finite_element.coefficients();
      const auto element_index = core_->element_indices.global_index(entity, 0);
      core_->fe_data[element_index] = geometry_to_scaling_factors_map.at(geometry_type);
      auto& local_switches = core_->fe_data[element_index];
      for (auto&& intersection : intersections(core_->grid_view, entity)) {
        if (intersection.neighbor() && element_index < core_->element_indices.global_index(intersection.outside(), 0)) {
          const auto intersection_index = XT::Common::numeric_cast<unsigned int>(intersection.indexInInside());
          for (size_t ii = 0; ii < coeffs.size(); ++ii) {
            const auto& local_key = coeffs.local_key(ii);
            const auto DoF_subentity_index = local_key.subEntity();
            if (local_key.codim() == 1 && DoF_subentity_index == intersection_index)
              local_switches[DoF_subentity_index] *= -1.;
          }
        }
      }
    }
    // update mapper, basis and communicator
    core_->mapper.update_after_adapt();
    core_->basis.update_after_adapt();
    this->create_communicator();
  } // ... update_after_adapt(...)

private:
  std::shared_ptr<Core> core_;
}; // class RaviartThomasSpace


/**
 * \sa RaviartThomasSpace
 */
template <class GV, class R = double>
RaviartThomasSpace<GV, R> make_raviart_thomas_space(GV grid_view, const int order)
{
  return RaviartThomasSpace<GV, R>(grid_view, order);
}


} // namespace GDT
} // namespace Dune

#endif // DUNE_GDT_SPACES_HDIV_RAVIART_THOMAS_HH
