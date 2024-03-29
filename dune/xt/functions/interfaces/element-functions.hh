// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2017 - 2020)
//   René Fritze     (2018 - 2020)
//   Tim Keil        (2018)
//   Tobias Leibner  (2017, 2019 - 2021)

#ifndef DUNE_XT_FUNCTIONS_INTERFACES_ELEMENT_FUNCTIONS_HH
#define DUNE_XT_FUNCTIONS_INTERFACES_ELEMENT_FUNCTIONS_HH

#include <array>
#include <sstream>
#include <vector>
#include <type_traits>

#include <dune/common/fvector.hh>

#include <dune/geometry/referenceelements.hh>

#include <dune/xt/common/float_cmp.hh>
#include <dune/xt/common/fmatrix.hh>
#include <dune/xt/common/fvector.hh>
#include <dune/xt/common/memory.hh>
#include <dune/xt/common/parameter.hh>
#include <dune/xt/grid/bound-object.hh>
#include <dune/xt/grid/entity.hh>
#include <dune/xt/grid/type_traits.hh>

#include <dune/xt/functions/exceptions.hh>
#include <dune/xt/functions/type_traits.hh>

namespace Dune::XT::Functions {


// forwards, required in operator+-*
template <class MinuendType, class SubtrahendType>
class ConstDifferenceElementFunction;

template <class MinuendType, class SubtrahendType>
class DifferenceElementFunction;

template <class LeftSummandType, class RightSummandType>
class ConstSumElementFunction;

template <class LeftSummandType, class RightSummandType>
class SumElementFunction;

template <class LeftFactorType, class RightFactorType>
class ConstProductElementFunction;

template <class LeftFactorType, class RightFactorType>
class ProductElementFunction;

template <class LeftFactorType, class RightFactorType>
class ConstFractionElementFunction;

template <class LeftFactorType, class RightFactorType>
class FractionElementFunction;


namespace internal {


template <class, class, class CombinationType>
struct CombinedHelper;


}


/**
 * \brief Interface for a set of globalvalued functions, which can be evaluated locally on one Element.
 *
 * \sa    RangeTypeSelector
 * \sa    DerivativeRangeTypeSelector
 *
 *        See in particular RangeTypeSelector and DerivativeRangeTypeSelector for the interpretation of a function and
 *        its derivatives.
 **/
template <class Element, size_t rangeDim = 1, size_t rangeDimCols = 1, class RangeField = double>
class ElementFunctionSetInterface
  : public Common::ParametricInterface
  , public XT::Grid::ElementBoundObject<Element>
{
  static_assert(XT::Grid::is_entity<Element>::value);
  using ThisType = ElementFunctionSetInterface;

public:
  using typename XT::Grid::ElementBoundObject<Element>::ElementType;
  using DomainFieldType = double;
  static constexpr size_t domain_dim = ElementType::dimension;
  using RangeFieldType = RangeField;
  static constexpr size_t range_dim = rangeDim;
  static constexpr size_t range_dim_cols = rangeDimCols;

  using E = ElementType;
  using D = DomainFieldType;
  static constexpr size_t d = domain_dim;
  using R = RangeFieldType;
  static constexpr size_t r = range_dim;
  static constexpr size_t rC = range_dim_cols;

  using DomainType = Dune::FieldVector<D, d>;

  using RangeSelector = RangeTypeSelector<R, r, rC>;
  using DerivativeRangeSelector = DerivativeRangeTypeSelector<d, R, r, rC>;

  /**
   * \name ``These types are the _standard_ types to be used.''
   * \{
   */

  using RangeType = typename RangeSelector::type;
  using DerivativeRangeType = typename DerivativeRangeSelector::type;
  using SingleDerivativeRangeType = typename DerivativeRangeSelector::single_type;

  /**
   * \}
   * \name ``These types are used for large dimensions.''
   * \{
   */

  using DynamicRangeType = typename RangeSelector::dynamic_type;
  using DynamicDerivativeRangeType = typename DerivativeRangeSelector::dynamic_type;

  /// \}


  ElementFunctionSetInterface(const XT::Common::ParameterType& param_type = {})
    : Common::ParametricInterface(param_type)
  {
  }

  ElementFunctionSetInterface(const ThisType& other) = default;

  ElementFunctionSetInterface(ThisType&& source) = default;

  ~ElementFunctionSetInterface() override = default;

  /**
   * \name ´´These methods have to be implemented.''
   * \{
   **/

  /**
   * The number of functions in this set.
   *
   * \note Will throw Exceptions::not_bound_to_an_element_yet error if not bound yet!
   **/
  virtual size_t size(const Common::Parameter& /*param*/ = {}) const = 0;

  /**
   * The maximum number of functions in this set over all elements. The following has to hold true:
\code
auto local_function_set = ...;
size_t max_set_size = 0;
for (auto&& element : grid_view)
  max_set_size = std::max(max_set_size, local_function_set.bind(element).size(param));
assert(max_set_size <= local_function_set.max_size());
\endcode
   **/
  virtual size_t max_size(const Common::Parameter& /*param*/ = {}) const = 0;

  /**
   * \note Will throw Exceptions::not_bound_to_an_element_yet error if not bound yet!
   **/
  virtual int order(const Common::Parameter& /*param*/ = {}) const = 0;

  /**
   * \}
   * \name ´´These methods should be implemented to define the functionality of the set of functions.''
   * \{
   **/

  /**
   * \note Will throw Exceptions::not_bound_to_an_element_yet error if not bound yet!
   **/
  virtual void evaluate(const DomainType& /*point_in_reference_element*/,
                        std::vector<RangeType>& /*result*/,
                        const Common::Parameter& /*param*/ = {}) const
  {
    DUNE_THROW(NotImplemented,
               "This set of element functions does not provide evaluations, override the 'evaluate' method!");
  }

  /**
   * \note Will throw Exceptions::not_bound_to_an_element_yet error if not bound yet!
   **/
  virtual void jacobians(const DomainType& /*point_in_reference_element*/,
                         std::vector<DerivativeRangeType>& /*result*/,
                         const Common::Parameter& /*param*/ = {}) const
  {
    DUNE_THROW(NotImplemented,
               "This set of element functions does not provide jacobians, override the 'jacobians' method!");
  }

  /**
   * \note Will throw Exceptions::not_bound_to_an_element_yet error if not bound yet!
   **/
  virtual void derivatives(const std::array<size_t, d>& alpha,
                           const DomainType& point_in_reference_element,
                           std::vector<DerivativeRangeType>& result,
                           const Common::Parameter& param = {}) const
  {
    if (Common::FloatCmp::eq(alpha, FieldVector<size_t, d>(1)))
      this->jacobians(point_in_reference_element, result, param);
    else
      DUNE_THROW(
          NotImplemented,
          "This set of element functions does not provide arbitrary derivatives, override the 'derivatives' method!");
  }

  /**
   * \{
   * \name ´´These methods are provided for convenience and should not be used within library code.''
   * \{
   **/

  /**
   * \note Will throw Exceptions::not_bound_to_an_element_yet error if not bound yet!
   **/
  virtual std::vector<RangeType> evaluate_set(const DomainType& point_in_reference_element,
                                              const Common::Parameter& param = {}) const
  {
    std::vector<RangeType> result(this->size(param));
    this->evaluate(point_in_reference_element, result, param);
    return result;
  }

  /**
   * \note Will throw Exceptions::not_bound_to_an_element_yet error if not bound yet!
   **/
  virtual std::vector<DerivativeRangeType> jacobians_of_set(const DomainType& point_in_reference_element,
                                                            const Common::Parameter& param = {}) const
  {
    std::vector<DerivativeRangeType> result(this->size(param));
    this->jacobians(point_in_reference_element, result, param);
    return result;
  }

  /**
   * \note Will throw Exceptions::not_bound_to_an_element_yet error if not bound yet!
   **/
  virtual std::vector<DerivativeRangeType> derivatives_of_set(const std::array<size_t, d>& alpha,
                                                              const DomainType& point_in_reference_element,
                                                              const Common::Parameter& param = {}) const
  {
    std::vector<DerivativeRangeType> result(this->size(param));
    this->derivatives(alpha, point_in_reference_element, result, param);
    return result;
  }

  /**
   * \{
   * \name ´´These methods are used to access individual range dimensions and should be overridden to improve their
   *         performance.''
   * \{
   **/

  /**
   * \note Will throw Exceptions::not_bound_to_an_element_yet error if not bound yet!
   **/
  virtual void evaluate(const DomainType& point_in_reference_element,
                        std::vector<R>& result,
                        const size_t row,
                        const size_t col = 0,
                        const Common::Parameter& param = {}) const
  {
    assert_correct_dims(row, col, "evaluate");
    const auto tmp_values = this->evaluate_set(point_in_reference_element, param);
    if (result.size() < tmp_values.size())
      result.resize(tmp_values.size());
    if constexpr (rC == 1) {
      for (size_t ii = 0; ii < tmp_values.size(); ++ii)
        result[ii] = tmp_values[ii][row];
    } else {
      for (size_t ii = 0; ii < tmp_values.size(); ++ii)
        result[ii] = tmp_values[ii][row][col];
    }
  }

  /**
   * \note Will throw Exceptions::not_bound_to_an_element_yet error if not bound yet!
   **/
  virtual void jacobians(const DomainType& point_in_reference_element,
                         std::vector<SingleDerivativeRangeType>& result,
                         const size_t row,
                         const size_t col = 0,
                         const Common::Parameter& param = {}) const
  {
    assert_correct_dims(row, col, "jacobians");
    const auto tmp_values = this->jacobians_of_set(point_in_reference_element, param);
    if (result.size() < tmp_values.size())
      result.resize(tmp_values.size());
    single_derivative_helper_call(tmp_values, row, col, result);
  }

  /**
   * \note Will throw Exceptions::not_bound_to_an_element_yet error if not bound yet!
   **/
  virtual void derivatives(const std::array<size_t, d>& alpha,
                           const DomainType& point_in_reference_element,
                           std::vector<SingleDerivativeRangeType>& result,
                           const size_t row,
                           const size_t col = 0,
                           const Common::Parameter& param = {}) const
  {
    assert_correct_dims(row, col, "derivatives");
    const auto tmp_values = this->derivatives_of_set(alpha, point_in_reference_element, param);
    if (result.size() < tmp_values.size())
      result.resize(tmp_values.size());
    single_derivative_helper_call(tmp_values, row, col, result);
  }

  /**
   * \{
   * \name ´´These methods are provided for large dimensions (when RangeType or DerivativeRangeType do not fit on the
   *         stack) and should be overridden to improve their performance.''
   * \{
   **/

  /**
   * \note Will throw Exceptions::not_bound_to_an_element_yet error if not bound yet!
   **/
  virtual void evaluate(const DomainType& point_in_reference_element,
                        std::vector<DynamicRangeType>& result,
                        const Common::Parameter& param = {}) const
  {
    // prepare result
    const auto sz = this->size();
    if (result.size() < sz)
      result.resize(sz);
    for (size_t ii = 0; ii < sz; ++ii)
      RangeSelector::ensure_size(result[ii]);
    // call actual evaluate
    auto tmp_result = std::make_unique<std::vector<RangeType>>(sz);
    this->evaluate(point_in_reference_element, *tmp_result, param);
    // convert
    for (size_t ii = 0; ii < sz; ++ii)
      RangeSelector::convert((*tmp_result)[ii], result[ii]);
  } // ... evaluate(...)

  /**
   * \note Will throw Exceptions::not_bound_to_an_element_yet error if not bound yet!
   **/
  virtual void jacobians(const DomainType& point_in_reference_element,
                         std::vector<DynamicDerivativeRangeType>& result,
                         const Common::Parameter& param = {}) const
  {
    // prepare result
    const auto sz = this->size();
    if (result.size() < sz)
      result.resize(sz);
    for (size_t ii = 0; ii < sz; ++ii)
      DerivativeRangeSelector::ensure_size(result[ii]);
    // call actual jacobians
    auto tmp_result = std::make_unique<std::vector<DerivativeRangeType>>(sz);
    this->jacobians(point_in_reference_element, *tmp_result, param);
    // convert
    for (size_t ii = 0; ii < sz; ++ii)
      DerivativeRangeSelector::convert((*tmp_result)[ii], result[ii]);
  } // ... jacobians(...)

  /**
   * \note Will throw Exceptions::not_bound_to_an_element_yet error if not bound yet!
   **/
  virtual void derivatives(const std::array<size_t, d>& alpha,
                           const DomainType& point_in_reference_element,
                           std::vector<DynamicDerivativeRangeType>& result,
                           const Common::Parameter& param = {}) const
  {
    // prepare result
    const auto sz = this->size();
    if (result.size() < sz)
      result.resize(sz);
    for (size_t ii = 0; ii < sz; ++ii)
      DerivativeRangeSelector::ensure_size(result[ii]);
    // call actual derivatives
    auto tmp_result = std::make_unique<std::vector<DerivativeRangeType>>(sz);
    this->derivatives(alpha, point_in_reference_element, *tmp_result, param);
    // convert
    for (size_t ii = 0; ii < sz; ++ii)
      DerivativeRangeSelector::convert((*tmp_result)[ii], result[ii]);
  } // ... derivatives(...)

  /**
   * \}
   **/
protected:
#ifndef DUNE_XT_FUNCTIONS_DISABLE_CHECKS
  void assert_inside_reference_element(const DomainType& point_in_reference_element) const
  {
    if (!ReferenceElements<D, d>::general(this->element().type()).checkInside(point_in_reference_element)) {
      std::stringstream error_message;
      error_message << "Given point_in_reference_element is not inside the current elements reference element!\n\n";
      XT::Grid::print_entity(this->element(), XT::Common::Typename<E>::value(), error_message, "   ");
      error_message << "\n   point_in_reference_element = " << point_in_reference_element << std::endl;
      DUNE_THROW(XT::Functions::Exceptions::wrong_input_given, error_message.str());
    }
  }
#else // DUNE_XT_FUNCTIONS_DISABLE_CHECKS
  void assert_inside_reference_element(const DomainType& /*point_in_reference_element*/) const {}
#endif

#ifndef DUNE_XT_FUNCTIONS_DISABLE_CHECKS
  static void assert_correct_dims(const size_t row, const size_t col, const std::string& caller)
  {
    if (row >= r || col >= rC)
      DUNE_THROW(XT::Common::Exceptions::shapes_do_not_match,
                 "in " << caller << ": this function maps from " << d << " -> " << r << "x" << rC << "\n   "
                       << "and you are trying to access the entry in row " << row << " and column " << col
                       << " of its range!");
  }
#else // DUNE_XT_FUNCTIONS_DISABLE_CHECKS
  static void assert_correct_dims(const size_t /*row*/, const size_t /*col*/, const std::string& /*caller*/) {}
#endif

private:
  template <class FullType, class SingleType>
  void single_derivative_helper_call(const std::vector<FullType>& val,
                                     const size_t row,
                                     [[maybe_unused]] const size_t col,
                                     std::vector<SingleType>& ret) const
  {
    if constexpr (rC == 1) {
      assert(col == 0);
      for (size_t ii = 0; ii < val.size(); ++ii)
        ret[ii] = val[ii][row];
    } else {
      for (size_t ii = 0; ii < val.size(); ++ii)
        for (size_t dd = 0; dd < d; ++dd)
          ret[ii][dd] = val[ii][row][col][dd];
    }
  }
}; // class ElementFunctionSetInterface


/**
 *  \brief  Interface for a globalvalued function, which can be evaluated locally on one Element.
 */
template <class Element, size_t range_dim = 1, size_t range_dim_cols = 1, class RangeField = double>
class ElementFunctionInterface : public ElementFunctionSetInterface<Element, range_dim, range_dim_cols, RangeField>
{
  using BaseType = ElementFunctionSetInterface<Element, range_dim, range_dim_cols, RangeField>;
  using ThisType = ElementFunctionInterface;

public:
  using BaseType::d;
  using BaseType::r;
  using BaseType::rC;
  using typename BaseType::DomainType;
  using typename BaseType::ElementType;
  using typename BaseType::R;

