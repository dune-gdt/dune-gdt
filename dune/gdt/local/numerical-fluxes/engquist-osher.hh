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
 * \file  engquist-osher.hh
 * \brief Engquist-Osher numerical flux for advection problems.
 **/
#ifndef DUNE_GDT_LOCAL_NUMERICAL_FLUXES_ENGQUIST_OSHER_HH
#define DUNE_GDT_LOCAL_NUMERICAL_FLUXES_ENGQUIST_OSHER_HH

#include <dune/geometry/quadraturerules.hh>
#include <dune/geometry/type.hh>

#include "interface.hh"
#include <dune/xt/common/type_traits.hh>

namespace Dune {
namespace GDT {


/**
 * \brief Implementation of NumericalFluxInterface using the Engquist-Osher numerical flux (only available for m == 1).
 */
template <class I, size_t d, size_t m = 1, class R = double>
class NumericalEngquistOsherFlux : public internal::ThisNumericalFluxIsNotAvailableForTheseDimensions<I, d, m, R>
{
public:
  template <class... Args, typename = XT::Common::require_not_self_t<NumericalEngquistOsherFlux, Args...>>
  explicit NumericalEngquistOsherFlux(Args&&... /*args*/)
    : internal::ThisNumericalFluxIsNotAvailableForTheseDimensions<I, d, m, R>()
  {
  }
};

template <class I, size_t d, class R>
class NumericalEngquistOsherFlux<I, d, 1, R> : public NumericalFluxInterface<I, d, 1, R>
{
  static constexpr size_t m = 1;
  using ThisType = NumericalEngquistOsherFlux;
  using BaseType = NumericalFluxInterface<I, d, m, R>;

public:
  using typename BaseType::FluxType;
  using typename BaseType::LocalFluxType;
  using typename BaseType::LocalIntersectionCoords;
  using typename BaseType::PhysicalDomainType;
  using typename BaseType::StateType;
  using typename BaseType::XIndependentFluxType;

  NumericalEngquistOsherFlux(const FluxType& flx)
    : BaseType(flx)
  {
  }

  NumericalEngquistOsherFlux(const XIndependentFluxType& flx)
    : BaseType(flx)
  {
  }

  NumericalEngquistOsherFlux(const ThisType& other) = default;

  std::unique_ptr<BaseType> copy() const override final
  {
    return std::make_unique<ThisType>(*this);
  }

  using BaseType::apply;

  StateType apply(const LocalIntersectionCoords& x_in_intersection_coords,
                  const StateType& u,
                  const StateType& v,
                  const PhysicalDomainType& n,
                  const XT::Common::Parameter& param = {}) const override final
  {
    this->compute_entity_coords(x_in_intersection_coords);
    auto integrate_f = [&](const LocalFluxType& local_flux, const auto& x, const auto& s, const auto& min_max) {
      if (!(s[0] > 0.))
        return 0.;
      // integrate over the state interval [0, s[0]] by mapping a reference line quadrature affinely
      double ret = 0.;
      const auto quadrature_rule_state = QuadratureRules<R, 1>::rule(GeometryTypes::line, local_flux.order(param));
      for (const auto& quadrature_point : quadrature_rule_state) {
        const StateType uu(quadrature_point.position()[0] * s[0]);
        const auto df = local_flux.jacobian(x, uu, param);
        ret += s[0] * quadrature_point.weight() * min_max(n * df[0], 0.);
      }
      return ret;
    };
    return (local_flux_inside_->evaluate(x_in_inside_coords_, 0., param) * n)
           + integrate_f(*local_flux_inside_,
                         x_in_inside_coords_,
                         u,
                         [](const double& a, const double& b) { return std::max(a, b); })
           + integrate_f(this->intersection().neighbor() ? *local_flux_outside_ : *local_flux_inside_,
                         x_in_outside_coords_,
                         v,
                         [](const double& a, const double& b) { return std::min(a, b); });
  }

private:
  using BaseType::local_flux_inside_;
  using BaseType::local_flux_outside_;
  using BaseType::x_in_inside_coords_;
  using BaseType::x_in_outside_coords_;
}; // class NumericalEngquistOsherFlux


/**
 * \brief Creates a NumericalEngquistOsherFlux from an x- and state-dependent flux function.
 */
template <class I, size_t d, size_t m, class R>
NumericalEngquistOsherFlux<I, d, m, R>
make_numerical_engquist_osher_flux(const XT::Functions::FluxFunctionInterface<I, m, d, m, R>& flux)
{
  return NumericalEngquistOsherFlux<I, d, m, R>(flux);
}

/**
 * \brief Creates a NumericalEngquistOsherFlux from a state-dependent (x-independent) flux function.
 */
template <class I, size_t d, size_t m, class R>
NumericalEngquistOsherFlux<I, d, m, R>
make_numerical_engquist_osher_flux(const XT::Functions::FunctionInterface<m, d, m, R>& flux)
{
  return NumericalEngquistOsherFlux<I, d, m, R>(flux);
}


} // namespace GDT
} // namespace Dune

#endif // DUNE_GDT_LOCAL_NUMERICAL_FLUXES_ENGQUIST_OSHER_HH
