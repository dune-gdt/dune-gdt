// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Barbara Verfürth (2015)
//   Felix Schindler  (2014 - 2018, 2020)
//   René Fritze      (2014 - 2016, 2018 - 2020)
//   Tobias Leibner   (2014 - 2015, 2017 - 2020)

#ifndef DUNE_XT_LA_CONTAINER_EIGEN_SPARSE_HH
#define DUNE_XT_LA_CONTAINER_EIGEN_SPARSE_HH

#include <memory>
#include <type_traits>
#include <utility>
#include <vector>
#include <complex>
#include <mutex>

#include <boost/numeric/conversion/cast.hpp>

#if HAVE_EIGEN
#  include <dune/xt/common/disable_warnings.hh>
#  include <Eigen/SparseCore>
#  include <dune/xt/common/reenable_warnings.hh>
#endif

#include <dune/common/typetraits.hh>
#include <dune/common/ftraits.hh>

#include <dune/xt/common/crtp.hh>
#include <dune/xt/common/exceptions.hh>
#include <dune/xt/common/float_cmp.hh>
#include <dune/xt/common/math.hh>
#include <dune/xt/common/numeric_cast.hh>

#include "dune/xt/la/container/interfaces.hh"
#include "dune/xt/la/container/pattern.hh"

#include "dense.hh"

