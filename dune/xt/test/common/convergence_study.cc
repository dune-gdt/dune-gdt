// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   René Fritze    (2026)

// Covers dune/xt/common/convergence-study.hh: the default implementations of norms(), estimates(), quantities() and
// expected_rate(), the input validation of run() and the shape of the data it returns.
//
// The studies below do not do any actual discretizing: compute() simply hands out precomputed numbers whose EOCs are
// known exactly, which is enough to drive every branch of run() and print_eoc().

#include <dune/xt/test/main.hxx> // <- This one has to come first, includes config.h!

#include <cmath>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <dune/xt/common/convergence-study.hh>
#include <dune/xt/common/exceptions.hh>
#include <dune/xt/common/float_cmp.hh>

using namespace Dune::XT::Common;

namespace {


//! Implements only the pure virtual interface, so that the defaults of the others can be checked.
class MinimalStudy : public ConvergenceStudy
{
public:
  size_t num_refinements() const override
  {
    return 1;
  }

  std::vector<std::string> targets() const override
  {
    return {"h"};
  }

  std::string discretization_info_title() const override
  {
    return "elements";
  }

  std::string discretization_info(const size_t refinement_level) override
  {
    return to_string(refinement_level);
  }

  std::map<std::string, std::map<std::string, double>>
  compute(const size_t /*refinement_level*/,
          const std::vector<std::string>& /*actual_norms*/,
          const std::vector<std::pair<std::string, std::string>>&
          /*actual_estimates*/,
          const std::vector<std::string>& /*actual_quantities*/) override
  {
    return {};
  }
};


//! A study over three levels with h = 2^-level, an error which converges with rate 2 and one extra quantity.
class QuadraticStudy : public ConvergenceStudy
{
public:
  explicit QuadraticStudy(const double first_error = 1.)
    : first_error_(first_error)
  {
  }

  size_t num_refinements() const override
  {
    return 2;
  }

  std::vector<std::string> targets() const override
  {
    return {"h"};
  }

  std::vector<std::string> norms() const override
  {
    return {"L_2"};
  }

  std::vector<std::string> quantities() const override
  {
    // Deliberately longer than the column width, to exercise run()'s header word wrapping.
    return {"time to solution"};
  }

  std::string discretization_info_title() const override
  {
    return "elements";
  }

  std::string discretization_info(const size_t refinement_level) override
  {
    last_level_ = refinement_level;
    return to_string(size_t(1) << refinement_level);
  }

  std::map<std::string, std::map<std::string, double>>
  compute(const size_t refinement_level,
          const std::vector<std::string>& actual_norms,
          const std::vector<std::pair<std::string, std::string>>& /*actual_estimates*/,
          const std::vector<std::string>& actual_quantities) override
  {
    EXPECT_EQ(last_level_, refinement_level) << "discretization_info() is documented to be called first!";
    const double h = std::pow(0.5, double(refinement_level));
    std::map<std::string, std::map<std::string, double>> ret;
    ret["target"]["h"] = h;
    for (const auto& id : actual_norms)
      ret["norm"][id] = (refinement_level == 0) ? first_error_ : h * h;
    for (const auto& id : actual_quantities)
      ret["quantity"][id] = double(refinement_level) + 1.;
    return ret;
  }

private:
  const double first_error_;
  size_t last_level_{0};
};


//! Reports a discretization without any target, which run() has to reject.
class TargetlessStudy : public MinimalStudy
{
public:
  std::vector<std::string> targets() const override
  {
    return {};
  }
};


} // namespace


GTEST_TEST(ConvergenceStudy, the_optional_parts_of_the_interface_default_to_empty)
{
  MinimalStudy study;
  EXPECT_TRUE(study.norms().empty());
  EXPECT_TRUE(study.estimates().empty());
  EXPECT_TRUE(study.quantities().empty());
}


GTEST_TEST(ConvergenceStudy, expected_rate_defaults_to_one)
{
  MinimalStudy study;
  EXPECT_DOUBLE_EQ(1., study.expected_rate("norm", "L_2"));
  EXPECT_DOUBLE_EQ(1., study.expected_rate("quantity", "anything"));
}


