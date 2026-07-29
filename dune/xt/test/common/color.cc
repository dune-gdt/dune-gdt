// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2012, 2014 - 2017)
//   René Fritze     (2012 - 2016, 2018 - 2019)
//   Stefan Girke    (2012)
//   Tobias Leibner  (2014, 2016, 2018, 2020)

#include <dune/xt/test/main.hxx>

#include <string>

#include <dune/xt/common/color.hh>

#include <dune/xt/test/common/env_guard.hh>

using namespace Dune::XT::Common;
using Dune::XT::Common::Test::ScopedEnvVar;


GTEST_TEST(Color, All)
{
  std::cout << "Testing some color codes for this terminal." << std::endl;
  std::cout << "WARNING: This test will succeed although some of the color codes" << std::endl;
  std::cout << "are not supported by your terminal!" << std::endl;
  std::cout << "(Sometimes 'blink', 'reverse' or 'italic' are not supported.)" << std::endl;
  std::cout << StreamModifiers::underline << "a simple 'hello world':" << StreamModifiers::endunderline << " "
            << "hello world" << std::endl;
  std::cout << StreamModifiers::underline << "a colored 'hello world':" << StreamModifiers::endunderline << " "
            << highlight_string("hello world", 1) << std::endl;
  std::cout << StreamModifiers::underline << "a colored 'hello world':" << StreamModifiers::endunderline << " "
            << Colors::bgreen << "hello world" << StreamModifiers::normal << std::endl;
  std::cout << StreamModifiers::underline << "a blinking 'hello world':" << StreamModifiers::endunderline << " "
            << StreamModifiers::blink << "hello world" << StreamModifiers::endblink << std::endl;
  std::cout << StreamModifiers::underline << "an italic 'hello world':" << StreamModifiers::endunderline << " "
            << StreamModifiers::italic << "hello world" << StreamModifiers::enditalic << std::endl;
  std::cout << StreamModifiers::underline << "an underlined 'hello world':" << StreamModifiers::endunderline << " "
            << StreamModifiers::underline << "hello world" << StreamModifiers::endunderline << std::endl;
  std::cout << StreamModifiers::underline << "a reverse 'hello world':" << StreamModifiers::endunderline << " "
            << StreamModifiers::reverse << "hello world" << StreamModifiers::endreverse << std::endl;
  std::cout << StreamModifiers::underline
            << "a 'hello world' with highlighted substrings ('o'):" << StreamModifiers::endunderline << " "
            << highlight_search_string("hello world", "o", 3) << std::endl;
  std::cout << StreamModifiers::underline << "a highlighted 'hello world'-template:" << StreamModifiers::endunderline
            << " " << highlight_template("Hello< World, Hello< World, Hello< World< Hello, World > > > >") << std::endl;
  std::cout << StreamModifiers::underline
            << "a highlighted 'hello world'-template only showing two levels:" << StreamModifiers::endunderline << " "
            << highlight_template("Hello< World, Hello< World, Hello< World< Hello, World > > > >", 2) << std::endl;
  std::cout << StreamModifiers::underline
            << "colored 'hello world' for all available colors( 0 - 255):" << StreamModifiers::endunderline << " "
            << std::endl;
  for (size_t i = 0; i < 256; ++i)
    std::cout << highlight_string("hello world - ", i);
  std::cout << std::endl;
} // main

GTEST_TEST(Color, color_by_number)
{
  // color(i) is the plain 256 color foreground escape sequence, without any wrap-around ...
  EXPECT_EQ("\033[38;5;0m", color(size_t(0)));
  EXPECT_EQ("\033[38;5;42m", color(size_t(42)));
  EXPECT_EQ("\033[38;5;255m", color(size_t(255)));
  EXPECT_EQ("\033[38;5;256m", color(size_t(256)));
  // ... and backcolor(i) currently yields the very same sequence.
  EXPECT_EQ(color(size_t(0)), backcolor(size_t(0)));
  EXPECT_EQ(color(size_t(42)), backcolor(size_t(42)));
}

GTEST_TEST(Color, color_map)
{
  const auto& map = color_map();
  // The map is built on the first call and holds the 16 foreground colors of Colors.
  EXPECT_EQ(16u, map.size());
  EXPECT_EQ(Colors::black, map.at("black"));
  EXPECT_EQ(Colors::red, map.at("red"));
  EXPECT_EQ(Colors::white, map.at("white"));
  EXPECT_EQ(Colors::darkgray, map.at("darkgray"));
  EXPECT_EQ(0u, map.count("chartreuse"));

  // It is a singleton: repeated calls hand out the very same map, built only once.
  EXPECT_EQ(&map, &color_map());
  EXPECT_EQ(16u, color_map().size());
}

