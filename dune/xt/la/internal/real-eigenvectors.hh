// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2017 - 2019)
//   René Fritze     (2018 - 2020)
//   Tobias Leibner  (2017 - 2020)

/// \file
/// \brief Shared helpers to recover real eigenvectors from a complex eigendecomposition.
///
/// Used by both dune/xt/la/eigen-solver/internal/base.hh and
/// dune/xt/la/generalized-eigen-solver/internal/base.hh, which otherwise end up with byte-identical
/// copies of this logic.

#ifndef DUNE_XT_LA_INTERNAL_REAL_EIGENVECTORS_HH
#define DUNE_XT_LA_INTERNAL_REAL_EIGENVECTORS_HH

#include <functional>
#include <set>
#include <utility>
#include <vector>

#include <dune/xt/common/float_cmp.hh>
#include <dune/xt/common/matrix.hh>
#include <dune/xt/common/numeric.hh>

#include <dune/xt/la/container/common/vector/dense.hh>

namespace Dune::XT::LA::internal {


/**
 * \brief Groups the indices of equal real eigenvalues together.
 *
 * \sa process_eigenvalue_group()
 */
template <class RealType>
std::pair<std::vector<std::vector<size_t>>, std::vector<size_t>>
compute_eigenvalue_groups(const std::vector<RealType>& real_eigenvalues, const size_t rows)
{
  struct Cmp
  {
    bool operator()(const RealType& a, const RealType& b) const
    {
      return XT::Common::FloatCmp::lt(a, b);
    }
  };
  std::vector<std::vector<size_t>> eigenvalue_groups;
  std::vector<size_t> eigenvalue_multiplicity;
  std::set<RealType, Cmp> eigenvalues_done;
  for (size_t jj = 0; jj < rows; ++jj) {
    const auto curr_eigenvalue = real_eigenvalues[jj];
    if (eigenvalues_done.count(curr_eigenvalue))
      continue;
    std::vector<size_t> curr_group;
    curr_group.push_back(jj);
    eigenvalue_multiplicity.push_back(1);
    for (size_t kk = jj + 1; kk < rows; ++kk) {
      if (XT::Common::FloatCmp::eq(curr_eigenvalue, real_eigenvalues[kk])) {
        curr_group.push_back(kk);
        ++(eigenvalue_multiplicity.back());
      }
    } // kk
    eigenvalue_groups.push_back(curr_group);
    eigenvalues_done.insert(curr_eigenvalue);
  } // jj
  return {eigenvalue_groups, eigenvalue_multiplicity};
} // ... compute_eigenvalue_groups(...)


/**
 * \brief For a single eigenvalue, calculates an orthogonal basis of the real eigenspace from the real and
 *        imaginary parts of the complex eigenvectors, and writes the result into real_eigenvectors.
 *
 * \param raise_not_real Called (and expected to throw, via DUNE_THROW) if the eigenvectors turn out not to be
 *                        recoverable as real. Left to the caller (instead of taking an exception type and building
 *                        the message here) so that DUNE_THROW's "#E" stringification of the exception type and the
 *                        exact (implementation-specific) message text are preserved unchanged at each call site.
 */
template <class RealType, class RealMatrixType, class ComplexMatrixType>
void process_eigenvalue_group(const ComplexMatrixType& eigenvectors,
                              RealMatrixType& real_eigenvectors,
                              const std::function<void()>& raise_not_real,
                              const std::vector<size_t>& group,
                              const size_t multiplicity,
                              const size_t rows,
                              const size_t cols)
{
  using RM = XT::Common::MatrixAbstraction<RealMatrixType>;
  using CM = XT::Common::MatrixAbstraction<ComplexMatrixType>;
  using RealVectorType = typename XT::LA::CommonDenseVector<RealType>;
  std::vector<RealVectorType> input_vectors(2 * multiplicity, RealVectorType(rows, 0.));
  size_t index = 0;
  for (const auto& jj : group) {
    for (size_t ll = 0; ll < cols; ++ll) {
      input_vectors[index][ll] = CM::get_entry(eigenvectors, ll, jj).real();
      input_vectors[index + 1][ll] = CM::get_entry(eigenvectors, ll, jj).imag();
    }
    index += 2;
  } // jj

  // orthonormalize
  for (size_t ii = 0; ii < input_vectors.size(); ++ii) {
    auto& v_i = input_vectors[ii];
    for (size_t jj = 0; jj < ii; ++jj) {
      const auto& v_j = input_vectors[jj];
      const auto vj_vj = v_j.dot(v_j);
      if (XT::Common::FloatCmp::eq(vj_vj, 0.))
        continue;
      const auto vj_vi = v_j.dot(v_i);
      for (size_t rr = 0; rr < rows; ++rr)
        v_i[rr] -= vj_vi / vj_vj * v_j[rr];
    } // jj
    RealType l2_norm = std::sqrt(
        Common::reduce(v_i.begin(), v_i.end(), 0., [](const RealType& a, const RealType& b) { return a + b * b; }));
    if (XT::Common::FloatCmp::ne(l2_norm, 0.))
      v_i *= 1. / l2_norm;
  } // ii

  // copy eigenvectors back to eigenvectors matrix
  index = 0;
  for (size_t ii = 0; ii < input_vectors.size(); ++ii) {
    if (XT::Common::FloatCmp::eq(input_vectors[ii], RealVectorType(rows, 0.)))
      continue;
    if (index >= multiplicity)
      raise_not_real();
    for (size_t rr = 0; rr < rows; ++rr)
      RM::set_entry(real_eigenvectors, rr, group[index], input_vectors[ii].get_entry(rr));
    index++;
  } // ii
  if (index < multiplicity)
    raise_not_real();
} // ... process_eigenvalue_group(...)


} // namespace Dune::XT::LA::internal

#endif // DUNE_XT_LA_INTERNAL_REAL_EIGENVECTORS_HH
