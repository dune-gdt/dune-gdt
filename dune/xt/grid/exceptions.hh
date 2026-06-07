// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2012, 2014, 2016 - 2018, 2020)
//   René Fritze     (2012 - 2013, 2015 - 2016, 2018 - 2020)
//   Tobias Leibner  (2014, 2016, 2020)

/// \file
/// \brief Exception types used throughout the dune-xt grid module.

#ifndef DUNE_XT_GRID_EXCEPTIONS_HH
#define DUNE_XT_GRID_EXCEPTIONS_HH

#include <dune/xt/common/exceptions.hh>

namespace Dune::XT::Grid::Exceptions {


/// \brief Thrown when an invalid or unknown boundary type is encountered.
class boundary_type_error : public Dune::Exception
{};

/// \brief Thrown when boundary information is invalid or inconsistent.
class boundary_info_error : public Dune::Exception
{};

/// \brief Thrown when an object is accessed before being bound to an element.
class not_bound_to_an_element_yet : public Dune::InvalidStateException
{};

/// \brief Thrown when a functor is used in an invalid state.
class functor_error : public Dune::InvalidStateException
{};

/// \brief Thrown when an operation is attempted with an unexpected dimension.
class wrong_dimension : public Dune::Exception
{};

/// \brief Thrown when an operation is attempted with an unexpected codimension.
class wrong_codimension : public Dune::Exception
{};


} // namespace Dune::XT::Grid::Exceptions

#endif // DUNE_XT_GRID_EXCEPTIONS_HH