  using typename BaseType::DerivativeRangeSelector;
  using typename BaseType::RangeSelector;

  /**
   * \name ``These types are the _standard_ types used by the LocalFunctionSetInterface.''
   * \{
   */

  using typename BaseType::DerivativeRangeType;
  using typename BaseType::RangeType;
  using typename BaseType::SingleDerivativeRangeType;

  /**
   * \}
   * \name ``These types are used by the LocalFunctionSetInterface for large dimensions.''
   * \{
   */

  using typename BaseType::DynamicDerivativeRangeType;
  using typename BaseType::DynamicRangeType;

  /**
   * \name ``These types are the _standard_ types to be used for LocalFunctionInterface.''
   * \{
   */

  using RangeReturnType = typename RangeSelector::return_type;
  using DerivativeRangeReturnType = typename DerivativeRangeSelector::return_type;
  using SingleDerivativeRangeReturnType = typename DerivativeRangeSelector::return_single_type;

  /// \}

  ElementFunctionInterface(const XT::Common::ParameterType& param_type = {})
    : BaseType(param_type)
  {
  }

  ElementFunctionInterface(const ThisType& other) = default;
  ElementFunctionInterface(ThisType&& source) = default;

  ~ElementFunctionInterface() override = default;

  ThisType& operator=(const ThisType& other) = default;
  ThisType& operator=(ThisType&& source) = default;

