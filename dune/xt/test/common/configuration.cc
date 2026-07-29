// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2014 - 2017)
//   René Fritze     (2012 - 2020)
//   Tobias Leibner  (2014, 2016, 2020)

// This one has to come first (includes the config.h)!
#include <dune/xt/test/main.hxx>

#include <array>
#include <map>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

#include <boost/filesystem.hpp>

#include <boost/assign/list_of.hpp>
#include <boost/array.hpp>

#include <dune/xt/common/configuration.hh>
#include <dune/xt/common/exceptions.hh>
#include <dune/xt/common/float_cmp.hh>
#include <dune/xt/common/fvector.hh>
#include <dune/xt/common/fmatrix.hh>
#include <dune/xt/common/logging.hh>
#include <dune/xt/common/math.hh>
#include <dune/xt/common/matrix.hh>
#include <dune/xt/common/random.hh>
#include <dune/xt/common/tuple.hh>
#include <dune/xt/common/type_traits.hh>
#include <dune/xt/common/validation.hh>

#include <dune/xt/common/filesystem.hh>

#include <dune/xt/test/common.hh>
#include <dune/xt/test/common/float_cmp.hh>
#include <dune/xt/test/common/scoped_test_dir.hh>

// uncomment this for output
// std::ostream& test_out = std::cout;
std::ostream& test_out = DXTC_LOG.devnull();

using namespace Dune::XT::Common;
using namespace Dune::XT::Common::FloatCmp;

struct CreateByOperator
{
  static Configuration create()
  {
    Configuration config;
    config["string"] = "string";
    config["sub1.int"] = "1";
    config["sub2.size_t"] = "1";
    config["sub2.subsub1.vector"] = "[0 1]";
    config["sub2.subsub1.matrix"] = "[0 1; 1 2]";
    return config;
  }
};

struct CreateByOperatorAndAssign
{
  static Configuration create()
  {
    Configuration config;
    config["string"] = "string";
    config["sub1.int"] = "1";
    config["sub2.size_t"] = "1";
    config["sub2.subsub1.vector"] = "[0 1]";
    config["sub2.subsub1.matrix"] = "[0 1; 1 2]";
    Configuration config2;
    config2 = config;
    return config2;
  }
};

struct CreateByKeyAndValueAndAddConfiguration
{
  static Configuration create()
  {
    Configuration config({"string"}, {"string"});
    config.set("sub1.int", "1");
    config.set("sub2.size_t", 1);
    config.add(Configuration({"vector"}, {"[0 1]"}), "sub2.subsub1");
    config.add(Configuration({"matrix"}, {"[0 1; 1 2]"}), "sub2.subsub1");
    Configuration config_1({"bool"}, {true});
    Configuration config_2({"int"}, {int(1)});
    return config;
  }
};

struct CreateByKeyAndValueAndAddParameterTree
{
  static Configuration create()
  {
    const auto str = std::string("string");
    Configuration config({str}, {str});
    config.set("sub1.int", "1");
    config.set("sub2.size_t", 1);
    Dune::ParameterTree paramtree;
    paramtree["vector"] = "[0 1]";
    paramtree["matrix"] = "[0 1; 1 2]";
    config.add(paramtree, "sub2.subsub1");
    return config;
  }
};

struct CreateByKeyAndValueVectorsAndAddParameterTree
{
  static Configuration create()
  {
    using namespace std;
    const auto str = string("string");
    Configuration config(vector<string>({str, "sub1.int"}), {str, string("1")});
    config.set("sub2.size_t", 1);
    Dune::ParameterTree paramtree;
    paramtree["vector"] = "[0 1]";
    paramtree["matrix"] = "[0 1; 1 2]";
    config.add(paramtree, "sub2.subsub1");
    return config;
  }
};

struct CreateByKeysAndValues
{
  static Configuration create()
  {
    return Configuration({"string", "sub1.int", "sub2.size_t", "sub2.subsub1.vector", "sub2.subsub1.matrix"},
                         {"string", "1", "1", "[0 1]", "[0 1; 1 2]"});
  }
};

struct CreateByParameterTree
{
  static Configuration create()
  {
    Dune::ParameterTree paramtree;
    paramtree["string"] = "string";
    paramtree["sub1.int"] = "1";
    paramtree["sub2.size_t"] = "1";
    paramtree["sub2.subsub1.vector"] = "[0 1]";
    paramtree["sub2.subsub1.matrix"] = "[0 1; 1 2]";
    return Configuration(paramtree);
  }
};

