// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2020)
//   René Fritze     (2019 - 2020)
//   Tobias Leibner  (2019 - 2020)

#define DUNE_XT_COMMON_TEST_MAIN_ENABLE_DEBUG_LOGGING 1
#define DUNE_XT_COMMON_TEST_MAIN_ENABLE_INFO_LOGGING 1
#define DUNE_XT_COMMON_TEST_MAIN_ENABLE_TIMED_LOGGING 1

#include <dune/xt/test/main.hxx> // <- This one has to come first, includes config.h!
#include <dune/xt/test/common/float_cmp.hh>
#include <dune/xt/test/gtest/gtest.h>

#include <dune/xt/common/type_traits.hh>
#include <dune/xt/la/solver/istl/saddlepoint.hh>

using namespace Dune;

// Data for saddle point system (A B; B^T C) (u; p) = (f; g) from Taylor-Hood P2-P1 continuous finite element
// discretization on a 2x2 cubic grid in 2 dimensions.
template <class Matrix, class Vector>
struct SaddlePointTestData
{

  SaddlePointTestData()
    : A_(XT::Common::from_string<Matrix>(
        "[4.74074074074074 0 0 0 0 0 0 0 0 0 0 0 -0.592592592592593 0 0 0 0 0 0 0 0 0 -0.592592592592592 0 0 0 0 0 0 "
        "0 0 0 0 0 0 0 0 0 0 0 -0.592592592592593 0 0 0 0 0 0 0 0 0; 0 4.74074074074074 0 0 0 0 0 0 0 0 0 0 0 "
        "-0.592592592592593 0 0 0 0 0 0 0 0 0 -0.592592592592592 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 "
        "-0.592592592592593 0 0 0 0 0 0 0 0; 0 0 4.74074074074074 0 0 0 0 0 0 0 0 0 0 0 -0.592592592592593 0 0 0 0 0 "
        "0 0 -0.592592592592594 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 -0.592592592592592 0 0 0 0 0 0 0 0 0; 0 0 0 "
        "4.74074074074074 0 0 0 0 0 0 0 0 0 0 0 -0.592592592592593 0 0 0 0 0 0 0 -0.592592592592594 0 0 0 0 0 0 0 0 "
        "0 0 0 0 0 0 0 0 0 -0.592592592592592 0 0 0 0 0 0 0 0; 0 0 0 0 4.74074074074074 0 0 0 0 0 0 0 "
        "-0.592592592592593 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 -0.592592592592592 0 0 0 0 0 0 0 0 0 0 0 "
        "-0.592592592592592 0 0 0 0 0 0 0 0 0; 0 0 0 0 0 4.74074074074074 0 0 0 0 0 0 0 -0.592592592592593 0 0 0 0 0 "
        "0 0 0 0 0 0 0 0 0 0 -0.592592592592592 0 0 0 0 0 0 0 0 0 0 0 -0.592592592592592 0 0 0 0 0 0 0 0; 0 0 0 0 0 "
        "0 4.74074074074074 0 0 0 0 0 0 0 -0.592592592592593 0 0 0 0 0 0 0 0 0 0 0 0 0 -0.592592592592594 0 0 0 0 0 "
        "0 0 0 0 0 0 -0.592592592592592 0 0 0 0 0 0 0 0 0; 0 0 0 0 0 0 0 4.74074074074074 0 0 0 0 0 0 0 "
        "-0.592592592592593 0 0 0 0 0 0 0 0 0 0 0 0 0 -0.592592592592594 0 0 0 0 0 0 0 0 0 0 0 -0.592592592592592 0 "
        "0 0 0 0 0 0 0; -0 0 0 0 0 0 0 0 1 0 0 0 -0 0 0 0 0 0 0 0 0 0 -0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 "
        "0 0 0 0 0; 0 -0 0 0 0 0 0 0 0 1 0 0 0 -0 0 0 0 0 0 0 0 0 0 -0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 "
        "0 0 0; 0 0 -0 0 0 0 0 0 0 0 1 0 0 0 -0 0 0 0 0 0 0 0 -0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 "
        "0; 0 0 0 -0 0 0 0 0 0 0 0 1 0 0 0 -0 0 0 0 0 0 0 0 -0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0; "
        "-0.592592592592593 0 0 0 -0.592592592592593 0 0 0 0 0 0 0 3.25925925925926 0 0 0 0 0 0 0 0 0 "
        "-0.592592592592593 0 0 0 0 0 -0.592592592592592 0 0 0 0 0 0 0 0 0 0 0 -0.0740740740740739 0 0 0 0 0 0 0 0 "
        "0; 0 -0.592592592592593 0 0 0 -0.592592592592593 0 0 0 0 0 0 0 3.25925925925926 0 0 0 0 0 0 0 0 0 "
        "-0.592592592592593 0 0 0 0 0 -0.592592592592592 0 0 0 0 0 0 0 0 0 0 0 -0.0740740740740739 0 0 0 0 0 0 0 0; "
        "0 0 -0.592592592592593 0 0 0 -0.592592592592593 0 0 0 0 0 0 0 3.25925925925926 0 0 0 0 0 0 0 "
        "-0.592592592592592 0 0 0 0 0 -0.592592592592593 0 0 0 0 0 0 0 0 0 0 0 -0.0740740740740742 0 0 0 0 0 0 0 0 "
        "0; 0 0 0 -0.592592592592593 0 0 0 -0.592592592592593 0 0 0 0 0 0 0 3.25925925925926 0 0 0 0 0 0 0 "
        "-0.592592592592592 0 0 0 0 0 -0.592592592592593 0 0 0 0 0 0 0 0 0 0 0 -0.0740740740740742 0 0 0 0 0 0 0 0; "
        "0 0 0 0 -0 0 0 0 0 0 0 0 -0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0 -0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0; 0 0 "
        "0 0 0 -0 0 0 0 0 0 0 0 -0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0 -0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0; 0 0 0 0 "
        "0 0 -0 0 0 0 0 0 0 0 -0 0 0 0 1 0 0 0 0 0 0 0 0 0 -0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0; 0 0 0 0 0 0 "
        "0 -0 0 0 0 0 0 0 0 -0 0 0 0 1 0 0 0 0 0 0 0 0 0 -0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0; -0 0 0 0 0 0 0 "
        "0 0 0 0 0 -0 0 0 0 0 0 0 0 1 0 -0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0; 0 -0 0 0 0 0 0 0 0 "
        "0 0 0 0 -0 0 0 0 0 0 0 0 1 0 -0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0; -0.592592592592592 0 "
        "-0.592592592592594 0 0 0 0 0 0 0 0 0 -0.592592592592593 0 -0.592592592592592 0 0 0 0 0 0 0 3.25925925925926 "
        "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 -0.0740740740740741 0 0 0 0 0 0 0 0 0; 0 -0.592592592592592 0 "
        "-0.592592592592594 0 0 0 0 0 0 0 0 0 -0.592592592592593 0 -0.592592592592592 0 0 0 0 0 0 0 3.25925925925926 "
        "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 -0.0740740740740741 0 0 0 0 0 0 0 0; 0 0 -0 0 0 0 0 0 0 0 0 0 0 0 -0 0 0 "
        "0 0 0 0 0 -0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0; 0 0 0 -0 0 0 0 0 0 0 0 0 0 0 0 -0 0 0 0 "
        "0 0 0 0 -0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0; 0 0 0 0 -0 0 0 0 0 0 0 0 -0 0 0 0 0 0 0 0 0 "
        "0 0 0 0 0 1 0 -0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0; 0 0 0 0 0 -0 0 0 0 0 0 0 0 -0 0 0 0 0 0 0 0 0 0 "
        "0 0 0 0 1 0 -0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0; 0 0 0 0 -0.592592592592592 0 -0.592592592592594 0 0 "
        "0 0 0 -0.592592592592592 0 -0.592592592592593 0 0 0 0 0 0 0 0 0 0 0 0 0 3.25925925925926 0 0 0 0 0 0 0 0 0 "
        "0 0 -0.0740740740740745 0 0 0 0 0 0 0 0 0; 0 0 0 0 0 -0.592592592592592 0 -0.592592592592594 0 0 0 0 0 "
        "-0.592592592592592 0 -0.592592592592593 0 0 0 0 0 0 0 0 0 0 0 0 0 3.25925925925926 0 0 0 0 0 0 0 0 0 0 0 "
        "-0.0740740740740745 0 0 0 0 0 0 0 0; 0 0 0 0 0 0 -0 0 0 0 0 0 0 0 -0 0 0 0 0 0 0 0 0 0 0 0 0 0 -0 0 1 0 0 0 "
        "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0; 0 0 0 0 0 0 0 -0 0 0 0 0 0 0 0 -0 0 0 0 0 0 0 0 0 0 0 0 0 0 -0 0 1 0 0 0 0 "
        "0 0 0 0 0 0 0 0 0 0 0 0 0 0; -0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 0 0 0 0 0 "
        "0 -0 0 0 0 0 0 0 0 0 0; 0 -0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0 "
        "-0 0 0 0 0 0 0 0 0; -0 0 -0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 -0 0 0 0 0 0 0 0 0 0 0 0 1 0 0 0 0 0 -0 0 "
        "0 0 0 0 0 0 0 0; 0 -0 0 -0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 -0 0 0 0 0 0 0 0 0 0 0 0 1 0 0 0 0 0 -0 0 "
        "0 0 0 0 0 0 0; 0 0 -0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 0 0 -0 0 0 0 0 "
        "0 0 0 0 0; 0 0 0 -0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 0 0 -0 0 0 0 0 0 "
        "0 0 0; -0 0 0 0 -0 0 0 0 0 0 0 0 -0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 -0 0 0 0 0 0 0 0 "
        "0 0; 0 -0 0 0 0 -0 0 0 0 0 0 0 0 -0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 -0 0 0 0 0 0 0 0 "
        "0; -0.592592592592593 0 -0.592592592592592 0 -0.592592592592592 0 -0.592592592592592 0 0 0 0 0 "
        "-0.0740740740740739 0 -0.0740740740740742 0 0 0 0 0 0 0 -0.0740740740740741 0 0 0 0 0 -0.0740740740740745 0 "
        "0 0 0 0 0 0 0 0 0 0 2.07407407407407 0 0 0 0 0 0 0 0 0; 0 -0.592592592592593 0 -0.592592592592592 0 "
        "-0.592592592592592 0 -0.592592592592592 0 0 0 0 0 -0.0740740740740739 0 -0.0740740740740742 0 0 0 0 0 0 0 "
        "-0.0740740740740741 0 0 0 0 0 -0.0740740740740745 0 0 0 0 0 0 0 0 0 0 0 2.07407407407407 0 0 0 0 0 0 0 0; 0 "
        "0 -0 0 0 0 -0 0 0 0 0 0 0 0 -0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 -0 0 1 0 0 0 0 0 0 0; 0 0 "
        "0 -0 0 0 0 -0 0 0 0 0 0 0 0 -0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 -0 0 1 0 0 0 0 0 0; 0 0 0 "
        "0 -0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 -0 0 0 0 1 0 0 0 0 0; 0 0 0 0 0 "
        "-0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 -0 0 0 0 1 0 0 0 0; 0 0 0 0 -0 0 "
        "-0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 -0 0 0 0 0 0 0 0 0 0 0 0 -0 0 0 0 0 0 1 0 0 0; 0 0 0 0 0 -0 0 "
        "-0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 -0 0 0 0 0 0 0 0 0 0 0 0 -0 0 0 0 0 0 1 0 0; 0 0 0 0 0 0 -0 0 "
        "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 -0 0 0 0 0 0 0 0 1 0; 0 0 0 0 0 0 0 -0 0 0 "
        "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 -0 0 0 0 0 0 0 0 1]"))
    , B_(XT::Common::from_string<Matrix>(
          "[0 0.222222222222222 0 -0.222222222222222 0.222222222222222 0 0 0 0; 0 -0.222222222222222 0 "
          "0.222222222222222 0.222222222222222 0 0 0 0; 0 -0.222222222222222 0.222222222222222 0 -0.222222222222222 "
          "0.222222222222222 0 0 0; 0 -0.222222222222222 -0.222222222222222 0 0.222222222222222 0.222222222222222 0 0 "
          "0; 0 0 0 -0.222222222222222 0.222222222222222 0 -0.222222222222222 0.222222222222222 0; 0 0 0 "
          "-0.222222222222222 -0.222222222222222 0 0.222222222222222 0.222222222222222 0; 0 0 0 0 -0.222222222222222 "
          "0.222222222222222 0 -0.222222222222222 0.222222222222222; 0 0 0 0 -0.222222222222222 -0.222222222222222 0 "
          "0.222222222222222 0.222222222222222; 0 0 0 -0 -0 0 0 0 0; 0 0 0 0 0 0 0 0 0; 0 -0 0 0 -0 -0 0 0 0; 0 0 0 0 "
          "0 0 0 0 0; 0 -1.47451495458029e-17 0 -0.222222222222222 0.222222222222222 0 -1.10588621593521e-17 "
          "-8.67361737988404e-17 0; 0 -0.0555555555555556 0 0 -5.55111512312578e-17 0 0.0555555555555555 "
          "0.0555555555555555 0; 0 -7.80625564189563e-18 -1.47451495458029e-17 0 -0.222222222222222 0.222222222222222 "
          "0 -1.10588621593521e-17 -8.67361737988404e-17; 0 -0.0555555555555556 -0.0555555555555556 0 0 "
          "-5.55111512312578e-17 0 0.0555555555555555 0.0555555555555555; 0 0 0 -0 -0 0 -0 0 0; 0 0 0 -0 -0 0 -0 -0 0; "
          "0 0 0 0 -0 -0 0 -0 0; 0 0 0 0 -0 -0 0 -0 -0; 0 0 0 0 0 0 0 0 0; 0 -0 0 0 -0 0 0 0 0; 0 "
          "-5.55111512312578e-17 0.0555555555555555 -0.0555555555555556 -1.11022302462516e-16 0.0555555555555554 0 0 "
          "0; 0 -0.222222222222222 -2.278900733657e-17 3.03576608295941e-18 0.222222222222222 -5.78462730903991e-17 0 "
          "0 0; 0 -0 -0 0 -0 -0 0 0 0; 0 -0 -0 0 0 0 0 0 0; 0 0 0 0 0 0 0 0 0; 0 0 0 -0 -0 0 0 -0 0; 0 0 0 "
          "-0.0555555555555555 -5.55111512312578e-17 0.0555555555555555 -0.0555555555555556 -1.11022302462516e-16 "
          "0.0555555555555554; 0 0 0 -7.80625564189563e-18 -0.222222222222222 -2.278900733657e-17 3.03576608295941e-18 "
          "0.222222222222222 -5.78462730903991e-17; 0 0 0 0 -0 -0 0 -0 -0; 0 0 0 0 -0 -0 0 0 0; 0 0 0 0 0 0 0 0 0; 0 0 "
          "0 0 0 0 0 0 0; 0 0 0 -0 0 0 0 0 0; 0 0 0 0 0 0 0 0 0; 0 -0 -0 0 -0 0 0 0 0; 0 0 0 0 0 0 0 0 0; 0 0 0 0 0 0 "
          "0 0 0; 0 -0 0 0 0 0 0 0 0; 0 1.95156391047391e-17 1.62630325872826e-17 -0.0555555555555555 "
          "1.38777878078145e-16 0.0555555555555557 -6.07153216591882e-18 8.56519716263549e-17 7.80625564189563e-17; 0 "
          "-0.0555555555555556 -7.25377369846565e-18 1.47451495458029e-17 1.11022302462516e-16 4.01007997789875e-17 "
          "1.23599047663348e-17 0.0555555555555556 5.12577012984009e-17; 0 0 0 0 -0 -0 0 -0 0; 0 0 -0 0 0 0 0 0 0; 0 0 "
          "0 0 0 0 0 0 0; 0 0 0 -0 -0 0 -0 0 0; 0 0 0 0 0 0 -0 0 0; 0 0 0 0 -0 -0 0 -0 0; 0 0 0 0 0 0 0 -0 -0; 0 0 0 0 "
          "0 -0 0 0 -0]"))
    , C_(XT::Common::from_string<Matrix>(
          "[1 0 0 0 0 0 0 0 0; 0 0 0 0 0 0 0 0 0; 0 0 0 0 0 0 0 0 0; 0 0 0 0 0 0 0 0 0; 0 0 0 0 0 0 0 0 0; 0 0 0 0 0 0 "
          "0 0 0; 0 0 0 0 0 0 0 0 0; 0 0 0 0 0 0 0 0 0; 0 0 0 0 0 0 0 0 0]"))
    , f_(XT::Common::from_string<Vector>(
          "[1.81687717492897 1.03679656984776 5.87375029840605 3.0623906655763 -1.81687717492898 1.03679656984776 "
          "-5.87375029840606 3.0623906655763 0 0 0 0 8.32667268468867e-16 -0.170568600172334 6.77236045021345e-15 "
          "0.0245172535467617 0 0 0 0 0 0 1.57913634656985 0.815600008121092 0 0 0 0 -1.57913634656985 "
          "0.815600008121088 0 0 0 0 0 0 0 0 0 0 -3.19189119579733e-15 -0.47283974442817 0 0 0 0 0 0 0 0]"))
    , g_(XT::Common::from_string<Vector>(
          "[0 -0.390767628501963 0.292824934559828 -6.13451300300473e-18 2.77186829999535e-17 1.38777878078145e-17 "
          "0.20789648268931 0.390767628501963 -0.292824934559828]"))
    , expected_u_(XT::Common::from_string<Vector>(
          "[0.547674166676229 0.152254190144879 1.49087159573962 0.395553046436061 -0.547674166676231 0.15225419014488 "
          "-1.49087159573962 0.395553046436061 0 0 0 0 8.67058231598939e-16 0.0197655266613152 1.71469967357026e-15 "
          "0.0330831095991677 0 0 0 0 0 0 0.922657734461829 0.249685297674183 0 0 0 0 -0.922657734461827 "
          "0.249685297674182 0 0 0 0 0 0 0 0 0 0 -2.23728869125248e-15 0.0182088235007909 0 0 0 0 0 0 0 0]"))
    , expected_p_(
          XT::Common::from_string<Vector>("[0 -1.04734764334764 -3.96031138440079 0.568619962719988 0.568619962719984 "
                                          "0.568619962719989 1.13723992543995 2.18458756878762 5.09755130984076]"))
  {
  }

