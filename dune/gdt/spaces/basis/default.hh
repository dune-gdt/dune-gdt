// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2018)
//   René Fritze     (2018)
//   Tobias Leibner  (2018)

/**
 * \file  default.hh
 * \brief Default global basis built from a grid-function-based local finite element family.
 **/
#ifndef DUNE_GDT_SPACES_BASIS_DEFAULT_HH
#define DUNE_GDT_SPACES_BASIS_DEFAULT_HH

#include <algorithm>
#include <map>
#include <utility>
#include <vector>

#include <dune/xt/common/memory.hh>
#include <dune/xt/functions/interfaces/grid-function.hh>

#include <dune/gdt/exceptions.hh>
#include <dune/gdt/local/finite-elements/interfaces.hh>

#include "interface.hh"

namespace Dune {
namespace GDT {


/**
 * Applies no transformation in evaluate, but left-multiplication by the geometry transformations jacobian inverse
 * transpose in jacobian.
 */
template <class GV, size_t r = 1, size_t rC = 1, class R = double>
class DefaultGlobalBasis : public GlobalBasisInterface<GV, r, rC, R>
{
  using ThisType = DefaultGlobalBasis;
  using BaseType = GlobalBasisInterface<GV, r, rC, R>;

public:
  using BaseType::d;
  using typename BaseType::D;
  using typename BaseType::E;
  using typename BaseType::ElementType;
  using typename BaseType::GridViewType;
  using typename BaseType::LocalizedType;
  using FiniteElementFamilyType = LocalFiniteElementFamilyInterface<D, d, R, r, rC>;

  DefaultGlobalBasis(const ThisType&) = default;
  DefaultGlobalBasis(ThisType&&) noexcept = default;

  ThisType& operator=(const ThisType&) = delete;
  ThisType& operator=(ThisType&&) = delete;

  DefaultGlobalBasis(const GridViewType& grid_view,
                     const FiniteElementFamilyType& local_finite_elements,
                     const int order)
    : grid_view_(grid_view)
    , local_finite_elements_(local_finite_elements)
    , fe_order_(order)
    , max_size_(0)
  {
  }

  size_t max_size() const override final
  {
    return max_size_;
  }

  using BaseType::localize;

  std::unique_ptr<LocalizedType> localize() const override final
  {
    return std::make_unique<LocalizedDefaultGlobalBasis>(*this);
  }

  void update_after_adapt() override final
  {
    max_size_ = 0;
    for (const auto& gt : grid_view_.indexSet().types(0))
      max_size_ = std::max(max_size_, local_finite_elements_.get(gt, fe_order_).size());
  }

private:
  class LocalizedDefaultGlobalBasis : public LocalizedGlobalFiniteElementInterface<E, r, rC, R>
  {
    using ThisType = LocalizedDefaultGlobalBasis;
    using BaseType = LocalizedGlobalFiniteElementInterface<E, r, rC, R>;
    static_assert(rC == 1,
                  "Not implemented for rC > 1, check that all functions (especially jacobians(...)) work properly "
                  "before removing this assert!");

  public:
    using typename BaseType::DerivativeRangeType;
    using typename BaseType::DomainType;
    using typename BaseType::ElementType;
    using typename BaseType::LocalFiniteElementType;
    using typename BaseType::RangeType;

    LocalizedDefaultGlobalBasis(const DefaultGlobalBasis<GV, r, rC, R>& self)
      : BaseType()
      , self_(self)
    {
    }

    LocalizedDefaultGlobalBasis(const ThisType&) = delete;
    LocalizedDefaultGlobalBasis(ThisType&&) noexcept = default;

    ThisType& operator=(const ThisType&) = delete;
    ThisType& operator=(ThisType&&) = delete;

    // required by XT::Functions::ElementFunctionSet

    size_t max_size(const XT::Common::Parameter& /*param*/ = {}) const override final
    {
      return self_.max_size_;
    }

  protected:
    void post_bind(const ElementType& elemnt) override final
    {
      auto&& new_geometry_type = elemnt.type();
      if (geometry_type_ != new_geometry_type) {
        geometry_type_ = new_geometry_type;
        current_local_fe_ = XT::Common::ConstStorageProvider<LocalFiniteElementInterface<D, d, R, r, rC>>(
            self_.local_finite_elements_.get(geometry_type_, self_.fe_order_));
        size_ = current_local_fe_.access().size();
        order_ = current_local_fe_.access().basis().order();
      }
    }

  public:
    size_t size(const XT::Common::Parameter& /*param*/ = {}) const override final
    {
      DUNE_THROW_IF(!current_local_fe_.valid(), Exceptions::not_bound_to_an_element_yet, "");
      return size_;
    }

    int order(const XT::Common::Parameter& /*param*/ = {}) const override final
    {
      DUNE_THROW_IF(!current_local_fe_.valid(), Exceptions::not_bound_to_an_element_yet, "");
      return order_;
    }

    using BaseType::evaluate;
    using BaseType::jacobians;

