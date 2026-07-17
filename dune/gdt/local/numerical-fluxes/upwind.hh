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
 * \file  upwind.hh
 * \brief Upwind numerical flux for advection problems.
 **/
#ifndef DUNE_GDT_LOCAL_NUMERICAL_FLUXES_UPWIND_HH
#define DUNE_GDT_LOCAL_NUMERICAL_FLUXES_UPWIND_HH

#include "interface.hh"
#include <dune/xt/common/type_traits.hh>

namespace Dune {
namespace GDT {


/**
 * \brief Implementation of NumericalFluxInterface using the upwind numerical flux (only available for m == 1).
 */
template <class I, size_t d, size_t m = 1, class R = double>
class NumericalUpwindFlux : public internal::ThisNumericalFluxIsNotAvailableForTheseDimensions<I, d, m, R>
{
public:
  template <class... Args, typename = XT::Common::require_not_self_t<NumericalUpwindFlux, Args...>>
  explicit NumericalUpwindFlux(Args&&... /*args*/)
    : internal::ThisNumericalFluxIsNotAvailableForTheseDimensions<I, d, m, R>()
  {
  }
};

template <class I, size_t d, class R>
class NumericalUpwindFlux<I, d, 1, R> : public NumericalFluxInterface<I, d, 1, R>
{
  static constexpr size_t m = 1;
  using ThisType = NumericalUpwindFlux;
  using BaseType = NumericalFluxInterface<I, d, m, R>;

public:
  using typename BaseType::FluxType;
  using typename BaseType::LocalIntersectionCoords;
  using typename BaseType::PhysicalDomainType;
  using typename BaseType::StateType;
  using typename BaseType::XIndependentFluxType;

  NumericalUpwindFlux(const FluxType& flx)
    : BaseType(flx)
  {
  }

  NumericalUpwindFlux(const XIndependentFluxType& func)
    : BaseType(func)
  {
  }

  NumericalUpwindFlux(const ThisType& other) = default;

  std::unique_ptr<BaseType> copy() const override final
  {
    return std::make_unique<ThisType>(*this);
  }

  using BaseType::apply;

  StateType apply(const LocalIntersectionCoords& x,
                  const StateType& u,
                  const StateType& v,
                  const PhysicalDomainType& n,
                  const XT::Common::Parameter& param = {}) const override final
  {
    this->compute_entity_coords(x);
    // The flux is R^m -> R^{d x m}, so for the scalar (m == 1) case bound here its jacobian w.r.t.
    // the state is (d x 1)-shaped: df[jj][0] = d f_jj / d u, and the upwind direction indicator is
    // the normal dotted with that d-vector of derivatives. The previous `n * df[0]` dotted the
    // d-dimensional normal with the 1-dimensional row 0 instead -- correct for d == 1 (the only
    // case the C++ test suite exercises), but for d > 1 it trips dune-common's size-consistency
    // assert in debug builds and reads past the end of row 0 in release builds.
    const auto df = local_flux_inside_->jacobian(x_in_inside_coords_, (u + v) / 2., param);
    R n_times_df = 0.;
    for (size_t jj = 0; jj < d; ++jj)
      n_times_df += n[jj] * df[jj][0];
    if (n_times_df > 0)
      return local_flux_inside_->evaluate(x_in_inside_coords_, u, param) * n;
    else
      return local_flux_outside_->evaluate(x_in_outside_coords_, v, param) * n;
  }

private:
  using BaseType::local_flux_inside_;
  using BaseType::local_flux_outside_;
  using BaseType::x_in_inside_coords_;
  using BaseType::x_in_outside_coords_;
}; // class NumericalUpwindFlux


/**
 * \brief Creates a NumericalUpwindFlux from an x- and state-dependent flux function.
 */
template <class E, size_t d, size_t m, class R>
auto make_numerical_upwind_flux(const XT::Functions::FluxFunctionInterface<E, m, d, m, R>& flux)
{
  using I = XT::Grid::extract_entity_t<E>;
  return NumericalUpwindFlux<I, d, m, R>(flux);
}

/**
 * \brief Creates a NumericalUpwindFlux from a state-dependent (x-independent) flux function.
 */
template <class I, // <- has to be specified manually
          size_t d,
          size_t m,
          class R>
auto make_numerical_upwind_flux(const XT::Functions::FunctionInterface<m, d, m, R>& flux)
{
  return NumericalUpwindFlux<I, d, m, R>(flux);
}


} // namespace GDT
} // namespace Dune

#endif // DUNE_GDT_LOCAL_NUMERICAL_FLUXES_UPWIND_HH
