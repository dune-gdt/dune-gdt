// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   René Fritze    (2019 - 2020)
//   Tobias Leibner (2019 - 2020)

#ifndef DUNE_XT_LA_CONTAINER_MATRIX_VIEW_HH
#define DUNE_XT_LA_CONTAINER_MATRIX_VIEW_HH

#include <mutex>

#include <dune/xt/common/exceptions.hh>

#include "matrix-interface.hh"

namespace Dune::XT {
namespace LA {


// forwards
template <class MatrixImp>
class ConstMatrixView;

template <class MatrixImp>
class MatrixView;


namespace internal {


template <class MatrixImp>
class ConstMatrixViewTraits
  : public MatrixTraitsBase<typename MatrixImp::ScalarType,
                            ConstMatrixView<MatrixImp>,
                            typename MatrixImp::Traits::BackendType,
                            MatrixImp::Traits::backend_type,
                            MatrixImp::Traits::vector_type,
                            MatrixImp::Traits::sparse>
{};

template <class MatrixImp>
class MatrixViewTraits
  : public MatrixTraitsBase<typename MatrixImp::ScalarType,
                            MatrixView<MatrixImp>,
                            typename MatrixImp::Traits::BackendType,
                            MatrixImp::Traits::backend_type,
                            MatrixImp::Traits::vector_type,
                            MatrixImp::Traits::sparse>
{};

template <class MatrixImp>
MatrixImp& empty_matrix_ref()
{
  static MatrixImp matrix_;
  return matrix_;
}


} // namespace internal


template <class MatrixImp>
class ConstMatrixView
  : public MatrixInterface<internal::ConstMatrixViewTraits<MatrixImp>, typename MatrixImp::ScalarType>
{
  using BaseType = MatrixInterface<internal::ConstMatrixViewTraits<MatrixImp>, typename MatrixImp::ScalarType>;
  using ThisType = ConstMatrixView;

public:
  using ScalarType = typename BaseType::ScalarType;
  using RealType = typename BaseType::RealType;
  using Matrix = MatrixImp;

  // This constructor is only here for the interface to compile
  explicit ConstMatrixView(const size_t /*rr*/ = 0,
                           const size_t /*cc*/ = 0,
                           const ScalarType /*value*/ = ScalarType(0),
                           const size_t /*num_mutexes*/ = 1)
    : matrix_(internal::empty_matrix_ref<MatrixImp>())
    , pattern_(nullptr)
  {
    DUNE_THROW(XT::Common::Exceptions::you_are_using_this_wrong, "This constructor does not make sense for MatrixView");
  }

  // This constructor is only here for the interface to compile
  ConstMatrixView(const size_t /*rr*/,
                  const size_t /*cc*/,
                  const SparsityPatternDefault& /*pattern*/,
                  const size_t /*num_mutexes*/ = 1)
    : matrix_(internal::empty_matrix_ref<MatrixImp>())
    , pattern_(nullptr)
  {
    DUNE_THROW(XT::Common::Exceptions::you_are_using_this_wrong, "This constructor does not make sense for MatrixView");
  }

  // This is the actual constructor
  ConstMatrixView(const Matrix& matrix,
                  const size_t first_row,
                  const size_t past_last_row,
                  const size_t first_col,
                  const size_t past_last_col)
    : matrix_(matrix)
    , first_row_(first_row)
    , past_last_row_(past_last_row)
    , first_col_(first_col)
    , past_last_col_(past_last_col)
    , pattern_init_flag_(std::make_shared<std::once_flag>())
    , pattern_(nullptr)
  {
  }


  // We cannot copy a view (we could make a shallow copy, but that is error-prone).
  // However, the MatrixInterface does not compile for non-copyable types, so we fail at runtime here.
  ConstMatrixView(const ThisType& other)
    : matrix_(other.matrix_)
    , pattern_(nullptr)
  {
    DUNE_THROW(Dune::NotImplemented, "Copying is disabled, simply create a new view!");
  }

  // Moving from another view is fine
  ConstMatrixView(ThisType&& other) = default;
  ThisType& operator=(ThisType&& other) = default;

  // No assignment as this is a const view
  ThisType& operator=(const ThisType& other) = delete;

  inline ThisType copy() const
  {
    return *this;
  }

  size_t row_index(const size_t ii) const
  {
    assert(ii < rows());
    return first_row_ + ii;
  }

  size_t col_index(const size_t jj) const
  {
    assert(jj < cols());
    return first_col_ + jj;
  }

  inline size_t rows() const
  {
    return past_last_row_ - first_row_;
  }

  inline size_t cols() const
  {
    return past_last_col_ - first_col_;
  }

  inline void scal(const ScalarType& /*alpha*/)
  {
    DUNE_THROW(XT::Common::Exceptions::you_are_using_this_wrong, "You cannot use non-const methods on ConstMatrixView");
  }

  inline void axpy(const ScalarType& /*alpha*/, const ThisType& /*xx*/)
  {
    DUNE_THROW(XT::Common::Exceptions::you_are_using_this_wrong, "You cannot use non-const methods on ConstMatrixView");
  }

  template <class XX, class YY>
  inline void mv(const XX& xx, YY& yy) const
  {
    assert(xx.size() == cols() && yy.size() == rows());
    const auto& patt = get_pattern();
    for (size_t ii = 0; ii < rows(); ++ii) {
      yy[ii] = 0.;
      for (auto&& jj : patt.inner(ii))
        yy[ii] += get_entry(ii, jj) * xx[jj];
    }
  }

  template <class XX, class YY>
  inline void mtv(const XX& xx, YY& yy) const
  {
    assert(xx.size() == rows() && yy.size() == cols());
    const auto& patt = get_pattern();
    std::fill(yy.begin(), yy.end(), 0.);
    for (size_t ii = 0; ii < rows(); ++ii) {
      for (auto&& jj : patt.inner(ii))
        yy[jj] += get_entry(ii, jj) * xx[ii];
    }
  }

  inline void add_to_entry(const size_t /*ii*/, const size_t /*jj*/, const ScalarType& /*value*/)
  {
    DUNE_THROW(XT::Common::Exceptions::you_are_using_this_wrong, "You cannot use non-const methods on ConstMatrixView");
  }

  inline void set_entry(const size_t /*ii*/, const size_t /*jj*/, const ScalarType& /*value*/)
  {
    DUNE_THROW(XT::Common::Exceptions::you_are_using_this_wrong, "You cannot use non-const methods on ConstMatrixView");
  }

  inline ScalarType get_entry(const size_t ii, const size_t jj) const
  {
    assert(ii < rows() && jj < cols());
    return matrix_.get_entry(row_index(ii), col_index(jj));
  }

  inline void clear_row(const size_t /*ii*/)
  {
    DUNE_THROW(XT::Common::Exceptions::you_are_using_this_wrong, "You cannot use non-const methods on ConstMatrixView");
  }

  inline void clear_col(const size_t /*jj*/)
  {
    DUNE_THROW(XT::Common::Exceptions::you_are_using_this_wrong, "You cannot use non-const methods on ConstMatrixView");
  }

  inline void unit_row(const size_t /*ii*/)
  {
    DUNE_THROW(XT::Common::Exceptions::you_are_using_this_wrong, "You cannot use non-const methods on ConstMatrixView");
  }

  inline void unit_col(const size_t /*jj*/)
  {
    DUNE_THROW(XT::Common::Exceptions::you_are_using_this_wrong, "You cannot use non-const methods on ConstMatrixView");
  }

  inline bool valid() const
  {
    for (size_t ii = 0; ii < rows(); ++ii) {
      for (size_t jj = 0; jj < cols(); ++jj) {
        const auto entry = get_entry(ii, jj);
        if (Common::isnan(entry) || Common::isinf(entry))
          return false;
      }
    }
    return true;
  }

  RealType sup_norm() const final
  {
    RealType ret = 0;
    for (size_t ii = 0; ii < rows(); ++ii)
      for (size_t jj = 0; jj < cols(); ++jj)
        ret = std::max(ret, std::abs(get_entry(ii, jj)));
    return ret;
  } // ... sup_norm(...)

  SparsityPatternDefault pattern(const bool prune = false,
                                 const typename Common::FloatCmp::DefaultEpsilon<ScalarType>::Type eps =
                                     Common::FloatCmp::DefaultEpsilon<ScalarType>::value()) const final
  {
    SparsityPatternDefault ret(rows());
    auto matrix_patt = matrix_.pattern(prune, eps);
    for (size_t ii = 0; ii < rows(); ++ii)
      for (auto&& jj : matrix_patt.inner(row_index(ii)))
        if (jj >= first_col_ && jj < past_last_col_)
          ret.insert(ii, jj - first_col_);
    return ret;
  } // ... pattern(...)

  const SparsityPatternDefault& get_pattern() const
  {
    std::call_once(*pattern_init_flag_,
                   [this]() { this->pattern_ = std::make_shared<SparsityPatternDefault>(this->pattern()); });
    return *pattern_;
  }

  operator Matrix() const
  {
    const auto& patt = get_pattern();
    Matrix ret(rows(), cols(), patt);
    for (size_t ii = 0; ii < rows(); ++ii)
      for (auto&& jj : patt.inner(ii))
        ret.set_entry(ii, jj, get_entry(ii, jj));
    return ret;
  }

private:
  const Matrix& matrix_;
  const size_t first_row_{0};
  const size_t past_last_row_{0};
  const size_t first_col_{0};
  const size_t past_last_col_{0};
  std::shared_ptr<std::once_flag> pattern_init_flag_;
  mutable std::shared_ptr<SparsityPatternDefault> pattern_;
}; // class ConstMatrixView

template <class MatrixImp>
class MatrixView : public MatrixInterface<internal::MatrixViewTraits<MatrixImp>, typename MatrixImp::ScalarType>
{
  using BaseType = MatrixInterface<internal::MatrixViewTraits<MatrixImp>, typename MatrixImp::ScalarType>;
  using ConstMatrixViewType = ConstMatrixView<MatrixImp>;
  using ThisType = MatrixView;

public:
  using ScalarType = typename BaseType::ScalarType;
  using RealType = typename BaseType::RealType;
  using Matrix = MatrixImp;