  using BaseType::evaluate;

  /**
   * \}
   * \name ´´These methods should be implemented to define the functionality of the function.''
   * \{
   **/

  virtual RangeReturnType evaluate(const DomainType& /*point_in_reference_element*/,
                                   const Common::Parameter& /*param*/ = {}) const
  {
    DUNE_THROW(NotImplemented, "This local function does not provide evaluations, override the 'evaluate' method!");
  }

  virtual DerivativeRangeReturnType jacobian(const DomainType& /*point_in_reference_element*/,
                                             const Common::Parameter& /*param*/ = {}) const
  {
    DUNE_THROW(NotImplemented, "This local function does not provide a jacobian, override the 'jacobian' method!");
  }

  virtual DerivativeRangeReturnType derivative(const std::array<size_t, d>& /*alpha*/,
                                               const DomainType& /*point_in_reference_element*/,
                                               const Common::Parameter& /*param*/ = {}) const
  {
    DUNE_THROW(NotImplemented,
               "This local function does not provide arbitrary derivatives, override the 'derivative' method!");
  }

  /**
   * \}
   * \name ´´These methods are used to access individual range dimensions and should be overridden to improve their
   *         performance.''
   * \{
   **/

  virtual R evaluate(const DomainType& point_in_reference_element,
                     const size_t row,
                     const size_t col = 0,
                     const Common::Parameter& param = {}) const
  {
    this->assert_correct_dims(row, col, "evaluate");
    const auto value = this->evaluate(point_in_reference_element, param);
    if constexpr (rC == 1) {
      return value[row];
    } else {
      return value[row][col];
    }
  }

