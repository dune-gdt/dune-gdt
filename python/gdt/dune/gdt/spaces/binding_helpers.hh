// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2020)

#ifndef PYTHON_DUNE_GDT_SPACES_BINDING_HELPERS_HH
#define PYTHON_DUNE_GDT_SPACES_BINDING_HELPERS_HH

#include <sstream>
#include <string>
#include <type_traits>

#include <pybind11/pybind11.h>

#include <dune/xt/common/string.hh>
#include <dune/xt/common/type_traits.hh>

namespace Dune {
namespace GDT {
namespace bindings {


/**
 * \brief The name under which a space binding registers its python class, e.g.
 *        "ContinuousLagrangeSpace2dCubeYaspgridTo2d".
 *
 * Every space binding derives its class name the same way - class_id, grid, range dimension, field - and every one of
 * them used to spell that derivation out again. Keeping it here makes the naming convention a single decision rather
 * than one repeated per space.
 *
 * \param always_append_range_dim  RaviartThomasSpace is vector valued by construction (r == d) and tags even its 1d
 *                                 binding "..._to_1d"; the others only tag r > 1.
 */
template <size_t r, class R>
std::string
space_class_name(const std::string& class_id, const std::string& grid_id, const bool always_append_range_dim = false)
{
  std::string class_name = class_id + "_" + grid_id;
  if (always_append_range_dim || r > 1)
    class_name += "_to_" + XT::Common::to_string(r) + "d";
  if (!std::is_same<R, double>::value)
    class_name += "_" + XT::Common::Typename<R>::value(/*fail_wo_typeid=*/true);
  return XT::Common::to_camel_case(class_name);
} // ... space_class_name(...)


/// \brief The name of the module-level factory a space binding adds next to its class, e.g. "ContinuousLagrangeSpace".
template <class R>
std::string space_factory_name(const std::string& class_id)
{
  std::string space_type_name = class_id;
  if (!std::is_same<R, double>::value)
    space_type_name += "_" + XT::Common::Typename<R>::value(/*fail_wo_typeid=*/true);
  return XT::Common::to_camel_case(space_type_name);
}


/// \brief Adds the __repr__ that every space binding shares (the space's own operator<<).
template <class BoundType>
void add_space_repr(BoundType& c)
{
  c.def("__repr__", [](const typename BoundType::type& self) {
    std::stringstream ss;
    ss << self;
    return ss.str();
  });
}


} // namespace bindings
} // namespace GDT
} // namespace Dune

#endif // PYTHON_DUNE_GDT_SPACES_BINDING_HELPERS_HH