  // This constructor is only here for the interface to compile
  explicit MatrixView(const size_t /*rr*/ = 0,
                      const size_t /*cc*/ = 0,
                      const ScalarType /*value*/ = ScalarType(0),
                      const size_t /*num_mutexes*/ = 1)
    : const_matrix_view_()
    , matrix_(internal::empty_matrix_ref<MatrixImp>())
  {
    DUNE_THROW(XT::Common::Exceptions::you_are_using_this_wrong, "This constructor does not make sense for MatrixView");
  }

  // This constructor is only here for the interface to compile
  MatrixView(const size_t /*rr*/,
             const size_t /*cc*/,
             const SparsityPatternDefault& /*pattern*/,
             const size_t /*num_mutexes*/ = 1)
    : const_matrix_view_()
    , matrix_(internal::empty_matrix_ref<MatrixImp>())
  {
    DUNE_THROW(XT::Common::Exceptions::you_are_using_this_wrong, "This constructor does not make sense for MatrixView");
  }

  // This is the actual constructor
  MatrixView(Matrix& matrix,
             const size_t first_row,
             const size_t past_last_row,
             const size_t first_col,
             const size_t past_last_col)
    : const_matrix_view_(matrix, first_row, past_last_row, first_col, past_last_col)
    , matrix_(matrix)
  {
  }