using TestTypes =
    testing::Types<double, float, std::string, std::complex<double>, int, unsigned int, unsigned long, long long, char>;

using ConfigurationCreators = testing::Types<CreateByOperator,
                                             CreateByKeyAndValueAndAddConfiguration,
                                             CreateByKeyAndValueAndAddParameterTree,
                                             CreateByKeyAndValueVectorsAndAddParameterTree,
                                             CreateByKeysAndValues,
                                             CreateByParameterTree,
                                             CreateByOperatorAndAssign>;

constexpr auto SEED = std::random_device::result_type(0);

template <class T>
static DefaultRNG<T> rng_setup()
{
  return DefaultRNG<T>(std::numeric_limits<T>::min(), std::numeric_limits<T>::max(), T(SEED));
}

template <>
DefaultRNG<std::string> rng_setup()
{
  return DefaultRNG<std::string>(12, SEED);
}

template <>
DefaultRNG<std::complex<double>> rng_setup()
{
  return DefaultRNG<std::complex<double>>(-2, 2, SEED);
}

template <>
DefaultRNG<double> rng_setup()
{
  return DefaultRNG<double>(-2, 2, SEED);
}

template <class T>
static void val_compare_eq(const T& aa, const T& bb)
{
  DXTC_EXPECT_FLOAT_EQ(aa, bb);
}

static void val_compare_eq(const std::string& aa, const std::string& bb)
{
  EXPECT_EQ(aa, bb);
}

template <class T>
struct ConfigTest : public testing::Test
{
  static constexpr int count = 2;
  DefaultRNG<T> rng;
  RandomStrings key_gen;
  // std::array is not assignable from list_of it seems. Don't make the following two arrays const (triggers boost/intel
  // compiler bug, last tested with icpc version 14.0.3).
  boost::array<T, count> values;
  boost::array<std::string, count> keys;
  ConfigTest()
    : rng(rng_setup<T>())
    , key_gen(8)
    , values(boost::assign::list_of<T>().repeat_fun(values.size() - 1, rng))
    , keys(boost::assign::list_of<std::string>().repeat_fun(values.size() - 1, key_gen))
  {
  }

  ~ConfigTest() override = default;

  void get()
  {
    std::set<std::string> uniq_keys;
    for (T val : values) {
      const auto key = key_gen();
      const auto got_val = DXTC_CONFIG_GET(key, val);
      // since the value invariably goes through string conversion, we need to adjust the expected value as well
      const T adjusted_val = from_string<T>(to_string(val));
      val_compare_eq(adjusted_val, got_val);
      //! TODO add a float compare check that makes sure introduced error is only due to limited precision in str conv
      uniq_keys.insert(key);
    }
    EXPECT_EQ(values.size(), uniq_keys.size());
  }

  void set()
  {
    for (T val : values) {
      auto key = key_gen();
      // since the value invariably goes through string conversion, we need to adjust the expected value as well
      const T adjusted_val = from_string<T>(to_string(val));
      DXTC_CONFIG.set(key, val);
      // get with default diff from expected
      auto re = DXTC_CONFIG.get(key, T(val + Epsilon<T>::value));
      val_compare_eq(re, adjusted_val);
    }
  }

  void other()
  {
    auto key = this->key_gen();
    DXTC_CONFIG.set(key, T());
    EXPECT_THROW(DXTC_CONFIG.get(key, T(), ValidateNone<T>()), Exceptions::configuration_error);
  }

  void issue_42()
  {
    using namespace Dune::XT::Common;
    using namespace std;
    Configuration empty;
    Configuration to_add(vector<string>{"subsection.key"}, {0l});
    empty.add(to_add, "", true);
    EXPECT_TRUE(empty.has_sub("subsection"));
  }

  void add_overwrite(bool do_overwrite)
  {
    using namespace Dune::XT::Common;
    using namespace std;
    Configuration base(vector<string>{"subsection.key"}, {1l});
    Configuration to_add(vector<string>{"subsection.otherkey"}, {0l});
    base.add(to_add, "", do_overwrite);
    EXPECT_TRUE(base.has_sub("subsection"));
    EXPECT_EQ(base.get<long>("subsection.key"), 1l);
    EXPECT_EQ(base.get<long>("subsection.otherkey"), 0l);
  }

