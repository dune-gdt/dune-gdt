// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   René Fritze (2026)

// This is a native-only test: VectorView / ConstVectorView are not exposed by any Python binding,
// so they cannot be exercised through the bindings. See issue #346.

#include <dune/xt/test/main.hxx> // <- This one has to come first, includes config.h!

#include <dune/xt/common/exceptions.hh>
#include <dune/xt/la/container/common.hh>
#include <dune/xt/la/container/vector-view.hh>

using namespace Dune;
using namespace Dune::XT;

using VectorType = LA::CommonDenseVector<double>;
using ConstView = LA::ConstVectorView<VectorType>;
using MutableView = LA::VectorView<VectorType>;

namespace {

// [0, 1, 2, 3, 4, 5]
VectorType counting_vector()
{
  VectorType vector(6, 0.);
  for (size_t ii = 0; ii < vector.size(); ++ii)
    vector.set_entry(ii, double(ii));
  return vector;
}

} // namespace


GTEST_TEST(ConstVectorView, read_access)
{
  auto vector = counting_vector();
  // view onto entries [1, 2, 3]
  const ConstView view(vector, 1, 4);

  EXPECT_EQ(size_t(3), view.size());
  EXPECT_EQ(size_t(1), view.first_entry());
  EXPECT_EQ(size_t(4), view.past_last_entry());
  EXPECT_EQ(size_t(1), view.index(0));
  EXPECT_EQ(size_t(3), view.index(2));

  for (size_t ii = 0; ii < view.size(); ++ii) {
    EXPECT_DOUBLE_EQ(double(ii + 1), view.get_entry(ii));
    EXPECT_DOUBLE_EQ(double(ii + 1), view[ii]);
  }

  // vec() must reference the original vector
  EXPECT_TRUE(&view.vec() == &vector);
  EXPECT_DOUBLE_EQ(2., view.vec().get_entry(2));

  // dot with a plain vector: 1*1 + 2*1 + 3*1
  VectorType ones(3, 1.);
  EXPECT_DOUBLE_EQ(6., view.dot(ones));

  // norms exercise get_unchecked_ref
  EXPECT_DOUBLE_EQ(6., view.l1_norm());
  EXPECT_DOUBLE_EQ(3., view.sup_norm());
}

GTEST_TEST(ConstVectorView, mutating_methods_throw)
{
  auto vector = counting_vector();
  ConstView view(vector, 1, 4);
  ConstView other(vector, 0, 3);

  EXPECT_THROW(view.resize(2), Dune::Exception);
  EXPECT_THROW(view.set_entry(0, 1.), Dune::Exception);
  EXPECT_THROW(view.add_to_entry(0, 1.), Dune::Exception);
  EXPECT_THROW(view.set_entry(0, 0, 1.), Dune::Exception);
  EXPECT_THROW(view.add_to_entry(0, 0, 1.), Dune::Exception);
  EXPECT_THROW(view.scal(2.), Dune::Exception);
  EXPECT_THROW(view.axpy(2., other), Dune::Exception);
  EXPECT_THROW(view[0] = 1., Dune::Exception);

  // methods returning a new view are not implemented
  EXPECT_THROW(view.add(other), Dune::Exception);
  EXPECT_THROW(view.sub(other), Dune::Exception);
  EXPECT_THROW(view + other, Dune::Exception);
  EXPECT_THROW(view - other, Dune::Exception);

  // copying is disabled
  EXPECT_THROW(view.copy(), Dune::Exception);
}

GTEST_TEST(ConstVectorView, invalid_construction_throws)
{
  // the size/value constructor only exists so the interface compiles
  EXPECT_THROW(ConstView(size_t(3)), Dune::Exception);
}


GTEST_TEST(VectorView, read_write_access)
{
  auto vector = counting_vector();
  // view onto entries [1, 2, 3]
  MutableView view(vector, 1, 4);

  EXPECT_EQ(size_t(3), view.size());
  EXPECT_EQ(size_t(2), view.index(1));

  for (size_t ii = 0; ii < view.size(); ++ii) {
    EXPECT_DOUBLE_EQ(double(ii + 1), view.get_entry(ii));
    EXPECT_DOUBLE_EQ(double(ii + 1), view[ii]);
  }

  // set_entry writes through to the underlying vector
  view.set_entry(0, 10.);
  EXPECT_DOUBLE_EQ(10., view.get_entry(0));
  EXPECT_DOUBLE_EQ(10., vector.get_entry(1));

  // add_to_entry writes through as well
  view.add_to_entry(0, 5.);
  EXPECT_DOUBLE_EQ(15., view.get_entry(0));
  EXPECT_DOUBLE_EQ(15., vector.get_entry(1));

  // operator[] gives writable access into the original vector
  view[2] = 30.;
  EXPECT_DOUBLE_EQ(30., view[2]);
  EXPECT_DOUBLE_EQ(30., vector.get_entry(3));

  // entries outside the view are untouched
  EXPECT_DOUBLE_EQ(0., vector.get_entry(0));
  EXPECT_DOUBLE_EQ(4., vector.get_entry(4));
  EXPECT_DOUBLE_EQ(5., vector.get_entry(5));
}