  // We cannot copy a view (we could make a shallow copy, but that is error-prone).
  // However, the MatrixInterface does not compile for non-copyable types, so we fail at runtime here.
  MatrixView(const ThisType& other)
    : const_matrix_view_(other.const_matrix_view_)
    , matrix_(other.matrix_)
  {
    DUNE_THROW(Dune::NotImplemented, "Copying is disabled, simply create a new view!");
  }

  // Moving from another view is fine
  MatrixView(ThisType&& other) = default;
  ThisType& operator=(ThisType&& other) = default;

  // Allow copy assignment from any dune-xt-la matrix
  template <class Mat>
  std::enable_if_t<XT::LA::is_matrix<Mat>::value, ThisType>& operator=(const Mat& other)
  {
    const auto& patt = get_pattern();
    assert(pattern_assignable(other));
    for (size_t ii = 0; ii < rows(); ++ii)
      for (auto&& jj : patt.inner(ii))
        set_entry(ii, jj, other.get_entry(ii, jj));
    return *this;
  }

  // Ensure assignment from another view copies the contents
  ThisType& operator=(const ThisType& other)
  {
    return operator=<ThisType>(other);
  }

  inline ThisType copy() const
  {
    return *this;
  }

  size_t row_index(const size_t ii) const
  {
    return const_matrix_view_.row_index(ii);
  }

