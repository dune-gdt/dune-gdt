// This file is part of the dune-xt-la project:
//   https://github.com/dune-community/dune-xt-la
// Copyright 2009-2018 dune-xt-la developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2017)
//   Rene Milk       (2018)
//   Tobias Leibner  (2017)

#ifndef DUNE_XT_LA_CONTAINER_COMMON_MATRIX_DENSE_HH
#define DUNE_XT_LA_CONTAINER_COMMON_MATRIX_DENSE_HH

#include <cmath>
#include <memory>
#include <mutex>
#include <vector>

#include <dune/common/ftraits.hh>
#include <dune/common/unused.hh>

#include <dune/xt/common/exceptions.hh>
#include <dune/xt/common/float_cmp.hh>
#include <dune/xt/common/vector.hh>

#include <dune/xt/la/container/matrix-interface.hh>
#include <dune/xt/la/container/pattern.hh>

namespace Dune {
namespace XT {
namespace LA {


template <class ScalarImp>
class CommonDenseMatrix;

template <class ScalarImp>
class CommonSparseVector;


namespace internal {


template <class ScalarType>
struct RowMajorMatrixBackend
{
  RowMajorMatrixBackend(size_t num_rows, size_t num_cols, const ScalarType value = ScalarType(0))
    : num_rows_(num_rows)
    , num_cols_(num_cols)
    , entries_(num_rows_ * num_cols_, value)
  {
  }

  ScalarType& get_entry_ref(size_t rr, size_t cc)
  {
    return entries_[rr * num_cols_ + cc];
  }

  const ScalarType& get_entry_ref(size_t rr, size_t cc) const
  {
    return entries_[rr * num_cols_ + cc];
  }

