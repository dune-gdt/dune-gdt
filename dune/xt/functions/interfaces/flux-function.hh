// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2019)
//   René Fritze     (2019 - 2020)
//   Tobias Leibner  (2019 - 2020)

#ifndef DUNE_XT_FUNCTIONS_INTERFACES_FLUX_FUNCTION_HH
#define DUNE_XT_FUNCTIONS_INTERFACES_FLUX_FUNCTION_HH

#include <memory>
#include <ostream>
#include <string>
#include <type_traits>
#include <vector>

#include <dune/common/parallel/mpihelper.hh>

#include <dune/grid/io/file/vtk.hh>

#include <dune/xt/common/filesystem.hh>
#include <dune/xt/common/memory.hh>
#include <dune/xt/common/type_traits.hh>
#include <dune/xt/grid/grids.hh>
#include <dune/xt/grid/layers.hh>
#include <dune/xt/grid/view/from-part.hh>
#include <dune/xt/grid/type_traits.hh>

#include <dune/xt/functions/exceptions.hh>
#include <dune/xt/functions/type_traits.hh>
#include <dune/xt/functions/base/visualization.hh>

#include "element-flux-functions.hh"

namespace Dune::XT::Functions {


/**
 * \brief Interface for functions f(x, u) which can be partially localized to an element in the first variable x.
 */
template <class Element, size_t stateDim = 1, size_t rangeDim = 1, size_t rangeDimCols = 1, class RangeField = double>
class FluxFunctionInterface : public Common::ParametricInterface
{
  using ThisType = FluxFunctionInterface;

public:
  using LocalFunctionType = ElementFluxFunctionInterface<Element, stateDim, rangeDim, rangeDimCols, RangeField>;

  using ElementType = typename LocalFunctionType::ElementType;
  using DomainFieldType = typename LocalFunctionType::DomainFieldType;
  using StateType = typename LocalFunctionType::StateType;
  using DynamicStateType = typename LocalFunctionType::DynamicStateType;
  using DomainType = typename LocalFunctionType::DomainType;
  static constexpr size_t domain_dim = LocalFunctionType::domain_dim;
  static constexpr size_t state_dim = LocalFunctionType::state_dim;
  using RangeFieldType = typename LocalFunctionType::RangeFieldType;
  static constexpr size_t range_dim = LocalFunctionType::range_dim;
  static constexpr size_t range_dim_cols = LocalFunctionType::range_dim_cols;

  using E = typename LocalFunctionType::E;
  using D = typename LocalFunctionType::D;
  static constexpr size_t d = LocalFunctionType::d;
  static constexpr size_t s = LocalFunctionType::s;
  using R = typename LocalFunctionType::R;
  using S = typename LocalFunctionType::S;
  static constexpr size_t r = LocalFunctionType::r;
  static constexpr size_t rC = LocalFunctionType::rC;

  static constexpr bool available = false;

  FluxFunctionInterface(const Common::ParameterType& param_type = {})
    : Common::ParametricInterface(param_type)
  {
  }

  ~FluxFunctionInterface() override = default;

  virtual bool x_dependent() const
  {
    return true;
  }

  /**
   * \name ´´This method has to be implemented.''
   * \{
   **/

  virtual std::unique_ptr<LocalFunctionType> local_function() const = 0;

  /**
   * \}
   * \name ´´These methods should be implemented in order to identify the function.''
   * \{
   */

  virtual std::string name() const
  {
    return "dune.xt.functions.fluxfunction";
  }

  /// \}
}; // class FluxFunctionInterface


} // namespace Dune::XT::Functions

#endif // DUNE_XT_FUNCTIONS_INTERFACES_LOCALIZABLE_FUNCTION_HH
