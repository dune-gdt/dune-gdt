// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2014, 2016 - 2017)
//   René Fritze     (2014 - 2016, 2018 - 2019)
//   Tobias Leibner  (2016, 2020)

#ifndef DUNE_XT_COMMON_TEST_MAIN_CATCH_EXCEPTIONS
#  define DUNE_XT_COMMON_TEST_MAIN_CATCH_EXCEPTIONS 0
#endif

#include "config.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>

#include <gtest/gtest.h>
#include <dune/xt/common/color.hh>
#include <dune/xt/common/exceptions.hh>
#include <dune/xt/common/timedlogging.hh>

#include <dune/xt/test/common.hh>

using namespace Dune;
using namespace Dune::XT;
using namespace Dune::XT::Common;

void before_create()
{
  TimedLogger().get("before_create").info() << "this info should not be visible" << std::endl;
  TimedLogger().get("before_create").debug() << "this debug should not be visible" << std::endl;
  TimedLogger().get("before_create").warn() << "this warning should be visible in red" << std::endl;
}

void after_create_inner()
{
  TimedLogger().get("after_create_inner").info() << "this info should be visible in blue" << std::endl;
  TimedLogger().get("after_create_inner").debug() << "this debug should not be visible" << std::endl;
  TimedLogger().get("after_create_inner").warn() << "this warning should not be visible" << std::endl;
}

void after_create()
{
  auto logger = TimedLogger().get("after_create");
  logger.info() << "this info should be visible in blue" << std::endl;
  logger.debug() << "this debug should be visible in yellow" << std::endl;
  logger.warn() << "this warning should not be visible" << std::endl;
  after_create_inner();
}

void fool_level_tracking_inner()
{
  TimedLogger().get("fool_level_tracking_inner").info() << "this info should be visible in blue" << std::endl;
  TimedLogger().get("fool_level_tracking_inner").debug() << "this debug should be visible in yellow" << std::endl;
  TimedLogger().get("fool_level_tracking_inner").warn() << "this warning should not be visible" << std::endl;
}

void fool_level_tracking()
{
  TimedLogger().get("fool_level_tracking").info() << "this info should be visible in blue" << std::endl;
  TimedLogger().get("fool_level_tracking").debug() << "this debug should be visible in yellow" << std::endl;
  TimedLogger().get("fool_level_tracking").warn() << "this warning should not be visible" << std::endl;
  fool_level_tracking_inner();
}

GTEST_TEST(TimedPrefixedLogStream, all)
{
  Timer timer;
  TimedPrefixedLogStream out(timer, "prefix: ", std::cout);
  out << "sample\nline" << std::flush;
  busywait(2000);
  out << "\n" << 3 << "\n\nend" << std::endl;
} // TEST(TimedPrefixedLogStream, all)

GTEST_TEST(TimedLogger, before_create)
{
  auto logger = TimedLogger().get("main");
  auto& info = logger.info();
  info << "this info should be visible in " << TimedLogging::default_info_color() << std::endl;
  logger.debug() << "this debug should not be visible" << std::endl;
  logger.warn() << "this warning should be visible in " << TimedLogging::default_warning_color() << std::endl;
  before_create();
}

GTEST_TEST(TimedLogger, after_create)
{
  TimedLogger().create(10, 1, false, true, "blue", "yellow");
  auto logger = TimedLogger().get("main");
  logger.info() << "this info should be visible in blue" << std::endl;
  logger.debug() << "this debug should be visible in yellow" << std::endl;
  logger.warn() << "this warning should not be visible" << std::endl;
  after_create();
}

GTEST_TEST(TimedLogger, fool_level_tracking)
{
  auto logger = TimedLogger().get("");
  logger.info() << "this info should be visible in blue" << std::endl;
  logger.debug() << "this debug should be visible in yellow" << std::endl;
  logger.warn() << "this warning should not be visible" << std::endl;
  fool_level_tracking();
}

namespace {


//! Redirects std::cout and std::cerr into a stringstream for as long as it is alive.
class CapturedOutput
{
public:
  CapturedOutput()
    : cout_buffer_(std::cout.rdbuf(captured_.rdbuf()))
    , cerr_buffer_(std::cerr.rdbuf(captured_.rdbuf()))
  {
  }

  ~CapturedOutput()
  {
    std::cout.rdbuf(cout_buffer_);
    std::cerr.rdbuf(cerr_buffer_);
  }