  virtual SingleDerivativeRangeReturnType jacobian(const DomainType& point_in_reference_element,
                                                   const size_t row,
                                                   const size_t col = 0,
                                                   const Common::Parameter& param = {}) const
  {
    this->assert_correct_dims(row, col, "jacobian");
    return single_derivative_helper_call(this->jacobian(point_in_reference_element, param), row, col);
  }

  virtual SingleDerivativeRangeReturnType derivative(const std::array<size_t, d>& alpha,
                                                     const DomainType& point_in_reference_element,
                                                     const size_t row,
                                                     const size_t col = 0,
                                                     const Common::Parameter& param = {}) const
  {
    this->assert_correct_dims(row, col, "derivative");
    return single_derivative_helper_call(this->derivative(alpha, point_in_reference_element, param), row, col);
  }

  /**
   * \}
   * \name ´´These methods are provided for large dimensions (when RangeType or DerivativeRangeType do not fit on the
   *         stack) and should be overridden to improve their performance.''
   * \{
   **/

  virtual void evaluate(const DomainType& point_in_reference_element,
                        DynamicRangeType& result,
                        const Common::Parameter& param = {}) const
  {
    // prepare result
    RangeSelector::ensure_size(result);
    // call actual evaluate
    auto tmp_result = std::make_unique<std::vector<RangeType>>(1);
    this->evaluate(point_in_reference_element, *tmp_result, param);
    // convert
    RangeSelector::convert((*tmp_result)[0], result);
  } // ... evaluate(...)

