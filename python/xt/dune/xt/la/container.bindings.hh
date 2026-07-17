// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2017 - 2020)
//   René Fritze     (2018 - 2019)
//   Tobias Leibner  (2020)

#ifndef DUNE_XT_LA_CONTAINER_BINDINGS_HH
#define DUNE_XT_LA_CONTAINER_BINDINGS_HH

#include <complex>

#include <dune/xt/la/container.hh>


// this is used in other headers
using COMMON_DENSE_VECTOR = Dune::XT::LA::CommonDenseVector<double>;
using COMMON_DENSE_MATRIX = Dune::XT::LA::CommonDenseMatrix<double>;
using COMMON_SPARSE_MATRIX_CSR = Dune::XT::LA::CommonSparseMatrixCsr<double>;
using COMMON_SPARSE_MATRIX_CSC = Dune::XT::LA::CommonSparseMatrixCsc<double>;
// kept for backwards compatibility, identical to COMMON_SPARSE_MATRIX_CSR (csr is the default storage layout)
using COMMON_SPARSE_MATRIX = Dune::XT::LA::CommonSparseMatrix<double>;
#if HAVE_EIGEN
using EIGEN_DENSE_VECTOR = Dune::XT::LA::EigenDenseVector<double>;
using EIGEN_DENSE_MATRIX = Dune::XT::LA::EigenDenseMatrix<double>;
using EIGEN_SPARSE_MATRIX = Dune::XT::LA::EigenRowMajorSparseMatrix<double>;
#endif
using ISTL_DENSE_VECTOR = Dune::XT::LA::IstlDenseVector<double>;
using ISTL_SPARSE_MATRIX = Dune::XT::LA::IstlRowMajorSparseMatrix<double>;

// std::complex<double>-valued counterparts (WP5, #320): one dense/sparse pair per backend family,
// reusing the same container class templates (they are generic over the scalar type already).
using COMMON_DENSE_VECTOR_COMPLEX = Dune::XT::LA::CommonDenseVector<std::complex<double>>;
using COMMON_DENSE_MATRIX_COMPLEX = Dune::XT::LA::CommonDenseMatrix<std::complex<double>>;
using COMMON_SPARSE_MATRIX_CSR_COMPLEX = Dune::XT::LA::CommonSparseMatrixCsr<std::complex<double>>;
using COMMON_SPARSE_MATRIX_CSC_COMPLEX = Dune::XT::LA::CommonSparseMatrixCsc<std::complex<double>>;
#if HAVE_EIGEN
using EIGEN_DENSE_VECTOR_COMPLEX = Dune::XT::LA::EigenDenseVector<std::complex<double>>;
using EIGEN_DENSE_MATRIX_COMPLEX = Dune::XT::LA::EigenDenseMatrix<std::complex<double>>;
using EIGEN_SPARSE_MATRIX_COMPLEX = Dune::XT::LA::EigenRowMajorSparseMatrix<std::complex<double>>;
#endif
using ISTL_DENSE_VECTOR_COMPLEX = Dune::XT::LA::IstlDenseVector<std::complex<double>>;
using ISTL_SPARSE_MATRIX_COMPLEX = Dune::XT::LA::IstlRowMajorSparseMatrix<std::complex<double>>;

namespace Dune::XT::LA::bindings {


template <Backends bb>
struct backend_name
{
  static_assert(AlwaysFalse<typename Container<double, bb>::VectorType1>::value,
                "Please add a specialization for this backend!");

  static std::string value()
  {
    return "";
  }
};

template <>
struct backend_name<Backends::common_dense>
{
  static std::string value()
  {
    return "common_dense";
  }
};

template <>
struct backend_name<Backends::common_sparse>
{
  static std::string value()
  {
    return "common_sparse";
  }
};

template <>
struct backend_name<Backends::istl_dense>
{
  static std::string value()
  {
    return "istl_dense";
  }
};

template <>
struct backend_name<Backends::istl_sparse>
{
  static std::string value()
  {
    return "istl_sparse";
  }
};

template <>
struct backend_name<Backends::eigen_dense>
{
  static std::string value()
  {
    return "eigen_dense";
  }
};

template <>
struct backend_name<Backends::eigen_sparse>
{
  static std::string value()
  {
    return "eigen_sparse";
  }
};


template <class T>
struct container_name
{
  static_assert(AlwaysFalse<T>::value, "Please add a specialization for this container!");

