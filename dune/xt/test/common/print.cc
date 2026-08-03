// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2016 - 2017, 2020)
//   René Fritze     (2015 - 2016, 2018 - 2019)
//   Tobias Leibner  (2016, 2020)

#include <dune/xt/test/main.hxx>

#include <sstream>
#include <string>
#include <vector>

#include <dune/common/dynmatrix.hh>
#include <dune/common/fmatrix.hh>

#include <dune/xt/common/configuration.hh>
#include <dune/xt/common/parameter.hh>
#include <dune/xt/common/print.hh>
#include <dune/xt/common/type_traits.hh>

using namespace Dune::XT::Common;
using namespace std;


GTEST_TEST(print, some_types)
{
  std::cout << print(int(1)) << std::endl;
  std::cout << print(double(2)) << std::endl;
  std::cout << print(std::string("foo")) << std::endl;
  std::cout << print(std::vector<double>({1, 2})) << std::endl;
}


GTEST_TEST(repr, some_types)
{
  std::cout << repr(int(1)) << std::endl;
  std::cout << repr(double(2)) << std::endl;
  std::cout << repr(std::string("foo")) << std::endl;
  std::cout << repr(std::vector<double>({1, 2})) << std::endl;
}


GTEST_TEST(OutputIterator, All)
{
  const vector<int> ints{0, 1, 2};
  const string s_ints("0,1,2");
  const vector<string> strings{"a", "b", "c"};
  const string s_strings("a,b,c");
  stringstream stream;
  std::copy(ints.begin(), ints.end(), PrefixOutputIterator<int>(stream, ","));
  EXPECT_EQ(stream.str(), s_ints);
  stream.str("");
  std::copy(strings.begin(), strings.end(), PrefixOutputIterator<string>(stream, ","));
  EXPECT_EQ(stream.str(), s_strings);
}


GTEST_TEST(print, empty_and_nonempty_vectors)
{
  // str() of an empty vector is "[]", repr() names the type and stops after the opening parenthesis.
  std::stringstream str_out;
  str_out << print(std::vector<double>{});
  EXPECT_EQ("[]", str_out.str());

  std::stringstream repr_out;
  repr_out << repr(std::vector<double>{});
  EXPECT_NE(std::string::npos, repr_out.str().find("(")) << repr_out.str();
  EXPECT_EQ(std::string::npos, repr_out.str().find("{")) << repr_out.str();

  std::stringstream one;
  one << print(std::vector<double>{1.});
  EXPECT_EQ("[1]", one.str());

  std::stringstream several;
  several << print(std::vector<double>{1., 2., 3.});
  EXPECT_EQ("[1, 2, 3]", several.str());

  std::stringstream several_repr;
  several_repr << repr(std::vector<double>{1., 2.});
  EXPECT_NE(std::string::npos, several_repr.str().find("{1, 2}")) << several_repr.str();
}


GTEST_TEST(print, matrices)
{
  Dune::FieldMatrix<double, 2, 2> matrix;
  matrix[0][0] = 1.;
  matrix[0][1] = 2.;
  matrix[1][0] = 3.;
  matrix[1][1] = 4.;

  // The default is one line per row ...
  std::stringstream multiline;
  multiline << print(matrix);
  EXPECT_EQ("[[1, 2],\n [3, 4]]", multiline.str());

  // ... which the "oneline" option collapses.
  std::stringstream oneline;
  oneline << print(matrix, {{"oneline", "true"}});
  EXPECT_EQ("[[1, 2], [3, 4]]", oneline.str());

  // repr() prefixes the type name and always spans several lines.
  std::stringstream representation;
  representation << repr(matrix);
  EXPECT_NE(std::string::npos, representation.str().find("[[1, 2],")) << representation.str();
  EXPECT_EQ('(', representation.str().at(representation.str().find('(')));

  // An empty matrix has neither rows nor entries to print.
  const Dune::FieldMatrix<double, 0, 0> empty;
  std::stringstream empty_out;
  empty_out << print(empty);
  EXPECT_EQ("[]", empty_out.str());
  std::stringstream empty_repr;
  empty_repr << repr(empty);
  EXPECT_EQ(std::string::npos, empty_repr.str().find('[')) << empty_repr.str();
}