  virtual void jacobian(const DomainType& point_in_reference_element,
                        DynamicDerivativeRangeType& result,
                        const Common::Parameter& param = {}) const
  {
    // prepare result
    DerivativeRangeSelector::ensure_size(result);
    // call actual jacobians
    auto tmp_result = std::make_unique<std::vector<DerivativeRangeType>>(1);
    this->jacobians(point_in_reference_element, *tmp_result, param);
    // convert
    DerivativeRangeSelector::convert((*tmp_result)[0], result);
  } // ... jacobians(...)

  virtual void derivative(const std::array<size_t, d>& alpha,
                          const DomainType& point_in_reference_element,
                          DynamicDerivativeRangeType& result,
                          const Common::Parameter& param = {}) const
  {
    // prepare result
    DerivativeRangeSelector::ensure_size(result);
    // call actual derivatives
    auto tmp_result = std::make_unique<std::vector<DerivativeRangeType>>(1);
    this->derivatives(alpha, point_in_reference_element, *tmp_result, param);
    // convert
    DerivativeRangeSelector::convert((*tmp_result)[0], result);
  } // ... derivatives(...)

  /**
   * \{
   * \name ´´These methods are required by ElementFunctionSetInterface and are provided by this interface.''
   * \{
   **/

  size_t size(const Common::Parameter& /*param*/ = {}) const override
  {
    return 1;
  }

