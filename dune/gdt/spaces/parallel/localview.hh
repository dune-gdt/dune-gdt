// This file is part of the dune-gdt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-gdt
// Copyright 2010-2021 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2018)
//   René Fritze     (2017 - 2018)
//   Tobias Leibner  (2019)

#ifndef DUNE_GDT_SPACES_PARALLEL_LOCALVIEW_HH
#define DUNE_GDT_SPACES_PARALLEL_LOCALVIEW_HH

#include <boost/container/vector.hpp>

#include <dune/xt/common/ranges.hh>
#include <dune/xt/grid/entity.hh>
#include <dune/xt/grid/type_traits.hh>
#include <dune/xt/la/container/vector-interface.hh>


namespace Dune {
namespace GDT {


template <class VectorType, class ScalarType, class SpaceType, class Descriptor>
class LocalView
{
  static_assert(XT::LA::is_vector<VectorType>::value, "");
  using EntityType = XT::Grid::extract_entity_t<typename SpaceType::GridViewType>;

private:
  void resize(size_t size)
  {
    global_indices_.resize(size);
    value_cache_.resize(size);
  }

public:
  using value_type = ScalarType;

  LocalView(VectorType& vector, const SpaceType& space, const Descriptor& descriptor)
    : vector_(vector)
    , space_(space)
    , global_indices_(0)
    , value_cache_(0)
    , descriptor_(descriptor)
  {}

  void bind(const EntityType& entity)
  {
    const size_t size{descriptor_.size(space_, entity)};
    resize(size);
    space_.mapper().global_indices(entity, global_indices_);
    for (auto i : XT::Common::value_range(size)) {
      assert(i < global_indices_.size());
      const auto global = global_indices_[i];
      assert(global < vector_.size());
      value_cache_[i] = vector_[global];
    }
  }

  //! this needs to exist for communication to allow gather/scatter to define for codim non-zero. even if never called
  template <class OtherEntities>
  void bind(const OtherEntities&)
  {
    DUNE_THROW(NotImplemented, "");
    resize(0);
  }

  void commit()
  {
    assert(value_cache_.size() <= global_indices_.size());
    for (auto i : XT::Common::value_range(value_cache_.size())) {
      const auto global = global_indices_[i];
      vector_[global] = value_cache_[i];
    }
  }

  ScalarType& operator[](const size_t ii)
  {
    return value_cache_[ii];
  }

  const ScalarType& operator[](const size_t ii) const
  {
    return value_cache_[ii];
  }

  size_t size() const
  {
    assert(value_cache_.size() <= global_indices_.size());
    return value_cache_.size();
  }

private:
  VectorType& vector_;
  const SpaceType& space_;
  Dune::DynamicVector<size_t> global_indices_;
  //! must use something that doesn't have specialized data storage for bool
  boost::container::vector<ScalarType> value_cache_;
  const Descriptor& descriptor_;
};


} // namespace GDT
} // namespace Dune

#endif // DUNE_GDT_SPACES_PARALLEL_LOCALVIEW_HH
