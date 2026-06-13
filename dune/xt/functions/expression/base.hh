// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Andreas Buhr    (2014)
//   Felix Schindler (2013 - 2017, 2020)
//   René Fritze     (2013, 2015 - 2016, 2018 - 2020)
//   Tim Keil        (2018)
//   Tobias Leibner  (2014 - 2015, 2017, 2019 - 2020)

/// \file
/// \brief Provides MathExpressionBase, a base class that turns string math expressions into an evaluable function.

#ifndef DUNE_XT_FUNCTIONS_EXPRESSION_BASE_HH
#define DUNE_XT_FUNCTIONS_EXPRESSION_BASE_HH

#include <algorithm>
#include <array>
#include <cassert>
#include <iostream>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

#include <dune/common/dynvector.hh>
#include <dune/common/exceptions.hh>
#include <dune/common/fvector.hh>

#include <dune/xt/common/exceptions.hh>
#include <dune/xt/common/string.hh>

#include "engine.hh"

#ifndef DUNE_XT_FUNCTIONS_EXPRESSION_BASE_MAX_DYNAMIC_SIZE
#  define DUNE_XT_FUNCTIONS_EXPRESSION_BASE_MAX_DYNAMIC_SIZE 64
#endif


namespace Dune::XT::Functions {


/**
 *  \brief      Base class that makes a function out of mathematical expression strings.
 *  \attention  Most surely you do not want to use this class directly, but Functions::ExpressionFunction!
 */
template <class DomainField, size_t domainDim, class RangeField, size_t rangeDim>
class MathExpressionBase
{
  static_assert((domainDim > 0), "Really?");
  static_assert((rangeDim > 0), "Really?");

public:
  using ThisType = MathExpressionBase;

  using DomainFieldType = DomainField;
  static constexpr size_t domain_dim = domainDim;

  using RangeFieldType = RangeField;
  static constexpr size_t range_dim = rangeDim;

  MathExpressionBase(std::string var, const Common::FieldVector<std::string, range_dim>& exprs)
    : variable_(std::move(var))
    , expressions_(exprs)
  {
    setup();
  }

  MathExpressionBase(const ThisType& other)
    : variable_(other.variable_)
    , expressions_(other.expressions_)
  {
    setup();
  }

  ThisType& operator=(const ThisType& other)
  {
    if (this != &other) {
      variable_ = other.variable_;
      expressions_ = other.expressions_;
      setup();
    }
    return *this;
  }

  ~MathExpressionBase() = default;

  std::string variable() const
  {
    return variable_;
  }

  const Common::FieldVector<std::string, range_dim>& expression() const
  {
    return expressions_;
  }

  void evaluate(const Dune::FieldVector<DomainFieldType, domain_dim>& arg,
                Dune::FieldVector<RangeFieldType, range_dim>& ret) const
  {
    std::lock_guard<std::mutex> guard(mutex_);
    std::array<double, domain_dim> values{};
    for (size_t ii = 0; ii < domain_dim; ++ii)
      values[ii] = static_cast<double>(arg[ii]);
    evaluate_into(values.data(), domain_dim, ret);
  }

  /**
   *  \attention  arg will be used up to its size, ret will be resized!
   */
  void evaluate(const Dune::DynamicVector<DomainFieldType>& arg, Dune::DynamicVector<RangeFieldType>& ret) const
  {
    std::lock_guard<std::mutex> guard(mutex_);
    assert(arg.size() > 0);
    if (ret.size() != range_dim)
      ret = Dune::DynamicVector<RangeFieldType>(range_dim);
    std::array<double, domain_dim> values{};
    for (size_t ii = 0; ii < std::min(domain_dim, arg.size()); ++ii)
      values[ii] = static_cast<double>(arg[ii]);
    evaluate_into(values.data(), domain_dim, ret);
  }

  void evaluate(const Dune::FieldVector<DomainFieldType, domain_dim>& arg,
                Dune::DynamicVector<RangeFieldType>& ret) const
  {
    std::lock_guard<std::mutex> guard(mutex_);
    if (ret.size() != range_dim)
      ret = Dune::DynamicVector<RangeFieldType>(range_dim);
    std::array<double, domain_dim> values{};
    for (size_t ii = 0; ii < domain_dim; ++ii)
      values[ii] = static_cast<double>(arg[ii]);
    evaluate_into(values.data(), domain_dim, ret);
  }

  /**
   *  \attention  arg will be used up to its size
   */
  void evaluate(const Dune::DynamicVector<DomainFieldType>& arg,
                Dune::FieldVector<RangeFieldType, range_dim>& ret) const
  {
    std::lock_guard<std::mutex> guard(mutex_);
    assert(arg.size() > 0);
    std::array<double, domain_dim> values{};
    for (size_t ii = 0; ii < std::min(domain_dim, arg.size()); ++ii)
      values[ii] = static_cast<double>(arg[ii]);
    evaluate_into(values.data(), domain_dim, ret);
  }

