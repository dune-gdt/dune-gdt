// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2018)
//   René Fritze     (2018)

/**
 * \file  sparsity-pattern.hh
 * \brief Tools for computing the sparsity pattern of matrices arising from discrete spaces.
 **/
#ifndef DUNE_GDT_TOOLS_SPARSITY_PATTERN_HH
#define DUNE_GDT_TOOLS_SPARSITY_PATTERN_HH

#include <dune/common/dynvector.hh>

#include <dune/grid/common/gridview.hh>
#include <dune/grid/common/rangegenerators.hh>

#include <dune/xt/la/container/pattern.hh>

#include <dune/gdt/exceptions.hh>
#include <dune/gdt/spaces/interface.hh>
#include <dune/gdt/type_traits.hh>

namespace Dune {
namespace GDT {
namespace internal {


// Inserts all (row_indices[i], col_indices[j]) pairs for i in [0, n_rows), j in [0, n_cols) into the pattern.
template <class P, class V>
void insert_index_cross_product(
    P& pattern, const size_t n_rows, const size_t n_cols, const V& row_indices, const V& col_indices)
{
  for (size_t i = 0; i < n_rows; ++i)
    for (size_t j = 0; j < n_cols; ++j)
      pattern.insert(row_indices[i], col_indices[j]);
}


// Adds the coupling entries between an element and all its (existing) neighbours to the pattern. Keeping this in a
// dedicated helper avoids deeply nested control flow (cpp:S134) and the duplication of this loop between the
// intersection-only and element-and-intersection pattern builders.
template <class P, class TestSpace, class AnsatzSpace, class GV, class Element, class V>
void insert_intersection_couplings(P& pattern,
                                   const TestSpace& test_space,
                                   const AnsatzSpace& ansatz_space,
                                   const GV& grid_view,
                                   const Element& element,
                                   const V& row_indices,
                                   V& column_indices)
{
  for (auto&& intersection : intersections(grid_view, element)) {
    if (!intersection.neighbor())
      continue;
    const auto neighbour = intersection.outside();
    ansatz_space.mapper().global_indices(neighbour, column_indices);
    insert_index_cross_product(pattern,
                               test_space.mapper().local_size(element),
                               ansatz_space.mapper().local_size(neighbour),
                               row_indices,
                               column_indices);
  }
}


} // namespace internal


/**
 *  \brief Computes an element sparsity pattern, where the test space determines the rows (outer) and the ansatz space
 *         determines the columns (inner).
 */
template <class TGV, size_t t_r, size_t t_rC, class TR, class AGV, size_t a_r, size_t a_rC, class AR, class GV>
XT::LA::SparsityPatternDefault make_element_sparsity_pattern(const SpaceInterface<TGV, t_r, t_rC, TR>& test_space,
                                                             const SpaceInterface<AGV, a_r, a_rC, AR>& ansatz_space,
                                                             const GV& grid_view)
{
  XT::LA::SparsityPatternDefault pattern(test_space.mapper().size());
  DynamicVector<size_t> row_indices(test_space.mapper().max_local_size(), 0);
  DynamicVector<size_t> column_indices(ansatz_space.mapper().max_local_size(), 0);
  for (auto&& element : elements(grid_view)) {
    test_space.mapper().global_indices(element, row_indices);
    ansatz_space.mapper().global_indices(element, column_indices);
    for (size_t ii = 0; ii < test_space.mapper().local_size(element); ++ii)
      for (size_t jj = 0; jj < ansatz_space.mapper().local_size(element); ++jj)
        pattern.insert(row_indices[ii], column_indices[jj]);
  }
  pattern.sort();
  return pattern;
} // ... make_element_sparsity_pattern(...)


template <class SGV, size_t r, size_t rC, class R, class GV>
XT::LA::SparsityPatternDefault make_element_sparsity_pattern(const SpaceInterface<SGV, r, rC, R>& space,
                                                             const GV& grid_view)
{
  return make_element_sparsity_pattern(space, space, grid_view);
}


template <class GV, size_t r, size_t rC, class R>
XT::LA::SparsityPatternDefault make_element_sparsity_pattern(const SpaceInterface<GV, r, rC, R>& space)
{
  return make_element_sparsity_pattern(space, space, space.grid_view());
}


/**
 *  \brief Computes a coupling sparsity pattern, where the test space determines the rows (outer) and the ansatz space
 *         determines the columns (inner).
 */
template <class TGV, size_t t_r, size_t t_rC, class TR, class AGV, size_t a_r, size_t a_rC, class AR, class GV>
XT::LA::SparsityPatternDefault
make_intersection_sparsity_pattern(const SpaceInterface<TGV, t_r, t_rC, TR>& test_space,
                                   const SpaceInterface<AGV, a_r, a_rC, AR>& ansatz_space,
                                   const GV& grid_view)
{
  XT::LA::SparsityPatternDefault pattern(test_space.mapper().size());
  DynamicVector<size_t> row_indices(test_space.mapper().max_local_size(), 0);
  DynamicVector<size_t> column_indices(ansatz_space.mapper().max_local_size(), 0);
  for (auto&& element : elements(grid_view)) {
    test_space.mapper().global_indices(element, row_indices);
    internal::insert_intersection_couplings(
        pattern, test_space, ansatz_space, grid_view, element, row_indices, column_indices);
  }
  pattern.sort();
  return pattern;
} // ... make_intersection_sparsity_pattern(...)


template <class SGV, size_t r, size_t rC, class R, class GV>
XT::LA::SparsityPatternDefault make_intersection_sparsity_pattern(const SpaceInterface<SGV, r, rC, R>& space,
                                                                  const GV& grid_view)
{
  return make_intersection_sparsity_pattern(space, space, grid_view);
}


template <class GV, size_t r, size_t rC, class R>
XT::LA::SparsityPatternDefault make_intersection_sparsity_pattern(const SpaceInterface<GV, r, rC, R>& space)
{
  return make_intersection_sparsity_pattern(space, space, space.grid_view());
}


/**
 *  \brief Computes an element and coupling sparsity pattern, where the test space determines the rows (outer) and the
 *         ansatz space determines the columns (inner).
 */
template <class TGV, size_t t_r, size_t t_rC, class TR, class AGV, size_t a_r, size_t a_rC, class AR, class GV>
XT::LA::SparsityPatternDefault
make_element_and_intersection_sparsity_pattern(const SpaceInterface<TGV, t_r, t_rC, TR>& test_space,
                                               const SpaceInterface<AGV, a_r, a_rC, AR>& ansatz_space,
                                               const GV& grid_view)
{
  XT::LA::SparsityPatternDefault pattern(test_space.mapper().size());
  DynamicVector<size_t> row_indices(test_space.mapper().max_local_size(), 0);
  DynamicVector<size_t> column_indices(ansatz_space.mapper().max_local_size(), 0);
  for (auto&& element : elements(grid_view)) {
    test_space.mapper().global_indices(element, row_indices);
    ansatz_space.mapper().global_indices(element, column_indices);
    for (size_t ii = 0; ii < test_space.mapper().local_size(element); ++ii)
      for (size_t jj = 0; jj < ansatz_space.mapper().local_size(element); ++jj)
        pattern.insert(row_indices[ii], column_indices[jj]);
    internal::insert_intersection_couplings(
        pattern, test_space, ansatz_space, grid_view, element, row_indices, column_indices);
  }
  pattern.sort();
  return pattern;
} // ... make_element_and_intersection_sparsity_pattern(...)


template <class SGV, size_t r, size_t rC, class R, class GV>
XT::LA::SparsityPatternDefault
make_element_and_intersection_sparsity_pattern(const SpaceInterface<SGV, r, rC, R>& space, const GV& grid_view)
{
  return make_element_and_intersection_sparsity_pattern(space, space, grid_view);
}


template <class GV, size_t r, size_t rC, class R>
XT::LA::SparsityPatternDefault make_element_and_intersection_sparsity_pattern(const SpaceInterface<GV, r, rC, R>& space)
{
  return make_element_and_intersection_sparsity_pattern(space, space, space.grid_view());
}


template <class TGV, size_t t_r, size_t t_rC, class TR, class AGV, size_t a_r, size_t a_rC, class AR, class GV>
XT::LA::SparsityPatternDefault make_sparsity_pattern(const SpaceInterface<TGV, t_r, t_rC, TR>& test_space,
                                                     const SpaceInterface<AGV, a_r, a_rC, AR>& ansatz_space,
                                                     const GV& grid_view,
                                                     const Stencil stencil = Stencil::automatic)
{
  if (stencil == Stencil::element)
    return make_element_sparsity_pattern(test_space, ansatz_space, grid_view);
  else if (stencil == Stencil::intersection)
    return make_intersection_sparsity_pattern(test_space, ansatz_space, grid_view);
  else if (stencil == Stencil::element_and_intersection)
    return make_element_and_intersection_sparsity_pattern(test_space, ansatz_space, grid_view);
  else if (stencil == Stencil::automatic) {
    if (!(test_space.continuous(0) || ansatz_space.continuous(0) || test_space.continuous_normal_components()
          || ansatz_space.continuous_normal_components()))
      return make_element_and_intersection_sparsity_pattern(test_space, ansatz_space, grid_view);
    else
      return make_element_sparsity_pattern(test_space, ansatz_space, grid_view);
  } else
    DUNE_THROW(XT::Common::Exceptions::wrong_input_given,
               "Unknown Stencil encountered (see below), add an appropriate method!" << "\n   stencil = " << stencil);
} // ... make_sparsity_pattern(...)


template <class SGV, size_t r, size_t rC, class R, class GV>
XT::LA::SparsityPatternDefault make_sparsity_pattern(const SpaceInterface<SGV, r, rC, R>& space,
                                                     const GV& grid_view,
                                                     const Stencil stencil = Stencil::automatic)
{
  return make_sparsity_pattern(space, space, grid_view, stencil);
}


template <class GV, size_t r, size_t rC, class R>
XT::LA::SparsityPatternDefault make_sparsity_pattern(const SpaceInterface<GV, r, rC, R>& space,
                                                     const Stencil stencil = Stencil::automatic)
{
  return make_sparsity_pattern(space, space.grid_view(), stencil);
}


/**
 *  \brief TODO
 */
template <class TGV, size_t t_r, size_t t_rC, class TR, class AGV, size_t a_r, size_t a_rC, class AR, class GV>
XT::LA::SparsityPatternDefault make_coupling_sparsity_pattern(const SpaceInterface<TGV, t_r, t_rC, TR>& test_space,
                                                              const SpaceInterface<AGV, a_r, a_rC, AR>& ansatz_space,
                                                              const GV& grid_view)
{
  XT::LA::SparsityPatternDefault pattern(test_space.mapper().size());
  DynamicVector<size_t> global_indices_in(test_space.mapper().max_local_size());
  DynamicVector<size_t> global_indices_out(test_space.mapper().max_local_size());
  for (auto&& inside : elements(grid_view)) {
    for (auto&& intersection : intersections(grid_view, inside)) {
      const auto outside = intersection.outside();
      test_space.mapper().global_indices(inside, global_indices_in);
      ansatz_space.mapper().global_indices(outside, global_indices_out);
      const auto inside_size = test_space.mapper().local_size(inside);
      const auto outside_size = ansatz_space.mapper().local_size(outside);
      internal::insert_index_cross_product(pattern, inside_size, inside_size, global_indices_in, global_indices_in);
      internal::insert_index_cross_product(pattern, inside_size, outside_size, global_indices_in, global_indices_out);
      internal::insert_index_cross_product(pattern, inside_size, outside_size, global_indices_out, global_indices_in);
      internal::insert_index_cross_product(pattern, outside_size, outside_size, global_indices_out, global_indices_out);
    }
  }
  pattern.sort();
  return pattern;
} // ... make_coupling_sparsity_pattern(...)


template <class SGV, size_t r, size_t rC, class R, class GV>
XT::LA::SparsityPatternDefault make_coupling_sparsity_pattern(const SpaceInterface<SGV, r, rC, R>& space,
                                                              const GV& grid_view)
{
  return make_coupling_sparsity_pattern(space, space, grid_view);
}

} // namespace GDT
} // namespace Dune

#endif // DUNE_GDT_TOOLS_SPARSITY_PATTERN_HH
