// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)

#ifndef PYTHON_XT_DUNE_XT_FUNCTIONS_CLASS_NAME_HH
#define PYTHON_XT_DUNE_XT_FUNCTIONS_CLASS_NAME_HH

#include <string>
#include <type_traits>

#include <dune/xt/common/string.hh>
#include <dune/xt/common/type_traits.hh>

namespace Dune::XT::Functions::bindings {


// The "<class_id>_<d>_to_<r>[x<rC>]d" naming convention shared by every (d, r, rC)-templated
// FunctionInterface binding in this directory (ConstantFunction, ExpressionFunction, ...).
inline std::string range_class_name(const std::string& class_id, size_t d, size_t r, size_t rC)
{
  std::string class_name = class_id + "_" + Common::to_string(d) + "_to_" + Common::to_string(r);
  if (rC > 1)
    class_name += "x" + Common::to_string(rC);
  class_name += "d";
  return Common::to_camel_case(class_name);
}


// The "<class_id>_<grid_id>[_<layer_id>]_to_<r>[x<rC>]d[_<R>]" naming convention shared by every
// grid-entity-templated GridFunctionInterface binding in this directory (CheckerboardFunction,
// GridFunction, ...).
template <class R>
std::string grid_range_class_name(
    const std::string& class_id, const std::string& grid_id, const std::string& layer_id, size_t r, size_t rC)
{
  std::string class_name = class_id;
  class_name += "_" + grid_id;
  if (!layer_id.empty())
    class_name += "_" + layer_id;
  class_name += "_to_" + Common::to_string(r);
  if (rC > 1)
    class_name += "x" + Common::to_string(rC);
  class_name += "d";
  if (!std::is_same_v<R, double>)
    class_name += "_" + Common::Typename<R>::value(/*fail_wo_typeid=*/true);
  return Common::to_camel_case(class_name);
}


} // namespace Dune::XT::Functions::bindings

#endif // PYTHON_XT_DUNE_XT_FUNCTIONS_CLASS_NAME_HH