namespace Dune::XT {
namespace LA {


// forwards
template <class ScalarType>
class EigenRowMajorSparseMatrix;

class EigenMatrixInterfaceDynamic
{};


#if HAVE_EIGEN

namespace internal {


/**
 * \brief Traits for EigenRowMajorSparseMatrix.
 */
template <class ScalarImp = double>
class EigenRowMajorSparseMatrixTraits
  : public MatrixTraitsBase<ScalarImp,
                            EigenRowMajorSparseMatrix<ScalarImp>,
                            ::Eigen::SparseMatrix<ScalarImp, ::Eigen::RowMajor>,
                            Backends::eigen_sparse,
                            Backends::eigen_dense,
                            true>
{};


} // namespace internal


/**
 * \brief A sparse matrix implementation of the MatrixInterface with row major memory layout.
 */
template <class ScalarImp = double>
class EigenRowMajorSparseMatrix
  : public MatrixInterface<internal::EigenRowMajorSparseMatrixTraits<ScalarImp>, ScalarImp>
  , public ProvidesBackend<internal::EigenRowMajorSparseMatrixTraits<ScalarImp>>
{

  using ThisType = EigenRowMajorSparseMatrix;
  using InterfaceType = MatrixInterface<internal::EigenRowMajorSparseMatrixTraits<ScalarImp>, ScalarImp>;

public:
  using typename InterfaceType::RealType;
  using typename InterfaceType::ScalarType;
  using Traits = typename InterfaceType::Traits;
  using typename ProvidesBackend<Traits>::BackendType;

private:
  using MutexesType = typename Traits::MutexesType;
  using EIGEN_size_t = typename BackendType::Index;

public:
  /**
   * \brief This is the constructor of interest which creates a sparse matrix.
   */
  EigenRowMajorSparseMatrix(const size_t rr,
                            const size_t cc,
                            const SparsityPatternDefault& pattern_in,
                            const size_t num_mutexes = 1)
    : backend_(
        std::make_shared<BackendType>(Common::numeric_cast<EIGEN_size_t>(rr), Common::numeric_cast<EIGEN_size_t>(cc)))
    , mutexes_(std::make_unique<MutexesType>(num_mutexes))
  {
    if (rr > 0 && cc > 0) {
      if (size_t(pattern_in.size()) != rr)
        DUNE_THROW(Common::Exceptions::shapes_do_not_match,
                   "The size of the pattern (" << pattern_in.size() << ") does not match the number of rows of this ("
                                               << rr << ")!");
      for (size_t row = 0; row < size_t(pattern_in.size()); ++row) {
        backend_->startVec(static_cast<EIGEN_size_t>(row));
        const auto& columns = pattern_in.inner(row);
        for (const auto& column : columns) {
#  ifndef NDEBUG
          if (column >= cc)
            DUNE_THROW(Common::Exceptions::shapes_do_not_match,
                       "The size of row " << row << " of the pattern does not match the number of columns of this ("
                                          << cc << ")!");
#  endif // NDEBUG
          backend_->insertBackByOuterInner(static_cast<EIGEN_size_t>(row), static_cast<EIGEN_size_t>(column));
        }
        // create entry (insertBackByOuterInner() can not handle empty rows)
        if (columns.empty())
          backend_->insertBackByOuterInner(static_cast<EIGEN_size_t>(row), 0);
      }
      backend_->finalize();
      backend_->makeCompressed();
    }
  } // EigenRowMajorSparseMatrix(...)

  explicit EigenRowMajorSparseMatrix(const size_t rr = 0, const size_t cc = 0, const size_t num_mutexes = 1)
    : backend_(
        std::make_shared<BackendType>(Common::numeric_cast<EIGEN_size_t>(rr), Common::numeric_cast<EIGEN_size_t>(cc)))
    , mutexes_(std::make_unique<MutexesType>(num_mutexes))
  {
  }

  explicit EigenRowMajorSparseMatrix(const size_t rr,
                                     const size_t cc,
                                     const ScalarType& val,
                                     const size_t num_mutexes = 1)
    : backend_(
        std::make_shared<BackendType>(Common::numeric_cast<EIGEN_size_t>(rr), Common::numeric_cast<EIGEN_size_t>(cc)))
    , mutexes_(std::make_unique<MutexesType>(num_mutexes))
  {
    if (val != 0.) {
      for (size_t ii = 0; ii < rows(); ++ii)
        for (size_t jj = 0; jj < cols(); ++jj) {
          backend_->insert(ii, jj);
          set_entry(ii, jj, val);
        }
    }
  }

  EigenRowMajorSparseMatrix(const ThisType& other)
    : backend_(std::make_shared<BackendType>(*other.backend_))
    , mutexes_(std::make_unique<MutexesType>(other.mutexes_->size()))
  {
  }

  explicit EigenRowMajorSparseMatrix(const BackendType& mat,
                                     const bool prune = false,
                                     const typename Common::FloatCmp::DefaultEpsilon<ScalarType>::Type eps =
                                         Common::FloatCmp::DefaultEpsilon<ScalarType>::value(),
                                     const size_t num_mutexes = 1)
    : mutexes_(std::make_unique<MutexesType>(num_mutexes))
  {
    if (prune) {
      // we do this here instead of using pattern(true), since we can build the triplets along the way which is more
      // efficient
      using TripletType = ::Eigen::Triplet<ScalarType>;
      const ScalarType zero(0);
      std::vector<TripletType> triplets;
      triplets.reserve(mat.nonZeros());
      for (EIGEN_size_t row = 0; row < mat.outerSize(); ++row) {
        for (typename BackendType::InnerIterator row_it(mat, row); row_it; ++row_it) {
          const EIGEN_size_t col = row_it.col();
          const auto val = mat.coeff(row, col);
          if (Common::FloatCmp::ne<Common::FloatCmp::Style::absolute>(val, zero, eps))
            triplets.emplace_back(row, col, val);
        }
      }
      backend_ = std::make_shared<BackendType>(mat.rows(), mat.cols());
      backend_->setFromTriplets(triplets.begin(), triplets.end());
    } else
      backend_ = std::make_shared<BackendType>(mat);
  } // EigenRowMajorSparseMatrix(...)

  /**
   *  \note Takes ownership of backend_ptr in the sense that you must not delete it afterwards!
   */
  explicit EigenRowMajorSparseMatrix(BackendType* backend_ptr, const size_t num_mutexes = 1)
    : backend_(backend_ptr)
    , mutexes_(std::make_unique<MutexesType>(num_mutexes))
  {
  }

  explicit EigenRowMajorSparseMatrix(std::shared_ptr<BackendType> backend_ptr, const size_t num_mutexes = 1)
    : backend_(std::move(backend_ptr))
    , mutexes_(std::make_unique<MutexesType>(num_mutexes))
  {
  }

  ThisType& operator=(const ThisType& other)
  {
    if (this != &other) {
      *backend_ = *other.backend_;
      mutexes_ = std::make_unique<MutexesType>(other.mutexes_->size());
    }
    return *this;
  }

  /**
   *  \note Does a deep copy.
   */
  ThisType& operator=(const BackendType& other)
  {
    backend_ = std::make_shared<BackendType>(other);
    return *this;
  }

  /// \name Required by the ProvidesBackend interface.
  /// \{

  BackendType& backend()
  {
    return *backend_;
  }

  const BackendType& backend() const
  {
    return *backend_;
  }

  /// \}
  /// \name Required by ContainerInterface.
  /// \{

  ThisType copy() const
  {
    return ThisType(*backend_);
  }

  void scal(const ScalarType& alpha)
  {
    [[maybe_unused]] const internal::VectorLockGuard guard(*mutexes_);
    backend() *= alpha;
  }

  void set_to_zero()
  {
    backend() *= 0.;
  }

  void axpy(const ScalarType& alpha, const ThisType& xx)
  {
    if (!has_equal_shape(xx))
      DUNE_THROW(Common::Exceptions::shapes_do_not_match,
                 "The shape of xx (" << xx.rows() << "x" << xx.cols() << ") does not match the shape of this ("
                                     << rows() << "x" << cols() << ")!");
    [[maybe_unused]] const internal::VectorLockGuard guard(*mutexes_);
    backend() += alpha * xx.backend();
  } // ... axpy(...)

  bool has_equal_shape(const ThisType& other) const
  {
    return (rows() == other.rows()) && (cols() == other.cols());
  }

  /// \}
  /// \name Required by MatrixInterface.
  /// \{

  inline size_t rows() const
  {
    return backend_->rows();
  }

  inline size_t cols() const
  {
    return backend_->cols();
  }

  using InterfaceType::mv;

  template <class T1, class T2>
  inline void mv(const EigenBaseVector<T1, ScalarType>& xx, EigenBaseVector<T2, ScalarType>& yy) const
  {
    yy.backend().transpose() = backend() * xx.backend();
  }

  template <class V1, class V2>
  inline std::enable_if_t<XT::Common::is_vector<V1>::value && XT::Common::is_vector<V2>::value
                              && (!is_eigen_vector<V1>::value || !is_eigen_vector<V2>::value),
                          void>
  mv(const V1& xx, V2& yy) const
  {
    EigenDenseVector<ScalarType> xx_eigen(xx.size()), yy_eigen(yy.size());
    for (size_t ii = 0; ii < xx.size(); ++ii)
      xx_eigen.set_entry(ii, XT::Common::VectorAbstraction<V1>::get_entry(xx, ii));
    mv(xx_eigen, yy_eigen);
    for (size_t ii = 0; ii < yy.size(); ++ii)
      XT::Common::VectorAbstraction<V2>::set_entry(yy, ii, yy_eigen[ii]);
  }

  using InterfaceType::mtv;

  template <class T1, class T2>
  inline void mtv(const EigenBaseVector<T1, ScalarType>& xx, EigenBaseVector<T2, ScalarType>& yy) const
  {
    yy.backend().transpose() = backend().transpose() * xx.backend();
  }

  template <class V1, class V2>
  inline std::enable_if_t<XT::Common::is_vector<V1>::value && XT::Common::is_vector<V2>::value
                              && !is_eigen_vector<V1>::value,
                          void>
  mtv(const V1& xx, V2& yy) const
  {
    EigenDenseVector<ScalarType> xx_eigen(xx.size()), yy_eigen(yy.size());
    for (size_t ii = 0; ii < xx.size(); ++ii)
      xx_eigen.set_entry(ii, XT::Common::VectorAbstraction<V1>::get_entry(xx, ii));
    mtv(xx_eigen, yy_eigen);
    for (size_t ii = 0; ii < yy.size(); ++ii)
      XT::Common::VectorAbstraction<V2>::set_entry(yy, ii, yy_eigen[ii]);
  }

  void add_to_entry(const size_t ii, const size_t jj, const ScalarType& value)
  {
    assert(these_are_valid_indices(ii, jj));
    [[maybe_unused]] const internal::LockGuard lock(*mutexes_, ii, rows());
    backend().coeffRef(static_cast<EIGEN_size_t>(ii), static_cast<EIGEN_size_t>(jj)) += value;
  }

  void set_entry(const size_t ii, const size_t jj, const ScalarType& value)
  {
    assert(these_are_valid_indices(ii, jj));
    backend().coeffRef(static_cast<EIGEN_size_t>(ii), static_cast<EIGEN_size_t>(jj)) = value;
  }

  ScalarType get_entry(const size_t ii, const size_t jj) const
  {
    assert(ii < rows());
    assert(jj < cols());
    return backend().coeff(static_cast<EIGEN_size_t>(ii), static_cast<EIGEN_size_t>(jj));
  }

  void clear_row(const size_t ii)
  {
    if (ii >= rows())
      DUNE_THROW(Common::Exceptions::index_out_of_range,
                 "Given ii (" << ii << ") is larger than the rows of this (" << rows() << ")!");
    backend().row(static_cast<EIGEN_size_t>(ii)) *= ScalarType(0);
  }

  void clear_col(const size_t jj)
  {
    if (jj >= cols())
      DUNE_THROW(Common::Exceptions::index_out_of_range,
                 "Given jj (" << jj << ") is larger than the cols of this (" << cols() << ")!");
    for (size_t row = 0; static_cast<EIGEN_size_t>(row) < backend().outerSize(); ++row) {
      for (typename BackendType::InnerIterator row_it(backend(), static_cast<EIGEN_size_t>(row)); row_it; ++row_it) {
        const size_t col = row_it.col();
        if (col == jj) {
          backend().coeffRef(static_cast<EIGEN_size_t>(row), static_cast<EIGEN_size_t>(jj)) = ScalarType(0);
          break;
        }
        if (col > jj)
          break;
      }
    }
  } // ... clear_col(...)

  void unit_row(const size_t ii)
  {
    if (ii >= cols())
      DUNE_THROW(Common::Exceptions::index_out_of_range,
                 "Given ii (" << ii << ") is larger than the cols of this (" << cols() << ")!");
    if (ii >= rows())
      DUNE_THROW(Common::Exceptions::index_out_of_range,
                 "Given ii (" << ii << ") is larger than the rows of this (" << rows() << ")!");
    if (!these_are_valid_indices(ii, ii))
      DUNE_THROW(Common::Exceptions::index_out_of_range,
                 "Diagonal entry (" << ii << ", " << ii << ") is not contained in the sparsity pattern!");
    backend().row(static_cast<EIGEN_size_t>(ii)) *= ScalarType(0);
    backend().coeffRef(static_cast<EIGEN_size_t>(ii), static_cast<EIGEN_size_t>(ii)) = ScalarType(1);
  } // ... unit_row(...)

  void unit_col(const size_t jj)
  {
    if (jj >= cols())
      DUNE_THROW(Common::Exceptions::index_out_of_range,
                 "Given jj (" << jj << ") is larger than the cols of this (" << cols() << ")!");
    if (jj >= rows())
      DUNE_THROW(Common::Exceptions::index_out_of_range,
                 "Given jj (" << jj << ") is larger than the rows of this (" << rows() << ")!");
    for (size_t row = 0; static_cast<EIGEN_size_t>(row) < backend().outerSize(); ++row) {
      for (typename BackendType::InnerIterator row_it(backend(), static_cast<EIGEN_size_t>(row)); row_it; ++row_it) {
        const size_t col = row_it.col();
        if (col == jj) {
          if (col == row)
            backend().coeffRef(static_cast<EIGEN_size_t>(row), static_cast<EIGEN_size_t>(col)) = ScalarType(1);
          else
            backend().coeffRef(static_cast<EIGEN_size_t>(row), static_cast<EIGEN_size_t>(jj)) = ScalarType(0);
          break;
        }
        if (col > jj)
          break;
      }
    }
  } // ... unit_col(...)

  bool valid() const
  {
    // iterate over non-zero entries
    using InnerIterator = typename BackendType::InnerIterator;
    for (EIGEN_size_t ii = 0; ii < backend().outerSize(); ++ii) {
      for (InnerIterator it(backend(), ii); it; ++it) {
        if (Common::isnan(it.value()) || Common::isinf(it.value()))
          return false;
      }
    }
    return true;
  }

  size_t non_zeros() const final
  {
    return backend_->nonZeros();
  }

  SparsityPatternDefault
  pattern(const bool prune = false,
          const ScalarType eps = Common::FloatCmp::DefaultEpsilon<ScalarType>::value()) const override
  {
    SparsityPatternDefault ret(rows());
    const auto zero = typename Common::FloatCmp::DefaultEpsilon<ScalarType>::Type(0);
    if (prune) {
      for (EIGEN_size_t row = 0; row < backend().outerSize(); ++row) {
        for (typename BackendType::InnerIterator row_it(backend(), row); row_it; ++row_it) {
          const EIGEN_size_t col = row_it.col();
          const auto val = backend().coeff(row, col);
          if (Common::FloatCmp::ne(val, zero, eps))
            ret.insert(static_cast<size_t>(row), static_cast<size_t>(col));
        }
      }
    } else {
      for (EIGEN_size_t row = 0; row < backend().outerSize(); ++row) {
        for (typename BackendType::InnerIterator row_it(backend(), row); row_it; ++row_it)
          ret.insert(static_cast<size_t>(row), static_cast<size_t>(row_it.col()));
      }
    }
    ret.sort();
    return ret;
  } // ... pattern(...)

  ThisType pruned(const ScalarType eps = Common::FloatCmp::DefaultEpsilon<ScalarType>::value()) const final
  {
    return ThisType(*backend_, true, eps);
  }

  /// \}

  ScalarType* entries()
  {
    backend_->makeCompressed();
    return backend().valuePtr();
  }

  const ScalarType* entries() const
  {
    backend_->makeCompressed();
    return backend().valuePtr();
  }

  int* outer_index_ptr()
  {
    backend_->makeCompressed();
    return backend().outerIndexPtr();
  }

  const int* outer_index_ptr() const
  {
    return backend().outerIndexPtr();
    backend_->makeCompressed();
  }

  int* inner_index_ptr()
  {
    backend_->makeCompressed();
    return backend().innerIndexPtr();
  }

  const int* inner_index_ptr() const
  {
    backend_->makeCompressed();
    return backend().innerIndexPtr();
  }

  void set_num_mutexes(const size_t num_mutexes)
  {
    mutexes_ = std::make_unique<MutexesType>(num_mutexes);
  }

  using InterfaceType::operator+;
  using InterfaceType::operator-;
  using InterfaceType::operator+=;
  using InterfaceType::operator-=;

private:
  bool these_are_valid_indices(const size_t ii, const size_t jj) const
  {
    if (ii >= rows())
      return false;
    if (jj >= cols())
      return false;
    for (size_t row = ii; static_cast<EIGEN_size_t>(row) < backend().outerSize(); ++row) {
      for (typename BackendType::InnerIterator row_it(backend(), static_cast<EIGEN_size_t>(row)); row_it; ++row_it) {
        const size_t col = row_it.col();
        if ((ii == row) && (jj == col))
          return true;
        if ((row > ii) && (col > jj))
          return false;
      }
    }
    return false;
  } // ... these_are_valid_indices(...)


  std::shared_ptr<BackendType> backend_;
  std::unique_ptr<MutexesType> mutexes_;
}; // class EigenRowMajorSparseMatrix


#else // HAVE_EIGEN

template <class ScalarImp>
class EigenRowMajorSparseMatrix
{
  static_assert(AlwaysFalse<ScalarImp>::value, "You are missing Eigen!");
};

#endif // HAVE_EIGEN


} // namespace LA
namespace Common {


#if HAVE_EIGEN

template <class T>
struct MatrixAbstraction<LA::EigenRowMajorSparseMatrix<T>>
  : public LA::internal::MatrixAbstractionBase<LA::EigenRowMajorSparseMatrix<T>>
{
  using BaseType = LA::internal::MatrixAbstractionBase<LA::EigenRowMajorSparseMatrix<T>>;

  static constexpr Common::StorageLayout storage_layout = Common::StorageLayout::csr;

  template <size_t rows = BaseType::static_rows, size_t cols = BaseType::static_cols, class FieldType = T>
  using MatrixTypeTemplate = LA::EigenRowMajorSparseMatrix<FieldType>;
};

#endif // HAVE_EIGEN

} // namespace Common
} // namespace Dune::XT


