// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2017 - 2018, 2020)
//   René Fritze     (2018 - 2020)
//   Tim Keil        (2018)
//   Tobias Leibner  (2018, 2020)

/// \file
/// \brief Exception types used throughout dune-xt-functions.

#ifndef DUNE_XT_FUNCTIONS_EXCEPTIONS_HH
#define DUNE_XT_FUNCTIONS_EXCEPTIONS_HH

#include <dune/xt/common/exceptions.hh>
#include <dune/xt/grid/exceptions.hh>

namespace Dune::XT::Functions::Exceptions {


/// \brief Thrown when a function receives invalid input.
class wrong_input_given : public Common::Exceptions::wrong_input_given
{};

/// \brief Thrown when a local function is evaluated before being bound to an element.
class not_bound_to_an_element_yet : public Grid::Exceptions::not_bound_to_an_element_yet
{};

/// \brief Thrown when reinterpreting a function (e.g. its dimensions) fails.
class reinterpretation_error : public Dune::Exception
{};

/// \brief Thrown on errors related to function parameters.
class parameter_error : public Common::Exceptions::parameter_error
{};

/// \brief Thrown when a required SPE10 data file is missing.
class spe10_data_file_missing : public Dune::IOError
{};

/// \brief Thrown on errors within an element function.
class element_function_error : public Dune::Exception
{};

/// \brief Thrown on errors when combining functions.
class combined_error : public Dune::Exception
{};

/// \brief Thrown on errors within a function.
class function_error : public Dune::Exception
{};

/// \brief Thrown on errors within a grid function.
class grid_function_error : public Dune::Exception
{};

/// \brief Thrown on errors within a flux function.
class flux_function_error : public Dune::Exception
{};


} // namespace Dune::XT::Functions::Exceptions

#endif // DUNE_XT_FUNCTIONS_EXCEPTIONS_HH
