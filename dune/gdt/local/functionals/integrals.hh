// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2015 - 2018)
//   René Fritze     (2016, 2018)
//   René Milk       (2017)

/**
 * \file  integrals.hh
 * \brief Local functionals defined by numerically integrating an integrand over elements or intersections.
 **/
#ifndef DUNE_GDT_LOCAL_FUNCTIONALS_INTEGRALS_HH
#define DUNE_GDT_LOCAL_FUNCTIONALS_INTEGRALS_HH

#include <type_traits>
#include <utility>

#include <dune/geometry/quadraturerules.hh>

#include <dune/gdt/local/integrands/interfaces.hh>
#include <dune/gdt/local/integrands/generic.hh>

#include "interfaces.hh"

namespace Dune {
namespace GDT {


/**
 * \brief Local element functional given by the quadrature of a local unary element integrand.
 */
template <class E, size_t r = 1, size_t rC = 1, class R = double, class F = R>
class LocalElementIntegralFunctional : public LocalElementFunctionalInterface<E, r, rC, R, F>
{
  using ThisType = LocalElementIntegralFunctional;
  using BaseType = LocalElementFunctionalInterface<E, r, rC, R, F>;

public:
  using BaseType::d;
  using typename BaseType::D;
  using typename BaseType::LocalBasisType;
  using IntegrandType = LocalUnaryElementIntegrandInterface<E, r, rC, R>;
  using GenericIntegrand = GenericLocalUnaryElementIntegrand<E, r, rC, R>;

  LocalElementIntegralFunctional(const IntegrandType& integrand, const int over_integrate = 0)
    : BaseType(integrand.parameter_type())
    , integrand_(integrand.copy_as_unary_element_integrand())
    , over_integrate_(over_integrate)
  {
  }

  LocalElementIntegralFunctional(
      typename GenericIntegrand::GenericOrderFunctionType order_function,
      typename GenericIntegrand::GenericEvaluateFunctionType evaluate_function,
      typename GenericIntegrand::GenericPostBindFunctionType post_bind_function = [](const E&) {},
      const XT::Common::ParameterType& param_type = {},
      const int over_integrate = 0)
    : BaseType(param_type)
    , integrand_(
          GenericIntegrand(order_function, evaluate_function, post_bind_function).copy_as_unary_element_integrand())
    , over_integrate_(over_integrate)
  {
  }

  LocalElementIntegralFunctional(const ThisType& other)
    : BaseType(other)
    , integrand_(other.integrand_->copy_as_unary_element_integrand())
    , over_integrate_(other.over_integrate_)
  {
  }

  LocalElementIntegralFunctional(ThisType&& source) noexcept = default;

  std::unique_ptr<BaseType> copy() const override final
  {
    return std::make_unique<ThisType>(*this);
  }

  using BaseType::apply;

