// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   René Fritze    (2026)

#ifndef DUNE_XT_TEST_COMMON_ENV_GUARD_HH
#define DUNE_XT_TEST_COMMON_ENV_GUARD_HH

#include <cstdlib>
#include <string>
#include <utility>

namespace Dune::XT::Common::Test {


/**
 * \brief Sets an environment variable for the duration of a scope and restores it afterwards.
 *
 * Restoration happens in the destructor rather than at the end of the test body, so it also runs when an ASSERT_*
 * aborts the test early -- a failing test should not leak its environment into the ones which run after it.
 *
 * The main user is TERM: everything in dune-xt which decides whether to emit ANSI escape sequences goes through
 * Dune::XT::Common::terminal_supports_color(), which inspects TERM. Tests which assert on such output have to pin
 * TERM down, or they pass or fail depending on the terminal they happen to be run from.
 */
class ScopedEnvVar
{
public:
  //! Pass nullptr as value to remove the variable from the environment for the duration of the scope.
  ScopedEnvVar(std::string name, const char* value)
    : name_(std::move(name))
  {
    if (const char* previous = std::getenv(name_.c_str()); previous != nullptr) {
      was_set_ = true;
      previous_ = previous;
    }
    assign(value);
  }

  // Restoring the environment twice would undo the wrong thing, so this guard is neither copyable nor movable.
  ScopedEnvVar(const ScopedEnvVar&) = delete;
  ScopedEnvVar(ScopedEnvVar&&) = delete;
  ScopedEnvVar& operator=(const ScopedEnvVar&) = delete;
  ScopedEnvVar& operator=(ScopedEnvVar&&) = delete;

  ~ScopedEnvVar()
  {
    assign(was_set_ ? previous_.c_str() : nullptr);
  }

private:
  void assign(const char* value) const
  {
    if (value == nullptr)
      ::unsetenv(name_.c_str());
    else
      ::setenv(name_.c_str(), value, 1);
  }

  const std::string name_;
  bool was_set_{false};
  std::string previous_;
};


} // namespace Dune::XT::Common::Test

#endif // DUNE_XT_TEST_COMMON_ENV_GUARD_HH