  size_t col_index(const size_t jj) const
  {
    return const_matrix_view_.col_index(jj);
  }

  inline size_t rows() const
  {
    return const_matrix_view_.rows();
  }

  inline size_t cols() const
  {
    return const_matrix_view_.cols();
  }

  inline void scal(const ScalarType& alpha)
  {
    const auto& patt = get_pattern();
    for (size_t ii = 0; ii < rows(); ++ii)
      for (auto&& jj : patt.inner(ii))
        set_entry(ii, jj, get_entry(ii, jj) * alpha);
  }

  inline void axpy(const ScalarType& alpha, const ThisType& xx)
  {
    const auto other_patt = xx.pattern();
#ifndef NDEBUG
    const auto& patt = get_pattern();
    if (xx.rows() != rows() || xx.cols() != cols())
      DUNE_THROW(Common::Exceptions::shapes_do_not_match, "Shapes do not match!");
    for (size_t ii = 0; ii < rows(); ++ii)
      for (auto&& jj : other_patt.inner(ii))
        if (!patt.contains(ii, jj))
          DUNE_THROW(Dune::MathError, "Pattern of xx has to be a subset of this pattern!");
#endif
    for (size_t ii = 0; ii < rows(); ++ii)
      for (auto&& jj : other_patt.inner(ii))
        add_to_entry(ii, jj, xx.get_entry(ii, jj) * alpha);
  }

  template <class OtherTraits>
  inline void axpy(const ScalarType& alpha, const MatrixInterface<OtherTraits, ScalarType>& xx)
  {
    const auto& patt = get_pattern();
#ifndef NDEBUG
    const auto& other_patt = xx.pattern();
    if (xx.rows() != rows() || xx.cols() != cols())
      DUNE_THROW(Common::Exceptions::shapes_do_not_match, "Shapes do not match!");
    for (size_t ii = 0; ii < rows(); ++ii)
      for (auto&& jj : other_patt.inner(ii)) {
        // The EigenRowMajorSparseMatrix automatically adds one entry to an empty row
        if (!patt.contains(ii, jj)
            && !(patt.inner(ii).size() == 0 && other_patt.inner(ii).size() == 1 && other_patt.inner(ii)[0] == 0))
          DUNE_THROW(Dune::MathError, "Pattern of xx has to be a subset of this pattern!");
      }
#endif
    for (size_t ii = 0; ii < rows(); ++ii)
      for (auto&& jj : patt.inner(ii))
        add_to_entry(ii, jj, xx.get_entry(ii, jj) * alpha);
  }

  template <class XX, class YY>
  inline void mv(const XX& xx, YY& yy) const
  {
    return const_matrix_view_.mv(xx, yy);
  }

  template <class XX, class YY>
  inline void mtv(const XX& xx, YY& yy) const
  {
    return const_matrix_view_.mtv(xx, yy);
  }

  inline bool valid() const
  {
    return const_matrix_view_.valid();
  }