  void subtree_ctor()
  {
    using namespace Dune::XT::Common;
    using namespace std;
    Configuration to_add(vector<string>{"key"}, {1l});
    Configuration base(to_add, string("subsection"));
    EXPECT_TRUE(base.has_sub("subsection"));
    EXPECT_EQ(base.get<long>("subsection.key"), 1l);
  }
}; // struct ConfigTest

struct StaticCheck
{
  using Ints = boost::mpl::vector<Int<1>, Int<2>>;

  template <class MatrixType>
  static void check_matrix_static_size(const Configuration& config)
  {
    using MT = MatrixAbstraction<MatrixType>;
    // r and c are non-const to avoid a warning that the lambda capture is unused
    // Maybe we could remove the capture and make r and c const, but then we
    // would have to test that it works with all relevant compilers
    auto r = MT::rows(MatrixType());
    auto c = MT::cols(MatrixType());

    const auto check = [&r, &c](const MatrixType& mat) {
      for (size_t cc = 0; cc < c; ++cc) {
        for (size_t rr = 0; rr < r; ++rr) {
          val_compare_eq(MT::get_entry(mat, rr, cc), double(rr + cc));
        }
      }
    };

    check(config.get("matrix", MatrixType(), r, c));
    check(config.get("matrix", MatrixType()));
    check(config.get<MatrixType>("matrix", r, c));
    check(config.get<MatrixType>("matrix"));
  }

  template <class U, class V>
  static void run(const Configuration& config)
  {
    const auto rows = U::value;
    const auto cols = V::value;
    check_matrix_static_size<FieldMatrix<double, rows, cols>>(config);
    check_matrix_static_size<Dune::FieldMatrix<double, rows, cols>>(config);
  }
};

template <class ConfigurationCreator>
struct ConfigurationTest : public ::testing::Test
{
  template <class VectorType>
  static void check_vector(const Configuration& config)
  {
    VectorType vec = config.get("vector", VectorType(), 1);
    EXPECT_EQ(1, vec.size());
    EXPECT_FLOAT_EQ(0.0, vec[0]);
    vec = config.get("vector", VectorType(), 2);
    EXPECT_EQ(2, vec.size());
    for (auto ii : {0.0, 1.0})
      EXPECT_FLOAT_EQ(ii, vec[ii]);
    vec = config.get<VectorType>("vector", 1);
    EXPECT_EQ(1, vec.size());
    EXPECT_FLOAT_EQ(0.0, vec[0]);
    vec = config.get<VectorType>("vector", 2);
    EXPECT_EQ(2, vec.size());
    for (auto ii : {0.0, 1.0})
      EXPECT_FLOAT_EQ(ii, vec[ii]);
    vec = config.get<VectorType>("vector");
    EXPECT_EQ(2, vec.size());
    for (auto ii : {0.0, 1.0})
      EXPECT_FLOAT_EQ(ii, vec[ii]);
  } // ... check_vector< ... >(...)

  template <class K, int d>
  static void check_field_vector(const Configuration& config)
  {
    using VectorType = FieldVector<K, d>;
    VectorType vec = config.get("vector", VectorType(), d);
    EXPECT_EQ(d, vec.size());
    for (size_t ii = 0; ii < d; ++ii)
      EXPECT_TRUE(FloatCmp::eq(vec[ii], double(ii)));

    vec = config.get<VectorType>("vector", d);
    EXPECT_EQ(vec.size(), d);

    for (size_t ii = 0; ii < d; ++ii)
      EXPECT_TRUE(FloatCmp::eq(vec[ii], double(ii)));

    vec = config.get<VectorType>("vector");
    EXPECT_EQ(vec.size(), d);

    for (size_t ii = 0; ii < d; ++ii)
      EXPECT_TRUE(FloatCmp::eq(vec[ii], double(ii)));
  } // ... check_field_vector< ... >(...)