GTEST_TEST(print, configurations)
{
  const Configuration config({{"key", "value"}, {"sub.other", "42"}});

  // Without "oneline" the Configuration is reported as usual (i.e. as an ini file) ...
  std::stringstream multiline;
  multiline << print(config);
  EXPECT_NE(std::string::npos, multiline.str().find("key = value")) << multiline.str();
  EXPECT_NE(std::string::npos, multiline.str().find("[sub]")) << multiline.str();

  // ... whereas "oneline" produces a json-like dict of the flattened tree.
  std::stringstream oneline;
  oneline << print(config, {{"oneline", "true"}});
  EXPECT_EQ("{\"key\": \"value\", \"sub.other\": \"42\"}", oneline.str());

  std::stringstream empty;
  empty << print(Configuration(), {{"oneline", "true"}});
  EXPECT_EQ("{}", empty.str());

  // repr() falls back to operator<<, i.e. to report().
  std::stringstream representation;
  representation << repr(config);
  EXPECT_NE(std::string::npos, representation.str().find("key = value")) << representation.str();
}


GTEST_TEST(print, parameter_types)
{
  std::stringstream empty_str;
  empty_str << print(ParameterType());
  EXPECT_EQ("{}", empty_str.str());
  std::stringstream empty_repr;
  empty_repr << repr(ParameterType());
  EXPECT_EQ("ParameterType({})", empty_repr.str());

  const ParameterType type({{"t", 1}, {"mu", 3}});
  std::stringstream str_out;
  str_out << print(type);
  EXPECT_EQ("{mu: 3, t: 1}", str_out.str());
  std::stringstream repr_out;
  repr_out << repr(type);
  EXPECT_EQ("ParameterType({mu: 3, t: 1})", repr_out.str());
}


GTEST_TEST(print, parameters)
{
  std::stringstream empty_str;
  empty_str << print(Parameter());
  EXPECT_EQ("{}", empty_str.str());
  std::stringstream empty_repr;
  empty_repr << repr(Parameter());
  EXPECT_EQ("Parameter({})", empty_repr.str());

  const Parameter param({{"t", {1.}}, {"mu", {2., 3.}}});
  std::stringstream str_out;
  str_out << print(param);
  EXPECT_EQ("{mu: [2, 3], t: [1]}", str_out.str());
  std::stringstream repr_out;
  repr_out << repr(param);
  EXPECT_EQ("Parameter({mu: [2, 3], t: [1]})", repr_out.str());
}


namespace {


//! A type without any operator<<, for which no Printer specialization exists either.
struct Unprintable
{};


} // namespace


GTEST_TEST(print, a_type_without_an_operator_gets_a_hint_instead)
{
  std::stringstream out;
  out << print(Unprintable());
  EXPECT_NE(std::string::npos, out.str().find("missing specialization for Printer<T>")) << out.str();
  EXPECT_NE(std::string::npos, out.str().find(Typename<Unprintable>::value())) << out.str();
}


GTEST_TEST(print, dim_to_axis_name)
{
  EXPECT_EQ("x", dim_to_axis_name(0));
  EXPECT_EQ("y", dim_to_axis_name(1));
  EXPECT_EQ("z", dim_to_axis_name(2));
  EXPECT_EQ("X", dim_to_axis_name(0, true));
  EXPECT_EQ("Y", dim_to_axis_name(1, true));
  EXPECT_EQ("Z", dim_to_axis_name(2, true));
}


GTEST_TEST(OutputIterator, without_a_prefix_the_values_are_concatenated)
{
  const vector<int> ints{1, 2, 3};
  stringstream stream;
  std::copy(ints.begin(), ints.end(), PrefixOutputIterator<int>(stream));
  EXPECT_EQ("123", stream.str());

  // An empty range produces no output at all.
  stringstream empty;
  std::copy(ints.begin(), ints.begin(), PrefixOutputIterator<int>(empty, ","));
  EXPECT_EQ("", empty.str());

  // A single value is never preceded by the prefix.
  stringstream single;
  std::copy(ints.begin(), ints.begin() + 1, PrefixOutputIterator<int>(single, ", "));
  EXPECT_EQ("1", single.str());
}
