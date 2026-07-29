// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   René Fritze    (2026)

// Covers dune/xt/common/signals.hh: install_signal_handler(), reset_signal() and the example handler
// handle_interrupt().
//
// handle_interrupt() deliberately terminates the process (it resets the handler and re-raises the signal), so it is
// exercised in a forked child whose exit status is then inspected -- see the death test below. Everything else runs
// in-process, restoring the previous disposition afterwards so that no test which runs later inherits a handler.

#include <dune/xt/test/main.hxx> // <- This one has to come first, includes config.h!

#include <csignal>

#include <unistd.h>

#include <dune/xt/common/signals.hh>

using namespace Dune::XT::Common;

namespace {


//! Set by the handler installed below, so that the test can tell whether it ran.
volatile std::sig_atomic_t handler_invocations = 0;
volatile std::sig_atomic_t last_signal = 0;

void counting_handler(int signal)
{
  // Plain assignments only: compound assignment on a volatile is deprecated since C++20.
  handler_invocations = handler_invocations + 1;
  last_signal = signal;
}

/**
 * \brief Restores the disposition a signal had before the test touched it.
 *
 * Note that this saves and restores the actual previous disposition rather than resetting to SIG_DFL: the test runner
 * may well have a handler of its own installed (SIGINT in particular), and a test has no business dropping it.
 */
struct SignalGuard
{
  explicit SignalGuard(int signal)
    : signal_(signal)
  {
    ::sigaction(signal_, nullptr, &previous_);
  }

  // Restoring the same signal twice would clobber a handler installed in between, so this guard is neither copyable
  // nor movable.
  SignalGuard(const SignalGuard&) = delete;
  SignalGuard(SignalGuard&&) = delete;
  SignalGuard& operator=(const SignalGuard&) = delete;
  SignalGuard& operator=(SignalGuard&&) = delete;

  ~SignalGuard()
  {
    ::sigaction(signal_, &previous_, nullptr);
  }

  const int signal_;
  struct sigaction previous_{};
};


} // namespace


GTEST_TEST(signals, install_signal_handler_installs_the_given_handler)
{
  // SIGUSR1 is not used by anything else here, so raising it is safe.
  SignalGuard guard(SIGUSR1);
  handler_invocations = 0;
  last_signal = 0;

  install_signal_handler(SIGUSR1, counting_handler);
  ASSERT_EQ(0, ::raise(SIGUSR1));
  EXPECT_EQ(1, handler_invocations);
  EXPECT_EQ(SIGUSR1, last_signal);

  // The handler stays installed until it is reset.
  ASSERT_EQ(0, ::raise(SIGUSR1));
  EXPECT_EQ(2, handler_invocations);
}


GTEST_TEST(signals, reset_signal_restores_the_default_disposition)
{
  SignalGuard guard(SIGUSR1);
  handler_invocations = 0;

  install_signal_handler(SIGUSR1, counting_handler);
  reset_signal(SIGUSR1);

  // SIG_DFL for SIGUSR1 is "terminate", so it must not be raised here; querying the disposition is enough.
  struct sigaction current{};
  ASSERT_EQ(0, sigaction(SIGUSR1, nullptr, &current));
  EXPECT_EQ(SIG_DFL, current.sa_handler);
  EXPECT_EQ(0, handler_invocations);
}


GTEST_TEST(signals, install_signal_handler_defaults_to_handle_interrupt_on_SIGINT)
{
  SignalGuard guard(SIGINT);

  install_signal_handler();

  struct sigaction current{};
  ASSERT_EQ(0, sigaction(SIGINT, nullptr, &current));
  // Both sides are of type void(*)(int), so they compare directly -- no cast to void* (which is not something a
  // function pointer may portably be converted to) is needed.
  EXPECT_EQ(&handle_interrupt, current.sa_handler);
  // install_signal_handler() clears sa_flags; the C library is free to add its own bits (glibc sets SA_RESTORER),
  // so only the flags which would change the semantics are checked here.
  EXPECT_EQ(0, current.sa_flags & (SA_SIGINFO | SA_RESETHAND | SA_NODEFER));
}


GTEST_TEST(signals_DeathTest, handle_interrupt_terminates_the_process_with_the_signal_it_was_given)
{
  // handle_interrupt() logs, resets the handler and then kills the process with the very same signal, so the child
  // has to die from SIGINT rather than exit normally.
  EXPECT_EXIT(
      {
        install_signal_handler(SIGINT, handle_interrupt);
        ::raise(SIGINT);
        // Not reached: handle_interrupt() re-raises SIGINT with the default disposition in place.
        _exit(0);
      },
      ::testing::KilledBySignal(SIGINT),
      "");
}