  size_t max_size(const Common::Parameter& /*param*/ = {}) const override
  {
    return 1;
  }

  void evaluate(const DomainType& point_in_reference_element,
                std::vector<RangeType>& result,
                const Common::Parameter& param = {}) const override
  {
    if (result.size() < 1)
      result.resize(1);
    result[0] = this->evaluate(point_in_reference_element, param);
  }

  void jacobians(const DomainType& point_in_reference_element,
                 std::vector<DerivativeRangeType>& result,
                 const Common::Parameter& param = {}) const override
  {
    if (result.size() < 1)
      result.resize(1);
    result[0] = this->jacobian(point_in_reference_element, param);
  }

  void derivatives(const std::array<size_t, d>& alpha,
                   const DomainType& point_in_reference_element,
                   std::vector<DerivativeRangeType>& result,
                   const Common::Parameter& param = {}) const override
  {
    if (result.size() < 1)
      result.resize(1);
    result[0] = this->derivative(alpha, point_in_reference_element, param);
  }

  /// \}

  /// \name Numerical operators (const variants).
  /// \{

  ConstDifferenceElementFunction<ThisType, ThisType> operator-(const ThisType& other) const
  {
    return ConstDifferenceElementFunction<ThisType, ThisType>(*this, other);
  }

  ConstSumElementFunction<ThisType, ThisType> operator+(const ThisType& other) const
  {
    return ConstSumElementFunction<ThisType, ThisType>(*this, other);
  }

  template <class OtherType>
  std::enable_if_t<is_element_function<OtherType>::value
                       && internal::CombinedHelper<ThisType, OtherType, CombinationType::product>::available,
                   ConstProductElementFunction<ThisType, as_element_function_interface_t<OtherType>>>
  operator*(const OtherType& other) const
  {
    return ConstProductElementFunction<ThisType, as_element_function_interface_t<OtherType>>(*this, other);
  }

  template <class OtherType>
  std::enable_if_t<is_element_function<OtherType>::value
                       && internal::CombinedHelper<ThisType, OtherType, CombinationType::fraction>::available,
                   ConstFractionElementFunction<ThisType, as_element_function_interface_t<OtherType>>>
  operator/(const OtherType& other) const
  {
    return ConstFractionElementFunction<ThisType, as_element_function_interface_t<OtherType>>(*this, other);
  }

  /// \}

  /// \name Numerical operators (mutable variants).
  /// \{

  DifferenceElementFunction<ThisType, ThisType> operator-(ThisType& other)
  {
    return DifferenceElementFunction<ThisType, ThisType>(*this, other);
  }

  SumElementFunction<ThisType, ThisType> operator+(ThisType& other)
  {
    return SumElementFunction<ThisType, ThisType>(*this, other);
  }

  template <class OtherType>
  std::enable_if_t<is_element_function<OtherType>::value
                       && internal::CombinedHelper<ThisType, OtherType, CombinationType::product>::available,
                   ProductElementFunction<ThisType, as_element_function_interface_t<OtherType>>>
  operator*(OtherType& other)
  {
    return ProductElementFunction<ThisType, as_element_function_interface_t<OtherType>>(*this, other);
  }

  template <class OtherType>
  std::enable_if_t<is_element_function<OtherType>::value
                       && internal::CombinedHelper<ThisType, OtherType, CombinationType::fraction>::available,
                   FractionElementFunction<ThisType, as_element_function_interface_t<OtherType>>>
  operator/(OtherType& other)
  {
    return FractionElementFunction<ThisType, as_element_function_interface_t<OtherType>>(*this, other);
  }

  /// \}

private:
  template <class FullType>
  static SingleDerivativeRangeType
  single_derivative_helper_call(const FullType& val, const size_t row, [[maybe_unused]] const size_t col)
  {
    if constexpr (rC == 1) {
      assert(col == 0);
      return val[row];
    } else {
      SingleDerivativeRangeType ret;
      for (size_t dd = 0; dd < d; ++dd)
        ret[dd] = val[row][col][dd];
      return ret;
    }
  }
}; // class ElementFunctionInterface


} // namespace Dune::XT::Functions

#include <dune/xt/functions/base/combined-element-functions.hh>

#endif // DUNE_XT_FUNCTIONS_INTERFACES_ELEMENT_FUNCTIONS_HH