  std::string str() const
  {
    return captured_.str();
  }

private:
  std::stringstream captured_;
  std::streambuf* cout_buffer_;
  std::streambuf* cerr_buffer_;
};


//! Restores the process wide default_logger_state(), which the tests below change.
class DefaultLoggerStateGuard
{
public:
  DefaultLoggerStateGuard()
    : previous_(default_logger_state())
  {
  }

  ~DefaultLoggerStateGuard()
  {
    default_logger_state() = previous_;
  }

private:
  const std::array<bool, 3> previous_;
};


//! Sets TERM for the duration of a test and restores whatever was there before.
class TermGuard
{
public:
  explicit TermGuard(const char* term)
  {
    const char* previous = std::getenv("TERM");
    if (previous != nullptr) {
      had_term_ = true;
      previous_ = previous;
    }
    if (term == nullptr)
      ::unsetenv("TERM");
    else
      ::setenv("TERM", term, 1);
  }

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


} // namespace


GTEST_TEST(DefaultLogger, the_initial_state_is_what_it_was_constructed_with)
{
  DefaultLogger all_off("off", {{false, false, false}});
  EXPECT_FALSE(all_off.info_enabled());
  EXPECT_FALSE(all_off.debug_enabled());
  EXPECT_FALSE(all_off.warn_enabled());
  EXPECT_EQ("off", all_off.prefix);
  EXPECT_EQ(0u, all_off.copy_count);

  DefaultLogger all_on("on", {{true, true, true}});
  EXPECT_TRUE(all_on.info_enabled());
  EXPECT_TRUE(all_on.debug_enabled());
  EXPECT_TRUE(all_on.warn_enabled());

  // The default is whatever default_logger_state() currently says.
  DefaultLoggerStateGuard guard;
  default_logger_state() = {{true, false, true}};
  DefaultLogger defaulted;
  EXPECT_TRUE(defaulted.info_enabled());
  EXPECT_FALSE(defaulted.debug_enabled());
  EXPECT_TRUE(defaulted.warn_enabled());
  EXPECT_EQ("", defaulted.prefix);
}


GTEST_TEST(DefaultLogger, get_state_or_and_get_state_and_do_not_modify_the_logger)
{
  const std::array<bool, 3> initial{{true, false, false}};
  DefaultLogger logger("logger", initial);

  const std::array<bool, 3> other{{false, true, false}};
  EXPECT_EQ((std::array<bool, 3>{{true, true, false}}), logger.get_state_or(other));
  EXPECT_EQ((std::array<bool, 3>{{false, false, false}}), logger.get_state_and(other));
  // Both are const and only compute; the logger itself is untouched.
  EXPECT_EQ(initial, logger.state);

  EXPECT_EQ(initial, logger.get_state_or({{false, false, false}}));
  EXPECT_EQ(initial, logger.get_state_and({{true, true, true}}));
}


GTEST_TEST(DefaultLogger, state_or_state_and_and_disable_modify_the_logger)
{
  DefaultLogger logger("logger", {{true, false, false}});

  logger.state_or({{false, true, false}});
  EXPECT_EQ((std::array<bool, 3>{{true, true, false}}), logger.state);

  logger.state_and({{false, true, true}});
  EXPECT_EQ((std::array<bool, 3>{{false, true, false}}), logger.state);

  logger.disable();
  EXPECT_EQ((std::array<bool, 3>{{false, false, false}}), logger.state);
  EXPECT_FALSE(logger.info_enabled());
  EXPECT_FALSE(logger.debug_enabled());
  EXPECT_FALSE(logger.warn_enabled());
}


GTEST_TEST(DefaultLogger, enable_resets_the_state_and_optionally_the_prefix)
{
  DefaultLoggerStateGuard guard;
  default_logger_state() = {{true, true, false}};

  DefaultLogger logger("original", {{false, false, false}});
  // Without an argument only the state is reset ...
  logger.enable();
  EXPECT_EQ((std::array<bool, 3>{{true, true, false}}), logger.state);
  EXPECT_EQ("original", logger.prefix);

  // ... with one the prefix is replaced as well (and the copy count starts over).
  DefaultLogger copy(logger);
  ASSERT_EQ(1u, copy.copy_count);
  copy.enable("renamed");
  EXPECT_EQ("renamed", copy.prefix);
  EXPECT_EQ(0u, copy.copy_count);
  EXPECT_EQ((std::array<bool, 3>{{true, true, false}}), copy.state);
}


GTEST_TEST(DefaultLogger, enable_like_copies_only_the_state)
{
  DefaultLogger source("source", {{true, false, true}});
  DefaultLogger target("target", {{false, true, false}});

  target.enable_like(source);
  EXPECT_EQ(source.state, target.state);
  // The prefix is not part of the state.
  EXPECT_EQ("target", target.prefix);
  EXPECT_EQ("source", source.prefix);
}


GTEST_TEST(DefaultLogger, copies_are_counted_so_that_their_output_can_be_told_apart)
{
  DefaultLogger original("logger", {{true, true, true}});
  EXPECT_EQ(0u, original.copy_count);

  DefaultLogger first_copy(original);
  EXPECT_EQ(1u, first_copy.copy_count);
  EXPECT_EQ("logger", first_copy.prefix);
  EXPECT_EQ(original.state, first_copy.state);

  DefaultLogger second_copy(first_copy);
  EXPECT_EQ(2u, second_copy.copy_count);

  // Assignment takes the copy count over as is, rather than incrementing it.
  DefaultLogger assigned("other", {{false, false, false}});
  assigned = second_copy;
  EXPECT_EQ(2u, assigned.copy_count);
  EXPECT_EQ("logger", assigned.prefix);
  EXPECT_EQ(second_copy.state, assigned.state);

  // Self assignment is a no-op (through a reference, so that no compiler warns about the obvious).
  DefaultLogger& alias = assigned;
  assigned = alias;
  EXPECT_EQ(2u, assigned.copy_count);
  EXPECT_EQ("logger", assigned.prefix);
}


GTEST_TEST(DefaultLogger, an_enabled_stream_prints_with_its_prefix)
{
  // A color capable terminal would wrap the prefix into escape sequences, which would make the assertions below
  // depend on the environment the test is run in.
  TermGuard term("dumb");
  std::string captured;
  {
    CapturedOutput capture;
    DefaultLogger logger("prfx", {{true, true, true}});
    logger.info() << "an info" << std::endl;
    logger.debug() << "a debug" << std::endl;
    logger.warn() << "a warning" << std::endl;
    captured = capture.str();
  }
  EXPECT_NE(std::string::npos, captured.find("prfx: an info")) << captured;
  EXPECT_NE(std::string::npos, captured.find("prfx: a debug")) << captured;
  EXPECT_NE(std::string::npos, captured.find("prfx: a warning")) << captured;
}


GTEST_TEST(DefaultLogger, an_empty_prefix_falls_back_to_the_stream_name)
{
  TermGuard term("dumb");
  std::string captured;
  {
    CapturedOutput capture;
    DefaultLogger logger("", {{true, true, true}});
    logger.info() << "an info" << std::endl;
    logger.debug() << "a debug" << std::endl;
    logger.warn() << "a warning" << std::endl;
    captured = capture.str();
  }
  EXPECT_NE(std::string::npos, captured.find("info: an info")) << captured;
  EXPECT_NE(std::string::npos, captured.find("debug: a debug")) << captured;
  EXPECT_NE(std::string::npos, captured.find("warn: a warning")) << captured;
}


GTEST_TEST(DefaultLogger, a_disabled_stream_discards_everything)
{
  std::string captured;
  {
    CapturedOutput capture;
    DefaultLogger logger("prfx", {{false, false, false}});
    logger.info() << "an info" << std::endl;
    logger.debug() << "a debug" << std::endl;
    logger.warn() << "a warning" << std::endl;
    captured = capture.str();
  }
  EXPECT_EQ("", captured);
}


GTEST_TEST(TimedLogging, create_may_only_be_called_once_per_instance)
{
  TimedLogging logging;
  EXPECT_NO_THROW(logging.create(1, 1));
  EXPECT_THROW(logging.create(1, 1), Exceptions::logger_error);
}


GTEST_TEST(TimedLogging, the_level_is_tracked_across_nested_managers)
{
  // TimedLogging::create() only overwrites the color prefixes, not the suffixes the ctor derived from the terminal,
  // so pin the terminal down to keep the assertions below independent of the environment.
  TermGuard term("dumb");
  std::string captured;
  {
    CapturedOutput capture;
    TimedLogging logging;
    // Only the outermost level logs info, nothing logs debug.
    logging.create(0, -1, /*enable_warnings=*/true, /*enable_colors=*/false);
    {
      auto outer = logging.get("outer");
      outer.info() << "visible" << std::endl;
      outer.debug() << "never visible" << std::endl;
      outer.warn() << "a warning" << std::endl;
      {
        auto inner = logging.get("inner");
        inner.info() << "too deep" << std::endl;
      }
      // Note that the level is only decremented when a manager is destroyed, not restored to what it was when the
      // manager was created: as long as `outer` is alive, every further manager sits one level deeper.
      auto sibling = logging.get("sibling");
      sibling.info() << "still too deep" << std::endl;
    }
    // With all managers gone the level is back to where it started.
    logging.get("after").info() << "visible again" << std::endl;
    captured = capture.str();
  }
  EXPECT_NE(std::string::npos, captured.find("outer: visible")) << captured;
  EXPECT_NE(std::string::npos, captured.find("outer: a warning")) << captured;
  EXPECT_NE(std::string::npos, captured.find("after: visible again")) << captured;
  EXPECT_EQ(std::string::npos, captured.find("never visible")) << captured;
  EXPECT_EQ(std::string::npos, captured.find("too deep")) << captured;
}


GTEST_TEST(TimedLogging, an_empty_id_falls_back_to_the_stream_name)
{
  TermGuard term("dumb");
  std::string captured;
  {
    CapturedOutput capture;
    TimedLogging logging;
    logging.create(1, 1, true, false);
    auto logger = logging.get("");
    logger.info() << "an info" << std::endl;
    logger.debug() << "a debug" << std::endl;
    logger.warn() << "a warning" << std::endl;
    captured = capture.str();
  }
  EXPECT_NE(std::string::npos, captured.find("info: an info")) << captured;
  EXPECT_NE(std::string::npos, captured.find("warn: a warning")) << captured;
}


GTEST_TEST(TimedLogging, colors_are_only_used_on_a_color_capable_terminal)
{
  {
    TermGuard guard("xterm");
    // The ctor already runs update_colors(), which turns the color names into escape sequences.
    TimedLogging colored;
    std::string captured;
    {
      CapturedOutput capture;
      colored.create(1, 1, true, true, "red", "green", "blue");
      colored.get("colored").info() << "an info" << std::endl;
      captured = capture.str();
    }
    EXPECT_NE(std::string::npos, captured.find(color("red"))) << captured;
    EXPECT_NE(std::string::npos, captured.find(StreamModifiers::bold)) << captured;
    EXPECT_NE(std::string::npos, captured.find(StreamModifiers::normal)) << captured;

    // An unknown color name maps to the empty escape sequence, which clears the suffix as well.
    TimedLogging unknown_color;
    std::string uncolored_capture;
    {
      CapturedOutput capture;
      unknown_color.create(1, 1, true, true, "chartreuse", "chartreuse", "chartreuse");
      unknown_color.get("plain").info() << "an info" << std::endl;
      uncolored_capture = capture.str();
    }
    EXPECT_NE(std::string::npos, uncolored_capture.find("plain: an info")) << uncolored_capture;
    EXPECT_EQ(std::string::npos, uncolored_capture.find(StreamModifiers::bold)) << uncolored_capture;
  }
  {
    TermGuard guard("dumb");
    TimedLogging uncolored;
    std::string captured;
    {
      CapturedOutput capture;
      uncolored.create(1, 1, true, /*enable_colors=*/true, "red", "green", "blue");
      uncolored.get("uncolored").info() << "an info" << std::endl;
      captured = capture.str();
    }
    EXPECT_NE(std::string::npos, captured.find("uncolored: an info")) << captured;
    EXPECT_EQ(std::string::npos, captured.find(color("red"))) << captured;
  }
}


GTEST_TEST(TimedLogging, warnings_can_be_switched_off)
{
  TermGuard term("dumb");
  std::string captured;
  {
    CapturedOutput capture;
    TimedLogging logging;
    logging.create(1, 1, /*enable_warnings=*/false, false);
    auto logger = logging.get("quiet");
    logger.warn() << "should not show up" << std::endl;
    logger.info() << "should show up" << std::endl;
    captured = capture.str();
  }
  EXPECT_EQ(std::string::npos, captured.find("should not show up")) << captured;
  EXPECT_NE(std::string::npos, captured.find("should show up")) << captured;
}


int main(int argc, char** argv)
{
#if DUNE_XT_COMMON_TEST_MAIN_CATCH_EXCEPTIONS
  try {
#endif
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
#if DUNE_XT_COMMON_TEST_MAIN_CATCH_EXCEPTIONS
  } catch (Dune::Exception& e) {
    std::cerr << "\nDune reported error: " << e.what() << std::endl;
    std::abort();
  } catch (std::exception& e) {
    std::cerr << "\n" << e.what() << std::endl;
    std::abort();
  } catch (...) {
    std::cerr << "Unknown exception thrown!" << std::endl;
    std::abort();
  } // try
#endif // DUNE_XT_COMMON_TEST_MAIN_CATCH_EXCEPTIONS
} // ... main(...)
