// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2017)
//   René Fritze     (2018 - 2019)
//   Tobias Leibner  (2018, 2020)

/// \file
/// \brief Factory functions to create or fill identity matrices for arbitrary matrix types.

#ifndef DUNE_XT_LA_CONTAINER_EYE_MATRIX_HH
#define DUNE_XT_LA_CONTAINER_EYE_MATRIX_HH

#include <dune/xt/common/matrix.hh>

#include <dune/xt/la/type_traits.hh>

#include "pattern.hh"

namespace Dune::XT::LA {
namespace internal {


template <class MatrixType>
typename std::enable_if<Common::is_matrix<MatrixType>::value, void>::type set_diagonal_to_one(MatrixType& mat)
{
  using M = Common::MatrixAbstraction<MatrixType>;
  for (size_t ii = 0; ii < std::min(M::rows(mat), M::cols(mat)); ++ii)
    M::set_entry(mat, ii, ii, 1.);
}


} // namespace internal


/// \brief Creates a rows x cols identity matrix of the given LA matrix type (optionally with a sparsity pattern).
template <class MatrixType>
typename std::enable_if<is_matrix<MatrixType>::value, MatrixType>::type
eye_matrix(const size_t rows, const size_t cols, const SparsityPatternDefault& pattern = SparsityPatternDefault())
{
  MatrixType mat = MatrixType(rows, cols, pattern.size() == 0 ? diagonal_pattern(rows, cols) : pattern);
  internal::set_diagonal_to_one(mat);
  return mat;
}

/// \brief Overwrites the given matrix with the identity (zeros off-diagonal, ones on the diagonal).
template <class MatrixType>
typename std::enable_if<Common::is_matrix<MatrixType>::value, void>::type eye_matrix(MatrixType& matrix)
{
  matrix *= 0.;
  internal::set_diagonal_to_one(matrix);
}

/// \brief Creates a rows x cols identity matrix for a generic (non-LA) matrix type.
template <class MatrixType>
typename std::enable_if<Common::is_matrix<MatrixType>::value && !is_matrix<MatrixType>::value, MatrixType>::type
eye_matrix(const size_t rows, const size_t cols, const SparsityPatternDefault& /*pattern*/ = SparsityPatternDefault())
{
  using M = Common::MatrixAbstraction<MatrixType>;
  auto mat = M::create(rows, cols, typename M::ScalarType(0.));
  for (size_t ii = 0; ii < std::min(rows, cols); ++ii)
    M::set_entry(mat, ii, ii, 1);
  return mat;
}


/// \brief Creates a square size x size identity matrix of the given matrix type.
template <class MatrixType>
typename std::enable_if<Common::is_matrix<MatrixType>::value, MatrixType>::type
eye_matrix(const size_t size, const SparsityPatternDefault& pattern = SparsityPatternDefault())
{
  return eye_matrix<MatrixType>(size, size, pattern);
}


} // namespace Dune::XT::LA


#endif // DUNE_XT_LA_CONTAINER_EYE_MATRIX_HH
