// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2026 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   René Fritze (2026)

// Tests of genuine generalized eigenvalue problems lhs*v = lambda*rhs*v with a right hand side other than the
// identity: generalized-eigensolver_for_real_matrix_with_real_evs uses rhs = Id throughout, which reduces to the
// standard problem and never exercises lapack's Cholesky reduction of the pair.
//
// Both pairs below are symmetric with a positive definite right hand side (the requirement of dsygv, see
// dune/xt/la/generalized-eigen-solver/internal/lapacke.hh) and are chosen so that the eigenvalues are exactly
// representable: with rhs = 2*Id the problem reduces to the eigenvalues of lhs/2, and lhs = [a b; b a] has the
// eigenvalues a +- b. One pair has a strictly positive, the other a strictly negative spectrum, which is what makes
// both directions of the assert_positive_eigenvalues/assert_negative_eigenvalues post checks reachable
// (GeneralizedEigenSolverBase::post_checks(), dune/xt/la/generalized-eigen-solver/internal/base.hh).

#include <dune/xt/test/main.hxx> // <- has to come first (includes the config.h)!

#include <dune/xt/test/la/generalized-eigensolver.hh>

{% for T_NAME, TESTMATRIXTYPE, TESTFIELDTYPE, TESTCOMPLEXMATRIXTYPE, TESTREALMATRIXTYPE in config.testtypes %}
struct GeneralizedEigenSolverForMatrixPairWithPositiveEigenvalues_{{T_NAME}}
    : public GeneralizedEigenSolverTestForMatricesWithRealEigenvaluesAndVectors<{{TESTMATRIXTYPE}},
                                                                     {{TESTFIELDTYPE}},
                                                                     {{TESTCOMPLEXMATRIXTYPE}},
                                                                     {{TESTREALMATRIXTYPE}}>
{
  using BaseType = GeneralizedEigenSolverTestForMatricesWithRealEigenvaluesAndVectors;
  using typename BaseType::MatrixType;
  using typename BaseType::EigenValuesType;
  using typename BaseType::RealEigenValuesType;

  GeneralizedEigenSolverForMatrixPairWithPositiveEigenvalues_{{T_NAME}}()
  {
    matrix_ = XT::Common::from_string<MatrixType>("[3 1; 1 3]");
    rhs_matrix_ = XT::Common::from_string<MatrixType>("[2 0; 0 2]");
    rhs_matrix_is_given_ = true;
    expected_eigenvalues_ = XT::Common::from_string<EigenValuesType>("[1 2]");
    expected_real_eigenvalues_ = XT::Common::from_string<RealEigenValuesType>("[1 2]");
    expected_max_ev_ = 2;
    expected_min_ev_ = 1;
    all_matrices_and_expected_eigenvalues_and_vectors_are_computed_ = true;
  }

  using BaseType::all_matrices_and_expected_eigenvalues_and_vectors_are_computed_;
  using BaseType::matrix_;
  using BaseType::rhs_matrix_;
  using BaseType::rhs_matrix_is_given_;
  using BaseType::expected_eigenvalues_;
  using BaseType::expected_real_eigenvalues_;
  using BaseType::expected_max_ev_;
  using BaseType::expected_min_ev_;
}; // struct GeneralizedEigenSolverForMatrixPairWithPositiveEigenvalues_{{T_NAME}}


struct GeneralizedEigenSolverForMatrixPairWithNegativeEigenvalues_{{T_NAME}}
    : public GeneralizedEigenSolverTestForMatricesWithRealEigenvaluesAndVectors<{{TESTMATRIXTYPE}},
                                                                     {{TESTFIELDTYPE}},
                                                                     {{TESTCOMPLEXMATRIXTYPE}},
                                                                     {{TESTREALMATRIXTYPE}}>
{
  using BaseType = GeneralizedEigenSolverTestForMatricesWithRealEigenvaluesAndVectors;
  using typename BaseType::MatrixType;
  using typename BaseType::EigenValuesType;
  using typename BaseType::RealEigenValuesType;

  GeneralizedEigenSolverForMatrixPairWithNegativeEigenvalues_{{T_NAME}}()
  {
    matrix_ = XT::Common::from_string<MatrixType>("[-3 1; 1 -3]");
    rhs_matrix_ = XT::Common::from_string<MatrixType>("[2 0; 0 2]");
    rhs_matrix_is_given_ = true;
    expected_eigenvalues_ = XT::Common::from_string<EigenValuesType>("[-2 -1]");
    expected_real_eigenvalues_ = XT::Common::from_string<RealEigenValuesType>("[-2 -1]");
    expected_max_ev_ = -1;
    expected_min_ev_ = -2;
    all_matrices_and_expected_eigenvalues_and_vectors_are_computed_ = true;
  }

  using BaseType::all_matrices_and_expected_eigenvalues_and_vectors_are_computed_;
  using BaseType::matrix_;
  using BaseType::rhs_matrix_;
  using BaseType::rhs_matrix_is_given_;
  using BaseType::expected_eigenvalues_;
  using BaseType::expected_real_eigenvalues_;
  using BaseType::expected_max_ev_;
  using BaseType::expected_min_ev_;
}; // struct GeneralizedEigenSolverForMatrixPairWithNegativeEigenvalues_{{T_NAME}}


