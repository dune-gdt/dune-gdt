// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   René Fritze    (2019 - 2020)
//   Tobias Leibner (2019 - 2020)

#ifndef DUNE_XT_LA_CONTAINER_VECTOR_VIEW_HH
#define DUNE_XT_LA_CONTAINER_VECTOR_VIEW_HH

#include <dune/xt/common/exceptions.hh>

#include "vector-interface.hh"

namespace Dune::XT {
namespace LA {


// forwards
template <class VectorImp>
class ConstVectorView;

template <class VectorImp>
class VectorView;


namespace internal {


template <class VectorImp, bool is_la_vector = LA::is_vector<VectorImp>::value>
struct VectorBackendHelper
{
  static constexpr Backends backend = default_backend;
  static constexpr Backends dense_backend = default_dense_backend;
  static constexpr Backends sparse_backend = default_sparse_backend;
  using BackendType = void;
};

template <class VectorImp>
struct VectorBackendHelper<VectorImp, true>
{
  static constexpr Backends backend = VectorImp::Traits::backend_type;
  static constexpr Backends dense_backend = VectorImp::Traits::dense_matrix_type;
  static constexpr Backends sparse_backend = VectorImp::Traits::sparse_matrix_type;
  using BackendType = typename VectorImp::Traits::BackendType;
};


template <class VectorImp>
class ConstVectorViewTraits
  : public VectorTraitsBase<typename Common::VectorAbstraction<VectorImp>::ScalarType,
                            ConstVectorView<VectorImp>,
                            typename VectorBackendHelper<VectorImp>::BackendType,
                            VectorBackendHelper<VectorImp>::backend,
                            VectorBackendHelper<VectorImp>::dense_backend,
                            VectorBackendHelper<VectorImp>::sparse_backend>
{};

template <class VectorImp>
class VectorViewTraits
  : public VectorTraitsBase<typename Common::VectorAbstraction<VectorImp>::ScalarType,
                            VectorView<VectorImp>,
                            typename VectorBackendHelper<VectorImp>::BackendType,
                            VectorBackendHelper<VectorImp>::backend,
                            VectorBackendHelper<VectorImp>::dense_backend,
                            VectorBackendHelper<VectorImp>::sparse_backend>
{};

template <class VectorImp>
VectorImp& empty_vector_ref()
{
  static VectorImp vector_;
  return vector_;
}


} // namespace internal


template <class VectorImp>
class ConstVectorView
  : public VectorInterface<internal::ConstVectorViewTraits<VectorImp>,
                           typename Common::VectorAbstraction<VectorImp>::ScalarType>
{
  using BaseType = VectorInterface<internal::ConstVectorViewTraits<VectorImp>,
                                   typename Common::VectorAbstraction<VectorImp>::ScalarType>;
  using ThisType = ConstVectorView;

public:
  using ScalarType = typename BaseType::ScalarType;
  using RealType = typename BaseType::RealType;
  using Vector = VectorImp;

  // This constructor is only here for the interface to compile
  explicit ConstVectorView(const size_t /*sz*/ = 0,
                           const ScalarType /*value*/ = ScalarType(0),
                           const size_t /*num_mutexes*/ = 1)
    : vector_(internal::empty_vector_ref<VectorImp>())

  {
    DUNE_THROW(XT::Common::Exceptions::you_are_using_this_wrong, "This constructor does not make sense for VectorView");
  }

  // This is the actual constructor
  ConstVectorView(const Vector& vector, const size_t first_entry, const size_t past_last_entry)
    : vector_(vector)
    , first_entry_(first_entry)
    , past_last_entry_(past_last_entry)
  {
  }

  // We cannot copy a view (we could make a shallow copy, but that is error-prone).
  // However, the VectorInterface does not compile for non-copyable types, so we fail at runtime here.
  ConstVectorView(const ThisType& other)
    : vector_(other.vector_)
    , first_entry_(other.first_entry_)
    , past_last_entry_(other.past_last_entry_)
  {
    DUNE_THROW(Dune::NotImplemented, "Copying is disabled, simply create a new view!");
  }
  // Moving from another view is fine
  ConstVectorView(ThisType&& other) = default;
  // No assignment as this is a const view
  ThisType& operator=(const ThisType& other) = delete;
  ThisType& operator=(ThisType&& other) = delete;

  inline ThisType copy() const
  {
    return *this;
  }

  size_t index(const size_t ii) const
  {
    return first_entry_ + ii;
  }

  inline size_t size() const
  {
    return past_last_entry_ - first_entry_;
  }

  inline void resize(const size_t /*new_size*/)
  {
    DUNE_THROW(XT::Common::Exceptions::you_are_using_this_wrong, "You cannot use non-const methods on ConstVectorView");
  }

  inline void add_to_entry(const size_t /*ii*/, const ScalarType& /*value*/)
  {
    DUNE_THROW(XT::Common::Exceptions::you_are_using_this_wrong, "You cannot use non-const methods on ConstVectorView");
  }

  inline void set_entry(const size_t /*ii*/, const ScalarType& /*value*/)
  {
    DUNE_THROW(XT::Common::Exceptions::you_are_using_this_wrong, "You cannot use non-const methods on ConstVectorView");
  }

  inline ScalarType get_entry(const size_t ii) const
  {
    return vector_[index(ii)];
  }

  inline ScalarType& operator[](const size_t /*ii*/)
  {
    DUNE_THROW(XT::Common::Exceptions::you_are_using_this_wrong, "You cannot use non-const methods on ConstVectorView");
    static ScalarType ret;
    return ret;
  }

  inline const ScalarType& operator[](const size_t ii) const
  {
    return vector_[index(ii)];
  }

  ThisType add(const ThisType& other) const final
  {
    DUNE_THROW(XT::Common::Exceptions::you_are_using_this_wrong,
               "Methods returning a new VectorView are not implemented!");
    return other;
  } // ... add(...)

  ThisType sub(const ThisType& other) const final
  {
    DUNE_THROW(XT::Common::Exceptions::you_are_using_this_wrong,
               "Methods returning a new VectorView are not implemented!");
    return other;
  } // ... sub(...)

  ThisType operator+(const ThisType& other) const final
  {
    DUNE_THROW(XT::Common::Exceptions::you_are_using_this_wrong,
               "Methods returning a new VectorView are not implemented!");
    return other;
  }

  ThisType operator-(const ThisType& other) const final
  {
    DUNE_THROW(XT::Common::Exceptions::you_are_using_this_wrong,
               "Methods returning a new VectorView are not implemented!");
    return other;
  }

  inline void scal(const ScalarType& /*alpha*/)
  {
    DUNE_THROW(XT::Common::Exceptions::you_are_using_this_wrong, "You cannot use non-const methods on ConstVectorView");
  }

  inline void axpy(const ScalarType& /*alpha*/, const ThisType& /*xx*/)
  {
    DUNE_THROW(XT::Common::Exceptions::you_are_using_this_wrong, "You cannot use non-const methods on ConstVectorView");
  }

  inline void add_to_entry(const size_t /*ii*/, const size_t /*jj*/, const ScalarType& /*value*/)
  {
    DUNE_THROW(XT::Common::Exceptions::you_are_using_this_wrong, "You cannot use non-const methods on ConstVectorView");
  }

  inline void set_entry(const size_t /*ii*/, const size_t /*jj*/, const ScalarType& /*value*/)
  {
    DUNE_THROW(XT::Common::Exceptions::you_are_using_this_wrong, "You cannot use non-const methods on ConstVectorView");
  }

  const Vector& vec() const
  {
    return vector_;
  }

  size_t first_entry() const
  {
    return first_entry_;
  }

  size_t past_last_entry() const
  {
    return past_last_entry_;
  }

  template <class T>
  ScalarType dot(const VectorInterface<T, ScalarType>& other) const
  {
    return BaseType::template complex_switch<>::dot(*this, other);
  }

protected:
  inline const ScalarType& get_unchecked_ref(const size_t ii) const
  {
    return vector_[first_entry_ + ii];
  }

private:
  friend class VectorInterface<internal::ConstVectorViewTraits<VectorImp>, ScalarType>;

  const Vector& vector_;
  const size_t first_entry_{0};
  const size_t past_last_entry_{0};
}; // class ConstVectorView


template <class VectorImp>
class VectorView
  : public VectorInterface<internal::VectorViewTraits<VectorImp>,
                           typename Common::VectorAbstraction<VectorImp>::ScalarType>
{
  using BaseType =
      VectorInterface<internal::VectorViewTraits<VectorImp>, typename Common::VectorAbstraction<VectorImp>::ScalarType>;
  using ConstVectorViewType = ConstVectorView<VectorImp>;
  using ThisType = VectorView;
  using V = Common::VectorAbstraction<VectorImp>;

public:
  using ScalarType = typename BaseType::ScalarType;
  using RealType = typename BaseType::RealType;
  using Vector = VectorImp;

  // This constructor is only here for the interface to compile
  explicit VectorView(const size_t /*sz*/ = 0,
                      const ScalarType /*value*/ = ScalarType(0),
                      const size_t /*num_mutexes*/ = 1)
    : const_vector_view_()
    , vector_(internal::empty_vector_ref<VectorImp>())
  {
    DUNE_THROW(XT::Common::Exceptions::you_are_using_this_wrong, "This constructor does not make sense for VectorView");
  }

  // This is the actual constructor
  VectorView(Vector& vector, const size_t first_entry, const size_t past_last_entry)
    : const_vector_view_(vector, first_entry, past_last_entry)
    , vector_(vector)
  {
  }

  // We cannot copy a view (we could make a shallow copy, but that is error-prone).
  // However, the VectorInterface does not compile for non-copyable types, so we fail at runtime here.
  VectorView(const ThisType& other)
    : const_vector_view_(other.const_vector_view_)
    , vector_(other.vector_)
  {
    DUNE_THROW(Dune::NotImplemented, "Copying is disabled, simply create a new view!");
  }

  // Moving from another view is fine
  VectorView(ThisType&& other) = default;
  ThisType& operator=(VectorView&& other) = default;

  // Allow copy assignment from any vector
  template <class Vec>
  std::enable_if_t<XT::Common::is_vector<Vec>::value, ThisType>& operator=(const Vec& other)
  {
    for (size_t ii = 0; ii < size(); ++ii)
      set_entry(ii, other.get_entry(ii));
    return *this;
  }

  // Ensure assignment from another view copies the contents
  ThisType& operator=(const ThisType& other)
  {
    return operator=<ThisType>(other);
  }

  ThisType& operator=(const ScalarType& val)
  {
    for (size_t ii = 0; ii < size(); ++ii)
      set_entry(ii, val);
    return *this;
  }

  size_t index(const size_t ii) const
  {
    return const_vector_view_.index(ii);
  }

  inline size_t size() const
  {
    return const_vector_view_.size();
  }

  inline void scal(const ScalarType& alpha)
  {
    for (size_t ii = 0; ii < size(); ++ii)
      set_entry(ii, get_entry(ii) * alpha);
  }

  inline void axpy(const ScalarType& alpha, const ThisType& xx)
  {
    for (size_t ii = 0; ii < size(); ++ii)
      add_to_entry(ii, alpha * xx[ii]);
  }

  void axpy(const ScalarType& alpha, const Vector& xx)
  {
    for (size_t ii = 0; ii < size(); ++ii)
      add_to_entry(ii, alpha * xx.get_entry(ii));
  }

  inline void add_to_entry(const size_t ii, const ScalarType& value)
  {
    assert(ii < size());
    V::add_to_entry(vector_, index(ii), value);
  }

  inline void set_entry(const size_t ii, const ScalarType& value)
  {
    assert(ii < size());
    V::set_entry(vector_, index(ii), value);
  }

  inline ScalarType get_entry(const size_t ii) const
  {
    return const_vector_view_.get_entry(ii);
  }

  inline ScalarType& operator[](const size_t ii)
  {
    return vector_[index(ii)];
  }

  inline const ScalarType& operator[](const size_t ii) const
  {
    return vector_[index(ii)];
  }

  template <class T>
  ScalarType dot(const VectorInterface<T, ScalarType>& other) const
  {
    return BaseType::complex_switch::dot(*this, other);
  }

  template <class T>
  ThisType& operator+=(const VectorInterface<T, ScalarType>& other)
  {
    iadd(other);
    return *this;
  }

  template <class T>
  ThisType& operator-=(const VectorInterface<T, ScalarType>& other)
  {
    isub(other);
    return *this;
  }

  template <class T>
  void iadd(const VectorInterface<T, ScalarType>& other)
  {
    if (other.size() != size())
      DUNE_THROW(Common::Exceptions::shapes_do_not_match,
                 "The size of other (" << other.size() << ") does not match the size of this (" << size() << ")!");
    for (size_t ii = 0; ii < size(); ++ii)
      add_to_entry(ii, other.get_entry(ii));
  } // ... iadd(...)

  template <class T>
  void isub(const VectorInterface<T, ScalarType>& other)
  {
    if (other.size() != size())
      DUNE_THROW(Common::Exceptions::shapes_do_not_match,
                 "The size of other (" << other.size() << ") does not match the size of this (" << size() << ")!");
    for (size_t ii = 0; ii < size(); ++ii)
      add_to_entry(ii, -other.get_entry(ii));
  } // ... isub(...)

protected:
  inline ScalarType& get_unchecked_ref(const size_t ii)
  {
    return vector_[const_vector_view_.first_entry() + ii];
  }

  inline const ScalarType& get_unchecked_ref(const size_t ii) const
  {
    return vector_[const_vector_view_.first_entry() + ii];
  }

private:
  friend class VectorInterface<internal::VectorViewTraits<VectorImp>, ScalarType>;

  ConstVectorView<VectorImp> const_vector_view_;
  Vector& vector_;
}; // class VectorView


} // namespace LA
namespace Common {


template <class VectorImp>
struct VectorAbstraction<LA::ConstVectorView<VectorImp>>
  : public LA::internal::VectorAbstractionBase<LA::ConstVectorView<VectorImp>>
{
  static constexpr bool is_contiguous = Common::VectorAbstraction<VectorImp>::is_contiguous;
};


template <class VectorImp>
struct VectorAbstraction<LA::VectorView<VectorImp>>
  : public LA::internal::VectorAbstractionBase<LA::VectorView<VectorImp>>
{
  static constexpr bool is_contiguous = Common::VectorAbstraction<VectorImp>::is_contiguous;
};


} // namespace Common
} // namespace Dune::XT

#endif // DUNE_XT_LA_CONTAINER_VECTOR_VIEW_HH
