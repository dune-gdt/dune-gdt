// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2014 - 2018)
//   René Fritze     (2016, 2018)

/**
 * \file  exceptions.hh
 * \brief Exception types thrown throughout dune-gdt.
 **/
#ifndef DUNE_GDT_EXCEPTIONS_HH
#define DUNE_GDT_EXCEPTIONS_HH

#include <dune/xt/common/exceptions.hh>
#include <dune/xt/la/exceptions.hh>
#include <dune/xt/grid/exceptions.hh>
#include <dune/xt/functions/exceptions.hh>

namespace Dune {
namespace GDT {
namespace Exceptions {


/**
 * \brief Exception thrown when an object that requires binding to a grid element is used before being bound.
 */
class not_bound_to_an_element_yet : public XT::Grid::Exceptions::not_bound_to_an_element_yet
{};

/**
 * \brief Exception thrown on errors related to degree-of-freedom vectors.
 */
class dof_vector_error : public Exception
{};

/**
 * \brief Exception thrown on errors related to integrands.
 */
class integrand_error : public Exception
{};

/**
 * \brief Exception thrown on errors during assembly.
 */
class assembler_error : public Exception
{};

/**
 * \brief Exception thrown on errors related to functionals.
 */
class functional_error : public Exception
{};

/**
 * \brief Exception thrown on errors related to numerical fluxes.
 */
class numerical_flux_error : public Exception
{};

/**
 * \brief Exception thrown on errors related to operators.
 */
class operator_error : public Exception
{};

/**
 * \brief Exception thrown on errors related to bilinear forms.
 */
class bilinear_form_error : public operator_error
{};

/**
 * \brief Exception thrown on errors during interpolation.
 */
class interpolation_error : public operator_error
{};

/**
 * \brief Exception thrown on errors during prolongation.
 */
class prolongation_error : public operator_error
{};

/**
 * \brief Exception thrown on errors during projection.
 */
class projection_error : public operator_error
{};

/**
 * \brief Exception thrown on errors related to finite elements.
 */
class finite_element_error : public Exception
{};

/**
 * \brief Exception thrown on errors related to discrete function spaces.
 */
class space_error : public Exception
{};

/**
 * \brief Exception thrown on errors related to restricted spaces.
 */
class restricted_space_error : public space_error
{};

/**
 * \brief Exception thrown on errors related to mappers.
 */
class mapper_error : public space_error
{};

/**
 * \brief Exception thrown on errors related to bases.
 */
class basis_error : public space_error
{};

/**
 * \brief Exception thrown on errors related to discrete functions.
 */
class discrete_function_error : public Exception
{};

/**
 * \brief Exception thrown on errors during Newton iterations.
 */
class newton_error : public operator_error
{};


} // namespace Exceptions
} // namespace GDT
} // namespace Dune

#endif // DUNE_GDT_EXCEPTIONS_HH
