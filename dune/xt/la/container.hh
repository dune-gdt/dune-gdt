// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2013 - 2014, 2016 - 2017, 2019)
//   René Fritze     (2015 - 2016, 2018 - 2020)
//   Tobias Leibner  (2017 - 2018, 2020)

/// \file
/// \brief Maps linear algebra backends to their vector/matrix container types and lists available types.

#ifndef DUNE_XT_LA_CONTAINER_HH
#define DUNE_XT_LA_CONTAINER_HH

#include <boost/tuple/tuple.hpp>

#include "container/interfaces.hh"
#include "container/common.hh"
#include "container/eigen.hh"
#include "container/istl.hh"

namespace Dune::XT::LA {


template <class ScalarType, Backends backend = default_backend>
struct Container;

/// \brief Vector and matrix container types for the common dense backend.
template <class ScalarType>
struct Container<ScalarType, Backends::common_dense>
{
  using VectorType = CommonDenseVector<ScalarType>;
  using MatrixType = CommonDenseMatrix<ScalarType>;
}; // struct Container<..., common_dense>

/// \brief Vector and matrix container types for the common sparse backend.
template <class ScalarType>
struct Container<ScalarType, Backends::common_sparse>
{
  using VectorType = CommonDenseVector<ScalarType>;
  using MatrixType = CommonSparseMatrix<ScalarType>;
}; // struct Container<..., common_sparse>

/// \brief Vector and matrix container types for the Eigen dense backend.
template <class ScalarType>
struct Container<ScalarType, Backends::eigen_dense>
{
  using VectorType = EigenDenseVector<ScalarType>;
  using MatrixType = EigenDenseMatrix<ScalarType>;
}; // struct Container<..., eigen_dense>

/// \brief Vector and matrix container types for the Eigen sparse backend.
template <class ScalarType>
struct Container<ScalarType, Backends::eigen_sparse>
{
  using VectorType = EigenDenseVector<ScalarType>;
  using MatrixType = EigenRowMajorSparseMatrix<ScalarType>;
}; // struct Container<..., eigen_sparse>

/// \brief Vector and matrix container types for the ISTL dense backend.
template <class ScalarType>
struct Container<ScalarType, Backends::istl_dense>
{
  using VectorType = IstlDenseVector<ScalarType>;
  using MatrixType = IstlRowMajorSparseMatrix<ScalarType>;
}; // struct Container<..., istl_dense>

/// \brief Vector and matrix container types for the ISTL sparse backend.
template <class ScalarType>
struct Container<ScalarType, Backends::istl_sparse>
{
  using VectorType = IstlDenseVector<ScalarType>;
  using MatrixType = IstlRowMajorSparseMatrix<ScalarType>;
}; // struct Container<..., istl_sparse>


/// \brief Tuple of all available vector container types (depending on enabled backends).
template <class S>
using AvailableVectorTypes = std::tuple<CommonDenseVector<S>,
                                        IstlDenseVector<S>
#if HAVE_EIGEN
                                        ,
                                        EigenDenseVector<S>
#endif
                                        >;


/// \brief Tuple of all available dense matrix container types (depending on enabled backends).
template <class S>
using AvailableDenseMatrixTypes = std::tuple<CommonDenseMatrix<S>
#if HAVE_EIGEN
                                             ,
                                             EigenDenseMatrix<S>
#endif
                                             >;


/// \brief Tuple of all available sparse matrix container types (depending on enabled backends).
template <class S>
using AvailableSparseMatrixTypes = std::tuple<CommonSparseMatrix<S>,
                                              IstlRowMajorSparseMatrix<S>
#if HAVE_EIGEN
                                              ,
                                              EigenRowMajorSparseMatrix<S>
#endif
                                              >;


} // namespace Dune::XT::LA

#endif // DUNE_XT_LA_CONTAINER_HH
