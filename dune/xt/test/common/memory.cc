// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2016 - 2017)
//   René Fritze     (2016, 2018 - 2020)
//   Tobias Leibner  (2020)

#include <dune/xt/test/main.hxx>

#include <string>

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

#include <dune/common/dynmatrix.hh>
#include <dune/common/tupleutility.hh>

#include <dune/xt/common/configuration.hh>
#include <dune/xt/common/memory.hh>
#include <dune/xt/common/fvector.hh>
#include <dune/xt/common/string.hh>

#include <dune/xt/test/common.hh>
#include <dune/xt/test/common/scoped_test_dir.hh>

using namespace Dune::XT::Common;


struct ScopeTest : public testing::Test
{
  using T = int;
  static constexpr T constant = 1;

  template <class P>
  void deref(P& p)
  {
    EXPECT_NE(p, nullptr);
    auto g = *p;
    EXPECT_EQ(constant, g);
  }

  template <class P>
  void access(const P& p)
  {
    auto g = p.access();
    EXPECT_EQ(constant, g);
  }

  template <template <class F> class Provider, class P>
  void scope(P& p)
  {
    Provider<T> shared_provider(p);
    deref(p);
  }

  void check_shared()
  {
    auto shared = std::make_shared<T>(constant);
    scope<ConstStorageProvider>(shared);
    deref(shared);
    scope<StorageProvider>(shared);
    deref(shared);
  }

  void check_const()
  {
    using CSP = ConstStorageProvider<T>;
    access(CSP{new T(constant)});
    CSP{};
    T e{constant};
    CSP{e};
  }
};


TEST_F(ScopeTest, All)
{
  this->check_shared();
  this->check_const();
}


namespace {


//! Reads the two-line CSV mem_usage() writes and returns {header, values}.
std::pair<std::string, std::string> read_memory_csv(const std::string& filename)
{
  boost::filesystem::ifstream in(filename);
  EXPECT_TRUE(in.is_open()) << "could not open " << filename;
  std::string header;
  std::string values;
  std::getline(in, header);
  std::getline(in, values);
  return {header, values};
}


} // namespace


GTEST_TEST(mem_usage, writes_the_peak_consumption_to_the_given_file)
{
  const Dune::XT::Common::Test::ScopedTestDir dir("test_memory_");
  const auto file = dir.file("memory.csv");
  ASSERT_FALSE(boost::filesystem::exists(file));

  mem_usage(file);

  ASSERT_TRUE(boost::filesystem::is_regular_file(file));
  const auto [header, values] = read_memory_csv(file);
  EXPECT_EQ("global.maxPeakMemoryConsumption,global.meanPeakMemoryConsumption", header);
  const auto tokens = tokenize(values, ",");
  ASSERT_EQ(2u, tokens.size()) << "values were: " << values;
  // getrusage reports this process' peak resident set size, which is certainly positive; on a single rank the
  // maximum and the mean over all ranks coincide.
  const auto max_peak = from_string<long>(tokens[0]);
  const auto mean_peak = from_string<long>(tokens[1]);
  EXPECT_GT(max_peak, 0);
  EXPECT_EQ(max_peak, mean_peak);
}


GTEST_TEST(mem_usage, defaults_to_memory_csv_below_the_configured_datadir)
{
  // Dune::ParameterTree has no way to remove a key again, so global.datadir stays set for the rest of this binary.
  // Nothing else in it reads that key, so the mutation is contained.
  const Dune::XT::Common::Test::ScopedTestDir datadir("test_memory_");
  DXTC_CONFIG.set("global.datadir", datadir.path(), /*overwrite=*/true);

  mem_usage();

  EXPECT_TRUE(boost::filesystem::is_regular_file(datadir.file("memory.csv")));
}
