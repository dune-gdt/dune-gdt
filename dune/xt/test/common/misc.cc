// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   René Fritze    (2026)

// Covers dune/xt/common/misc.hh (get_idx, array_length, both make_array overloads) and dune/xt/common/misc.cc
// (dump_environment).

#include <dune/xt/test/main.hxx> // <- This one has to come first, includes config.h!

#include <array>
#include <cstdlib>
#include <deque>
#include <list>
#include <string>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

#include <dune/xt/common/exceptions.hh>
#include <dune/xt/common/filesystem.hh>
#include <dune/xt/common/misc.hh>
#include <dune/xt/common/string.hh>

#include <dune/xt/test/common.hh>
#include <dune/xt/test/common/env_guard.hh>
#include <dune/xt/test/common/scoped_test_dir.hh>

using namespace Dune::XT::Common;


GTEST_TEST(misc, get_idx)
{
  const std::vector<int> vec{3, 1, 4, 1, 5};
  EXPECT_EQ(0, get_idx(vec, 3));
  EXPECT_EQ(2, get_idx(vec, 4));
  // The *first* match wins.
  EXPECT_EQ(1, get_idx(vec, 1));
  // Not found is reported as -1.
  EXPECT_EQ(-1, get_idx(vec, 42));
  EXPECT_EQ(-1, get_idx(std::vector<int>{}, 0));

  // It works for any std sequence, not just vectors.
  const std::list<std::string> list{"a", "b", "c"};
  EXPECT_EQ(1, get_idx(list, std::string("b")));
  EXPECT_EQ(-1, get_idx(list, std::string("z")));
  const std::deque<double> deque{1., 2.};
  EXPECT_EQ(1, get_idx(deque, 2.));
}


GTEST_TEST(misc, array_length)
{
  int one[1] = {0}; // NOLINT(modernize-avoid-c-arrays)
  double five[5] = {0., 0., 0., 0., 0.}; // NOLINT(modernize-avoid-c-arrays)
  const std::string three[3] = {"a", "b", "c"}; // NOLINT(modernize-avoid-c-arrays)
  EXPECT_EQ(1u, array_length(one));
  EXPECT_EQ(5u, array_length(five));
  EXPECT_EQ(3u, array_length(three));
}


GTEST_TEST(misc, make_array_from_a_single_value)
{
  const auto filled = make_array<int, 3>(7);
  EXPECT_EQ(3u, filled.size());
  for (const auto& entry : filled)
    EXPECT_EQ(7, entry);

  const auto strings = make_array<std::string, 2>(std::string("foo"));
  EXPECT_EQ("foo", strings[0]);
  EXPECT_EQ("foo", strings[1]);
}


GTEST_TEST(misc, make_array_from_a_vector)
{
  // A vector of matching size is copied entry by entry ...
  const auto exact = make_array<int, 3>(std::vector<int>{1, 2, 3});
  EXPECT_EQ(1, exact[0]);
  EXPECT_EQ(2, exact[1]);
  EXPECT_EQ(3, exact[2]);

  // ... a longer one is truncated to the first N entries ...
  const auto truncated = make_array<int, 2>(std::vector<int>{1, 2, 3, 4});
  EXPECT_EQ(1, truncated[0]);
  EXPECT_EQ(2, truncated[1]);

  // ... and a vector holding exactly one entry is broadcast.
  const auto broadcast = make_array<int, 4>(std::vector<int>{9});
  for (const auto& entry : broadcast)
    EXPECT_EQ(9, entry);

  // A size-1 vector is broadcast even when a single entry is all that is asked for.
  const auto single = make_array<int, 1>(std::vector<int>{9});
  EXPECT_EQ(9, single[0]);
}


GTEST_TEST(misc, make_array_from_a_too_short_vector_throws)
{
  EXPECT_THROW((make_array<int, 3>(std::vector<int>{1, 2})), Exceptions::shapes_do_not_match);
  EXPECT_THROW((make_array<int, 2>(std::vector<int>{})), Exceptions::shapes_do_not_match);
}


GTEST_TEST(misc, dump_environment)
{
  // dump_environment writes two lines: the names of all "key=value" entries of the environment and their values.
  // Setting a variable of our own gives us something to look for in either of them.
  const Dune::XT::Common::Test::ScopedEnvVar variable("DUNE_XT_TEST_MISC_VARIABLE", "dune_xt_test_misc_value");

  const Dune::XT::Common::Test::ScopedTestDir dir("test_misc_");
  const auto file = dir.file("environment.csv");
  {
    auto out = make_ofstream(file);
    ASSERT_TRUE(out->is_open());
    dump_environment(*out);
  }

  boost::filesystem::ifstream in(file);
  ASSERT_TRUE(in.is_open());
  std::string header;
  std::string values;
  ASSERT_TRUE(static_cast<bool>(std::getline(in, header)));
  ASSERT_TRUE(static_cast<bool>(std::getline(in, values)));

  // Environment variable names cannot contain the separator, so the header really is a list of names. The values are
  // written unquoted though, so a value containing the separator would blur the column boundaries -- which is why the
  // value is only searched for as a substring below instead of being addressed by column.
  const auto keys = tokenize(header, ",");
  EXPECT_GE(get_idx(keys, std::string("DUNE_XT_TEST_MISC_VARIABLE")), 0) << "header was: " << header;
  EXPECT_NE(std::string::npos, values.find("dune_xt_test_misc_value")) << "values were: " << values;

  // The separator is configurable.
  const auto other_file = dir.file("environment_semicolon.csv");
  {
    auto out = make_ofstream(other_file);
    dump_environment(*out, ";");
  }
  boost::filesystem::ifstream other_in(other_file);
  ASSERT_TRUE(other_in.is_open());
  std::string other_header;
  ASSERT_TRUE(static_cast<bool>(std::getline(other_in, other_header)));
  EXPECT_NE(std::string::npos, other_header.find(';')) << other_header;
}
