// This file is part of the dune-gdt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-gdt
// Copyright 2010-2021 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2017)
//   René Fritze     (2018 - 2020)
//   Tobias Leibner  (2017, 2020 - 2021)
//
// This file is part of the dune-xt project:

#ifndef DUNE_XT_COMMON_NUMERIC_CAST_HH
#define DUNE_XT_COMMON_NUMERIC_CAST_HH

#include <boost/numeric/conversion/cast.hpp>

#include "exceptions.hh"
#include "type_traits.hh"

namespace Dune::XT::Common {


/**
 * \brief Calls boost::numeric_cast and gives some information on failure.
 */
template <class T, class S>
T numeric_cast(S source)
{
  T target;
  try {
    target = boost::numeric_cast<T>(source);
  } catch (boost::bad_numeric_cast& ee) {
    DUNE_THROW(XT::Common::Exceptions::external_error,
               "  The following error occured in boost while converting '"
                   << source << "' to '" << XT::Common::Typename<T>::value() << "':\n\n   " << ee.what());
  }
  return target;
} // ... numeric_cast(...)


} // namespace Dune::XT::Common


#endif // DUNE_XT_COMMON_NUMERIC_CAST_HH