  static std::string value()
  {
    return "";
  }
};

template <>
struct container_name<CommonDenseVector<double>>
{
  static std::string value()
  {
    return "common_vector";
  }
};

template <>
struct container_name<CommonDenseVector<size_t>>
{
  static std::string value()
  {
    return "common_vector_size_t";
  }
};

template <>
struct container_name<CommonDenseMatrix<double>>
{
  static std::string value()
  {
    return "common_dense_matrix";
  }
};

// Note: CommonSparseMatrix<double> defaults its second (layout) template argument to
// Dune::XT::Common::StorageLayout::csr, i.e. this is the same type as CommonSparseMatrixCsr<double>.
template <>
struct container_name<CommonSparseMatrix<double, Dune::XT::Common::StorageLayout::csr>>
{
  static std::string value()
  {
    // kept "Matrix"-suffixed (rather than e.g. "...MatrixCsr") so that name-suffix-based discovery
    // (dune.xt.test.hypothesis_strategies.discover_matrix_types) still finds this class.
    return "common_sparse_csr_matrix";
  }
};

template <>
struct container_name<CommonSparseMatrix<double, Dune::XT::Common::StorageLayout::csc>>
{
  static std::string value()
  {
    return "common_sparse_csc_matrix";
  }
};

// std::complex<double>-valued containers (WP5, #320): "Complex" is inserted right after the
// backend name, matching the real-valued naming above (e.g. CommonVector -> CommonComplexVector)
// so that dune.xt.test.hypothesis_strategies can tell backend and field type apart by name alone.

template <>
struct container_name<CommonDenseVector<std::complex<double>>>
{
  static std::string value()
  {
    return "common_complex_vector";
  }
};

template <>
struct container_name<CommonDenseMatrix<std::complex<double>>>
{
  static std::string value()
  {
    return "common_complex_dense_matrix";
  }
};

template <>
struct container_name<CommonSparseMatrix<std::complex<double>, Dune::XT::Common::StorageLayout::csr>>
{
  static std::string value()
  {
    return "common_complex_sparse_csr_matrix";
  }
};

template <>
struct container_name<CommonSparseMatrix<std::complex<double>, Dune::XT::Common::StorageLayout::csc>>
{
  static std::string value()
  {
    return "common_complex_sparse_csc_matrix";
  }
};

#if HAVE_EIGEN

template <>
struct container_name<EigenDenseVector<double>>
{
  static std::string value()
  {
    return "eigen_vector";
  }
};

template <>
struct container_name<EigenDenseMatrix<double>>
{
  static std::string value()
  {
    return "eigen_dense_matrix";
  }
};

template <>
struct container_name<EigenRowMajorSparseMatrix<double>>
{
  static std::string value()
  {
    return "eigen_sparse_matrix";
  }
};

template <>
struct container_name<EigenDenseVector<std::complex<double>>>
{
  static std::string value()
  {
    return "eigen_complex_vector";
  }
};

template <>
struct container_name<EigenDenseMatrix<std::complex<double>>>
{
  static std::string value()
  {
    return "eigen_complex_dense_matrix";
  }
};

template <>
struct container_name<EigenRowMajorSparseMatrix<std::complex<double>>>
{
  static std::string value()
  {
    return "eigen_complex_sparse_matrix";
  }
};

#endif // HAVE_EIGEN

template <>
struct container_name<IstlDenseVector<double>>
{
  static std::string value()
  {
    return "istl_vector";
  }
};

template <>
struct container_name<IstlRowMajorSparseMatrix<double>>
{
  static std::string value()
  {
    return "istl_sparse_matrix";
  }
};

template <>
struct container_name<IstlDenseVector<std::complex<double>>>
{
  static std::string value()
  {
    return "istl_complex_vector";
  }
};

template <>
struct container_name<IstlRowMajorSparseMatrix<std::complex<double>>>
{
  static std::string value()
  {
    return "istl_complex_sparse_matrix";
  }
};


} // namespace Dune::XT::LA::bindings


#endif // DUNE_XT_LA_CONTAINER_BINDINGS_HH