  void apply(const LocalBasisType& basis,
             DynamicVector<F>& result,
             const XT::Common::Parameter& param = {}) const override final
  {
    // prepare integand
    const auto& element = basis.element();
    integrand_->bind(element);
    // prepare storage
    const auto size = basis.size(param);
    if (result.size() < size)
      result.resize(size);
    result *= 0;
    // loop over all quadrature points
    const auto integrand_order = integrand_->order(basis, param) + over_integrate_;
    const auto quadrature_rule = QuadratureRules<D, d>::rule(element.type(), integrand_order);
    const auto geometry = element.geometry();
    for (auto [point_in_reference_element, quadrature_weight] : quadrature_rule) {
      // integration factors
      const auto integration_factor = geometry.integrationElement(point_in_reference_element);
      // evaluate the integrand
      integrand_->evaluate(basis, point_in_reference_element, integrand_values_, param);
      assert(integrand_values_.size() >= size && "This must not happen!");
      // compute integral
      for (size_t ii = 0; ii < size; ++ii)
        result[ii] += integrand_values_[ii] * integration_factor * quadrature_weight;
    } // loop over all quadrature points
  } // ... apply(...)

private:
  mutable std::unique_ptr<IntegrandType> integrand_;
  const int over_integrate_;
  mutable DynamicVector<F> integrand_values_;
}; // class LocalElementIntegralFunctional


/**
 * \brief Statically dispatched variant of LocalElementIntegralFunctional.
 *
 * Keeps the concrete integrand as a template parameter and by value instead of type-erasing it, so the quadrature
 * loop calls the integrand via qualified (hence non-virtual, inlinable) calls; type erasure is confined to the
 * per-element apply() of the LocalElementFunctionalInterface (review D1).
 *
 * \sa StaticLocalElementIntegralBilinearForm
 * \sa make_local_element_integral_functional
 */
template <class Integrand>
class StaticLocalElementIntegralFunctional
  : public LocalElementFunctionalInterface<typename Integrand::E,
                                           Integrand::r,
                                           Integrand::rC,
                                           typename Integrand::R,
                                           typename Integrand::F>
{
  static_assert(std::is_base_of_v<LocalUnaryElementIntegrandInterface<typename Integrand::E,
                                                                      Integrand::r,
                                                                      Integrand::rC,
                                                                      typename Integrand::R,
                                                                      typename Integrand::F>,
                                  Integrand>,
                "Integrand has to implement LocalUnaryElementIntegrandInterface!");

  using ThisType = StaticLocalElementIntegralFunctional;
  using BaseType = LocalElementFunctionalInterface<typename Integrand::E,
                                                   Integrand::r,
                                                   Integrand::rC,
                                                   typename Integrand::R,
                                                   typename Integrand::F>;

public:
  using BaseType::d;
  using typename BaseType::D;
  using typename BaseType::F;
  using typename BaseType::LocalBasisType;

  using IntegrandType = Integrand;

  explicit StaticLocalElementIntegralFunctional(IntegrandType integrand, const int over_integrate = 0)
    : BaseType(integrand.parameter_type())
    , integrand_(std::move(integrand))
    , over_integrate_(over_integrate)
  {
  }

  StaticLocalElementIntegralFunctional(const ThisType& other) = default;

  StaticLocalElementIntegralFunctional(ThisType&& source) noexcept = default;

  std::unique_ptr<BaseType> copy() const override final
  {
    return std::make_unique<ThisType>(*this);
  }

  using BaseType::apply;

  void apply(const LocalBasisType& basis,
             DynamicVector<F>& result,
             const XT::Common::Parameter& param = {}) const override final
  {
    // prepare integand
    const auto& element = basis.element();
    integrand_.bind(element);
    // prepare storage
    const auto size = basis.size(param);
    if (result.size() < size)
      result.resize(size);
    result *= 0;
    // loop over all quadrature points (note the qualified calls into the integrand, which suppress virtual dispatch)
    const auto integrand_order = integrand_.IntegrandType::order(basis, param) + over_integrate_;
    const auto quadrature_rule = QuadratureRules<D, d>::rule(element.type(), integrand_order);
    const auto geometry = element.geometry();
    for (auto [point_in_reference_element, quadrature_weight] : quadrature_rule) {
      // integration factors
      const auto integration_factor = geometry.integrationElement(point_in_reference_element);
      // evaluate the integrand
      integrand_.IntegrandType::evaluate(basis, point_in_reference_element, integrand_values_, param);
      assert(integrand_values_.size() >= size && "This must not happen!");
      // compute integral
      for (size_t ii = 0; ii < size; ++ii)
        result[ii] += integrand_values_[ii] * integration_factor * quadrature_weight;
    } // loop over all quadrature points
  } // ... apply(...)

private:
  mutable IntegrandType integrand_;
  const int over_integrate_;
  mutable DynamicVector<F> integrand_values_;
}; // class StaticLocalElementIntegralFunctional


/**
 * \brief Creates a StaticLocalElementIntegralFunctional, deducing the concrete integrand type.
 */
template <class Integrand>
StaticLocalElementIntegralFunctional<Integrand> make_local_element_integral_functional(Integrand integrand,
                                                                                       const int over_integrate = 0)
{
  return StaticLocalElementIntegralFunctional<Integrand>(std::move(integrand), over_integrate);
}


/**
 * \brief Local intersection functional given by the quadrature of a local unary intersection integrand.
 */
template <class I, size_t r = 1, size_t rC = 1, class R = double, class F = R>
class LocalIntersectionIntegralFunctional : public LocalIntersectionFunctionalInterface<I, r, rC, R, F>
{
  using ThisType = LocalIntersectionIntegralFunctional;
  using BaseType = LocalIntersectionFunctionalInterface<I, r, rC, R, F>;

public:
  using BaseType::d;
  using typename BaseType::D;
  using typename BaseType::IntersectionType;
  using typename BaseType::LocalBasisType;
  using IntegrandType = LocalUnaryIntersectionIntegrandInterface<I, r, rC, R>;
  using GenericIntegrand = GenericLocalUnaryIntersectionIntegrand<I, r, rC, R>;

