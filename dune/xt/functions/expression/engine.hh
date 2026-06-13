// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   René Fritze (2026)

/// \file
/// \brief Compiles mathematical expression strings into an evaluable form (backed by the ExprTk parser).

#ifndef DUNE_XT_FUNCTIONS_EXPRESSION_ENGINE_HH
#define DUNE_XT_FUNCTIONS_EXPRESSION_ENGINE_HH

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace Dune::XT::Functions::internal {


/**
 * \brief   Parses and evaluates mathematical expression strings that depend on named variables.
 *
 * This is a thin, deliberately non-templated wrapper around the third-party ExprTk parser. By operating
 * on \c double only, the heavy ExprTk header stays confined to a single translation unit (engine.cc),
 * hidden behind a pimpl. The templated MathExpressionBase / DynamicMathExpressionBase wrappers convert
 * their field types to/from \c double and delegate the actual parsing and evaluation here.
 *
 * Supported operators and functions mirror the previously bundled parser: \c + \c - \c * \c / \c ^,
 * \c sin, \c cos, \c tan, \c exp, \c log (alias \c ln), \c sqrt, \c abs, \c asin, \c acos, \c atan and
 * the constant \c pi (plus the additional functionality ExprTk provides).
 *
 * \note Unlike the bundled parser, implicit multiplication by juxtaposition (e.g. "x[0]t" meaning
 *       "x[0]*t") is not supported; use an explicit \c * instead.
 */
class MathExpressionEngine
{
public:
  /**
   * \brief Parse \a expressions, each depending on the given named \a variables.
   *
   * Variable names may contain a bracketed integer index (e.g. "x[0]", "mu[1]") or a trailing underscore
   * (e.g. "t_"), neither of which ExprTk accepts as an identifier; they are transparently remapped to
   * internal placeholder identifiers before compilation.
   *
   * \throws Common::Exceptions::wrong_input_given if an expression cannot be parsed.
   */
  MathExpressionEngine(std::vector<std::string> variables, std::vector<std::string> expressions);

  ~MathExpressionEngine();

  MathExpressionEngine(const MathExpressionEngine&) = delete;
  MathExpressionEngine& operator=(const MathExpressionEngine&) = delete;

  const std::vector<std::string>& variables() const;
  const std::vector<std::string>& expressions() const;

  /**
   * \brief Evaluate all expressions for the given variable values.
   *
   * \a values holds one entry per variable (in the order given to the constructor) and \a results one
   * entry per expression.
   *
   * \note Not thread-safe: callers must serialize access (the wrappers hold a mutex), matching the
   *       behavior of the previously bundled parser.
   */
  void evaluate(const double* values, std::size_t num_values, double* results, std::size_t num_results) const;

private:
  class Impl;
  std::unique_ptr<Impl> impl_;
}; // class MathExpressionEngine


} // namespace Dune::XT::Functions::internal

#endif // DUNE_XT_FUNCTIONS_EXPRESSION_ENGINE_HH
