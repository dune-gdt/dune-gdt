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
 * \file  vijayasundaram.hh
 * \brief Vijayasundaram numerical flux for advection problems.
 **/
#ifndef DUNE_GDT_LOCAL_NUMERICAL_FLUXES_VIJAYASUNDARAM_HH
#define DUNE_GDT_LOCAL_NUMERICAL_FLUXES_VIJAYASUNDARAM_HH

#include <functional>
#include <tuple>

#include <dune/xt/common/fmatrix.hh>
#include <dune/xt/common/math.hh>
#include <dune/xt/la/eigen-solver.hh>

#include "interface.hh"

namespace Dune {
namespace GDT {


/**
 * \brief Implementation of NumericalFluxInterface using the Vijayasundaram numerical flux based on a flux Jacobian
 *        eigendecomposition.
 */
template <class I, size_t d, size_t m = 1, class R = double>
class NumericalVijayasundaramFlux : public NumericalFluxInterface<I, d, m, R>
{
  using ThisType = NumericalVijayasundaramFlux;
  using BaseType = NumericalFluxInterface<I, d, m, R>;

public:
  using typename BaseType::E;
  using typename BaseType::FluxType;
  using typename BaseType::LocalFluxType;
  using typename BaseType::LocalIntersectionCoords;
  using typename BaseType::PhysicalDomainType;
  using typename BaseType::StateType;
  using typename BaseType::XIndependentFluxType;

  using FluxEigenDecompositionLambdaType =
      std::function<std::tuple<std::vector<XT::Common::real_t<R>>,
                               XT::Common::FieldMatrix<XT::Common::real_t<R>, m, m>,
                               XT::Common::FieldMatrix<XT::Common::real_t<R>, m, m>>(
          const LocalFluxType&,
          const FieldVector<R, m>&,
          const FieldVector<double, d>&,
          const XT::Common::Parameter& param)>;

  static FluxEigenDecompositionLambdaType default_flux_eigen_decomposition()
  {
    // NOTE: this default (used whenever no explicit flux_eigen_decomposition is supplied, e.g. by
    //       InstationaryNonconformingHyperbolicEocStudy::make_lhs_operator()) is intentionally left unimplemented:
    //       for scalar (m == 1) fluxes, local_flux.jacobian(...) does not yield a square m x m matrix that df * n
    //       can turn into an eigen-decomposable P, which fails to compile (see #106). Callers that need the
    //       Vijayasundaram flux must supply their own flux_eigen_decomposition lambda, as
    //       dune/gdt/test/inviscid-compressible-flow/base.hh does for the Euler equations.
    return
        [](const LocalFluxType& /*local_flux*/,
           const StateType& /*w*/,
           const PhysicalDomainType& /*n*/,
           const XT::Common::Parameter& /*param*/) -> std::tuple<std::vector<XT::Common::real_t<R>>,
                                                                 XT::Common::FieldMatrix<XT::Common::real_t<R>, m, m>,
                                                                 XT::Common::FieldMatrix<XT::Common::real_t<R>, m, m>> {
          DUNE_THROW(
              Dune::NotImplemented,
              "default_flux_eigen_decomposition() is not implemented, supply your own flux_eigen_decomposition!");
          return std::make_tuple(std::vector<XT::Common::real_t<R>>(m),
                                 XT::Common::FieldMatrix<XT::Common::real_t<R>, m, m>(),
                                 XT::Common::FieldMatrix<XT::Common::real_t<R>, m, m>());
        };
  }

  NumericalVijayasundaramFlux(const FluxType& flx)
    : BaseType(flx)
    , flux_eigen_decomposition_(default_flux_eigen_decomposition())
  {
    if (flx->x_dependent())
      DUNE_THROW(Dune::NotImplemented, "This flux is not yet implemented for x-dependent fluxes!");
  }

  NumericalVijayasundaramFlux(const XIndependentFluxType& flx)
    : BaseType(flx)
    , flux_eigen_decomposition_(default_flux_eigen_decomposition())
  {
  }

  NumericalVijayasundaramFlux(const FluxType& flx, FluxEigenDecompositionLambdaType flux_eigen_decomposition)
    : BaseType(flx)
    , flux_eigen_decomposition_(flux_eigen_decomposition)
  {
    if (flx->x_dependent())
      DUNE_THROW(Dune::NotImplemented, "This flux is not yet implemented for x-dependent fluxes!");
  }

  NumericalVijayasundaramFlux(const XIndependentFluxType& flx,
                              FluxEigenDecompositionLambdaType flux_eigen_decomposition)
    : BaseType(flx)
    , flux_eigen_decomposition_(flux_eigen_decomposition)
  {
  }

  std::unique_ptr<BaseType> copy() const override final
  {
    return std::make_unique<ThisType>(*this);
  }

  NumericalVijayasundaramFlux(const ThisType& other) = default;

  using BaseType::apply;

  StateType apply(const LocalIntersectionCoords& x,
                  const StateType& u,
                  const StateType& v,
                  const PhysicalDomainType& n,
                  const XT::Common::Parameter& param = {}) const override final
  {
    // compute decomposition
    this->compute_entity_coords(x);
    const auto eigendecomposition = flux_eigen_decomposition_(*local_flux_inside_, 0.5 * (u + v), n, param);
    const auto& evs = std::get<0>(eigendecomposition);
    const auto& T = std::get<1>(eigendecomposition);
    const auto& T_inv = std::get<2>(eigendecomposition);
    DUNE_THROW_IF(evs.size() != m, Exceptions::numerical_flux_error, "evs.size() = " << evs.size() << "\n   m = " << m);
    // compute numerical flux [DF2016, p. 428, (8.108)]
    auto lambda_plus = XT::Common::zeros_like(T);
    auto lambda_minus = XT::Common::zeros_like(T);
    for (size_t ii = 0; ii < m; ++ii) {
      const auto& real_ev = evs[ii];
      XT::Common::set_matrix_entry(lambda_plus, ii, ii, XT::Common::max(real_ev, 0.));
      XT::Common::set_matrix_entry(lambda_minus, ii, ii, XT::Common::min(real_ev, 0.));
    }
    const auto P_plus = T * lambda_plus * T_inv;
    const auto P_minus = T * lambda_minus * T_inv;
    return P_plus * u + P_minus * v;
  } // ... apply(...)

private:
  using BaseType::local_flux_inside_;
  const FluxEigenDecompositionLambdaType flux_eigen_decomposition_;
}; // class NumericalVijayasundaramFlux


/**
 * \brief Creates a NumericalVijayasundaramFlux from an x- and state-dependent flux function.
 */
template <class I, size_t d, size_t m, class R, class... Args>
NumericalVijayasundaramFlux<I, d, m, R>
make_numerical_vijayasundaram_flux(const XT::Functions::FluxFunctionInterface<I, m, d, m, R>& flux, Args&&... args)
{
  return NumericalVijayasundaramFlux<I, d, m, R>(flux, std::forward<Args>(args)...);
}

/**
 * \brief Creates a NumericalVijayasundaramFlux from a state-dependent (x-independent) flux function.
 */
template <class I, size_t d, size_t m, class R, class... Args>
NumericalVijayasundaramFlux<I, d, m, R>
make_numerical_vijayasundaram_flux(const XT::Functions::FunctionInterface<m, d, m, R>& flux, Args&&... args)
{
  return NumericalVijayasundaramFlux<I, d, m, R>(flux, std::forward<Args>(args)...);
}


} // namespace GDT
} // namespace Dune

#endif // DUNE_GDT_LOCAL_NUMERICAL_FLUXES_VIJAYASUNDARAM_HH
