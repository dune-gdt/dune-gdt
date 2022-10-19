// This file is part of the dune-gdt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-gdt
// Copyright 2010-2021 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2013 - 2014, 2016 - 2018)
//   René Fritze     (2013, 2015 - 2016, 2018 - 2019)
//   Tobias Leibner  (2018 - 2021)
//
// This file is part of the dune-xt project:

#include "config.h"

#include "fix-ambiguous-std-math-overloads.hh"

namespace std {


long unsigned int abs(const long unsigned int& value)
{
  return value;
}


unsigned char abs(unsigned char value)
{
  return value;
}


} // namespace std
