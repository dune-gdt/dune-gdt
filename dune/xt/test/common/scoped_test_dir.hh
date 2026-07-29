// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   René Fritze    (2026)

#ifndef DUNE_XT_TEST_COMMON_SCOPED_TEST_DIR_HH
#define DUNE_XT_TEST_COMMON_SCOPED_TEST_DIR_HH

#include <string>

#include <boost/filesystem.hpp>

#include <dune/xt/test/common.hh>

namespace Dune::XT::Common::Test {


/**
 * \brief A working directory unique to the running test, removed again when the scope ends.
 *
 * The directory is named after get_unique_test_name(), so tests within one binary never collide. Removal happens in
 * the destructor rather than at the end of the test body, so it also runs when an ASSERT_* aborts the test early --
 * a failing test should not leave anything behind either.
 *
 * The directory is only created once something is written into it (make_ofstream() and friends create the leading
 * directories); path() and file() merely name it.
 */
class ScopedTestDir
{
public:
  explicit ScopedTestDir(const std::string& prefix)
    : path_(prefix + get_unique_test_name())
  {
  }

  // Removing the directory twice would remove whatever a later test recreated under the same name, so this guard is
  // neither copyable nor movable.
  ScopedTestDir(const ScopedTestDir&) = delete;
  ScopedTestDir(ScopedTestDir&&) = delete;
  ScopedTestDir& operator=(const ScopedTestDir&) = delete;
  ScopedTestDir& operator=(ScopedTestDir&&) = delete;

  ~ScopedTestDir()
  {
    boost::system::error_code ignored;
    boost::filesystem::remove_all(path_, ignored);
  }

  //! The directory itself.
  const std::string& path() const
  {
    return path_;
  }

  //! The path of an entry below this directory.
  std::string file(const std::string& name) const
  {
    return path_ + "/" + name;
  }

private:
  const std::string path_;
};


/**
 * \brief Switches the working directory for the duration of the scope, creating it if need be.
 *
 * Some of the code under test writes to hardcoded relative paths ("data/log/..." for instance). Running such a test
 * from inside a directory of its own keeps those files off the shared build directory, where a stale copy or a
 * concurrently running test binary could otherwise interfere.
 *
 * \note Declare this *after* the ScopedTestDir it moves into, so that it is destroyed first and the directory's own
 *       (relative) cleanup still resolves against the original working directory.
 */
class CurrentPathGuard
{
public:
  explicit CurrentPathGuard(const std::string& path)
  {
    boost::filesystem::create_directories(path);
    boost::filesystem::current_path(path);
  }

  CurrentPathGuard(const CurrentPathGuard&) = delete;
  CurrentPathGuard(CurrentPathGuard&&) = delete;
  CurrentPathGuard& operator=(const CurrentPathGuard&) = delete;
  CurrentPathGuard& operator=(CurrentPathGuard&&) = delete;

  ~CurrentPathGuard()
  {
    boost::system::error_code ignored;
    boost::filesystem::current_path(previous_, ignored);
  }

private:
  const boost::filesystem::path previous_{boost::filesystem::current_path()};
};


} // namespace Dune::XT::Common::Test

#endif // DUNE_XT_TEST_COMMON_SCOPED_TEST_DIR_HH