  template <class MatrixType>
  static void check_matrix(const Configuration& config)
  {
    MatrixType mat = config.get("matrix", MatrixType(), 1, 1);
    using MT = MatrixAbstraction<MatrixType>;
    EXPECT_FALSE(MT::rows(mat) != 1 || MT::cols(mat) != 1);
    EXPECT_TRUE(FloatCmp::eq(MT::get_entry(mat, 0, 0), 0.0));
    mat = config.get("matrix", MatrixType(), 1, 2);
    EXPECT_FALSE(MT::rows(mat) != 1 || MT::cols(mat) != 2);
    EXPECT_TRUE(FloatCmp::eq(MT::get_entry(mat, 0, 0), 0.0));
    EXPECT_TRUE(FloatCmp::eq(MT::get_entry(mat, 0, 1), 1.0));
    mat = config.get("matrix", MatrixType(), 2, 1);
    EXPECT_FALSE(MT::rows(mat) != 2 || MT::cols(mat) != 1);
    EXPECT_TRUE(FloatCmp::eq(MT::get_entry(mat, 0, 0), 0.0));
    EXPECT_TRUE(FloatCmp::eq(MT::get_entry(mat, 1, 0), 1.0));
    mat = config.get("matrix", MatrixType(), 2, 2);
    EXPECT_FALSE(MT::rows(mat) != 2 || MT::cols(mat) != 2);
    EXPECT_TRUE(FloatCmp::eq(MT::get_entry(mat, 0, 0), 0.0));
    EXPECT_TRUE(FloatCmp::eq(MT::get_entry(mat, 0, 1), 1.0));
    EXPECT_TRUE(FloatCmp::eq(MT::get_entry(mat, 1, 0), 1.0));
    EXPECT_TRUE(FloatCmp::eq(MT::get_entry(mat, 1, 1), 2.0));
    mat = config.get("matrix", MatrixType());
    EXPECT_FALSE(MT::rows(mat) != 2 || MT::cols(mat) != 2);
    EXPECT_TRUE(FloatCmp::eq(MT::get_entry(mat, 0, 0), 0.0));
    EXPECT_TRUE(FloatCmp::eq(MT::get_entry(mat, 0, 1), 1.0));
    EXPECT_TRUE(FloatCmp::eq(MT::get_entry(mat, 1, 0), 1.0));
    EXPECT_TRUE(FloatCmp::eq(MT::get_entry(mat, 1, 1), 2.0));

    mat = config.get<MatrixType>("matrix", 1, 1);
    EXPECT_FALSE(MT::rows(mat) != 1 || MT::cols(mat) != 1);
    EXPECT_TRUE(FloatCmp::eq(MT::get_entry(mat, 0, 0), 0.0));
    mat = config.get<MatrixType>("matrix", 1, 2);
    EXPECT_FALSE(MT::rows(mat) != 1 || MT::cols(mat) != 2);
    EXPECT_TRUE(FloatCmp::eq(MT::get_entry(mat, 0, 0), 0.0));
    EXPECT_TRUE(FloatCmp::eq(MT::get_entry(mat, 0, 1), 1.0));
    mat = config.get<MatrixType>("matrix", 2, 1);
    EXPECT_FALSE(MT::rows(mat) != 2 || MT::cols(mat) != 1);
    EXPECT_TRUE(FloatCmp::eq(MT::get_entry(mat, 0, 0), 0.0));
    EXPECT_TRUE(FloatCmp::eq(MT::get_entry(mat, 1, 0), 1.0));
    mat = config.get<MatrixType>("matrix", 2, 2);
    EXPECT_FALSE(MT::rows(mat) != 2 || MT::cols(mat) != 2);
    EXPECT_TRUE(FloatCmp::eq(MT::get_entry(mat, 0, 0), 0.0));
    EXPECT_TRUE(FloatCmp::eq(MT::get_entry(mat, 0, 1), 1.0));
    EXPECT_TRUE(FloatCmp::eq(MT::get_entry(mat, 1, 0), 1.0));
    EXPECT_TRUE(FloatCmp::eq(MT::get_entry(mat, 1, 1), 2.0));
    mat = config.get<MatrixType>("matrix");
    EXPECT_FALSE(MT::rows(mat) != 2 || MT::cols(mat) != 2);
    EXPECT_TRUE(FloatCmp::eq(MT::get_entry(mat, 0, 0), 0.0));
    EXPECT_TRUE(FloatCmp::eq(MT::get_entry(mat, 0, 1), 1.0));
    EXPECT_TRUE(FloatCmp::eq(MT::get_entry(mat, 1, 0), 1.0));
    EXPECT_TRUE(FloatCmp::eq(MT::get_entry(mat, 1, 1), 2.0));
  } // ... check_matrix< ... >(...)

