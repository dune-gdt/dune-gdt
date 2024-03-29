// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   René Fritze    (2019 - 2020)
//   Tobias Leibner (2019 - 2020)

#ifndef DUNE_XT_FUNCTIONS_BASE_FUNCTION_AS_FLUX_FUNCTION_HH
#define DUNE_XT_FUNCTIONS_BASE_FUNCTION_AS_FLUX_FUNCTION_HH

#include <dune/xt/common/memory.hh>

#include <dune/xt/functions/exceptions.hh>
#include <dune/xt/functions/interfaces/flux-function.hh>
#include <dune/xt/functions/interfaces/function.hh>
#include <dune/xt/functions/type_traits.hh>

namespace Dune::XT::Functions {


// wraps a function f(u) as flux function f(x, u)
template <class E, size_t s, size_t r, size_t rC, class R>
class StateFunctionAsFluxFunctionWrapper : public FluxFunctionInterface<E, s, r, rC, R>
{
  using BaseType = FluxFunctionInterface<E, s, r, rC, R>;
  using ThisType = StateFunctionAsFluxFunctionWrapper;

public:
  using BaseType::d;
  using typename BaseType::ElementType;
  using typename BaseType::LocalFunctionType;
  using FunctionType = FunctionInterface<s, r, rC, R>;

  StateFunctionAsFluxFunctionWrapper(const FunctionType& function)
    : function_storage_(function)
  {
  }

  StateFunctionAsFluxFunctionWrapper(std::unique_ptr<const FunctionType>&& function_ptr)
    : function_storage_(std::move(function_ptr))
  {
  }

  bool x_dependent() const final
  {
    return false;
  }

  /**
   * \name ´´This method is required by FluxFunctionInterface.''
   * \{
   **/

  std::unique_ptr<LocalFunctionType> local_function() const final
  {
    return std::make_unique<LocalFunction>(function_storage_.access());
  }

  /**
   * \}
   * \name ´´These methods are optionally required by FluxFunctionInterface.''
   * \{
   **/

  std::string name() const final
  {
    return function_storage_.access().name();
  }

  /**
   * \}
   **/

private:
  class LocalFunction : public LocalFunctionType
  {
    using BaseType = LocalFunctionType;

  public:
    using typename BaseType::DomainType;
    using typename BaseType::JacobianRangeReturnType;
    using typename BaseType::RangeReturnType;
    using typename BaseType::StateType;

    LocalFunction(const FunctionType& function)
      : BaseType()
      , function_(function)
    {
    }


    int order(const Common::Parameter& param = {}) const final
    {
      return function_.order(param);
    }

    using BaseType::evaluate;

    RangeReturnType evaluate(const DomainType& /*point_in_reference_element*/,
                             const StateType& u,
                             const Common::Parameter& param = {}) const final
    {
      return function_.evaluate(u, param);
    }

    using BaseType::jacobian;

    JacobianRangeReturnType jacobian(const DomainType& /*point_in_reference_element*/,
                                     const StateType& u,
                                     const Common::Parameter& param = {}) const final
    {
      return function_.jacobian(u, param);
    }

  private:
    const FunctionType& function_;
  }; // class LocalFunction

  const XT::Common::ConstStorageProvider<FunctionType> function_storage_;
}; // class FunctionAsFluxFunctionWrapper


} // namespace Dune::XT::Functions

#endif // DUNE_XT_FUNCTIONS_BASE_SMOOTH_LOCALIZABLE_FUNCTION_HH