  void report(const std::string& _name = "function.mathexpressionbase",
              std::ostream& stream = std::cout,
              const std::string& _prefix = "") const
  {
    const std::string tmp = _name + "(" + variable() + ") = ";
    stream << _prefix << tmp;
    if (expression().size() == 1)
      stream << expression()[0] << std::endl;
    else {
      stream << "[ " << expression()[0] << std::endl;
      const std::string whitespace = Common::whitespaceify(tmp + "[ ");
      for (size_t i = 1; i < expression().size() - 1; ++i)
        stream << _prefix << whitespace << expression()[i] << std::endl;
      stream << _prefix << whitespace << expression()[expression().size() - 1] << " ]" << std::endl;
    }
  } // void report(const std::string, std::ostream&, const std::string&) const

private:
  void setup()
  {
    // fill variables (i.e. "x[0]", "x[1]", ...)
    std::vector<std::string> variables(domain_dim);
    for (size_t ii = 0; ii < domain_dim; ++ii)
      variables[ii] = variable_ + "[" + Common::to_string(ii) + "]";
    std::vector<std::string> expressions(expressions_.begin(), expressions_.end());
    engine_ = std::make_unique<internal::MathExpressionEngine>(std::move(variables), std::move(expressions));
  } // ... setup(...)

  template <class RangeVector>
  void evaluate_into(const double* values, size_t num_values, RangeVector& ret) const
  {
    std::array<double, range_dim> results{};
    engine_->evaluate(values, num_values, results.data(), range_dim);
    for (size_t ii = 0; ii < range_dim; ++ii)
      ret[ii] = static_cast<RangeFieldType>(results[ii]);
  }

  std::string variable_;
  Common::FieldVector<std::string, range_dim> expressions_;
  std::unique_ptr<internal::MathExpressionEngine> engine_;
  mutable std::mutex mutex_;
}; // class MathExpressionBase


/**
 *  \brief      Base class that makes a function out of mathematical expression strings (dynamic domain dim).
 *  \attention  Most surely you do not want to use this class directly, but Functions::ParametricExpressionFunction!
 */
template <class D, class R, size_t r, size_t max_d = DUNE_XT_FUNCTIONS_EXPRESSION_BASE_MAX_DYNAMIC_SIZE>
class DynamicMathExpressionBase
{
public:
  using ThisType = DynamicMathExpressionBase;

  using DomainFieldType = D;
  static constexpr size_t maxDimDomain = max_d;

  using RangeFieldType = R;
  static constexpr size_t range_dim = r;

  DynamicMathExpressionBase(const std::vector<std::string>& vars, const std::vector<std::string>& exprs)
  {
    setup(vars, exprs);
  }

  DynamicMathExpressionBase(const ThisType& other)
  {
    setup(other.variables(), other.expressions());
  }

  DynamicMathExpressionBase(ThisType&&) = default;

  ~DynamicMathExpressionBase() = default;

  const std::vector<std::string>& variables() const
  {
    return variables_;
  }

  const std::vector<std::string>& expressions() const
  {
    return expressions_;
  }

  void evaluate(const DynamicVector<DomainFieldType>& arg, FieldVector<RangeFieldType, range_dim>& ret) const
  {
    std::lock_guard<std::mutex> guard(mutex_);
    // check for sizes
    if (arg.size() != variables_.size())
      DUNE_THROW(Common::Exceptions::shapes_do_not_match,
                 "arg.size(): " << arg.size() << "\n   "
                                << "variables.size(): " << variables_.size());
    std::vector<double> values(variables_.size());
    for (size_t ii = 0; ii < variables_.size(); ++ii)
      values[ii] = static_cast<double>(arg[ii]);
    std::array<double, range_dim> results{};
    engine_->evaluate(values.data(), values.size(), results.data(), range_dim);
    for (size_t ii = 0; ii < range_dim; ++ii)
      ret[ii] = static_cast<RangeFieldType>(results[ii]);
  }

private:
  void setup(const std::vector<std::string>& vars, const std::vector<std::string>& exprs)
  {
    static_assert((maxDimDomain > 0));
    static_assert((range_dim > 0));
    // set expressions
    if (exprs.size() != range_dim)
      DUNE_THROW(Common::Exceptions::shapes_do_not_match,
                 "expressions.size(): " << exprs.size() << "\n   "
                                        << "range_dim: " << range_dim);
    expressions_ = exprs;
    for (const auto& ex : expressions_)
      if (ex.empty())
        DUNE_THROW(Common::Exceptions::wrong_input_given, "Given expressions must not be empty!");
    if (vars.size() > maxDimDomain)
      DUNE_THROW(Common::Exceptions::shapes_do_not_match,
                 "This expression function of dynamic size was compiled to work for up to "
                     << DUNE_XT_FUNCTIONS_EXPRESSION_BASE_MAX_DYNAMIC_SIZE << " variables, but you provided "
                     << vars.size() << "!\n\n"
                     << "Configure dune-xt with a larger DUNE_XT_FUNCTIONS_EXPRESSION_BASE_MAX_DYNAMIC_SIZE!");
    variables_ = vars;
    for (const auto& var : variables_)
      if (var.empty())
        DUNE_THROW(Common::Exceptions::wrong_input_given, "Given variables must not be empty!");
    engine_ = std::make_unique<internal::MathExpressionEngine>(variables_, expressions_);
  } // void setup(const std::vector<std::string>& vars, const std::vector<std::string>& exprs)

  std::vector<std::string> variables_;
  std::vector<std::string> expressions_;
  std::unique_ptr<internal::MathExpressionEngine> engine_;
  mutable std::mutex mutex_;
}; // class DynamicMathExpressionBase


} // namespace Dune::XT::Functions

#endif // DUNE_XT_FUNCTIONS_EXPRESSION_BASE_HH
