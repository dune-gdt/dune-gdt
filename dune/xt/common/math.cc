// This file is part of the dune-xt-common project:
//   https://github.com/dune-community/dune-xt-common
// Copyright 2009-2017 dune-xt-common developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2013 - 2014, 2016 - 2017)
//   Rene Milk       (2013, 2015 - 2016)

#include "config.h"

#include "math.hh"

namespace Dune {
namespace XT {
namespace Common {


// this is just for the test to compile
const std::string Epsilon<std::string, false>::value = "_";


} // namespace Common
} // namespace XT
} // namespace Dune

namespace std {


long unsigned int abs(const long unsigned int& value)
{
  return value;
}


} // namespace std