  Matrix A_;
  Matrix B_;
  Matrix C_;
  Vector f_;
  Vector g_;
  Vector expected_u_;
  Vector expected_p_;
};

#if HAVE_DUNE_ISTL

GTEST_TEST(SaddlePointSolver, test_direct)
{
  using Matrix = XT::LA::IstlRowMajorSparseMatrix<double>;
  using Vector = XT::LA::IstlDenseVector<double>;
  SaddlePointTestData<Matrix, Vector> data;
  XT::LA::SaddlePointSolver<Vector, Matrix> solver(data.A_, data.B_, data.B_, data.C_);
  Vector u(data.f_.size()), p(data.g_.size());
  solver.apply(data.f_, data.g_, u, p, "direct");
  DXTC_EXPECT_FLOAT_EQ(0., (u - data.expected_u_).l2_norm(), 1e-12, 1e-12);
  DXTC_EXPECT_FLOAT_EQ(0., (p - data.expected_p_).l2_norm(), 1e-12, 1e-12);
}

GTEST_TEST(SaddlePointSolver, test_cg_direct_schurcomplement)
{
  using Matrix = XT::LA::IstlRowMajorSparseMatrix<double>;
  using Vector = XT::LA::IstlDenseVector<double>;
  SaddlePointTestData<Matrix, Vector> data;
  XT::LA::SaddlePointSolver<Vector, Matrix> solver(data.A_, data.B_, data.B_, data.C_);
  Vector u(data.f_.size()), p(data.g_.size());
  solver.apply(data.f_, data.g_, u, p, "cg_direct_schurcomplement");
  DXTC_EXPECT_FLOAT_EQ(0., (u - data.expected_u_).l2_norm(), 1e-12, 1e-12);
  DXTC_EXPECT_FLOAT_EQ(0., (p - data.expected_p_).l2_norm(), 1e-12, 1e-12);
}

