// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2012, 2014 - 2017)
//   René Fritze     (2012 - 2013, 2015 - 2016, 2018 - 2019)
//   Tobias Leibner  (2014, 2016, 2020)

#include <dune/xt/test/main.hxx>

#include <dune/xt/common/logging.hh>
#include <dune/xt/common/logstreams.hh>

#include <dune/xt/test/common/scoped_test_dir.hh>

void balh(std::ostream& out)
{
  static int c = 0;
  out << "balh " << c << "\n";
  c++;
}

void do_something_that_takes_long(std::ostream& out)
{
  out << "  there should appear five dots, but not too fast:" << std::flush;
  for (size_t i = 0; i < 5; ++i) {
    busywait(666);
    out << "." << std::flush;
  }
  out << std::endl;
} // void do_something_that_takes_long()

GTEST_TEST(LoggerTest, all)
{
  using namespace Dune::XT::Common;
  Logger().create(LOG_CONSOLE | LOG_ERROR);
  Logger().error() << "This should be in output\n";
  Logger().info() << "This should NOT be in output\n";
  Logger().debug() << "dito\n";
  Logger().flush();
  for (int i : {LOG_INFO, LOG_DEBUG, LOG_ERROR}) {
    const int id = Logger().add_stream(LOG_CONSOLE | i);
    Logger().get_stream(id) << "Create a new stream with id: " << id << std::endl;
  }
  DXTC_LOG_ERROR.suspend();
  DXTC_LOG_ERROR << "not in output\n";
  balh(DXTC_LOG_ERROR);
  DXTC_LOG_ERROR.resume();
  DXTC_LOG_ERROR << "in output\n";
  balh(DXTC_LOG_ERROR);

  // this should do nothing whatsoever
  balh(dev_null);
  Logger().flush();

  // this is the desired result:
  LogStream& err = Logger().error();
  std::cout << "begin std::cout test" << std::endl;
  do_something_that_takes_long(std::cout);
  std::cout << "end   std::cout test" << std::endl;
  std::cout << "begin Logger().error() test" << std::endl;
  do_something_that_takes_long(err);
  std::cout << "end   Logger().error() test" << std::endl;
}

GTEST_TEST(LoggerTest, file)
{
  using namespace Dune::XT::Common;
  Logger().create(LOG_INFO | LOG_CONSOLE | LOG_FILE, "test_common_logger", "", "");
  Logger().info() << "This output should be in 'test_common_logger.log'" << std::endl;
}

GTEST_TEST(LoggerTest, get_stream_flags)
{
  using namespace Dune::XT::Common;
  Logger().create(LOG_CONSOLE | LOG_INFO | LOG_ERROR, "test_common_logger_flags", "", "");
  // create() records the flags it was given for each of the three default streams.
  for (const int id : {LOG_ERROR, LOG_DEBUG, LOG_INFO})
    EXPECT_EQ(LOG_CONSOLE | LOG_INFO | LOG_ERROR, Logger().get_stream_flags(id));

  // set_stream_flags() replaces them for a single stream only.
  Logger().set_stream_flags(LOG_INFO, LOG_CONSOLE | LOG_INFO);
  EXPECT_EQ(LOG_CONSOLE | LOG_INFO, Logger().get_stream_flags(LOG_INFO));
  EXPECT_EQ(LOG_CONSOLE | LOG_INFO | LOG_ERROR, Logger().get_stream_flags(LOG_ERROR));

  // Asking for an unknown stream is an error rather than a silent default.
  EXPECT_THROW(Logger().get_stream_flags(1 << 20), Dune::InvalidStateException);
  EXPECT_THROW(Logger().get_stream(1 << 20), Dune::InvalidStateException);

  // Restore the flags for the streams the remaining tests use.
  Logger().set_stream_flags(LOG_INFO, LOG_CONSOLE | LOG_INFO | LOG_ERROR);
}

GTEST_TEST(LoggerTest, add_stream_hands_out_new_ids)
{
  using namespace Dune::XT::Common;
  Logger().create(LOG_CONSOLE | LOG_INFO | LOG_ERROR, "test_common_logger_add_stream", "", "");

  const int first = Logger().add_stream(LOG_CONSOLE | LOG_INFO);
  const int second = Logger().add_stream(LOG_CONSOLE | LOG_INFO);
  EXPECT_NE(first, second);
  EXPECT_GE(first, LOG_NEXT);
  // The stream id is or'ed into the flags of the new stream.
  EXPECT_EQ((LOG_CONSOLE | LOG_INFO | first), Logger().get_stream_flags(first));
  EXPECT_NO_THROW(Logger().get_stream(first) << "into the first added stream" << std::endl);
  EXPECT_NO_THROW(Logger().get_stream(second) << "into the second added stream" << std::endl);
  EXPECT_NO_THROW(Logger().flush());
}

