// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   René Fritze    (2026)

// Covers dune/xt/common/filesystem.hh: path splitting, directory creation, touch, the ofstream/ifstream factories,
// the filtered file-to-stream copy and meminfo.
//
// Everything that touches the disk does so below a per-test directory derived from get_unique_test_name(), which is
// removed again at the end of the test, so the tests neither collide with each other nor leave anything behind.

#include <dune/xt/test/main.hxx> // <- This one has to come first, includes config.h!

#include <sstream>
#include <string>

#include <boost/filesystem.hpp>

#include <dune/xt/common/filesystem.hh>
#include <dune/xt/common/logging.hh>
#include <dune/xt/common/logstreams.hh>

#include <dune/xt/test/common.hh>
#include <dune/xt/test/common/scoped_test_dir.hh>

using namespace Dune::XT::Common;

namespace {


using Dune::XT::Common::Test::ScopedTestDir;

//! Every test below works inside a directory of its own, which its ScopedTestDir removes again.
struct FilesystemTest : public ::testing::Test
{
  const ScopedTestDir dir{"test_filesystem_"};
};


} // namespace


TEST_F(FilesystemTest, directory_only)
{
  EXPECT_EQ("some/path", directory_only("some/path/file.txt"));
  EXPECT_EQ("some", directory_only("some/file.txt"));
  EXPECT_EQ("/absolute/path", directory_only("/absolute/path/file.txt"));
  // "return empty string if only filename present"
  EXPECT_EQ("", directory_only("file.txt"));
  EXPECT_EQ("", directory_only(""));
}


TEST_F(FilesystemTest, filename_only)
{
  EXPECT_EQ("file.txt", filename_only("some/path/file.txt"));
  EXPECT_EQ("file.txt", filename_only("/absolute/path/file.txt"));
  EXPECT_EQ("file.txt", filename_only("file.txt"));
  EXPECT_EQ("", filename_only(""));
}


TEST_F(FilesystemTest, create_directory_of_strips_the_filename)
{
  const auto nested = dir.file("a/b");
  ASSERT_FALSE(boost::filesystem::exists(nested));

  create_directory_of(nested + "/some_file.txt");
  EXPECT_TRUE(boost::filesystem::is_directory(nested));
  // The filename must not have been created as a directory of its own.
  EXPECT_FALSE(boost::filesystem::exists(nested + "/some_file.txt"));

  // Calling it again on an existing directory is fine ...
  EXPECT_NO_THROW(create_directory_of(nested + "/some_file.txt"));
  // ... and a path without any directory component is a no-op. This one has to stay a bare filename (that is the
  // case under test), so it is made unique to keep it from colliding with anything else in the working directory.
  const auto bare_filename = Dune::XT::Common::Test::get_unique_test_name() + ".txt";
  ASSERT_FALSE(boost::filesystem::exists(bare_filename));
  EXPECT_NO_THROW(create_directory_of(bare_filename));
  EXPECT_FALSE(boost::filesystem::exists(bare_filename));
}


TEST_F(FilesystemTest, create_directory_creates_the_path_itself)
{
  const auto nested = dir.file("a/b");
  ASSERT_FALSE(boost::filesystem::exists(nested));

  // Unlike create_directory_of(), no component is stripped: the given path is the directory to create.
  create_directory(nested);
  EXPECT_TRUE(boost::filesystem::is_directory(nested));

  // Calling it again on an existing directory is fine ...
  EXPECT_NO_THROW(create_directory(nested));
  // ... and so is a trailing slash (entity_visualization.hh relies on this).
  EXPECT_NO_THROW(create_directory(nested + "/"));
  EXPECT_TRUE(boost::filesystem::is_directory(nested));
  // ... and an empty path is a no-op.
  EXPECT_NO_THROW(create_directory(""));
}


TEST_F(FilesystemTest, touch)
{
  boost::filesystem::create_directories(dir.path());
  const auto file = dir.file("touched.txt");
  ASSERT_FALSE(boost::filesystem::exists(file));

  EXPECT_TRUE(touch(file));
  EXPECT_TRUE(boost::filesystem::is_regular_file(file));
  EXPECT_EQ(0, boost::filesystem::file_size(file));

  // touch does not create intermediate directories, so this one cannot be opened.
  EXPECT_FALSE(touch(dir.file("does/not/exist.txt")));
}


