// This file is part of the dune-gdt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-gdt
// Copyright 2010-2021 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2017, 2020)
//   René Fritze     (2018 - 2019)
//   Tobias Leibner  (2017, 2020 - 2021)
//
// This file is part of the dune-xt project:

#ifndef DUNE_XT_FUNCTIONS_PRINT_HH
#define DUNE_XT_FUNCTIONS_PRINT_HH

#include <dune/xt/common/print.hh>

namespace Dune::XT::Functions {


// Without the NOLINTs, these declarations are removed by clang-tidy since they are unused used in some compilation
// units (in particular in the headerchecks)
// NOLINTNEXTLINE(misc-unused-using-decls)
using XT::Common::print;
// NOLINTNEXTLINE(misc-unused-using-decls)
using XT::Common::repr;


} // namespace Dune::XT::Functions

#endif // DUNE_XT_FUNCTIONS_PRINT_HH