    /**
     * \note Since the shape functions of a finite element are identical for every element of a geometry type, their
     *       values at a given point in reference coordinates are cached (per geometry type) and shared between all
     *       elements this basis is bound to over its lifetime -- quadrature points thus trigger an actual shape
     *       function evaluation only once (review C3/D3). The cache is per localized basis (which is not shared
     *       between threads), so no synchronization is required.
     */
    void evaluate(const DomainType& point_in_reference_element,
                  std::vector<RangeType>& result,
                  const XT::Common::Parameter& /*param*/ = {}) const override final
    {
      DUNE_THROW_IF(!current_local_fe_.valid(), Exceptions::not_bound_to_an_element_yet, "");
      this->assert_inside_reference_element(point_in_reference_element);
      auto& cache = shape_values_cache_[geometry_type_];
      for (const auto& [cached_point, cached_values] : cache)
        if (cached_point == point_in_reference_element) {
          if (result.size() < size_)
            result.resize(size_);
          std::copy(cached_values.begin(), cached_values.end(), result.begin());
          return;
        }
      current_local_fe_.access().basis().evaluate(point_in_reference_element, result);
      if (cache.size() < max_cached_points_per_geometry_type)
        cache.emplace_back(point_in_reference_element, std::vector<RangeType>(result.begin(), result.begin() + size_));
    } // ... evaluate(...)

    /**
     * \note The element-independent shape function jacobians (in reference coordinates) are cached as in evaluate();
     *       only the geometry transformation is applied per element (review C3/D3).
     */
    void jacobians(const DomainType& point_in_reference_element,
                   std::vector<DerivativeRangeType>& result,
                   const XT::Common::Parameter& /*param*/ = {}) const override final
    {
      DUNE_THROW_IF(!current_local_fe_.valid(), Exceptions::not_bound_to_an_element_yet, "");
      this->assert_inside_reference_element(point_in_reference_element);
      // evaluate jacobian of shape functions (or look them up in the cache)
      bool found_in_cache = false;
      auto& cache = shape_jacobians_cache_[geometry_type_];
      for (const auto& [cached_point, cached_jacobians] : cache)
        if (cached_point == point_in_reference_element) {
          if (result.size() < size_)
            result.resize(size_);
          std::copy(cached_jacobians.begin(), cached_jacobians.end(), result.begin());
          found_in_cache = true;
          break;
        }
      if (!found_in_cache) {
        current_local_fe_.access().basis().jacobian(point_in_reference_element, result);
        if (cache.size() < max_cached_points_per_geometry_type)
          cache.emplace_back(point_in_reference_element,
                             std::vector<DerivativeRangeType>(result.begin(), result.begin() + size_));
      }
      // Apply transformation:
      // Let f: E -> R^r be a basis function, and g: E' -> E be the mapping from reference to actual element, then f
      // \circ g is a shape function. We have the chain rule J_f = J(f \circ g \circ g^{-1}) = J(f \circ g) J_g^{-1}.
      // Applying the transpose to that equation gives
      // J_f^T = J_g^{-T} J(f \circ g)^T,
      // so we have to multiply J_inv_T from the left to the transposed shape function jacobians (i.e. the shape
      // function gradients) to get the transposed jacobian of the basis function (basis function gradient).
      const auto J_inv_T = this->element().geometry().jacobianInverseTransposed(point_in_reference_element);
      auto tmp_value = result[0][0];
      const size_t basis_size = current_local_fe_.access().basis().size();
      for (size_t ii = 0; ii < basis_size; ++ii)
        for (size_t rr = 0; rr < r; ++rr) {
          J_inv_T.mv(result[ii][rr], tmp_value);
          result[ii][rr] = tmp_value;
        }
    } // ... jacobian(...)

    // required by LocalizedGlobalFiniteElementInterface

    const LocalFiniteElementType& finite_element() const override final
    {
      return current_local_fe_.access();
    }

    DynamicVector<R> default_data(const GeometryType& /*geometry_type*/) const override final
    {
      return DynamicVector<R>();
    }

    DynamicVector<R> backup() const override final
    {
      return DynamicVector<R>();
    }

    void restore(const ElementType& elemnt, const DynamicVector<R>& data) override final
    {
      this->bind(elemnt);
      DUNE_THROW_IF(data.size() != 0, Exceptions::finite_element_error, "data.size() = " << data.size());
    }

    using BaseType::interpolate;

    void interpolate(const std::function<RangeType(const DomainType&)>& element_function,
                     const int order,
                     DynamicVector<R>& dofs) const override final
    {
      current_local_fe_.access().interpolation().interpolate(element_function, order, dofs);
    }

  private:
    //! bounds the per-geometry-type memory of the shape value/jacobian caches; quadratures, interpolation points and
    //! visualization lattices stay well below this in practice
    static constexpr size_t max_cached_points_per_geometry_type = 256;

    const DefaultGlobalBasis<GV, r, rC, R>& self_;
    XT::Common::ConstStorageProvider<LocalFiniteElementInterface<D, d, R, r, rC>> current_local_fe_;
    size_t size_;
    int order_;
    Dune::GeometryType geometry_type_;
    mutable std::map<GeometryType, std::vector<std::pair<DomainType, std::vector<RangeType>>>> shape_values_cache_;
    mutable std::map<GeometryType, std::vector<std::pair<DomainType, std::vector<DerivativeRangeType>>>>
        shape_jacobians_cache_;
  }; // class LocalizedDefaultGlobalBasis

  const GridViewType& grid_view_;
  const FiniteElementFamilyType& local_finite_elements_;
  const int fe_order_;
  size_t max_size_;
}; // class DefaultGlobalBasis


} // namespace GDT
} // namespace Dune

#endif // DUNE_GDT_SPACES_BASIS_DEFAULT_HH
