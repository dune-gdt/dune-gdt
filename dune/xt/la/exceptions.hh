// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2017 - 2019)
//   René Fritze     (2018 - 2019)
//   Tobias Leibner  (2017, 2020)

/// \file
/// \brief Exception types thrown by dune-xt-la (solver, eigensolver and inversion failures).

#ifndef DUNE_XT_LA_EXCEPTIONS_HH
#define DUNE_XT_LA_EXCEPTIONS_HH

#include <dune/xt/common/exceptions.hh>

namespace Dune::XT::LA::Exceptions {


/// \brief Thrown when a requested feature or backend is not available.
class not_available : public Dune::Exception
{};

/// \brief Base exception thrown when solving a linear system fails.
class linear_solver_failed : public Dune::Exception
{};

/// \brief Linear solver failure because the input data did not fulfill the solver's requirements.
class linear_solver_failed_bc_data_did_not_fulfill_requirements : public linear_solver_failed
{};

/// \brief Linear solver failure because the iterative method did not converge.
class linear_solver_failed_bc_it_did_not_converge : public linear_solver_failed
{};

/// \brief Linear solver failure because the solver was not set up correctly.
class linear_solver_failed_bc_it_was_not_set_up_correctly : public linear_solver_failed
{};

/// \brief Linear solver failure because the computed solution does not solve the system.
class linear_solver_failed_bc_the_solution_does_not_solve_the_system : public linear_solver_failed
{};


/// \brief Base exception thrown when computing an eigendecomposition fails.
class eigen_solver_failed : public Dune::Exception
{};

/// \brief Eigensolver failure because the input data did not fulfill the solver's requirements.
class eigen_solver_failed_bc_data_did_not_fulfill_requirements : public eigen_solver_failed
{};

/// \brief Eigensolver failure because the solver was not set up correctly.
class eigen_solver_failed_bc_it_was_not_set_up_correctly : public eigen_solver_failed
{};

/// \brief Eigensolver failure because the result contained inf or nan values.
class eigen_solver_failed_bc_result_contained_inf_or_nan : public eigen_solver_failed
{};

/// \brief Eigensolver failure because the eigenvalues are not real as requested.
class eigen_solver_failed_bc_eigenvalues_are_not_real_as_requested : public eigen_solver_failed
{};

/// \brief Eigensolver failure because the eigenvalues are not positive as requested.
class eigen_solver_failed_bc_eigenvalues_are_not_positive_as_requested : public eigen_solver_failed
{};

/// \brief Eigensolver failure because the eigenvalues are not negative as requested.
class eigen_solver_failed_bc_eigenvalues_are_not_negative_as_requested : public eigen_solver_failed
{};

/// \brief Eigensolver failure because the eigenvectors are not real as requested.
class eigen_solver_failed_bc_eigenvectors_are_not_real_as_requested : public eigen_solver_failed
{};

/// \brief Eigensolver failure because the result is not a valid eigendecomposition.
class eigen_solver_failed_bc_result_is_not_an_eigendecomposition : public eigen_solver_failed
{};


/// \brief Base exception thrown when computing a generalized eigendecomposition fails.
class generalized_eigen_solver_failed : public Dune::Exception
{};

/// \brief Generalized eigensolver failure because the input data did not fulfill the solver's requirements.
class generalized_eigen_solver_failed_bc_data_did_not_fulfill_requirements : public generalized_eigen_solver_failed
{};

/// \brief Generalized eigensolver failure because the solver was not set up correctly.
class generalized_eigen_solver_failed_bc_it_was_not_set_up_correctly : public generalized_eigen_solver_failed
{};

/// \brief Generalized eigensolver failure because the result contained inf or nan values.
class generalized_eigen_solver_failed_bc_result_contained_inf_or_nan : public generalized_eigen_solver_failed
{};

/// \brief Generalized eigensolver failure because the eigenvalues are not real as requested.
class generalized_eigen_solver_failed_bc_eigenvalues_are_not_real_as_requested : public generalized_eigen_solver_failed
{};

/// \brief Generalized eigensolver failure because the eigenvalues are not positive as requested.
class generalized_eigen_solver_failed_bc_eigenvalues_are_not_positive_as_requested
  : public generalized_eigen_solver_failed
{};

/// \brief Generalized eigensolver failure because the eigenvalues are not negative as requested.
class generalized_eigen_solver_failed_bc_eigenvalues_are_not_negative_as_requested
  : public generalized_eigen_solver_failed
{};

/// \brief Generalized eigensolver failure because the eigenvectors are not real as requested.
class generalized_eigen_solver_failed_bc_eigenvectors_are_not_real_as_requested : public generalized_eigen_solver_failed
{};

// class generalized_eigen_solver_failed_bc_result_is_not_an_eigendecomposition : public generalized_eigen_solver_failed
//{};


/// \brief Base exception thrown when inverting a matrix fails.
class matrix_invert_failed : public Dune::Exception
{};

/// \brief Matrix inversion failure because the input data did not fulfill the requirements.
class matrix_invert_failed_bc_data_did_not_fulfill_requirements : public matrix_invert_failed
{};

/// \brief Matrix inversion failure because the inverter was not set up correctly.
class matrix_invert_failed_bc_it_was_not_set_up_correctly : public matrix_invert_failed
{};

/// \brief Matrix inversion failure because the result contained inf or nan values.
class matrix_invert_failed_bc_result_contained_inf_or_nan : public matrix_invert_failed
{};

/// \brief Matrix inversion failure because the result is not a left inverse.
class matrix_invert_failed_bc_result_is_not_a_left_inverse : public matrix_invert_failed
{};

/// \brief Matrix inversion failure because the result is not a right inverse.
class matrix_invert_failed_bc_result_is_not_a_right_inverse : public matrix_invert_failed
{};


} // namespace Dune::XT::LA::Exceptions

#endif // DUNE_XT_LA_EXCEPTIONS_HH