GTEST_TEST(Color, color_by_name)
{
  EXPECT_EQ(Colors::red, color(std::string("red")));
  EXPECT_EQ(Colors::lightblue, color(std::string("lightblue")));
  // An unknown name yields the empty string rather than throwing, so that callers can pass user input through.
  EXPECT_EQ("", color(std::string("chartreuse")));
  EXPECT_EQ("", color(std::string("")));
}

GTEST_TEST(Color, template_color_chooser)
{
  // The nesting level is mapped into the 256 color range.
  EXPECT_EQ(0u, template_color_chooser(0));
  EXPECT_EQ(255u, template_color_chooser(255));
  EXPECT_EQ(0u, template_color_chooser(256));
  EXPECT_EQ(1u, template_color_chooser(257));
}

GTEST_TEST(Color, terminal_supports_color)
{
  for (const auto* term : {"xterm", "xterm-color", "xterm-256color", "screen", "linux", "cygwin"}) {
    ScopedEnvVar guard("TERM", term);
    EXPECT_TRUE(terminal_supports_color()) << "TERM = " << term;
  }
  for (const auto* term : {"dumb", "vt100", ""}) {
    ScopedEnvVar guard("TERM", term);
    EXPECT_FALSE(terminal_supports_color()) << "TERM = " << term;
  }
  {
    // No TERM at all in the environment.
    ScopedEnvVar guard("TERM", nullptr);
    EXPECT_FALSE(terminal_supports_color());
  }
}

GTEST_TEST(Color, color_string_respects_the_terminal)
{
  {
    ScopedEnvVar guard("TERM", "xterm");
    EXPECT_EQ(std::string(Colors::red) + "text" + StreamModifiers::normal, color_string("text", Colors::red));
    // The default color is brown.
    EXPECT_EQ(std::string(Colors::brown) + "text" + StreamModifiers::normal, color_string("text"));
    EXPECT_EQ(std::string(Colors::red) + "text" + StreamModifiers::normal, color_string_red("text"));
  }
  {
    // Without a color capable terminal the string is handed back unchanged.
    ScopedEnvVar guard("TERM", "dumb");
    EXPECT_EQ("text", color_string("text", Colors::red));
    EXPECT_EQ("text", color_string("text"));
    EXPECT_EQ("text", color_string_red("text"));
  }
}

GTEST_TEST(Color, highlight_string)
{
  EXPECT_EQ("\033[38;5;0mtext\033[0m", highlight_string("text"));
  EXPECT_EQ("\033[38;5;7mtext\033[0m", highlight_string("text", 7));
  // The color number wraps around at 256.
  EXPECT_EQ(highlight_string("text", 7), highlight_string("text", 7 + 256));
}

GTEST_TEST(Color, highlight_search_string)
{
  // Every occurrence of the substring is wrapped into the color and a reset; the whole string is prefixed by a reset.
  const auto highlighted = highlight_search_string("aXbXc", "X", 1);
  EXPECT_EQ("\033[0ma\033[38;5;1mX\033[0mb\033[38;5;1mX\033[0mc", highlighted);
  // A substring which does not occur leaves the string alone (up to the leading reset).
  EXPECT_EQ("\033[0mabc", highlight_search_string("abc", "X", 1));
}

GTEST_TEST(Color, highlight_template)
{
  // Without any angle brackets nothing but the trailing reset is added.
  EXPECT_EQ("Foo\033[0m", highlight_template("Foo"));
  // The color of a level is inserted in front of its opening bracket and the color of the enclosing level behind its
  // closing one, so that everything between the two brackets is printed in the level's color.
  EXPECT_EQ("Foo\033[38;5;1m<Bar>\033[38;5;0m\033[0m", highlight_template("Foo<Bar>"));
  EXPECT_EQ("Foo\033[38;5;1m<Bar\033[38;5;2m<Baz>\033[38;5;1m>\033[0m", highlight_template("Foo<Bar<Baz>>"));

  // Deeper nestings are collapsed into an empty "<>" once maxlevel is reached.
  const std::string deeply_nested("A< B, A< B, A< B< A, B > > > >");
  const auto full = highlight_template(deeply_nested);
  const auto truncated = highlight_template(deeply_nested, 2);
  EXPECT_LT(truncated.size(), full.size());
  EXPECT_NE(std::string::npos, truncated.find("<>")) << truncated;
  EXPECT_EQ(std::string::npos, truncated.find("A, B")) << truncated;
  // The unrestricted version keeps everything.
  EXPECT_NE(std::string::npos, full.find("A, B")) << full;
}
