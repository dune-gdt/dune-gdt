// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2017, 2019)
//   René Fritze     (2017, 2019)
//   Tim Keil        (2018)
//   Tobias Leibner  (2018, 2020)

#include <dune/xt/test/main.hxx> // <- has to come first (includes the config.h)!

#include <dune/xt/test/la/generalized-eigensolver.hh>

{% for T_NAME, TESTMATRIXTYPE, TESTFIELDTYPE, TESTCOMPLEXMATRIXTYPE, TESTREALMATRIXTYPE in config.testtypes %}
struct GeneralizedEigenSolverForMatrixFullOfOnes_{{T_NAME}}
    : public GeneralizedEigenSolverTestForMatricesWithRealEigenvaluesAndVectors<{{TESTMATRIXTYPE}},
                                                                     {{TESTFIELDTYPE}},
                                                                     {{TESTCOMPLEXMATRIXTYPE}},
                                                                     {{TESTREALMATRIXTYPE}}>
{
  using BaseType = GeneralizedEigenSolverTestForMatricesWithRealEigenvaluesAndVectors;
  using typename BaseType::MatrixType;
  using typename BaseType::ComplexMatrixType;
  using typename BaseType::RealMatrixType;
  using typename BaseType::EigenValuesType;
  using typename BaseType::RealEigenValuesType;

  GeneralizedEigenSolverForMatrixFullOfOnes_{{T_NAME}}()
  {
    matrix_ = XT::Common::from_string<MatrixType>("[1 1; 1 1]");
    expected_eigenvalues_ = XT::Common::from_string<EigenValuesType>("[2 0]");
    expected_real_eigenvalues_ = XT::Common::from_string<RealEigenValuesType>("[2 0]");
    expected_max_ev_ = 2;
    expected_min_ev_ = 0;
    all_matrices_and_expected_eigenvalues_and_vectors_are_computed_ = true;
  }

  using BaseType::all_matrices_and_expected_eigenvalues_and_vectors_are_computed_;
  using BaseType::matrix_;
  using BaseType::expected_eigenvalues_;
  using BaseType::expected_real_eigenvalues_;
  using BaseType::expected_max_ev_;
  using BaseType::expected_min_ev_;
}; // struct GeneralizedEigenSolverForMatrixFullOfOnes_{{T_NAME}}


TEST_F(GeneralizedEigenSolverForMatrixFullOfOnes_{{T_NAME}}, exports_correct_types)
{
  exports_correct_types();
}

TEST_F(GeneralizedEigenSolverForMatrixFullOfOnes_{{T_NAME}}, has_types_and_options)
{
  has_types_and_options();
}

TEST_F(GeneralizedEigenSolverForMatrixFullOfOnes_{{T_NAME}}, throws_on_broken_matrix_construction)
{
  throws_on_broken_matrix_construction();
}

TEST_F(GeneralizedEigenSolverForMatrixFullOfOnes_{{T_NAME}}, throws_on_broken_rhs_matrix_construction)
{
  throws_on_broken_rhs_matrix_construction();
}

TEST_F(GeneralizedEigenSolverForMatrixFullOfOnes_{{T_NAME}}, allows_broken_matrix_construction_when_checks_disabled)
{
  allows_broken_matrix_construction_when_checks_disabled();
}

TEST_F(GeneralizedEigenSolverForMatrixFullOfOnes_{{T_NAME}}, allows_broken_matrix_construction_when_all_checks_disabled)
{
  allows_broken_matrix_construction_when_all_checks_disabled();
}

TEST_F(GeneralizedEigenSolverForMatrixFullOfOnes_{{T_NAME}}, throws_on_inconsistent_given_options)
{
  throws_on_inconsistent_given_options();
}

TEST_F(GeneralizedEigenSolverForMatrixFullOfOnes_{{T_NAME}}, throws_on_missing_type_in_options)
{
  throws_on_missing_type_in_options();
}

TEST_F(GeneralizedEigenSolverForMatrixFullOfOnes_{{T_NAME}}, throws_on_unknown_type)
{
  throws_on_unknown_type();
}

TEST_F(GeneralizedEigenSolverForMatrixFullOfOnes_{{T_NAME}}, throws_on_unknown_type_when_checks_are_disabled)
{
  throws_on_unknown_type_when_checks_are_disabled();
}

TEST_F(GeneralizedEigenSolverForMatrixFullOfOnes_{{T_NAME}}, throws_on_non_positive_real_tolerance)
{
  throws_on_non_positive_real_tolerance();
}

TEST_F(GeneralizedEigenSolverForMatrixFullOfOnes_{{T_NAME}}, throws_on_matrices_of_wrong_size)
{
  throws_on_matrices_of_wrong_size();
}

TEST_F(GeneralizedEigenSolverForMatrixFullOfOnes_{{T_NAME}}, is_constructible)
{
  is_constructible();
}

TEST_F(GeneralizedEigenSolverForMatrixFullOfOnes_{{T_NAME}}, is_constructible_from_options_pointer)
{
  is_constructible_from_options_pointer();
}

TEST_F(GeneralizedEigenSolverForMatrixFullOfOnes_{{T_NAME}}, exposes_the_given_matrices)
{
  exposes_the_given_matrices();
}

TEST_F(GeneralizedEigenSolverForMatrixFullOfOnes_{{T_NAME}}, gives_correct_eigenvalues)
{
  gives_correct_eigenvalues();
}

TEST_F(GeneralizedEigenSolverForMatrixFullOfOnes_{{T_NAME}}, gives_correct_eigenvalues_via_free_functions)
{
  gives_correct_eigenvalues_via_free_functions();
}

TEST_F(GeneralizedEigenSolverForMatrixFullOfOnes_{{T_NAME}}, throws_on_request_for_eigenvectors)
{
  throws_on_request_for_eigenvectors();
}

TEST_F(GeneralizedEigenSolverForMatrixFullOfOnes_{{T_NAME}}, throws_on_indefinite_rhs_matrix)
{
  throws_on_indefinite_rhs_matrix();
}

TEST_F(GeneralizedEigenSolverForMatrixFullOfOnes_{{T_NAME}}, gives_correct_real_eigenvalues)
{
  gives_correct_real_eigenvalues();
}

TEST_F(GeneralizedEigenSolverForMatrixFullOfOnes_{{T_NAME}}, gives_correct_max_eigenvalue)
{
  gives_correct_max_eigenvalue();
}

TEST_F(GeneralizedEigenSolverForMatrixFullOfOnes_{{T_NAME}}, gives_correct_min_eigenvalue)
{
  gives_correct_min_eigenvalue();
}

TEST_F(GeneralizedEigenSolverForMatrixFullOfOnes_{{T_NAME}}, gives_correct_extremal_eigenvalues)
{
  gives_correct_extremal_eigenvalues();
}

TEST_F(GeneralizedEigenSolverForMatrixFullOfOnes_{{T_NAME}}, checks_eigenvalue_assertions)
{
  checks_eigenvalue_assertions();
}

TEST_F(GeneralizedEigenSolverForMatrixFullOfOnes_{{T_NAME}}, computes_eigenvalues_required_for_assertions)
{
  computes_eigenvalues_required_for_assertions();
}

{% endfor %}