  static void behaves_correctly()
  {
    const Configuration config = ConfigurationCreator::create();
    config.report(); // <- this works as well but will produce output
    config.report(test_out);
    config.report(test_out, "'prefix '");
    test_out << config << std::endl;
    [[maybe_unused]] std::string report_str = config.report_string();
    std::string str = config.get("string", std::string("foo"));
    EXPECT_EQ(str, "string");
    str = config.get("foo", std::string("string"));
    EXPECT_EQ(str, "string");
    str = config.get<std::string>("string");
    EXPECT_EQ(str, "string");

    EXPECT_TRUE(config.has_sub("sub1"));
    Configuration sub1_config = config.sub("sub1");
    int nt = sub1_config.get("int", int(0));
    EXPECT_EQ(nt, 1);
    nt = sub1_config.get("intt", int(1));
    EXPECT_EQ(nt, 1);
    nt = sub1_config.get<int>("int");
    EXPECT_EQ(nt, 1);
    size_t st = config.get("sub2.size_t", size_t(0));
    EXPECT_EQ(st, 1);
    st = config.get("sub2.size_tt", size_t(1));
    EXPECT_EQ(st, 1);
    st = config.get<size_t>("sub2.size_t");
    EXPECT_EQ(st, 1);

    const auto subsub1 = config.sub("sub2.subsub1");
    check_vector<std::vector<double>>(subsub1);
    check_field_vector<double, 1>(subsub1);
    check_field_vector<double, 2>(subsub1);
    check_vector<Dune::DynamicVector<double>>(subsub1);
    check_matrix<Dune::DynamicMatrix<double>>(subsub1);

    TupleProduct::Combine<StaticCheck::Ints, StaticCheck::Ints, StaticCheck>::Generate<>::Run(subsub1);

  } // ... behaves_correctly(...)
}; // struct ConfigurationTest

TYPED_TEST_SUITE(ConfigTest, TestTypes);
TYPED_TEST(ConfigTest, Get)
{
  this->get();
}
TYPED_TEST(ConfigTest, Set)
{
  this->set();
}
TYPED_TEST(ConfigTest, Other)
{
  this->other();
  this->issue_42();
  this->add_overwrite(true);
  this->add_overwrite(false);
  this->subtree_ctor();
}

TYPED_TEST_SUITE(ConfigurationTest, ConfigurationCreators);
TYPED_TEST(ConfigurationTest, behaves_correctly)
{
  this->behaves_correctly();
}


// The tests below complement the type driven ones above: they cover the ctors which parse external input, the
// reporting code paths, the setters and the free comparison/ordering operators.

