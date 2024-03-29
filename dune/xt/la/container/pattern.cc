// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2013 - 2017)
//   René Fritze     (2015 - 2016, 2018 - 2019)
//   Tobias Leibner  (2017 - 2020)

#include <cassert>
#include <algorithm>

#include "config.h"
#include "pattern.hh"

namespace Dune::XT::LA {


// ================================
// ==== SparsityPatternDefault ====
// ================================
SparsityPatternDefault::SparsityPatternDefault(const size_t _size)
  : vector_of_vectors_(_size)
{
}

size_t SparsityPatternDefault::size() const
{
  return vector_of_vectors_.size();
}

typename SparsityPatternDefault::InnerType& SparsityPatternDefault::inner(const size_t ii)
{
  assert(ii < size() && "Wrong index requested!");
  return vector_of_vectors_[ii];
}

const typename SparsityPatternDefault::InnerType& SparsityPatternDefault::inner(const size_t ii) const
{
  assert(ii < size() && "Wrong index requested!");
  return vector_of_vectors_[ii];
}

typename SparsityPatternDefault::OuterIteratorType SparsityPatternDefault::begin()
{
  return vector_of_vectors_.begin();
}

typename SparsityPatternDefault::ConstOuterIteratorType SparsityPatternDefault::begin() const
{
  return vector_of_vectors_.begin();
}

typename SparsityPatternDefault::OuterIteratorType SparsityPatternDefault::end()
{
  return vector_of_vectors_.end();
}

typename SparsityPatternDefault::ConstOuterIteratorType SparsityPatternDefault::end() const
{
  return vector_of_vectors_.end();
}


bool SparsityPatternDefault::operator==(const SparsityPatternDefault& other) const
{
  return vector_of_vectors_ == other.vector_of_vectors_;
}

bool SparsityPatternDefault::operator!=(const SparsityPatternDefault& other) const
{
  return vector_of_vectors_ != other.vector_of_vectors_;
}

SparsityPatternDefault SparsityPatternDefault::operator+(const SparsityPatternDefault& other) const
{
  SparsityPatternDefault ret(std::max(this->size(), other.size()));
  for (size_t rr = 0; rr < this->size(); ++rr)
    for (const auto& cc : this->inner(rr))
      ret.insert(rr, cc);
  for (size_t rr = 0; rr < other.size(); ++rr)
    for (const auto& cc : other.inner(rr))
      if (!ret.contains(rr, cc))
        ret.insert(rr, cc);
  ret.sort();
  return ret;
}

void SparsityPatternDefault::insert(const size_t outer_index, const size_t inner_index)
{
  assert(outer_index < size() && "Wrong index requested!");
  if (std::find(vector_of_vectors_[outer_index].begin(), vector_of_vectors_[outer_index].end(), inner_index)
      == vector_of_vectors_[outer_index].end())
    vector_of_vectors_[outer_index].push_back(inner_index);
} // ... insert(...)

void SparsityPatternDefault::sort(const size_t outer_index)
{
  assert(outer_index < size() && "Wrong index requested!");
  std::sort(vector_of_vectors_[outer_index].begin(), vector_of_vectors_[outer_index].end());
}

void SparsityPatternDefault::sort()
{
  for (auto& inner_vector : vector_of_vectors_)
    std::sort(inner_vector.begin(), inner_vector.end());
}

bool SparsityPatternDefault::contains(const size_t outer_index, const size_t inner_index) const
{
  const auto& row = inner(outer_index);
  return std::find(row.begin(), row.end(), inner_index) != row.end();
}

bool SparsityPatternDefault::contains(const SparsityPatternDefault& other) const
{
  for (size_t rr = 0; rr < size(); ++rr)
    for (auto&& cc : other.inner(rr))
      if (!this->contains(rr, cc))
        return false;
  return true;
}

SparsityPatternDefault SparsityPatternDefault::transposed(const size_t cols) const
{
  SparsityPatternDefault transposed_pattern(cols);
  for (size_t rr = 0; rr < size(); ++rr)
    for (const auto& cc : inner(rr))
      transposed_pattern.insert(cc, rr);
  transposed_pattern.sort();
  return transposed_pattern;
}

SparsityPatternDefault dense_pattern(const size_t rows, const size_t cols)
{
  SparsityPatternDefault ret(rows);
  std::vector<size_t> columns(cols);
  for (size_t ii = 0; ii < cols; ++ii)
    columns[ii] = ii;
  for (auto& inner : ret)
    inner = columns;
  return ret;
}

SparsityPatternDefault tridiagonal_pattern(const size_t rows, const size_t cols)
{
  SparsityPatternDefault ret(rows);
  for (size_t ii = 0; ii < rows; ++ii) {
    // We have to be careful here if ii == 0 because then ii - 1 wraps around
    const size_t col_start = (ii == 0) ? size_t(0) : ii - 1;
    for (size_t jj = col_start; jj < std::min(ii + 2, cols); ++jj)
      ret.insert(ii, jj);
  }
  return ret;
}

SparsityPatternDefault triangular_pattern(const size_t rows, const size_t cols, const Common::MatrixPattern& uplo)
{
  SparsityPatternDefault ret(rows);
  for (size_t ii = 0; ii < rows; ++ii) {
    for (size_t jj = 0; jj < cols; ++jj) {
      if ((uplo == Common::MatrixPattern::lower_triangular && ii >= jj)
          || (uplo == Common::MatrixPattern::upper_triangular && ii <= jj))
        ret.insert(ii, jj);
    }
  }
  return ret;
}

SparsityPatternDefault diagonal_pattern(const size_t rows)
{
  return diagonal_pattern(rows, rows, 0);
}

SparsityPatternDefault diagonal_pattern(const size_t rows, const size_t cols, const int offset)
{
  SparsityPatternDefault pattern(rows);
  for (int ii = std::max(0, -offset); ii < std::min(int(rows), int(cols) - offset); ++ii)
    pattern.insert(ii, ii + offset);
  return pattern;
}

SparsityPatternDefault multiplication_pattern(const SparsityPatternDefault& lhs_pattern,
                                              const SparsityPatternDefault& rhs_pattern,
                                              const size_t rhs_cols)
{
  const size_t lhs_rows = lhs_pattern.size();
  SparsityPatternDefault pattern(lhs_rows);
  for (size_t ii = 0; ii < lhs_rows; ++ii) // rows of new pattern
    for (size_t jj = 0; jj < rhs_cols; ++jj) // cols of new pattern
      for (const auto& index : lhs_pattern.inner(ii)) { // entries in lhs_pattern in current row
        if (std::find(rhs_pattern.inner(index).begin(), rhs_pattern.inner(index).end(), jj)
            != rhs_pattern.inner(index).end()) {
          pattern.insert(ii, jj);
          break;
        }
      }
  return pattern;
}


} // namespace Dune::XT::LA
