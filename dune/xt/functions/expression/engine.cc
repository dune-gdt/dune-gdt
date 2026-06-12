// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   René Fritze (2026)

#include "config.h"

#include "engine.hh"

#include <cctype>
#include <unordered_map>

#include <dune/xt/common/exceptions.hh>

// ExprTk is a large header that triggers many warnings; silence them (it is also added as a SYSTEM
// include in CMake, but we keep this wrapper for compilers/configurations not covered by that).
#include <dune/xt/common/disable_warnings.hh>
#include <exprtk.hpp>
#include <dune/xt/common/reenable_warnings.hh>

namespace Dune::XT::Functions::internal {


class MathExpressionEngine::Impl
{
public:
  Impl(std::vector<std::string> variables, std::vector<std::string> expressions)
    : variables_(std::move(variables))
    , expressions_(std::move(expressions))
    , values_(variables_.size(), 0.)
  {
    // Map every (possibly bracketed) variable name to a safe placeholder identifier and bind that
    // placeholder to our value buffer. The buffer is never resized, so the pointers ExprTk stores
    // into it remain valid for the lifetime of this object.
    for (std::size_t ii = 0; ii < variables_.size(); ++ii) {
      const std::string placeholder = make_placeholder(ii);
      name_to_placeholder_[variables_[ii]] = placeholder;
      symbol_table_.add_variable(placeholder, values_[ii]);
    }
    symbol_table_.add_constants(); // adds pi (and e, epsilon, inf)

    exprtk::parser<double> parser;
    compiled_.resize(expressions_.size());
    for (std::size_t ii = 0; ii < expressions_.size(); ++ii) {
      compiled_[ii].register_symbol_table(symbol_table_);
      const std::string translated = translate(expressions_[ii]);
      if (!parser.compile(translated, compiled_[ii]))
        DUNE_THROW(Common::Exceptions::wrong_input_given,
                   "Could not parse the expression '" << expressions_[ii] << "'!\n   ExprTk reported: "
                                                      << parser.error());
    }
  }

  const std::vector<std::string>& variables() const
  {
    return variables_;
  }

  const std::vector<std::string>& expressions() const
  {
    return expressions_;
  }

  void evaluate(const double* values, std::size_t num_values, double* results, std::size_t num_results) const
  {
    if (num_values != values_.size())
      DUNE_THROW(Common::Exceptions::shapes_do_not_match,
                 "num_values: " << num_values << "\n   variables.size(): " << values_.size());
    if (num_results != compiled_.size())
      DUNE_THROW(Common::Exceptions::shapes_do_not_match,
                 "num_results: " << num_results << "\n   expressions.size(): " << compiled_.size());
    for (std::size_t ii = 0; ii < num_values; ++ii)
      values_[ii] = values[ii];
    for (std::size_t ii = 0; ii < num_results; ++ii)
      results[ii] = compiled_[ii].value();
  }

private:
  //! Placeholder identifier for the ii-th variable; a plain alphanumeric token (ExprTk rejects names
  //! starting with an underscore) that is unlikely to clash with user-provided function/variable names.
  static std::string make_placeholder(std::size_t index)
  {
    return "dxtvar" + std::to_string(index);
  }

  /**
   * \brief Replace every occurrence of a registered variable name in \a expr by its placeholder.
   *
   * We tokenize \a expr into maximal identifier runs (optionally followed by a bracketed integer index,
   * to capture names like "x[0]") and replace a token only if it matches a registered variable name
   * exactly. This is robust against substring clashes: a single-letter variable "t" never corrupts
   * function names such as "tan", and "mu[1]" is never confused with "mu[10]". Anything that is not a
   * registered variable (function names, the constant pi, numeric literals, operators) is passed
   * through unchanged.
   */
  std::string translate(const std::string& expr) const
  {
    auto is_ident_start = [](char c) { return (std::isalpha(static_cast<unsigned char>(c)) != 0) || c == '_'; };
    auto is_ident_char = [](char c) { return (std::isalnum(static_cast<unsigned char>(c)) != 0) || c == '_'; };
    auto is_digit = [](char c) { return std::isdigit(static_cast<unsigned char>(c)) != 0; };

    std::string out;
    out.reserve(expr.size());
    const std::size_t n = expr.size();
    std::size_t i = 0;
    while (i < n) {
      if (!is_ident_start(expr[i])) {
        out += expr[i];
        ++i;
        continue;
      }
      // consume the identifier ...
      std::size_t j = i + 1;
      while (j < n && is_ident_char(expr[j]))
        ++j;
      // ... and an optional immediately following bracketed integer index, e.g. "[0]"
      if (j < n && expr[j] == '[') {
        std::size_t k = j + 1;
        while (k < n && is_digit(expr[k]))
          ++k;
        if (k < n && expr[k] == ']' && k > j + 1)
          j = k + 1;
      }
      const std::string token = expr.substr(i, j - i);
      const auto it = name_to_placeholder_.find(token);
      out += (it != name_to_placeholder_.end()) ? it->second : token;
      i = j;
    }
    return out;
  }

  std::vector<std::string> variables_;
  std::vector<std::string> expressions_;
  mutable std::vector<double> values_;
  std::unordered_map<std::string, std::string> name_to_placeholder_;
  exprtk::symbol_table<double> symbol_table_;
  std::vector<exprtk::expression<double>> compiled_;
}; // class MathExpressionEngine::Impl


MathExpressionEngine::MathExpressionEngine(std::vector<std::string> variables, std::vector<std::string> expressions)
  : impl_(std::make_unique<Impl>(std::move(variables), std::move(expressions)))
{
}

MathExpressionEngine::~MathExpressionEngine() = default;

const std::vector<std::string>& MathExpressionEngine::variables() const
{
  return impl_->variables();
}

const std::vector<std::string>& MathExpressionEngine::expressions() const
{
  return impl_->expressions();
}

void MathExpressionEngine::evaluate(const double* values,
                                    std::size_t num_values,
                                    double* results,
                                    std::size_t num_results) const
{
  impl_->evaluate(values, num_values, results, num_results);
}


} // namespace Dune::XT::Functions::internal