GTEST_TEST(SaddlePointSolver, test_cg_cg_schurcomplement)
{
  using Matrix = XT::LA::IstlRowMajorSparseMatrix<double>;
  using Vector = XT::LA::IstlDenseVector<double>;
  SaddlePointTestData<Matrix, Vector> data;
  XT::LA::SaddlePointSolver<Vector, Matrix> solver(data.A_, data.B_, data.B_, data.C_);
  Vector u(data.f_.size()), p(data.g_.size());
  solver.apply(data.f_, data.g_, u, p, "cg_cg_schurcomplement");
  DXTC_EXPECT_FLOAT_EQ(0., (u - data.expected_u_).l2_norm(), 1e-12, 1e-12);
  DXTC_EXPECT_FLOAT_EQ(0., (p - data.expected_p_).l2_norm(), 1e-12, 1e-12);
}

#  if HAVE_EIGEN

GTEST_TEST(SaddlePointSolver, test_direct_eigen)
{
  using Matrix = XT::LA::EigenRowMajorSparseMatrix<double>;
  using Vector = XT::LA::EigenDenseVector<double>;
  SaddlePointTestData<Matrix, Vector> data;
  XT::LA::SaddlePointSolver<Vector, Matrix> solver(data.A_, data.B_, data.B_, data.C_);
  Vector u(data.f_.size()), p(data.g_.size());
  solver.apply(data.f_, data.g_, u, p, "direct");
  DXTC_EXPECT_FLOAT_EQ(0., (u - data.expected_u_).l2_norm(), 1e-12, 1e-12);
  DXTC_EXPECT_FLOAT_EQ(0., (p - data.expected_p_).l2_norm(), 1e-12, 1e-12);
}

GTEST_TEST(SaddlePointSolver, test_cg_direct_schurcomplement_eigen)
{
  using Matrix = XT::LA::EigenRowMajorSparseMatrix<double>;
  using Vector = XT::LA::EigenDenseVector<double>;
  SaddlePointTestData<Matrix, Vector> data;
  XT::LA::SaddlePointSolver<Vector, Matrix> solver(data.A_, data.B_, data.B_, data.C_);
  Vector u(data.f_.size()), p(data.g_.size());
  solver.apply(data.f_, data.g_, u, p, "cg_direct_schurcomplement");
  DXTC_EXPECT_FLOAT_EQ(0., (u - data.expected_u_).l2_norm(), 1e-12, 1e-12);
  DXTC_EXPECT_FLOAT_EQ(0., (p - data.expected_p_).l2_norm(), 1e-12, 1e-12);
}

#  endif // HAVE_EIGEN
#endif // HAVE_DUNE_ISTL