  size_t num_rows_;
  size_t num_cols_;
  std::vector<ScalarType> entries_;
};


template <class ScalarImp>
class CommonDenseMatrixTraits
{
public:
  using ScalarType = typename Dune::FieldTraits<ScalarImp>::field_type;
  using RealType = typename Dune::FieldTraits<ScalarImp>::real_type;
  using BackendType = RowMajorMatrixBackend<ScalarType>;
  using derived_type = CommonDenseMatrix<ScalarType>;
  static const Backends backend_type = Backends::common_dense;
  static const Backends vector_type = Backends::common_dense;
  static const constexpr bool sparse = false;
};


} // namespace internal


/**
 *  \brief  A dense matrix implementation of MatrixInterface using the a std::vector backend.
 */
template <class ScalarImp = double>
class CommonDenseMatrix : public MatrixInterface<internal::CommonDenseMatrixTraits<ScalarImp>, ScalarImp>,
                          public ProvidesBackend<internal::CommonDenseMatrixTraits<ScalarImp>>
{
  using ThisType = CommonDenseMatrix;
  using MatrixInterfaceType = MatrixInterface<internal::CommonDenseMatrixTraits<ScalarImp>, ScalarImp>;

public:
  using Traits = typename internal::CommonDenseMatrixTraits<ScalarImp>;
  using BackendType = typename Traits::BackendType;
  using ScalarType = typename Traits::ScalarType;
  using RealType = typename Traits::RealType;

  explicit CommonDenseMatrix(const size_t rr = 0,
                             const size_t cc = 0,
                             const ScalarType value = ScalarType(0),
                             const size_t num_mutexes = 1)
    : backend_(std::make_shared<BackendType>(rr, cc, value))
    , mutexes_(num_mutexes > 0 ? std::make_shared<std::vector<std::mutex>>(num_mutexes) : nullptr)
    , unshareable_(false)
  {
  }

  /// This constructors ignores the given pattern and initializes the matrix with 0.
  CommonDenseMatrix(const size_t rr,
                    const size_t cc,
                    const SparsityPatternDefault& /*pattern*/,
                    const size_t num_mutexes = 1)
    : backend_(std::make_shared<BackendType>(rr, cc, ScalarType(0)))
    , mutexes_(num_mutexes > 0 ? std::make_shared<std::vector<std::mutex>>(num_mutexes) : nullptr)
    , unshareable_(false)
  {
  }

  CommonDenseMatrix(const ThisType& other)
    : backend_(other.unshareable_ ? std::make_shared<BackendType>(*other.backend_) : other.backend_)
    , mutexes_(other.unshareable_ ? std::make_shared<std::vector<std::mutex>>(other.mutexes_->size()) : other.mutexes_)
    , unshareable_(false)
  {
  }

  /**
   * \note If prune == true, this implementation is not optimal!
   */
  explicit CommonDenseMatrix(const BackendType& other,
                             const bool prune = false,
                             const typename Common::FloatCmp::DefaultEpsilon<ScalarType>::Type eps =
                                 Common::FloatCmp::DefaultEpsilon<ScalarType>::value(),
                             const size_t num_mutexes = 1)
    : mutexes_(num_mutexes > 0 ? std::make_shared<std::vector<std::mutex>>(num_mutexes) : nullptr)
    , unshareable_(false)
  {
    if (prune)
      backend_ = ThisType(other).pruned(eps).backend_;
    else
      backend_ = std::make_shared<BackendType>(other);
  }

  template <class T>
  CommonDenseMatrix(const DenseMatrix<T>& other, const size_t num_mutexes = 1)
    : backend_(std::make_shared<BackendType>(other.rows(), other.cols()))
    , mutexes_(num_mutexes > 0 ? std::make_shared<std::vector<std::mutex>>(num_mutexes) : nullptr)
    , unshareable_(false)
  {
    for (size_t ii = 0; ii < other.rows(); ++ii)
      for (size_t jj = 0; jj < other.cols(); ++jj)
        set_entry(ii, jj, other[ii][jj]);
  } // CommonDenseMatrix(...)

  /**
   *  \note Takes ownership of backend_ptr in the sense that you must not delete it afterwards!
   */
  explicit CommonDenseMatrix(BackendType* backend_ptr, const size_t num_mutexes = 1)
    : backend_(backend_ptr)
    , mutexes_(num_mutexes > 0 ? std::make_shared<std::vector<std::mutex>>(num_mutexes) : nullptr)
    , unshareable_(false)
  {
  }

  explicit CommonDenseMatrix(std::shared_ptr<BackendType> backend_ptr, const size_t num_mutexes = 1)
    : backend_(backend_ptr)
    , mutexes_(num_mutexes > 0 ? std::make_shared<std::vector<std::mutex>>(num_mutexes) : nullptr)
    , unshareable_(false)
  {
  }

  ThisType& operator=(const ThisType& other)
  {
    if (this != &other) {
      backend_ = other.unshareable_ ? std::make_shared<BackendType>(*other.backend_) : other.backend_;
      mutexes_ = other.mutexes_
                     ? (other.unshareable_ ? std::make_shared<std::vector<std::mutex>>(other.mutexes_->size())
                                           : other.mutexes_)
                     : nullptr;
      unshareable_ = false;
    }
    return *this;
  }

  /**
   *  \note Does a deep copy.
   */
  ThisType& operator=(const BackendType& other)
  {
    backend_ = std::make_shared<BackendType>(other);
    unshareable_ = false;
    return *this;
  }

  /// \name Required by the ProvidesBackend interface.
  /// \{

  BackendType& backend()
  {
    ensure_uniqueness();
    unshareable_ = true;
    return *backend_;
  }

  const BackendType& backend() const
  {
    ensure_uniqueness();
    unshareable_ = true;
    return *backend_;
  }

  ScalarType* data()
  {
    return backend().entries_.data();
  }

  const ScalarType* data() const
  {
    return backend().entries_.data();
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
    ensure_uniqueness();
    const internal::VectorLockGuard DUNE_UNUSED(guard)(mutexes_);
    for (auto& entry : backend_->entries_)
      entry *= alpha;
  }

  void axpy(const ScalarType& alpha, const ThisType& xx)
  {
    ensure_uniqueness();
    const internal::VectorLockGuard DUNE_UNUSED(guard)(mutexes_);
    if (!has_equal_shape(xx))
      DUNE_THROW(Common::Exceptions::shapes_do_not_match,
                 "The shape of xx (" << xx.rows() << "x" << xx.cols() << ") does not match the shape of this ("
                                     << rows()
                                     << "x"
                                     << cols()
                                     << ")!");
    for (size_t ii = 0; ii < backend_->entries_.size(); ++ii)
      backend_->entries_[ii] += alpha * xx.backend_->entries_[ii];
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
    return backend_->num_rows_;
  }

  inline size_t cols() const
  {
    return backend_->num_cols_;
  }

  template <class FirstVectorType, class SecondVectorType>
  inline void mv(const FirstVectorType& xx, SecondVectorType& yy) const
  {
    using V1 = typename Common::VectorAbstraction<FirstVectorType>;
    using V2 = typename Common::VectorAbstraction<SecondVectorType>;
    static_assert(V1::is_vector && V2::is_vector, "");
    assert(xx.size() == cols() && yy.size() == rows());
    yy *= ScalarType(0.);
    for (size_t rr = 0; rr < rows(); ++rr) {
      V2::set_entry(yy, rr, 0.);
      for (size_t cc = 0; cc < cols(); ++cc)
        V2::add_to_entry(yy, rr, get_entry(rr, cc) * V1::get_entry(xx, cc));
    }
  }

  template <class FirstVectorType, class SecondVectorType>
  inline void mtv(const FirstVectorType& xx, SecondVectorType& yy) const
  {
    using V1 = typename Common::VectorAbstraction<FirstVectorType>;
    using V2 = typename Common::VectorAbstraction<SecondVectorType>;
    static_assert(V1::is_vector && V2::is_vector, "");
    assert(V1::size(xx) == rows() && V1::size(yy) == cols());
    yy *= ScalarType(0.);
    for (size_t cc = 0; cc < cols(); ++cc) {
      V2::set_entry(yy, cc, 0.);
      for (size_t rr = 0; rr < rows(); ++rr)
        V2::add_to_entry(yy, cc, get_entry(cc, rr) * V1::get_entry(xx, rr));
    }
  }

  void mtv(const CommonSparseVector<ScalarType>& xx, CommonSparseVector<ScalarType>& yy) const
  {
    yy.clear();
    const auto& vec_entries = xx.entries();
    const auto& vec_indices = xx.indices();
    thread_local std::vector<ScalarType> tmp_vec;
    tmp_vec.resize(cols(), 0.);
    std::fill(tmp_vec.begin(), tmp_vec.end(), 0.);
    for (size_t ii = 0; ii < vec_entries.size(); ++ii) {
      const size_t rr = vec_indices[ii];
      for (size_t cc = 0; cc < cols(); ++cc)
        tmp_vec[cc] += vec_entries[ii] * backend_->get_entry_ref(rr, cc);
    }
    for (size_t cc = 0; cc < cols(); ++cc)
      if (XT::Common::FloatCmp::ne(tmp_vec[cc], 0.))
        yy.set_new_entry(cc, tmp_vec[cc]);
  } // void mtv(...)

  void add_to_entry(const size_t ii, const size_t jj, const ScalarType& value)
  {
    ensure_uniqueness();
    internal::LockGuard DUNE_UNUSED(lock)(mutexes_, ii);
    assert(ii < rows());
    assert(jj < cols());
    backend_->get_entry_ref(ii, jj) += value;
  } // ... add_to_entry(...)

  void set_entry(const size_t ii, const size_t jj, const ScalarType& value)
  {
    ensure_uniqueness();
    assert(ii < rows());
    assert(jj < cols());
    backend_->get_entry_ref(ii, jj) = value;
  } // ... set_entry(...)

  ScalarType get_entry(const size_t ii, const size_t jj) const
  {
    assert(ii < rows());
    assert(jj < cols());
    return backend_->get_entry_ref(ii, jj);
  } // ... get_entry(...)

  void clear_row(const size_t ii)
  {
    ensure_uniqueness();
    if (ii >= rows())
      DUNE_THROW(Common::Exceptions::index_out_of_range,
                 "Given ii (" << ii << ") is larger than the rows of this (" << rows() << ")!");
    std::fill_n(&(backend_->get_entry_ref(ii, 0)), cols(), ScalarType(0));
  } // ... clear_row(...)

  void clear_col(const size_t jj)
  {
    ensure_uniqueness();
    if (jj >= cols())
      DUNE_THROW(Common::Exceptions::index_out_of_range,
                 "Given jj (" << jj << ") is larger than the cols of this (" << cols() << ")!");
    for (size_t ii = 0; ii < rows(); ++ii)
      backend_->get_entry_ref(ii, jj) = ScalarType(0);
  } // ... clear_col(...)

  void unit_row(const size_t ii)
  {
    ensure_uniqueness();
    if (ii >= cols())
      DUNE_THROW(Common::Exceptions::index_out_of_range,
                 "Given ii (" << ii << ") is larger than the cols of this (" << cols() << ")!");
    if (ii >= rows())
      DUNE_THROW(Common::Exceptions::index_out_of_range,
                 "Given ii (" << ii << ") is larger than the rows of this (" << rows() << ")!");
    clear_row(ii);
    set_entry(ii, ii, ScalarType(1));
  } // ... unit_row(...)

  void unit_col(const size_t jj)
  {
    ensure_uniqueness();
    if (jj >= cols())
      DUNE_THROW(Common::Exceptions::index_out_of_range,
                 "Given jj (" << jj << ") is larger than the cols of this (" << cols() << ")!");
    if (jj >= rows())
      DUNE_THROW(Common::Exceptions::index_out_of_range,
                 "Given jj (" << jj << ") is larger than the rows of this (" << rows() << ")!");
    clear_col(jj);
    set_entry(jj, jj, ScalarType(1));
  } // ... unit_col(...)

  bool valid() const
  {
    for (const auto& entry : backend_->entries_)
      if (Common::isnan(entry) || Common::isinf(entry))
        return false;
    return true;
  } // ... valid(...)

  /// \}

  using MatrixInterfaceType::operator+;
  using MatrixInterfaceType::operator-;
  using MatrixInterfaceType::operator+=;
  using MatrixInterfaceType::operator-=;

  void deep_copy(const ThisType& other)
  {
    ensure_uniqueness();
    *backend_ = *other.backend_;
  }

  template <class OtherMatrixType>
  void rightmultiply(const OtherMatrixType& other)
  {
    using M = typename Common::MatrixAbstraction<OtherMatrixType>;
    static_assert(M::is_matrix, "");
    ensure_uniqueness();
    BackendType new_backend(rows(), M::cols(other), ScalarType(0.));
    if (M::rows(other) != cols())
      DUNE_THROW(Dune::XT::Common::Exceptions::shapes_do_not_match,
                 "For rightmultiply, the number of columns of this has to match the number of rows of other!");
    for (size_t rr = 0; rr < rows(); ++rr)
      for (size_t cc = 0; cc < cols(); ++cc)
        for (size_t kk = 0; kk < cols(); ++kk)
          new_backend->get_entry_ref(rr, cc) += get_entry(rr, kk) * M::get_entry(other, kk, cc);
    *backend_ = new_backend;
  }

  virtual ThisType pruned(const typename Common::FloatCmp::DefaultEpsilon<ScalarType>::Type eps =
                              Common::FloatCmp::DefaultEpsilon<ScalarType>::value()) const override
  {
    auto ret = this->copy();
    for (auto& entry : ret.backend_->entries_)
      if (XT::Common::FloatCmp::eq<Common::FloatCmp::Style::absolute>(ScalarType(0.), entry, 0., eps))
        entry = ScalarType(0);
    return ret;
  } // ... pruned(...)

protected:
  /**
   * \see ContainerInterface
   */
  inline void ensure_uniqueness() const
  {
    if (!backend_.unique()) {
      assert(!unshareable_);
      const internal::VectorLockGuard DUNE_UNUSED(guard)(mutexes_);
      if (!backend_.unique()) {
        backend_ = std::make_shared<BackendType>(*backend_);
        mutexes_ = mutexes_ ? std::make_shared<std::vector<std::mutex>>(mutexes_->size()) : nullptr;
      }
    }
  } // ... ensure_uniqueness(...)

private:
  mutable std::shared_ptr<BackendType> backend_;
  mutable std::shared_ptr<std::vector<std::mutex>> mutexes_;
  mutable bool unshareable_;
}; // class CommonDenseMatrix


} // namespace LA
namespace Common {


template <class T>
struct MatrixAbstraction<LA::CommonDenseMatrix<T>>
    : public LA::internal::MatrixAbstractionBase<LA::CommonDenseMatrix<T>>
{
  static const constexpr Common::StorageLayout storage_layout = Common::StorageLayout::dense_row_major;

  static inline T* data(LA::CommonDenseMatrix<T>& mat)
  {
    return mat.data();
  }

  static inline const T* data(const LA::CommonDenseMatrix<T>& mat)
  {
    return mat.data();
  }
};


} // namespace Common
} // namespace XT
} // namespace Dune


// begin: this is what we need for the lib
#if DUNE_XT_WITH_PYTHON_BINDINGS


extern template class Dune::XT::LA::CommonDenseMatrix<double>;


#endif // DUNE_XT_WITH_PYTHON_BINDINGS
// end: this is what we need for the lib


#endif // DUNE_XT_LA_CONTAINER_COMMON_MATRIX_DENSE_HH