TEST_F(GeneralizedEigenSolverForMatrixPairWithPositiveEigenvalues_{{T_NAME}}, is_constructible)
{
  is_constructible();
}

TEST_F(GeneralizedEigenSolverForMatrixPairWithPositiveEigenvalues_{{T_NAME}}, gives_correct_eigenvalues)
{
  gives_correct_eigenvalues();
}

TEST_F(GeneralizedEigenSolverForMatrixPairWithPositiveEigenvalues_{{T_NAME}},
       gives_correct_eigenvalues_via_free_functions)
{
  gives_correct_eigenvalues_via_free_functions();
}

TEST_F(GeneralizedEigenSolverForMatrixPairWithPositiveEigenvalues_{{T_NAME}}, gives_correct_real_eigenvalues)
{
  gives_correct_real_eigenvalues();
}

TEST_F(GeneralizedEigenSolverForMatrixPairWithPositiveEigenvalues_{{T_NAME}}, gives_correct_max_eigenvalue)
{
  gives_correct_max_eigenvalue();
}

TEST_F(GeneralizedEigenSolverForMatrixPairWithPositiveEigenvalues_{{T_NAME}}, gives_correct_min_eigenvalue)
{
  gives_correct_min_eigenvalue();
}

TEST_F(GeneralizedEigenSolverForMatrixPairWithPositiveEigenvalues_{{T_NAME}}, gives_correct_extremal_eigenvalues)
{
  gives_correct_extremal_eigenvalues();
}

TEST_F(GeneralizedEigenSolverForMatrixPairWithPositiveEigenvalues_{{T_NAME}}, checks_eigenvalue_assertions)
{
  checks_eigenvalue_assertions();
}

TEST_F(GeneralizedEigenSolverForMatrixPairWithPositiveEigenvalues_{{T_NAME}},
       computes_eigenvalues_required_for_assertions)
{
  computes_eigenvalues_required_for_assertions();
}

TEST_F(GeneralizedEigenSolverForMatrixPairWithNegativeEigenvalues_{{T_NAME}}, is_constructible)
{
  is_constructible();
}

TEST_F(GeneralizedEigenSolverForMatrixPairWithNegativeEigenvalues_{{T_NAME}}, gives_correct_eigenvalues)
{
  gives_correct_eigenvalues();
}

TEST_F(GeneralizedEigenSolverForMatrixPairWithNegativeEigenvalues_{{T_NAME}},
       gives_correct_eigenvalues_via_free_functions)
{
  gives_correct_eigenvalues_via_free_functions();
}

TEST_F(GeneralizedEigenSolverForMatrixPairWithNegativeEigenvalues_{{T_NAME}}, gives_correct_real_eigenvalues)
{
  gives_correct_real_eigenvalues();
}

TEST_F(GeneralizedEigenSolverForMatrixPairWithNegativeEigenvalues_{{T_NAME}}, gives_correct_max_eigenvalue)
{
  gives_correct_max_eigenvalue();
}

TEST_F(GeneralizedEigenSolverForMatrixPairWithNegativeEigenvalues_{{T_NAME}}, gives_correct_min_eigenvalue)
{
  gives_correct_min_eigenvalue();
}

TEST_F(GeneralizedEigenSolverForMatrixPairWithNegativeEigenvalues_{{T_NAME}}, gives_correct_extremal_eigenvalues)
{
  gives_correct_extremal_eigenvalues();
}

TEST_F(GeneralizedEigenSolverForMatrixPairWithNegativeEigenvalues_{{T_NAME}}, checks_eigenvalue_assertions)
{
  checks_eigenvalue_assertions();
}

TEST_F(GeneralizedEigenSolverForMatrixPairWithNegativeEigenvalues_{{T_NAME}},
       computes_eigenvalues_required_for_assertions)
{
  computes_eigenvalues_required_for_assertions();
}

{% endfor %}