// begin: this is what we need for the lib
#if DUNE_XT_WITH_PYTHON_BINDINGS && HAVE_EIGEN


extern template class Dune::XT::LA::EigenRowMajorSparseMatrix<double>;
// extern template void Dune::XT::LA::EigenRowMajorSparseMatrix<double>::mv(const
// Dune::XT::LA::EigenDenseVector<double>&,
//                                                                         Dune::XT::LA::EigenDenseVector<double>&)
//                                                                         const;
// extern template void
// Dune::XT::LA::EigenRowMajorSparseMatrix<double>::mv(const Dune::XT::LA::EigenMappedDenseVector<double>&,
//                                                    Dune::XT::LA::EigenMappedDenseVector<double>&) const;
// extern template void
// Dune::XT::LA::EigenRowMajorSparseMatrix<double>::mv(const Dune::XT::LA::EigenDenseVector<double>&,
//                                                    Dune::XT::LA::EigenMappedDenseVector<double>&) const;
// extern template void
// Dune::XT::LA::EigenRowMajorSparseMatrix<double>::mv(const Dune::XT::LA::EigenMappedDenseVector<double>&,
//                                                    Dune::XT::LA::EigenDenseVector<double>&) const;


#endif // DUNE_XT_WITH_PYTHON_BINDINGS && HAVE_EIGEN
// end: this is what we need for the lib


#endif // DUNE_XT_LA_CONTAINER_EIGEN_SPARSE_HH
