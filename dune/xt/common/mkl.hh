// This file is part of the dune-gdt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-gdt
// Copyright 2010-2021 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2013 - 2014, 2016 - 2019)
//   René Fritze     (2013, 2015 - 2016, 2018 - 2020)
//   Tobias Leibner  (2018 - 2021)
//
// This file is part of the dune-xt project:

#ifndef DUNE_XT_COMMON_MKL_HH
#define DUNE_XT_COMMON_MKL_HH

namespace Dune::XT::Common::Mkl {


/**
 * \brief If true, calling any of the other methods makes sense.
 */
bool available();


/**
 * \brief Wrapper around MKL's vdExp
 * \sa    vdExp
 */
void exp(const int n, const double* a, double* y);


} // namespace Dune::XT::Common::Mkl

#endif // DUNE_XT_COMMON_MKL_HH