  RealType sup_norm() const final
  {
    return const_matrix_view_.sup_norm();
  } // ... sup_norm(...)

  SparsityPatternDefault pattern(const bool prune = false,
                                 const typename Common::FloatCmp::DefaultEpsilon<ScalarType>::Type eps =
                                     Common::FloatCmp::DefaultEpsilon<ScalarType>::value()) const final
  {
    return const_matrix_view_.pattern(prune, eps);
  } // ... pattern(...)

  inline void add_to_entry(const size_t ii, const size_t jj, const ScalarType& value)
  {
    assert(ii < rows() && jj < cols());
    matrix_.add_to_entry(row_index(ii), col_index(jj), value);
  }

  inline void set_entry(const size_t ii, const size_t jj, const ScalarType& value)
  {
    assert(ii < rows() && jj < cols());
    matrix_.set_entry(row_index(ii), col_index(jj), value);
  }

  inline ScalarType get_entry(const size_t ii, const size_t jj) const
  {
    return const_matrix_view_.get_entry(ii, jj);
  }

  inline void clear_row(const size_t ii)
  {
    const auto& patt = get_pattern();
    for (auto&& jj : patt.inner(ii))
      set_entry(ii, jj, 0.);
  }

  inline void clear_col(const size_t jj)
  {
    const auto& patt = get_pattern();
    for (size_t ii = 0; ii < rows(); ++ii)
      if (std::find(patt.inner(ii).begin(), patt.inner(ii).end(), jj) != patt.inner(ii).end())
        set_entry(ii, jj, 0.);
  }

  inline void unit_row(const size_t ii)
  {
    clear_row(ii);
    set_entry(ii, ii, 1.);
  }

  inline void unit_col(const size_t jj)
  {
    clear_col(jj);
    set_entry(jj, jj, 1.);
  }

  operator Matrix() const
  {
    return const_matrix_view_.operator Matrix();
  }

  const SparsityPatternDefault& get_pattern() const
  {
    return const_matrix_view_.get_pattern();
  }

private:
  bool pattern_assignable(const Matrix& other) const
  {
    const auto& patt = get_pattern();
    const auto& other_patt = other.pattern();
    for (size_t ii = 0; ii < rows(); ++ii)
      for (auto&& jj : other_patt.inner(ii))
        if (!patt.contains(ii, jj) && !XT::Common::is_zero(other.get_entry(ii, jj)))
          return false;
    return true;
  }

  ConstMatrixViewType const_matrix_view_;
  Matrix& matrix_;
}; // class MatrixView


} // namespace LA
namespace Common {


template <class MatrixImp>
struct MatrixAbstraction<LA::ConstMatrixView<MatrixImp>>
  : public LA::internal::MatrixAbstractionBase<LA::ConstMatrixView<MatrixImp>>
{
  using BaseType = LA::internal::MatrixAbstractionBase<MatrixImp>;
  static constexpr Common::StorageLayout storage_layout = MatrixAbstraction<MatrixImp>::storage_layout;

  template <size_t rows = BaseType::static_rows,
            size_t cols = BaseType::static_cols,
            class FieldType = typename MatrixImp::ScalarType>
  using MatrixTypeTemplate = LA::ConstMatrixView<MatrixImp>;
};


template <class MatrixImp>
struct MatrixAbstraction<LA::MatrixView<MatrixImp>>
  : public LA::internal::MatrixAbstractionBase<LA::MatrixView<MatrixImp>>
{
  using BaseType = LA::internal::MatrixAbstractionBase<MatrixImp>;
  static constexpr Common::StorageLayout storage_layout = MatrixAbstraction<MatrixImp>::storage_layout;

  template <size_t rows = BaseType::static_rows,
            size_t cols = BaseType::static_cols,
            class FieldType = typename MatrixImp::ScalarType>
  using MatrixTypeTemplate = LA::MatrixView<MatrixImp>;
};


} // namespace Common
} // namespace Dune::XT

#endif // DUNE_XT_LA_CONTAINER_MATRIX_VIEW_HH