GTEST_TEST(LoggerTest, named_streams_and_devnull)
{
  using namespace Dune::XT::Common;
  Logger().create(LOG_CONSOLE | LOG_INFO | LOG_ERROR | LOG_DEBUG, "test_common_logger_named", "", "");
  // The named accessors are just shorthands for get_stream().
  EXPECT_EQ(&Logger().get_stream(LOG_ERROR), &Logger().error());
  EXPECT_EQ(&Logger().get_stream(LOG_INFO), &Logger().info());
  EXPECT_EQ(&Logger().get_stream(LOG_DEBUG), &Logger().debug());
  // devnull() is a stream of its own which discards everything.
  EXPECT_NE(&Logger().get_stream(LOG_INFO), &Logger().devnull());
  EXPECT_NO_THROW(Logger().devnull() << "this goes nowhere" << std::endl);
  // The macros resolve to the very same streams.
  EXPECT_EQ(&Logger().info(), &DXTC_LOG_INFO);
  EXPECT_EQ(&Logger().debug(), &DXTC_LOG_DEBUG);
  EXPECT_EQ(&Logger().error(), &DXTC_LOG_ERROR);
  EXPECT_EQ(&Logger().devnull(), &DXTC_LOG_DEVNULL);
  // On a single rank the _0 variants are the plain streams.
  EXPECT_EQ(&Logger().info(), &DXTC_LOG_INFO_0);
  EXPECT_EQ(&Logger().debug(), &DXTC_LOG_DEBUG_0);
  EXPECT_EQ(&Logger().error(), &DXTC_LOG_ERROR_0);
}

GTEST_TEST(LoggerTest, log_forwards_to_the_given_stream)
{
  using namespace Dune::XT::Common;
  Logger().create(LOG_CONSOLE | LOG_INFO | LOG_ERROR, "test_common_logger_log", "", "");
  EXPECT_NO_THROW(Logger().log(std::string("a string via log()\n"), LOG_INFO));
  EXPECT_NO_THROW(Logger().log(42, LOG_INFO));
  EXPECT_NO_THROW(Logger().log('\n', LOG_INFO));
  EXPECT_NO_THROW(Logger().flush());
}

GTEST_TEST(LoggerTest, suspend_and_resume_all_streams)
{
  using namespace Dune::XT::Common;
  Logger().create(LOG_CONSOLE | LOG_INFO | LOG_ERROR, "test_common_logger_suspend", "", "");
  // Suspending the whole Logging suspends every one of its streams; both are no-ops when repeated.
  EXPECT_NO_THROW(Logger().suspend());
  EXPECT_NO_THROW(Logger().suspend());
  DXTC_LOG_ERROR << "must not show up" << std::endl;
  EXPECT_NO_THROW(Logger().resume());
  EXPECT_NO_THROW(Logger().resume());
  DXTC_LOG_ERROR << "this one is visible again" << std::endl;

  {
    // The RAII guards do the same for a scope.
    Logging::SuspendLocal suspended;
    DXTC_LOG_ERROR << "must not show up either" << std::endl;
    {
      Logging::ResumeLocal resumed;
      DXTC_LOG_ERROR << "visible inside the ResumeLocal scope" << std::endl;
    }
  }
  DXTC_LOG_ERROR << "visible after both guards went out of scope" << std::endl;
  Logger().flush();
}

GTEST_TEST(LoggerTest, set_prefix_reopens_the_logfile)
{
  using namespace Dune::XT::Common;
  // Both logfiles below are written to relative paths, the second one to the hardcoded default "data/log". Running
  // from inside a directory of our own keeps them off the shared build directory and removes them along with the
  // whole tree, which -- unlike an explicit remove() at the end of the test -- also happens if an assertion above
  // aborts the test early.
  // Fully qualified: inside the test body "Test" would resolve to the ::testing::Test base class.
  const Dune::XT::Common::Test::ScopedTestDir dir("test_logging_");
  const Dune::XT::Common::Test::CurrentPathGuard cwd(dir.path());

  Logger().create(LOG_FILE | LOG_INFO | LOG_ERROR, "test_common_logger_prefix_before", "", "");
  Logger().info() << "before the prefix change" << std::endl;
  Logger().flush();

  // set_prefix() tears the streams down and recreates them with a different logfile name, keeping the flags. It
  // forwards to create() without a datadir/logdir though, so the new logfile ends up below the defaults ("data/log")
  // rather than next to the old one -- which is what "this will probably not do what we want it to" refers to.
  EXPECT_NO_THROW(Logger().set_prefix("test_common_logger_prefix_after"));
  EXPECT_EQ(LOG_FILE | LOG_INFO | LOG_ERROR, Logger().get_stream_flags(LOG_INFO));
  Logger().info() << "after the prefix change" << std::endl;
  Logger().flush();
  EXPECT_TRUE(boost::filesystem::is_regular_file("data/log/test_common_logger_prefix_after.log"));
  // Note: no assertion on the "before" logfile. create() opens its ofstream without closing a previously opened one
  // first, so whether it was actually created depends on what an earlier test in this binary left behind; only
  // set_prefix() (which deinit()s first) is guaranteed to reopen. That is pre-existing behaviour of Logging::create()
  // and not what this test is about.

  // Leave the global Logger in the state main.hxx set it up in. This has to happen before the guards run, since it
  // closes the logfiles which are about to be removed along with the directory.
  Logger().create(LOG_CONSOLE | LOG_ERROR, "", "", "");
}