TEST_F(FilesystemTest, make_ofstream_creates_the_leading_directories)
{
  const auto file = dir.file("deeply/nested/output.txt");
  ASSERT_FALSE(boost::filesystem::exists(file));

  {
    auto out = make_ofstream(file);
    ASSERT_NE(nullptr, out);
    ASSERT_TRUE(out->is_open());
    *out << "written by make_ofstream" << std::endl;
  }
  EXPECT_TRUE(boost::filesystem::is_regular_file(file));
  EXPECT_GT(boost::filesystem::file_size(file), 0);

  // Reopening in truncating mode (the default) discards the previous content ...
  {
    auto out = make_ofstream(file);
    ASSERT_TRUE(out->is_open());
  }
  EXPECT_EQ(0, boost::filesystem::file_size(file));

  // ... whereas appending keeps it.
  {
    auto out = make_ofstream(file, std::ios_base::app);
    ASSERT_TRUE(out->is_open());
    *out << "appended" << std::endl;
  }
  const auto size_after_first_append = boost::filesystem::file_size(file);
  EXPECT_GT(size_after_first_append, 0);
  {
    auto out = make_ofstream(file, std::ios_base::app);
    *out << "appended again" << std::endl;
  }
  EXPECT_GT(boost::filesystem::file_size(file), size_after_first_append);
}


TEST_F(FilesystemTest, make_ifstream)
{
  const auto file = dir.file("input.txt");
  {
    auto out = make_ofstream(file);
    *out << "first line" << std::endl << "second line" << std::endl;
  }

  auto in = make_ifstream(file);
  ASSERT_NE(nullptr, in);
  ASSERT_TRUE(in->is_open());
  std::string line;
  ASSERT_TRUE(static_cast<bool>(std::getline(*in, line)));
  EXPECT_EQ("first line", line);
  ASSERT_TRUE(static_cast<bool>(std::getline(*in, line)));
  EXPECT_EQ("second line", line);
  EXPECT_FALSE(static_cast<bool>(std::getline(*in, line)));

  // A missing file yields a stream which is simply not open (make_ifstream does not throw).
  auto missing = make_ifstream(dir.file("no_such_file.txt"));
  ASSERT_NE(nullptr, missing);
  EXPECT_FALSE(missing->is_open());
}


TEST_F(FilesystemTest, file_to_stream_filtered)
{
  const auto file = dir.file("filtered.txt");
  {
    auto out = make_ofstream(file);
    *out << "keep me" << std::endl << "drop this" << std::endl << "keep me too" << std::endl;
  }

  std::stringstream matching;
  file_to_stream_filtered(matching, file, "keep");
  EXPECT_EQ("keep me\nkeep me too\n", matching.str());

  // The empty filter is contained in every line (including the empty one std::getline yields at EOF).
  std::stringstream everything;
  file_to_stream_filtered(everything, file, "");
  EXPECT_NE(std::string::npos, everything.str().find("drop this"));

  std::stringstream nothing;
  file_to_stream_filtered(nothing, file, "not in there");
  EXPECT_EQ("", nothing.str());

  // A file which cannot be opened simply produces no output.
  std::stringstream missing;
  file_to_stream_filtered(missing, dir.file("no_such_file.txt"), "anything");
  EXPECT_EQ("", missing.str());
}


TEST_F(FilesystemTest, meminfo)
{
  // meminfo() reads this process' /proc entries; we only check that it reports something through the given stream.
  std::stringstream out;
  int logflags = LOG_CONSOLE | LOG_INFO;
  OstreamLogStream stream(LOG_INFO, logflags, out);
  meminfo(stream);
  stream.flush();
  const auto reported = out.str();
  EXPECT_NE(std::string::npos, reported.find("Memory info:")) << reported;
  EXPECT_NE(std::string::npos, reported.find("------------")) << reported;
}