GTEST_TEST(ConvergenceStudy, a_study_without_targets_is_rejected)
{
  TargetlessStudy study;
  std::stringstream out;
  EXPECT_THROW(study.run({}, out), Exceptions::you_are_using_this_wrong);
}


GTEST_TEST(ConvergenceStudy, a_study_without_norms_or_quantities_is_rejected)
{
  // MinimalStudy inherits the empty defaults for norms() and quantities(), so there is nothing to tabulate.
  MinimalStudy study;
  std::stringstream out;
  EXPECT_THROW(study.run({}, out), Exceptions::you_are_using_this_wrong);
}


GTEST_TEST(ConvergenceStudy, run_returns_the_data_of_every_level)
{
  QuadraticStudy study;
  std::stringstream out;
  const auto data = study.run({}, out);

  ASSERT_EQ(1u, data.count("target"));
  ASSERT_EQ(1u, data.count("norm"));
  ASSERT_EQ(1u, data.count("quantity"));

  const auto& h = data.at("target").at("h");
  const auto& errors = data.at("norm").at("L_2");
  const auto& times = data.at("quantity").at("time to solution");
  ASSERT_EQ(3u, h.size()) << "num_refinements() == 2 means the levels 0, 1 and 2";
  ASSERT_EQ(3u, errors.size());
  ASSERT_EQ(3u, times.size());
  for (size_t level = 0; level <= 2; ++level) {
    EXPECT_TRUE(FloatCmp::eq(h.at(level), std::pow(0.5, double(level)))) << "level = " << level;
    EXPECT_TRUE(FloatCmp::eq(times.at(level), double(level) + 1.)) << "level = " << level;
  }
  EXPECT_TRUE(FloatCmp::eq(errors.at(0), 1.));
  EXPECT_TRUE(FloatCmp::eq(errors.at(1), 0.25));
  EXPECT_TRUE(FloatCmp::eq(errors.at(2), 0.0625));

  // The table has one line per level plus the header, mentions the column titles and reports the EOC of 2.
  const auto table = out.str();
  EXPECT_NE(std::string::npos, table.find("elements")) << table;
  EXPECT_NE(std::string::npos, table.find("L_2")) << table;
  EXPECT_NE(std::string::npos, table.find("EOC")) << table;
  // There is no EOC on the first level.
  EXPECT_NE(std::string::npos, table.find("----")) << table;
  EXPECT_NE(std::string::npos, table.find("2.00")) << table;
}


GTEST_TEST(ConvergenceStudy, run_only_reports_what_was_asked_for)
{
  QuadraticStudy study;
  std::stringstream out;
  const auto data = study.run({"h", "L_2"}, out);

  EXPECT_EQ(1u, data.count("target"));
  EXPECT_EQ(1u, data.count("norm"));
  // "time to solution" was filtered out, so compute() never produced it.
  EXPECT_EQ(0u, data.count("quantity")) << out.str();
  EXPECT_EQ(std::string::npos, out.str().find("solution")) << out.str();
}


GTEST_TEST(ConvergenceStudy, an_only_these_which_matches_no_target_falls_back_to_all_targets)
{
  // filter() returning nothing for the targets is treated as "no target restriction given".
  QuadraticStudy study;
  std::stringstream out;
  const auto data = study.run({"L_2"}, out);
  ASSERT_EQ(1u, data.count("target"));
  EXPECT_EQ(3u, data.at("target").at("h").size());
}


GTEST_TEST(ConvergenceStudy, a_vanishing_error_is_reported_as_an_infinite_eoc)
{
  // print_eoc() cannot form a quotient if the previous level's error is zero and prints "inf" instead.
  QuadraticStudy study(0.);
  std::stringstream out;
  const auto data = study.run({}, out);
  EXPECT_TRUE(FloatCmp::eq(data.at("norm").at("L_2").at(0), 0.));
  EXPECT_NE(std::string::npos, out.str().find("inf")) << out.str();
}