namespace {


using ScopedTestDir = Dune::XT::Common::Test::ScopedTestDir;

/**
 * \brief Changes the working directory for the duration of a scope and restores it afterwards.
 *
 * Needed by the tests around the default logfile: that path ("data/log/dxtc_parameter.log") is relative, so the only
 * way to keep those tests off the shared build directory is to move the working directory itself.
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

//! Writes content to a file below dir and returns its path.
std::string write_ini(const ScopedTestDir& dir, const std::string& name, const std::string& content)
{
  const auto path = dir.file(name);
  auto out = Dune::XT::Common::make_ofstream(path);
  *out << content;
  out->close();
  return path;
}


} // namespace


GTEST_TEST(ConfigurationCtors, from_an_istream)
{
  std::stringstream ini;
  ini << "key = value\n"
      << "[sub]\n"
      << "other = 42\n";
  const Configuration config(ini);
  EXPECT_EQ("value", config.get<std::string>("key"));
  EXPECT_EQ(42, config.get<int>("sub.other"));
}


GTEST_TEST(ConfigurationCtors, from_a_filename)
{
  const ScopedTestDir dir("test_configuration_");
  const auto file = write_ini(dir, "from_a_filename.ini", "key = value\n[sub]\nother = 42\n");
  const Configuration config(file);
  EXPECT_EQ("value", config.get<std::string>("key"));
  EXPECT_EQ(42, config.get<int>("sub.other"));
}


GTEST_TEST(ConfigurationCtors, from_the_command_line)
{
  const ScopedTestDir dir("test_configuration_");
  const auto file = write_ini(dir, "from_the_command_line.ini", "key = from_the_file\n");

  // A single argument is read as the name of a parameter file ...
  {
    std::vector<std::string> args{"program", file};
    std::vector<char*> argv{args[0].data(), args[1].data()};
    const Configuration config(2, argv.data());
    EXPECT_EQ("from_the_file", config.get<std::string>("key"));
  }
  // ... several ones as key/value pairs ...
  {
    std::vector<std::string> args{"program", "-key", "from_the_command_line", "-other", "17"};
    std::vector<char*> argv;
    for (auto& arg : args)
      argv.push_back(arg.data());
    const Configuration config(int(argv.size()), argv.data());
    EXPECT_EQ("from_the_command_line", config.get<std::string>("key"));
    EXPECT_EQ(17, config.get<int>("other"));
  }
  // ... and a "paramfile" key pulls in that file on top (without overwriting what was given explicitly).
  {
    std::vector<std::string> args{"program", "-paramfile", file, "-other", "17"};
    std::vector<char*> argv;
    for (auto& arg : args)
      argv.push_back(arg.data());
    const Configuration config(int(argv.size()), argv.data());
    EXPECT_EQ("from_the_file", config.get<std::string>("key"));
    EXPECT_EQ(17, config.get<int>("other"));
  }
  // A lone program name leaves the Configuration empty.
  {
    std::vector<std::string> args{"program"};
    std::vector<char*> argv{args[0].data()};
    const Configuration config(1, argv.data());
    EXPECT_TRUE(config.empty());
  }
}


GTEST_TEST(Configuration, read_command_line)
{
  const ScopedTestDir dir("test_configuration_");
  const auto file = write_ini(dir, "read_command_line.ini", "key = from_the_file\n");
  {
    std::vector<std::string> args{"program", file, "-other", "17"};
    std::vector<char*> argv;
    for (auto& arg : args)
      argv.push_back(arg.data());
    Configuration config;
    config.read_command_line(int(argv.size()), argv.data());
    EXPECT_EQ("from_the_file", config.get<std::string>("key"));
    EXPECT_EQ(17, config.get<int>("other"));
  }
  // Without any argument there is nothing to read, which is reported as an error including a usage hint.
  {
    std::vector<std::string> args{"program"};
    std::vector<char*> argv{args[0].data()};
    Configuration config;
    EXPECT_THROW(config.read_command_line(1, argv.data()), Dune::Exception);
  }
}


GTEST_TEST(Configuration, read_options)
{
  std::vector<std::string> args{"program", "-key", "value", "-sub.other", "42"};
  std::vector<char*> argv;
  for (auto& arg : args)
    argv.push_back(arg.data());
  Configuration config;
  config.read_options(int(argv.size()), argv.data());
  EXPECT_EQ("value", config.get<std::string>("key"));
  EXPECT_EQ(42, config.get<int>("sub.other"));
}


GTEST_TEST(Configuration, sub)
{
  const Configuration config({{"sub.key", "value"}});
  EXPECT_TRUE(config.has_sub("sub"));
  EXPECT_EQ("value", config.sub("sub").get<std::string>("key"));

  // A missing sub is an error by default ...
  EXPECT_THROW(config.sub("missing"), Exceptions::configuration_error);
  // ... unless a default is accepted, in which case it is handed back unchanged.
  const Configuration fallback({{"fallback", "yes"}});
  EXPECT_EQ(fallback, config.sub("missing", false, fallback));
  EXPECT_TRUE(config.sub("missing", false).empty());

  // An empty Configuration has nothing to hand out at all ...
  const Configuration empty;
  EXPECT_THROW(empty.sub("anything"), Exceptions::configuration_error);
  EXPECT_EQ(fallback, empty.sub("anything", false, fallback));
  // ... and an empty sub_id is never a valid request.
  EXPECT_THROW(config.sub(""), Exceptions::configuration_error);
}


GTEST_TEST(Configuration, operator_plus_does_not_modify_its_operands)
{
  const Configuration left({{"left", "1"}});
  const Configuration right({{"right", "2"}});

  const Configuration sum = left + right;
  EXPECT_EQ(1, sum.get<int>("left"));
  EXPECT_EQ(2, sum.get<int>("right"));
  EXPECT_FALSE(left.has_key("right"));
  EXPECT_FALSE(right.has_key("left"));

  // operator+= does modify the left hand side.
  Configuration accumulated(left);
  accumulated += right;
  EXPECT_EQ(1, accumulated.get<int>("left"));
  EXPECT_EQ(2, accumulated.get<int>("right"));
}


GTEST_TEST(Configuration, adding_a_conflicting_tree_reports_both_trees)
{
  Configuration config({{"key", "1"}});
  const Configuration conflicting({{"key", "2"}});

  try {
    config.add(conflicting);
    FAIL() << "add() did not throw!";
  } catch (const Exceptions::configuration_error& ee) {
    const std::string what(ee.what());
    EXPECT_NE(std::string::npos, what.find("There was an error adding other")) << what;
    EXPECT_NE(std::string::npos, what.find("key = 2")) << what;
  }
  // The conflicting value was not taken over.
  EXPECT_EQ(1, config.get<int>("key"));

  // With overwrite it is.
  config.add(conflicting, "", true);
  EXPECT_EQ(2, config.get<int>("key"));
}


GTEST_TEST(Configuration, report_of_an_empty_configuration_is_empty)
{
  const Configuration empty;
  EXPECT_EQ("", empty.report_string());
  std::stringstream out;
  empty.report(out);
  EXPECT_EQ("", out.str());
}


GTEST_TEST(Configuration, report_collapses_a_common_prefix)
{
  // A tree whose root holds nothing but a single chain of subs is reported under that chain as a section header.
  Configuration config;
  config["a.b.c"] = "1";
  config["a.b.d"] = "2";
  const auto report = config.report_string();
  EXPECT_NE(std::string::npos, report.find("[a.b]")) << report;
  EXPECT_NE(std::string::npos, report.find("c = 1")) << report;
  EXPECT_NE(std::string::npos, report.find("d = 2")) << report;
  // The collapsed keys are not repeated with their full path.
  EXPECT_EQ(std::string::npos, report.find("a.b.c = ")) << report;
}


GTEST_TEST(Configuration, report_prefixes_nested_subs_below_the_common_prefix)
{
  Configuration config;
  config["a.b.c.d.e"] = "1";
  config["a.b.f"] = "2";
  const auto report = config.report_string();
  EXPECT_NE(std::string::npos, report.find("[a.b]")) << report;
  EXPECT_NE(std::string::npos, report.find("f = 2")) << report;
  EXPECT_NE(std::string::npos, report.find("c.d.e = 1")) << report;
}


GTEST_TEST(Configuration, report_without_a_common_prefix_falls_back_to_sections)
{
  Configuration config;
  config["a.x"] = "1";
  config["b.y"] = "2";
  const auto report = config.report_string();
  EXPECT_NE(std::string::npos, report.find("[a]")) << report;
  EXPECT_NE(std::string::npos, report.find("[b]")) << report;
  EXPECT_NE(std::string::npos, report.find("x = 1")) << report;
  EXPECT_NE(std::string::npos, report.find("y = 2")) << report;

  // The prefix is prepended to every line.
  const auto prefixed = config.report_string("# ");
  EXPECT_NE(std::string::npos, prefixed.find("# [a]")) << prefixed;
  EXPECT_NE(std::string::npos, prefixed.find("# x = 1")) << prefixed;
}


GTEST_TEST(Configuration, set_warn_on_default_access)
{
  Configuration config;
  // Only observable through the warning it prints on stderr, so this merely checks that it is accepted and that
  // reading a missing key still yields the default.
  config.set_warn_on_default_access(true);
  EXPECT_EQ(42, config.get("missing", 42));
  config.set_warn_on_default_access(false);
  EXPECT_EQ(42, config.get("missing", 42));
}


GTEST_TEST(Configuration, set_logfile_rejects_an_empty_name)
{
  Configuration config;
  EXPECT_THROW(config.set_logfile(""), Exceptions::wrong_input_given);
  const ScopedTestDir dir("test_configuration_");
  EXPECT_NO_THROW(config.set_logfile(dir.file("some.log")));
}


GTEST_TEST(Configuration, log_on_exit_writes_the_configuration)
{
  // set_logfile() does not move the target (see below), so this test has to assert on the *default*, relative
  // location. Working from inside a directory of our own keeps that off the shared build directory, where a stale
  // copy or a concurrently running test binary could otherwise interfere.
  const ScopedTestDir dir("test_configuration_");
  const CurrentPathGuard cwd(dir.path());
  const auto default_logfile = std::string("data/log/dxtc_parameter.log");
  ASSERT_FALSE(boost::filesystem::exists(default_logfile));

  {
    Configuration config;
    config.set_logfile("elsewhere/dxtc_parameter.log");
    config.set_log_on_exit(true);
    // Switching it on again does not create the directory a second time.
    config.set_log_on_exit(true);
    config["key"] = "value";
  }
  // set_logfile() does not actually change where the Configuration logs to (it only validates its argument and
  // makes sure the directory of the current logfile exists), so the report ends up at the default location.
  EXPECT_TRUE(boost::filesystem::is_regular_file(default_logfile));
  EXPECT_FALSE(boost::filesystem::exists("elsewhere/dxtc_parameter.log"));
}


GTEST_TEST(Configuration, an_empty_configuration_is_not_logged_on_exit)
{
  const ScopedTestDir dir("test_configuration_");
  const CurrentPathGuard cwd(dir.path());
  {
    Configuration config;
    config.set_log_on_exit(true);
  }
  EXPECT_FALSE(boost::filesystem::exists("data/log/dxtc_parameter.log"));
}


GTEST_TEST(Configuration, a_failure_while_logging_on_exit_is_swallowed)
{
  // The dtor must not let exceptions escape. Pointing the logfile below an existing regular file makes the
  // directory creation in the dtor fail, which has to be reported on stderr instead of terminating the process.
  const ScopedTestDir dir("test_configuration_");
  const auto blocking_file = write_ini(dir, "not_a_directory", "");
  ASSERT_TRUE(boost::filesystem::is_regular_file(blocking_file));
  EXPECT_NO_THROW({
    Configuration config(Dune::ParameterTree(),
                         ConfigurationDefaults(false, true, blocking_file + "/dxtc_parameter.log"));
    config["key"] = "value";
  });
}


GTEST_TEST(Configuration, the_logfile_follows_datadir_and_logging_dir)
{
  // setup_() derives the logfile from global.datadir and logging.dir whenever both are present. It only runs in the
  // ctors which take a tree (the copy ctor keeps whatever the source was set up with), so the keys have to be in
  // place before the Configuration is built.
  Dune::ParameterTree tree;
  const ScopedTestDir dir("test_configuration_");
  tree["global.datadir"] = dir.path();
  tree["logging.dir"] = "logs";
  {
    Configuration config(tree);
    config.set_log_on_exit(true);
  }
  EXPECT_TRUE(boost::filesystem::is_regular_file(dir.file("logs/dxtc_parameter.log")));
}


GTEST_TEST(ParameterTreeOperators, equality)
{
  Dune::ParameterTree first;
  first["key"] = "value";
  Dune::ParameterTree second;
  second["key"] = "value";
  Dune::ParameterTree third;
  third["key"] = "other";

  EXPECT_TRUE(first == second);
  EXPECT_FALSE(first != second);
  EXPECT_TRUE(first != third);
  EXPECT_FALSE(first == third);

  // The same for Configuration, which compares its flattened contents.
  EXPECT_TRUE(Configuration(first) == Configuration(second));
  EXPECT_TRUE(Configuration(first) != Configuration(third));
}


GTEST_TEST(ParameterTreeOperators, ordering)
{
  Dune::ParameterTree lower;
  lower["key"] = "a";
  Dune::ParameterTree upper;
  upper["key"] = "b";

  const std::less<Dune::ParameterTree> tree_less{};
  EXPECT_TRUE(tree_less(lower, upper));
  EXPECT_FALSE(tree_less(upper, lower));
  EXPECT_FALSE(tree_less(lower, lower));

  const std::less<Configuration> config_less{};
  EXPECT_TRUE(config_less(Configuration(lower), Configuration(upper)));
  EXPECT_FALSE(config_less(Configuration(upper), Configuration(lower)));

  // Which is what makes them usable as keys in associative containers.
  std::map<Configuration, int> map;
  map[Configuration(lower)] = 1;
  map[Configuration(upper)] = 2;
  EXPECT_EQ(2u, map.size());
  EXPECT_EQ(1, map.at(Configuration(lower)));
}
