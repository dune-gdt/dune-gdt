// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2020)

#include "config.h"

#include <dune/xt/grid/grids.hh>
#include <python/xt/dune/xt/grid/grids.bindings.hh>

#include "interfaces.hh"

PYBIND11_MODULE(_functionals_interfaces_istl_3d, m)
{
  DUNE_GDT_BIND_FUNCTIONAL_INTERFACES_ISTL_MODULE(3);
}