GTEST_TEST(VectorView, assignment)
{
  auto vector = counting_vector();
  MutableView view(vector, 1, 4); // [1, 2, 3]

  // assign from a scalar
  view = 7.;
  for (size_t ii = 0; ii < view.size(); ++ii)
    EXPECT_DOUBLE_EQ(7., view.get_entry(ii));
  EXPECT_DOUBLE_EQ(7., vector.get_entry(1));
  EXPECT_DOUBLE_EQ(0., vector.get_entry(0));

  // assign from another vector type
  VectorType source(3, 0.);
  for (size_t ii = 0; ii < source.size(); ++ii)
    source.set_entry(ii, double(ii) + 100.);
  view = source;
  for (size_t ii = 0; ii < view.size(); ++ii)
    EXPECT_DOUBLE_EQ(double(ii) + 100., view.get_entry(ii));

  // assign from another view (copies contents, not the reference)
  VectorType other_vector(3, 0.);
  for (size_t ii = 0; ii < other_vector.size(); ++ii)
    other_vector.set_entry(ii, double(ii) + 42.); // [42, 43, 44]
  MutableView other_view(other_vector, 0, 3);
  view = other_view;
  for (size_t ii = 0; ii < view.size(); ++ii)
    EXPECT_DOUBLE_EQ(double(ii) + 42., view.get_entry(ii));
  // the target vector was written through
  EXPECT_DOUBLE_EQ(43., vector.get_entry(2));
  // assignment copies values, it does not alias the source view: mutating one side leaves the other intact
  other_view.set_entry(0, -1.);
  EXPECT_DOUBLE_EQ(42., view.get_entry(0));
  view.set_entry(1, -2.);
  EXPECT_DOUBLE_EQ(43., other_vector.get_entry(1));
}

GTEST_TEST(VectorView, arithmetic)
{
  auto vector = counting_vector();
  MutableView view(vector, 1, 4); // [1, 2, 3]

  // scal
  view.scal(2.);
  EXPECT_DOUBLE_EQ(2., vector.get_entry(1));
  EXPECT_DOUBLE_EQ(4., vector.get_entry(2));
  EXPECT_DOUBLE_EQ(6., vector.get_entry(3));

  // operator*=
  view *= 0.5; // back to [1, 2, 3]
  EXPECT_DOUBLE_EQ(1., view.get_entry(0));
  EXPECT_DOUBLE_EQ(3., view.get_entry(2));

  // axpy with a plain vector: view += 2 * ones
  VectorType ones(3, 1.);
  view.axpy(2., ones); // [3, 4, 5]
  EXPECT_DOUBLE_EQ(3., view.get_entry(0));
  EXPECT_DOUBLE_EQ(5., view.get_entry(2));

  // axpy with another view: view += (-1) * other
  VectorType other_vector(3, 0.);
  for (size_t ii = 0; ii < other_vector.size(); ++ii)
    other_vector.set_entry(ii, double(ii) + 1.); // [1, 2, 3]
  MutableView other_view(other_vector, 0, 3);
  view.axpy(-1., other_view); // [2, 2, 2]
  for (size_t ii = 0; ii < view.size(); ++ii)
    EXPECT_DOUBLE_EQ(2., view.get_entry(ii));

  // operator+= / operator-=
  view += other_view; // [3, 4, 5]
  EXPECT_DOUBLE_EQ(3., view.get_entry(0));
  EXPECT_DOUBLE_EQ(4., view.get_entry(1));
  EXPECT_DOUBLE_EQ(5., view.get_entry(2));
  view -= other_view; // [2, 2, 2]
  for (size_t ii = 0; ii < view.size(); ++ii)
    EXPECT_DOUBLE_EQ(2., view.get_entry(ii));

  // dot and norms
  EXPECT_DOUBLE_EQ(12., view.dot(other_view)); // 2*1 + 2*2 + 2*3
  EXPECT_DOUBLE_EQ(6., view.l1_norm());
  EXPECT_DOUBLE_EQ(2., view.sup_norm());
}

GTEST_TEST(VectorView, size_mismatch_throws)
{
  auto vector = counting_vector();
  MutableView view(vector, 1, 4); // size 3

  VectorType wrong_size(2, 1.);
  EXPECT_THROW(view.iadd(wrong_size), Dune::Exception);
  EXPECT_THROW(view.isub(wrong_size), Dune::Exception);
}

GTEST_TEST(VectorView, invalid_construction_throws)
{
  // the size/value constructor only exists so the interface compiles
  EXPECT_THROW(MutableView(size_t(3)), Dune::Exception);
}
