// This file is part of the dune-gdt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-gdt
// Copyright 2010-2021 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2020)
//   Tobias Leibner  (2021)
//
// This file is part of the dune-xt project:

#include <dune/xt/test/main.hxx> // <- has to come first, include config.h!
#include <dune/xt/functions/constant.hh>


GTEST_TEST(function_interface_operators, main)
{
  using namespace Dune::XT;

  Functions::ConstantFunction<1, 1, 1>({}) - Functions::ConstantFunction<1, 1, 1>({});
  Functions::ConstantFunction<2, 1, 1>({}) - Functions::ConstantFunction<2, 1, 1>({});
  Functions::ConstantFunction<2, 1, 2>({}) - Functions::ConstantFunction<2, 1, 2>({});
  Functions::ConstantFunction<2, 1, 3>({}) - Functions::ConstantFunction<2, 1, 3>({});
  Functions::ConstantFunction<2, 2, 1>({}) - Functions::ConstantFunction<2, 2, 1>({});
  Functions::ConstantFunction<2, 2, 2>({}) - Functions::ConstantFunction<2, 2, 2>({});
  Functions::ConstantFunction<2, 2, 3>({}) - Functions::ConstantFunction<2, 2, 3>({});
  Functions::ConstantFunction<2, 3, 1>({}) - Functions::ConstantFunction<2, 3, 1>({});
  Functions::ConstantFunction<2, 3, 2>({}) - Functions::ConstantFunction<2, 3, 2>({});
  Functions::ConstantFunction<2, 3, 3>({}) - Functions::ConstantFunction<2, 3, 3>({});

  Functions::ConstantFunction<1, 1, 1>({}) + Functions::ConstantFunction<1, 1, 1>({});
  Functions::ConstantFunction<2, 1, 1>({}) + Functions::ConstantFunction<2, 1, 1>({});
  Functions::ConstantFunction<2, 1, 2>({}) + Functions::ConstantFunction<2, 1, 2>({});
  Functions::ConstantFunction<2, 1, 3>({}) + Functions::ConstantFunction<2, 1, 3>({});
  Functions::ConstantFunction<2, 2, 1>({}) + Functions::ConstantFunction<2, 2, 1>({});
  Functions::ConstantFunction<2, 2, 2>({}) + Functions::ConstantFunction<2, 2, 2>({});
  Functions::ConstantFunction<2, 2, 3>({}) + Functions::ConstantFunction<2, 2, 3>({});
  Functions::ConstantFunction<2, 3, 1>({}) + Functions::ConstantFunction<2, 3, 1>({});
  Functions::ConstantFunction<2, 3, 2>({}) + Functions::ConstantFunction<2, 3, 2>({});
  Functions::ConstantFunction<2, 3, 3>({}) + Functions::ConstantFunction<2, 3, 3>({});

  Functions::ConstantFunction<1, 1, 1>({}) / Functions::ConstantFunction<1, 1, 1>({});
  Functions::ConstantFunction<2, 1, 1>({}) / Functions::ConstantFunction<2, 1, 1>({});
  Functions::ConstantFunction<2, 1, 2>({}) / Functions::ConstantFunction<2, 1, 1>({});
  Functions::ConstantFunction<2, 1, 3>({}) / Functions::ConstantFunction<2, 1, 1>({});
  Functions::ConstantFunction<2, 2, 1>({}) / Functions::ConstantFunction<2, 1, 1>({});
  Functions::ConstantFunction<2, 2, 2>({}) / Functions::ConstantFunction<2, 1, 1>({});
  Functions::ConstantFunction<2, 2, 3>({}) / Functions::ConstantFunction<2, 1, 1>({});
  Functions::ConstantFunction<2, 3, 1>({}) / Functions::ConstantFunction<2, 1, 1>({});
  Functions::ConstantFunction<2, 3, 2>({}) / Functions::ConstantFunction<2, 1, 1>({});
  Functions::ConstantFunction<2, 3, 3>({}) / Functions::ConstantFunction<2, 1, 1>({});

  Functions::ConstantFunction<1, 1, 1>({}) * Functions::ConstantFunction<1, 1, 1>({});
  Functions::ConstantFunction<2, 1, 1>({}) * Functions::ConstantFunction<2, 1, 1>({});
  Functions::ConstantFunction<2, 1, 2>({}) * Functions::ConstantFunction<2, 1, 1>({});
  Functions::ConstantFunction<2, 1, 3>({}) * Functions::ConstantFunction<2, 1, 1>({});
  Functions::ConstantFunction<2, 2, 1>({}) * Functions::ConstantFunction<2, 1, 1>({});
  Functions::ConstantFunction<2, 2, 2>({}) * Functions::ConstantFunction<2, 1, 1>({});
  Functions::ConstantFunction<2, 2, 3>({}) * Functions::ConstantFunction<2, 1, 1>({});
  Functions::ConstantFunction<2, 3, 1>({}) * Functions::ConstantFunction<2, 1, 1>({});
  Functions::ConstantFunction<2, 3, 2>({}) * Functions::ConstantFunction<2, 1, 1>({});
  Functions::ConstantFunction<2, 3, 3>({}) * Functions::ConstantFunction<2, 1, 1>({});

  Functions::ConstantFunction<2, 1, 2>({}) * Functions::ConstantFunction<2, 2, 1>({});
  Functions::ConstantFunction<2, 1, 2>({}) * Functions::ConstantFunction<2, 2, 2>({});
  Functions::ConstantFunction<2, 1, 2>({}) * Functions::ConstantFunction<2, 2, 3>({});

  Functions::ConstantFunction<2, 1, 3>({}) * Functions::ConstantFunction<2, 3, 1>({});
  Functions::ConstantFunction<2, 1, 3>({}) * Functions::ConstantFunction<2, 3, 2>({});
  Functions::ConstantFunction<2, 1, 3>({}) * Functions::ConstantFunction<2, 3, 3>({});

  Functions::ConstantFunction<2, 2, 1>({}) * Functions::ConstantFunction<2, 1, 2>({});
  Functions::ConstantFunction<2, 2, 1>({}) * Functions::ConstantFunction<2, 1, 3>({});
  Functions::ConstantFunction<2, 2, 1>({}) * Functions::ConstantFunction<2, 2, 1>({});

  Functions::ConstantFunction<2, 2, 2>({}) * Functions::ConstantFunction<2, 2, 1>({});
  Functions::ConstantFunction<2, 2, 2>({}) * Functions::ConstantFunction<2, 2, 2>({});
  Functions::ConstantFunction<2, 2, 2>({}) * Functions::ConstantFunction<2, 2, 3>({});

  Functions::ConstantFunction<2, 2, 3>({}) * Functions::ConstantFunction<2, 3, 1>({});
  Functions::ConstantFunction<2, 2, 3>({}) * Functions::ConstantFunction<2, 3, 2>({});
  Functions::ConstantFunction<2, 2, 3>({}) * Functions::ConstantFunction<2, 3, 3>({});

  Functions::ConstantFunction<2, 3, 1>({}) * Functions::ConstantFunction<2, 1, 2>({});
  Functions::ConstantFunction<2, 3, 1>({}) * Functions::ConstantFunction<2, 1, 3>({});
  Functions::ConstantFunction<2, 3, 1>({}) * Functions::ConstantFunction<2, 3, 1>({});

  Functions::ConstantFunction<2, 3, 2>({}) * Functions::ConstantFunction<2, 2, 1>({});
  Functions::ConstantFunction<2, 3, 2>({}) * Functions::ConstantFunction<2, 2, 2>({});
  Functions::ConstantFunction<2, 3, 2>({}) * Functions::ConstantFunction<2, 2, 3>({});

  Functions::ConstantFunction<2, 3, 3>({}) * Functions::ConstantFunction<2, 3, 1>({});
  Functions::ConstantFunction<2, 3, 3>({}) * Functions::ConstantFunction<2, 3, 2>({});
  Functions::ConstantFunction<2, 3, 3>({}) * Functions::ConstantFunction<2, 3, 3>({});
}
