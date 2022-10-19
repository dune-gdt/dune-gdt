// This file is part of the dune-gdt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-gdt
// Copyright 2010-2021 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2017)
//   René Fritze     (2018 - 2019)
//   Tobias Leibner  (2017, 2020 - 2021)
//
// This file is part of the dune-xt project:

#include <config.h>

#include "istl.hh"

template class Dune::XT::LA::IstlDenseVector<double>;
template class Dune::XT::LA::IstlRowMajorSparseMatrix<double>;
// template std::ostream& operator<<(std::ostream&, const Dune::XT::LA::IstlRowMajorSparseMatrix<double>&);
