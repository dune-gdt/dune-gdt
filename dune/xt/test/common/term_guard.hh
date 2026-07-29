// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   René Fritze    (2026)

#ifndef DUNE_XT_TEST_COMMON_TERM_GUARD_HH
#define DUNE_XT_TEST_COMMON_TERM_GUARD_HH

#include <cstdlib>
#include <string>

namespace Dune::XT::Common::Test {


/**
 * \brief Sets the TERM environment variable for the duration of a scope and restores it afterwards.
 *
 * Everything in dune-xt which decides whether to emit ANSI escape sequences goes through
 * Dune::XT::Common::terminal_supports_color(), which inspects TERM. Tests which assert on such output have to pin
 * TERM down, or they pass or fail depending on the terminal they happen to be run from.
 */
class TermGuard
{
public:
  //! Pass nullptr to remove TERM from the environment entirely.
  explicit TermGuard(const char* term)
  {
    if (const char* previous = std::getenv("TERM"); previous != nullptr) {
      had_term_ = true;
      previous_ = previous;
    }
    if (term == nullptr)
      ::unsetenv("TERM");
    else
      ::setenv("TERM", term, 1);
  }

  // Restoring the environment twice would undo the wrong thing, so this guard is neither copyable nor movable.
  TermGuard(const TermGuard&) = delete;
  TermGuard(TermGuard&&) = delete;
  TermGuard& operator=(const TermGuard&) = delete;
  TermGuard& operator=(TermGuard&&) = delete;

  ~TermGuard()
  {
    if (had_term_)
      ::setenv("TERM", previous_.c_str(), 1);
    else
      ::unsetenv("TERM");
  }

private:
  bool had_term_{false};
  std::string previous_;
};


} // namespace Dune::XT::Common::Test

#endif // DUNE_XT_TEST_COMMON_TERM_GUARD_HH
