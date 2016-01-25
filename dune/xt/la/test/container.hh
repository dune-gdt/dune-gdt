// This file is part of the dune-xt-la project:
//   https://github.com/dune-community/dune-xt-la
// The copyright lies with the authors of this file (see below).
// License: BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
// Authors:
//   Felix Schindler (2014, 2016)
//   Rene Milk       (2014 - 2015)
//   Tobias Leibner  (2014 - 2015)

#ifndef DUNE_XT_TEST_LA_CONTAINER_HH
#define DUNE_XT_TEST_LA_CONTAINER_HH

#include <complex>
#include <memory>
#include <type_traits>

#include <dune/xt/common/exceptions.hh>
#include <dune/xt/common/float_cmp.hh>
#include <dune/xt/la/container/interfaces.hh>
#include <dune/xt/la/container/common.hh>
#include <dune/xt/la/container/eigen.hh>
#include <dune/xt/la/container/istl.hh>
#include <dune/xt/la/container.hh>

template <class ContainerImp>
class ContainerFactory
{
public:
  static ContainerImp create(const size_t /*size*/)
  {
    static_assert(Dune::AlwaysFalse<ContainerImp>::value, "Please specialize this class for this ContainerImp!");
  }
};

template <class S>
class ContainerFactory<Dune::XT::LA::CommonDenseVector<S>>
{
public:
  static Dune::XT::LA::CommonDenseVector<S> create(const size_t size)
  {
    return Dune::XT::LA::CommonDenseVector<S>(size, S(1));
  }
};

template <class S>
class ContainerFactory<Dune::XT::LA::CommonDenseMatrix<S>>
{
public:
  static Dune::XT::LA::CommonDenseMatrix<S> create(const size_t size)
  {
    Dune::XT::LA::CommonDenseMatrix<S> matrix(size, size);
    for (size_t ii = 0; ii < size; ++ii)
      matrix.unit_row(ii);
    return matrix;
  }
};

#if HAVE_DUNE_ISTL
template <class S>
class ContainerFactory<Dune::XT::LA::IstlDenseVector<S>>
{
public:
  static Dune::XT::LA::IstlDenseVector<S> create(const size_t size)
  {
    return Dune::XT::LA::IstlDenseVector<S>(size, S(1));
  }
};

template <class S>
class ContainerFactory<Dune::XT::LA::IstlRowMajorSparseMatrix<S>>
{
public:
  static Dune::XT::LA::IstlRowMajorSparseMatrix<S> create(const size_t size)
  {
    Dune::XT::LA::SparsityPatternDefault pattern(size);
    for (size_t ii = 0; ii < size; ++ii)
      pattern.inner(ii).push_back(ii);
    Dune::XT::LA::IstlRowMajorSparseMatrix<S> matrix(size, size, pattern);
    for (size_t ii = 0; ii < size; ++ii)
      matrix.unit_row(ii);
    return matrix;
  }
};
#endif // HAVE_DUNE_ISTL

#if HAVE_EIGEN
template <class S>
class ContainerFactory<Dune::XT::LA::EigenDenseVector<S>>
{
public:
  static Dune::XT::LA::EigenDenseVector<S> create(const size_t size)
  {
    return Dune::XT::LA::EigenDenseVector<S>(size, S(1));
  }
};

template <class S>
class ContainerFactory<Dune::XT::LA::EigenMappedDenseVector<S>>
{
public:
  static Dune::XT::LA::EigenMappedDenseVector<S> create(const size_t size)
  {
    return Dune::XT::LA::EigenMappedDenseVector<S>(size, S(1));
  }
};

template <class S>
class ContainerFactory<Dune::XT::LA::EigenDenseMatrix<S>>
{
public:
  static Dune::XT::LA::EigenDenseMatrix<S> create(const size_t size)
  {
    Dune::XT::LA::EigenDenseMatrix<S> matrix(size, size);
    for (size_t ii = 0; ii < size; ++ii)
      matrix.unit_row(ii);
    return matrix;
  }
};

template <class S>
class ContainerFactory<Dune::XT::LA::EigenRowMajorSparseMatrix<S>>
{
public:
  static Dune::XT::LA::EigenRowMajorSparseMatrix<S> create(const size_t size)
  {
    Dune::XT::LA::SparsityPatternDefault pattern(size);
    for (size_t ii = 0; ii < size; ++ii)
      pattern.inner(ii).push_back(ii);
    Dune::XT::LA::EigenRowMajorSparseMatrix<S> matrix(size, size, pattern);
    for (size_t ii = 0; ii < size; ++ii)
      matrix.unit_row(ii);
    return matrix;
  }
};
#endif // HAVE_EIGEN

#define EXPECT_DOUBLE_OR_COMPLEX_EQ(expected, actual)                                                                  \
  {                                                                                                                    \
    EXPECT_DOUBLE_EQ(expected, std::real(actual));                                                                     \
    EXPECT_DOUBLE_EQ(0, std::imag(actual));                                                                            \
  }

#endif // DUNE_XT_TEST_LA_CONTAINER_HH
