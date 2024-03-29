// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2016 - 2017)
//   René Fritze     (2015 - 2016, 2018 - 2019)
//   Tobias Leibner  (2016, 2020)

#include <dune/xt/test/main.hxx>

#include <vector>

#include <dune/xt/common/algorithm.hh>

using namespace Dune::XT::Common;

struct Moveable
{
  Moveable(int i)
    : v(i)
  {
  }
  int v;
};

GTEST_TEST(MoveIfTest, All)
{
  using namespace std;
  using Vec = vector<unique_ptr<Moveable>>;
  using Ptr = Vec::value_type;
  const auto size = 6;
  Vec values;
  for (int i : value_range(size)) {
    values.emplace_back(new Moveable(i * 2));
  }
  Vec out;
  const auto half_size = size / 2;
  move_if(values.begin(), values.end(), std::back_inserter(out), [](Ptr& m) { return m->v < size; });
  EXPECT_EQ(out.size(), half_size);
  auto ctr = [](Ptr& ptr) { return ptr != nullptr; };
  EXPECT_EQ(std::count_if(values.begin(), values.end(), ctr), half_size);
  for (auto i : value_range(half_size)) {
    EXPECT_NE(out[i], nullptr);
    EXPECT_EQ(i * 2, out[i]->v);
  }
  for (auto i : value_range(half_size)) {
    EXPECT_EQ(nullptr, values[i]);
    EXPECT_EQ((i + half_size) * 2, values[i + half_size]->v);
  }
}