  LocalIntersectionIntegralFunctional(const IntegrandType& integrand, const int over_integrate = 0)
    : BaseType(integrand.parameter_type())
    , integrand_(integrand.copy_as_unary_intersection_integrand())
    , over_integrate_(over_integrate)
  {
  }

  LocalIntersectionIntegralFunctional(
      typename GenericIntegrand::GenericOrderFunctionType order_function,
      typename GenericIntegrand::GenericEvaluateFunctionType evaluate_function,
      typename GenericIntegrand::GenericPostBindFunctionType post_bind_function = [](const auto&) {},
      const XT::Common::ParameterType& param_type = {},
      const int over_integrate = 0)
    : BaseType(param_type)
    , integrand_(GenericIntegrand(order_function, evaluate_function, post_bind_function)
                     .copy_as_unary_intersection_integrand())
    , over_integrate_(over_integrate)
  {
  }

  LocalIntersectionIntegralFunctional(const ThisType& other)
    : BaseType(other)
    , integrand_(other.integrand_->copy_as_unary_intersection_integrand())
    , over_integrate_(other.over_integrate_)
  {
  }

  LocalIntersectionIntegralFunctional(ThisType&& source) noexcept = default;

  std::unique_ptr<BaseType> copy() const override final
  {
    return std::make_unique<ThisType>(*this);
  }

  bool inside() const override final
  {
    return integrand_->inside();
  }

  using BaseType::apply;

  void apply(const IntersectionType& intersection,
             const LocalBasisType& test_basis,
             DynamicVector<F>& result,
             const XT::Common::Parameter& param = {}) const override final
  {
    // prepare integand
    integrand_->bind(intersection);
    // prepare storage
    const size_t size = test_basis.size(param);
    if (result.size() < size)
      result.resize(size);
    result *= 0;
    // loop over all quadrature points
    const auto integrand_order = integrand_->order(test_basis, param) + over_integrate_;
    const auto quadrature_rule = QuadratureRules<D, d - 1>::rule(intersection.type(), integrand_order);
    const auto geometry = intersection.geometry();
    for (const auto& quadrature_point : quadrature_rule) {
      const auto point_in_reference_intersection = quadrature_point.position();
      // integration factors
      const auto integration_factor = geometry.integrationElement(point_in_reference_intersection);
      const auto quadrature_weight = quadrature_point.weight();
      // evaluate the integrand
      integrand_->evaluate(test_basis, point_in_reference_intersection, integrand_values_, param);
      assert(integrand_values_.size() >= size && "This must not happen!");
      // compute integral
      for (size_t ii = 0; ii < size; ++ii)
        result[ii] += integrand_values_[ii] * integration_factor * quadrature_weight;
    } // loop over all quadrature points
  } // ... apply(...)

private:
  mutable std::unique_ptr<IntegrandType> integrand_;
  const int over_integrate_;
  mutable DynamicVector<F> integrand_values_;
}; // class LocalIntersectionIntegralFunctional


} // namespace GDT
} // namespace Dune

#endif // DUNE_GDT_LOCAL_FUNCTIONALS_INTEGRALS_HH
