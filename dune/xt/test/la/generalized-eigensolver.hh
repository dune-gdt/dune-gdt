// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2017, 2019)
//   René Fritze     (2018 - 2019)
//   Tim Keil        (2018)
//   Tobias Leibner  (2018, 2020)

#ifndef DUNE_XT_LA_TEST_GENERALIZED_EIGENSOLVER_HH
#define DUNE_XT_LA_TEST_GENERALIZED_EIGENSOLVER_HH

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include <dune/xt/common/exceptions.hh>
#include <dune/xt/common/logging.hh>
#include <gtest/gtest.h>

#include <dune/xt/la/container.hh>
#include <dune/xt/la/container/conversion.hh>
#include <dune/xt/la/container/eye-matrix.hh>
#include <dune/xt/la/generalized-eigen-solver.hh>

using namespace Dune;
using namespace Dune::XT;
using namespace Dune::XT::LA;


template <class MatrixImp, class F, class C, class R>
struct GeneralizedEigenSolverTest : public ::testing::Test
{
  using MatrixType = MatrixImp;
  using FieldType = Common::real_t<F>;
  using ComplexMatrixType = C;
  using RealMatrixType = R;
  using RealType = Common::real_t<FieldType>;
  using EigenValuesType = std::vector<Common::complex_t<FieldType>>;
  using RealEigenValuesType = std::vector<Common::real_t<FieldType>>;
  using GeneralizedEigenSolverType = GeneralizedEigenSolver<MatrixType>;
  using GeneralizedEigenSolverOpts = GeneralizedEigenSolverOptions<MatrixType>;

  using M = Common::MatrixAbstraction<MatrixType>;

  GeneralizedEigenSolverTest()
    : broken_matrix_(M::create(M::has_static_size ? M::static_rows : 1, M::has_static_size ? M::static_cols : 1))
    , unit_matrix_(
          eye_matrix<MatrixType>(M::has_static_size ? M::static_rows : 1, M::has_static_size ? M::static_cols : 1))
  {
    M::set_entry(broken_matrix_, 0, 0, std::numeric_limits<typename M::S>::infinity());
  }

  /**
   * \brief (Re-)creates unit_matrix_ with the size of matrix_ and, unless the derived test provided one, uses it as
   *        the right hand side of the generalized eigenvalue problem.
   *
   * Deriving tests that set rhs_matrix_ (and rhs_matrix_is_given_) in their constructor solve a genuine generalized
   * problem lhs*v = lambda*rhs*v; all others fall back to rhs = Id, which reduces to the standard problem.
   */
  void make_unit_matrix()
  {
    ASSERT_TRUE(all_matrices_and_expected_eigenvalues_and_vectors_are_computed_);
    unit_matrix_ = eye_matrix<MatrixType>(M::has_static_size ? M::static_rows : M::rows(matrix_),
                                          M::has_static_size ? M::static_cols : M::cols(matrix_));
    if (!rhs_matrix_is_given_)
      rhs_matrix_ = unit_matrix_;
  }

  /// \brief Creates a rows x cols matrix filled with zeros, in the same way the fixture creates broken_matrix_.
  static MatrixType make_matrix(const size_t rows, const size_t cols)
  {
    return M::create(rows, cols, typename M::S(0));
  }

  static void exports_correct_types()
  {
    const bool MatrixType_is_correct = std::is_same<typename GeneralizedEigenSolverType::MatrixType, MatrixType>::value;
    EXPECT_TRUE(MatrixType_is_correct);
    const bool RealType_is_correct = std::is_same<typename GeneralizedEigenSolverType::RealType, RealType>::value;
    EXPECT_TRUE(RealType_is_correct);
    const bool RealMatrixType_is_correct =
        std::is_same<typename GeneralizedEigenSolverType::RealMatrixType, RealMatrixType>::value;
    EXPECT_TRUE(RealMatrixType_is_correct);
    const bool ComplexMatrixType_is_correct =
        std::is_same<typename GeneralizedEigenSolverType::ComplexMatrixType, ComplexMatrixType>::value;
    EXPECT_TRUE(ComplexMatrixType_is_correct);
  } // ... exports_correct_types()

  static void has_types_and_options()
  {
    std::vector<std::string> types = GeneralizedEigenSolverOpts::types();
    EXPECT_GT(types.size(), 0);
    Common::Configuration opts = GeneralizedEigenSolverOpts::options();
    EXPECT_TRUE(opts.has_key("type"));
    EXPECT_EQ(types[0], opts.get<std::string>("type"));
    for (const auto& tp : types) {
      Common::Configuration tp_opts = GeneralizedEigenSolverOpts::options(tp);
      EXPECT_TRUE(tp_opts.has_key("type"));
      EXPECT_EQ(tp, tp_opts.get<std::string>("type"));
    }
  } // ... has_types_and_options(...)

  void throws_on_broken_matrix_construction()
  {
    try {
      [[maybe_unused]] GeneralizedEigenSolverType default_solver{broken_matrix_, unit_matrix_};
      FAIL() << "Expected LA::Exceptions::generalized_eigen_solver_failed_bc_data_did_not_fulfill_requirements";
    } catch (const LA::Exceptions::generalized_eigen_solver_failed_bc_data_did_not_fulfill_requirements& /*ee*/) {
    } catch (...) {
      FAIL() << "Expected LA::Exceptions::generalized_eigen_solver_failed_bc_data_did_not_fulfill_requirements";
    }
    for (const auto& tp : GeneralizedEigenSolverOpts::types()) {
      try {
        [[maybe_unused]] GeneralizedEigenSolverType default_opts_solver(broken_matrix_, unit_matrix_, tp);
        FAIL() << "Expected LA::Exceptions::generalized_eigen_solver_failed_bc_data_did_not_fulfill_requirements";
      } catch (const LA::Exceptions::generalized_eigen_solver_failed_bc_data_did_not_fulfill_requirements& /*ee*/) {
      } catch (...) {
        FAIL() << "Expected LA::Exceptions::generalized_eigen_solver_failed_bc_data_did_not_fulfill_requirements";
      }
      try {
        [[maybe_unused]] GeneralizedEigenSolverType solver(
            broken_matrix_, unit_matrix_, GeneralizedEigenSolverOpts::options(tp));
        FAIL() << "Expected LA::Exceptions::generalized_eigen_solver_failed_bc_data_did_not_fulfill_requirements";
      } catch (const LA::Exceptions::generalized_eigen_solver_failed_bc_data_did_not_fulfill_requirements& /*ee*/) {
      } catch (...) {
        FAIL() << "Expected LA::Exceptions::generalized_eigen_solver_failed_bc_data_did_not_fulfill_requirements";
      }
    }
  } // ... throws_on_broken_matrix_construction(...)

  /// \brief The counterpart of throws_on_broken_matrix_construction() for the right hand side matrix.
  void throws_on_broken_rhs_matrix_construction()
  {
    // note: broken_matrix_ and unit_matrix_ are of matching size here as long as make_unit_matrix() was not called
    try {
      [[maybe_unused]] GeneralizedEigenSolverType default_solver{unit_matrix_, broken_matrix_};
      FAIL() << "Expected LA::Exceptions::generalized_eigen_solver_failed_bc_data_did_not_fulfill_requirements";
    } catch (const LA::Exceptions::generalized_eigen_solver_failed_bc_data_did_not_fulfill_requirements& /*ee*/) {
    } catch (...) {
      FAIL() << "Expected LA::Exceptions::generalized_eigen_solver_failed_bc_data_did_not_fulfill_requirements";
    }
    for (const auto& tp : GeneralizedEigenSolverOpts::types()) {
      try {
        [[maybe_unused]] GeneralizedEigenSolverType solver(
            unit_matrix_, broken_matrix_, GeneralizedEigenSolverOpts::options(tp));
        FAIL() << "Expected LA::Exceptions::generalized_eigen_solver_failed_bc_data_did_not_fulfill_requirements";
      } catch (const LA::Exceptions::generalized_eigen_solver_failed_bc_data_did_not_fulfill_requirements& /*ee*/) {
      } catch (...) {
        FAIL() << "Expected LA::Exceptions::generalized_eigen_solver_failed_bc_data_did_not_fulfill_requirements";
      }
    }
  } // ... throws_on_broken_rhs_matrix_construction(...)

  void allows_broken_matrix_construction_when_checks_disabled()
  {
    for (const auto& tp : GeneralizedEigenSolverOpts::types()) {
      Common::Configuration opts_with_disabled_check = GeneralizedEigenSolverOpts::options(tp);
      opts_with_disabled_check["check_for_inf_nan"] = "false";
      [[maybe_unused]] GeneralizedEigenSolverType solver(broken_matrix_, unit_matrix_, opts_with_disabled_check);
      [[maybe_unused]] GeneralizedEigenSolverType rhs_solver(unit_matrix_, broken_matrix_, opts_with_disabled_check);
    }
  }

  /// \brief 'disable_checks' skips pre_checks()/post_checks() as a whole, not just the inf/nan check.
  void allows_broken_matrix_construction_when_all_checks_disabled()
  {
    for (const auto& tp : GeneralizedEigenSolverOpts::types()) {
      Common::Configuration opts_with_disabled_checks = GeneralizedEigenSolverOpts::options(tp);
      opts_with_disabled_checks["disable_checks"] = "true";
      [[maybe_unused]] GeneralizedEigenSolverType solver(broken_matrix_, unit_matrix_, opts_with_disabled_checks);
    }
  }

  /// \brief Covers the 'Missing type in given options' branch of GeneralizedEigenSolverBase::pre_checks().
  void throws_on_missing_type_in_options()
  {
    Common::Configuration opts_without_type;
    opts_without_type["compute_eigenvalues"] = "true";
    try {
      [[maybe_unused]] GeneralizedEigenSolverType solver(unit_matrix_, unit_matrix_, opts_without_type);
      FAIL() << "Expected LA::Exceptions::generalized_eigen_solver_failed_bc_it_was_not_set_up_correctly";
    } catch (const LA::Exceptions::generalized_eigen_solver_failed_bc_it_was_not_set_up_correctly& /*ee*/) {
    } catch (...) {
      FAIL() << "Expected LA::Exceptions::generalized_eigen_solver_failed_bc_it_was_not_set_up_correctly";
    }
  } // ... throws_on_missing_type_in_options(...)

  /**
   * \brief Covers internal::ensure_generalized_eigen_solver_type() on all three ways an unknown type can enter:
   *        via the options factory, via the type-string ctor and via an already assembled configuration.
   */
  void throws_on_unknown_type()
  {
    const std::string unknown_type = "this_generalized_eigen_solver_type_does_not_exist";
    try {
      [[maybe_unused]] Common::Configuration opts = GeneralizedEigenSolverOpts::options(unknown_type);
      FAIL() << "Expected LA::Exceptions::generalized_eigen_solver_failed_bc_it_was_not_set_up_correctly";
    } catch (const LA::Exceptions::generalized_eigen_solver_failed_bc_it_was_not_set_up_correctly& /*ee*/) {
    } catch (...) {
      FAIL() << "Expected LA::Exceptions::generalized_eigen_solver_failed_bc_it_was_not_set_up_correctly";
    }
    try {
      [[maybe_unused]] GeneralizedEigenSolverType solver(unit_matrix_, unit_matrix_, unknown_type);
      FAIL() << "Expected LA::Exceptions::generalized_eigen_solver_failed_bc_it_was_not_set_up_correctly";
    } catch (const LA::Exceptions::generalized_eigen_solver_failed_bc_it_was_not_set_up_correctly& /*ee*/) {
    } catch (...) {
      FAIL() << "Expected LA::Exceptions::generalized_eigen_solver_failed_bc_it_was_not_set_up_correctly";
    }
    for (const auto& tp : GeneralizedEigenSolverOpts::types()) {
      Common::Configuration opts_with_unknown_type = GeneralizedEigenSolverOpts::options(tp);
      opts_with_unknown_type["type"] = unknown_type;
      try {
        [[maybe_unused]] GeneralizedEigenSolverType solver(unit_matrix_, unit_matrix_, opts_with_unknown_type);
        FAIL() << "Expected LA::Exceptions::generalized_eigen_solver_failed_bc_it_was_not_set_up_correctly";
      } catch (const LA::Exceptions::generalized_eigen_solver_failed_bc_it_was_not_set_up_correctly& /*ee*/) {
      } catch (...) {
        FAIL() << "Expected LA::Exceptions::generalized_eigen_solver_failed_bc_it_was_not_set_up_correctly";
      }
    }
  } // ... throws_on_unknown_type(...)

  /**
   * \brief With the checks disabled an unknown type reaches compute(), where it must not be silently ignored.
   *
   * This is the only way to reach the final else branch of GeneralizedEigenSolver<..., true>::compute(), which
   * pre_checks() otherwise guarantees to be unreachable.
   */
  void throws_on_unknown_type_when_checks_are_disabled()
  {
    for (const auto& tp : GeneralizedEigenSolverOpts::types()) {
      Common::Configuration opts = GeneralizedEigenSolverOpts::options(tp);
      opts["type"] = "this_generalized_eigen_solver_type_does_not_exist";
      opts["disable_checks"] = "true";
      GeneralizedEigenSolverType solver(unit_matrix_, unit_matrix_, opts);
      try {
        solver.eigenvalues();
        FAIL() << "Expected Common::Exceptions::internal_error";
      } catch (const Common::Exceptions::internal_error& /*ee*/) {
      } catch (...) {
        FAIL() << "Expected Common::Exceptions::internal_error";
      }
    }
  } // ... throws_on_unknown_type_when_checks_are_disabled(...)

  /// \brief Covers the 'non-positive tolerance' branch of GeneralizedEigenSolverBase::pre_checks().
  void throws_on_non_positive_real_tolerance()
  {
    for (const auto& tp : GeneralizedEigenSolverOpts::types()) {
      for (const std::string& tolerance : {"0", "-1e-15"}) {
        Common::Configuration opts = GeneralizedEigenSolverOpts::options(tp);
        opts["real_tolerance"] = tolerance;
        try {
          [[maybe_unused]] GeneralizedEigenSolverType solver(unit_matrix_, unit_matrix_, opts);
          FAIL() << "Expected LA::Exceptions::generalized_eigen_solver_failed_bc_it_was_not_set_up_correctly"
                 << "\n\nreal_tolerance: " << tolerance;
        } catch (const LA::Exceptions::generalized_eigen_solver_failed_bc_it_was_not_set_up_correctly& /*ee*/) {
        } catch (...) {
          FAIL() << "Expected LA::Exceptions::generalized_eigen_solver_failed_bc_it_was_not_set_up_correctly"
                 << "\n\nreal_tolerance: " << tolerance;
        }
      }
    }
  } // ... throws_on_non_positive_real_tolerance(...)

  /**
   * \brief Covers the three size checks in GeneralizedEigenSolverBase::check_size().
   *
   * Only meaningful for matrix types of dynamic size; for statically sized ones (FieldMatrix) a non-square or
   * mismatched pair cannot even be formed, so there is nothing to check.
   */
  void throws_on_matrices_of_wrong_size()
  {
    if constexpr (!M::has_static_size) {
      const auto square = make_matrix(2, 2);
      const auto non_square = make_matrix(2, 3);
      const auto larger_square = make_matrix(3, 3);
      const std::vector<std::pair<MatrixType, MatrixType>> broken_pairs{
          {non_square, square}, // <- lhs is not square
          {square, larger_square}, // <- rhs has the wrong size
          {square, non_square} // <- rhs is not square
      };
      for (const auto& pair : broken_pairs) {
        try {
          [[maybe_unused]] GeneralizedEigenSolverType solver(pair.first, pair.second);
          FAIL() << "Expected LA::Exceptions::generalized_eigen_solver_failed_bc_data_did_not_fulfill_requirements"
                 << "\n\nlhs: " << pair.first << "\n\nrhs: " << pair.second;
        } catch (const LA::Exceptions::generalized_eigen_solver_failed_bc_data_did_not_fulfill_requirements& /*ee*/) {
        } catch (...) {
          FAIL() << "Expected LA::Exceptions::generalized_eigen_solver_failed_bc_data_did_not_fulfill_requirements"
                 << "\n\nlhs: " << pair.first << "\n\nrhs: " << pair.second;
        }
      }
    }
  } // ... throws_on_matrices_of_wrong_size(...)

  void throws_on_inconsistent_given_options()
  {
    for (const auto& tp : GeneralizedEigenSolverOpts::types()) {
      Common::Configuration inconsistent_opts = GeneralizedEigenSolverOpts::options(tp);
      inconsistent_opts["assert_positive_eigenvalues"] = "1e-15";
      inconsistent_opts["assert_negative_eigenvalues"] = "1e-15";
      try {
        [[maybe_unused]] GeneralizedEigenSolverType solver(unit_matrix_, unit_matrix_, inconsistent_opts);
        FAIL() << "Expected LA::Exceptions::generalized_eigen_solver_failed_bc_it_was_not_set_up_correctly";
      } catch (const LA::Exceptions::generalized_eigen_solver_failed_bc_it_was_not_set_up_correctly& /*ee*/) {
      } catch (...) {
        FAIL() << "Expected LA::Exceptions::generalized_eigen_solver_failed_bc_it_was_not_set_up_correctly";
      }
    }
  } // ... throws_on_inconsistent_given_options(...)

  void is_constructible()
  {
    ASSERT_TRUE(all_matrices_and_expected_eigenvalues_and_vectors_are_computed_);
    this->make_unit_matrix();
    [[maybe_unused]] GeneralizedEigenSolverType default_solver{matrix_, rhs_matrix_};
    for (const auto& tp : GeneralizedEigenSolverOpts::types()) {
      [[maybe_unused]] GeneralizedEigenSolverType default_opts_solver(matrix_, rhs_matrix_, tp);
      [[maybe_unused]] GeneralizedEigenSolverType solver(matrix_, rhs_matrix_, GeneralizedEigenSolverOpts::options(tp));
    }
  } // ... is_constructible(...)

  /**
   * \brief Covers the third GeneralizedEigenSolverBase ctor, which keeps a pointer to externally owned options.
   *
   * That ctor exists for hot loops (see dune/gdt/operators/reconstruction/internal.hh, which keeps a static
   * Configuration around); unlike the by-value ctor it writes the missing defaults back into the caller's options.
   */
  void is_constructible_from_options_pointer()
  {
    ASSERT_TRUE(all_matrices_and_expected_eigenvalues_and_vectors_are_computed_);
    this->make_unit_matrix();
    for (const auto& tp : GeneralizedEigenSolverOpts::types()) {
      Common::Configuration opts = GeneralizedEigenSolverOpts::options(tp);
      GeneralizedEigenSolverType solver(matrix_, rhs_matrix_, &opts);
      EXPECT_EQ(tp, solver.options().template get<std::string>("type"));
      // the solver must not have copied the options, but refer to ours
      EXPECT_EQ(&opts, &solver.options());
      EXPECT_EQ(Common::get_matrix_rows(matrix_), solver.eigenvalues().size());
    }
  } // ... is_constructible_from_options_pointer(...)

  /// \brief Covers the lhs_matrix()/rhs_matrix() accessors of GeneralizedEigenSolverBase.
  void exposes_the_given_matrices()
  {
    ASSERT_TRUE(all_matrices_and_expected_eigenvalues_and_vectors_are_computed_);
    this->make_unit_matrix();
    for (const auto& tp : GeneralizedEigenSolverOpts::types()) {
      GeneralizedEigenSolverType solver(matrix_, rhs_matrix_, tp);
      EXPECT_EQ(&matrix_, &solver.lhs_matrix());
      EXPECT_EQ(&rhs_matrix_, &solver.rhs_matrix());
      EXPECT_EQ(tp, solver.options().template get<std::string>("type"));
    }
  } // ... exposes_the_given_matrices(...)

  static bool find_ev(const EigenValuesType& expected_evs,
                      const XT::Common::complex_t<FieldType>& actual_ev,
                      const double& tolerance)
  {
    for (const auto& expected_ev : expected_evs) {
      if (Common::FloatCmp::eq(actual_ev, expected_ev, {tolerance, tolerance}))
        return true;
    }
    return false;
  }

  static bool find_ev(const RealEigenValuesType& expected_evs, const RealType& actual_ev, const double& tolerance)
  {
    for (const auto& expected_ev : expected_evs) {
      if (Common::FloatCmp::eq(actual_ev, expected_ev, tolerance))
        return true;
    }
    return false;
  }

  void gives_correct_eigenvalues(const Common::Configuration& tolerances = {})
  {
    ASSERT_TRUE(all_matrices_and_expected_eigenvalues_and_vectors_are_computed_);
    this->make_unit_matrix();
    for (const auto& tp : GeneralizedEigenSolverOpts::types()) {
      const double tolerance = tolerances.get(tp, 1e-15);
      GeneralizedEigenSolverType solver(matrix_, rhs_matrix_, tp);
      try {
        const auto& eigenvalues = solver.eigenvalues();
        EXPECT_EQ(Common::get_matrix_rows(matrix_), eigenvalues.size());
        for (const auto& ev : eigenvalues) {
          EXPECT_TRUE(find_ev(expected_eigenvalues_, ev, tolerance))
              << "\n\nactual eigenvalue: " << ev << "\n\nexpected eigenvalues: " << expected_eigenvalues_
              << "\n\ntype: " << tp << "\n\ntolerance: " << tolerance;
        }
      } catch (const Dune::MathError&) {
        if (tolerance > 0) {
          FAIL() << "Dune::MathError thrown when trying to get eigenvalues!"
                 << "\n\ntype: " << tp << "\n\ntolerance: " << tolerance;
        }
      }
    }
  } // ... gives_correct_eigenvalues(...)

  /// \brief The free functions from dune/xt/la/generalized-eigen-solver.hh have to agree with the class interface.
  void gives_correct_eigenvalues_via_free_functions(const Common::Configuration& tolerances = {})
  {
    ASSERT_TRUE(all_matrices_and_expected_eigenvalues_and_vectors_are_computed_);
    this->make_unit_matrix();
    EXPECT_EQ(GeneralizedEigenSolverOpts::types(), generalized_eigen_solver_types(matrix_));
    EXPECT_EQ(GeneralizedEigenSolverOpts::options(), generalized_eigen_solver_options(matrix_));
    for (const auto& tp : GeneralizedEigenSolverOpts::types()) {
      const double tolerance = tolerances.get(tp, 1e-15);
      const auto opts = generalized_eigen_solver_options(matrix_, tp);
      EXPECT_EQ(tp, opts.template get<std::string>("type"));
      const auto solver_from_type = make_generalized_eigen_solver(matrix_, rhs_matrix_, tp);
      const auto solver_from_opts = make_generalized_eigen_solver(matrix_, rhs_matrix_, opts);
      for (const auto* solver : {&solver_from_type, &solver_from_opts}) {
        const auto& eigenvalues = solver->eigenvalues();
        EXPECT_EQ(Common::get_matrix_rows(matrix_), eigenvalues.size());
        for (const auto& ev : eigenvalues)
          EXPECT_TRUE(find_ev(expected_eigenvalues_, ev, tolerance))
              << "\n\nactual eigenvalue: " << ev << "\n\nexpected eigenvalues: " << expected_eigenvalues_
              << "\n\ntype: " << tp << "\n\ntolerance: " << tolerance;
      }
    }
  } // ... gives_correct_eigenvalues_via_free_functions(...)

  /**
   * \brief Eigenvectors of generalized eigenvalue problems are not implemented (yet).
   *
   * Asking for them up front makes compute() fail, while asking for them after a successful eigenvalue computation
   * has to be reported as a usage error instead of returning an empty matrix.
   */
  void throws_on_request_for_eigenvectors()
  {
    ASSERT_TRUE(all_matrices_and_expected_eigenvalues_and_vectors_are_computed_);
    this->make_unit_matrix();
    for (const auto& tp : GeneralizedEigenSolverOpts::types()) {
      Common::Configuration opts_with_eigenvectors = GeneralizedEigenSolverOpts::options(tp);
      opts_with_eigenvectors["compute_eigenvectors"] = "true";
      GeneralizedEigenSolverType eigenvector_solver(matrix_, rhs_matrix_, opts_with_eigenvectors);
      try {
        eigenvector_solver.eigenvalues();
        FAIL() << "Expected LA::Exceptions::generalized_eigen_solver_failed";
      } catch (const LA::Exceptions::generalized_eigen_solver_failed& /*ee*/) {
      } catch (...) {
        FAIL() << "Expected LA::Exceptions::generalized_eigen_solver_failed";
      }
      GeneralizedEigenSolverType solver(matrix_, rhs_matrix_, tp);
      EXPECT_EQ(Common::get_matrix_rows(matrix_), solver.eigenvalues().size());
      try {
        solver.eigenvectors();
        FAIL() << "Expected Common::Exceptions::you_are_using_this_wrong";
      } catch (const Common::Exceptions::you_are_using_this_wrong& /*ee*/) {
      } catch (...) {
        FAIL() << "Expected Common::Exceptions::you_are_using_this_wrong";
      }
      try {
        solver.real_eigenvectors();
        FAIL() << "Expected Common::Exceptions::you_are_using_this_wrong";
      } catch (const Common::Exceptions::you_are_using_this_wrong& /*ee*/) {
      } catch (...) {
        FAIL() << "Expected Common::Exceptions::you_are_using_this_wrong";
      }
    }
  } // ... throws_on_request_for_eigenvectors(...)

  /**
   * \brief lapack's dsygv requires the right hand side matrix to be positive definite.
   *
   * A zero right hand side passes all of our own checks (it is square, of matching size and free of inf/nan), so the
   * failure has to be reported by the backend error handling of the lapack based implementation.
   */
  void throws_on_indefinite_rhs_matrix()
  {
    ASSERT_TRUE(all_matrices_and_expected_eigenvalues_and_vectors_are_computed_);
    this->make_unit_matrix();
    const auto zero_matrix = make_matrix(Common::get_matrix_rows(matrix_), Common::get_matrix_cols(matrix_));
    for (const auto& tp : GeneralizedEigenSolverOpts::types()) {
      GeneralizedEigenSolverType solver(matrix_, zero_matrix, tp);
      try {
        solver.eigenvalues();
        FAIL() << "Expected LA::Exceptions::generalized_eigen_solver_failed"
               << "\n\ntype: " << tp;
      } catch (const LA::Exceptions::generalized_eigen_solver_failed& /*ee*/) {
      } catch (...) {
        FAIL() << "Expected LA::Exceptions::generalized_eigen_solver_failed"
               << "\n\ntype: " << tp;
      }
    }
  } // ... throws_on_indefinite_rhs_matrix(...)

  bool all_matrices_and_expected_eigenvalues_and_vectors_are_computed_{false};
  MatrixType broken_matrix_;
  MatrixType unit_matrix_;
  MatrixType matrix_;
  MatrixType rhs_matrix_;
  bool rhs_matrix_is_given_{false};
  EigenValuesType expected_eigenvalues_;
}; // ... struct GeneralizedEigenSolverTest


template <class M, class F, class C, class R>
struct GeneralizedEigenSolverTestForMatricesWithRealEigenvaluesAndVectors
  : public GeneralizedEigenSolverTest<M, F, C, R>
{
  using BaseType = GeneralizedEigenSolverTest<M, F, C, R>;
  using BaseType::find_ev;
  using typename BaseType::ComplexMatrixType;
  using typename BaseType::EigenValuesType;
  using typename BaseType::FieldType;
  using typename BaseType::GeneralizedEigenSolverOpts;
  using typename BaseType::GeneralizedEigenSolverType;
  using typename BaseType::MatrixType;
  using typename BaseType::RealEigenValuesType;
  using typename BaseType::RealMatrixType;
  using typename BaseType::RealType;

  void gives_correct_real_eigenvalues(const Common::Configuration& tolerances = {})
  {
    ASSERT_TRUE(all_matrices_and_expected_eigenvalues_and_vectors_are_computed_);
    this->make_unit_matrix();
    for (const auto& tp : GeneralizedEigenSolverOpts::types()) {
      const double tolerance = tolerances.get(tp, 1e-15);
      GeneralizedEigenSolverType solver(matrix_, rhs_matrix_, tp);
      const auto& eigenvalues = solver.eigenvalues();
      EXPECT_EQ(Common::get_matrix_rows(matrix_), eigenvalues.size());
      for (const auto& complex_ev : eigenvalues) {
        if (tolerance > 0)
          EXPECT_TRUE(Common::FloatCmp::eq(0., complex_ev.imag(), tolerance))
              << "\n  type: " << tp << "\n  tolerance: " << tolerance;
        else {
          // negative tolerance: we expect a failure
          EXPECT_FALSE(Common::FloatCmp::eq(0., complex_ev.imag()))
              << "\n\nTHIS IS A GOOD THING! UPDATE THE EXPECTATIONS IN tolerances!\n\n"
              << "\n  type: " << tp;
        }
        const auto real_ev = complex_ev.real();
        if (tolerance > 0)
          EXPECT_TRUE(find_ev(expected_real_eigenvalues_, real_ev, tolerance))
              << "\n\nactual eigenvalue: " << real_ev << "\n\nexpected eigenvalues: " << expected_real_eigenvalues_
              << "\n\ntype: " << tp << "\n\ntolerance: " << tolerance;
        else {
          // negative tolerance: we expect a failure
          EXPECT_FALSE(find_ev(expected_real_eigenvalues_, real_ev, 1e-15))
              << "\n\nTHIS IS A GOOD THING! UPDATE THE EXPECTATIONS IN tolerances!\n\n"
              << "\n\nactual eigenvalue: " << real_ev << "\n\nexpected eigenvalues: " << expected_real_eigenvalues_
              << "\n\ntype: " << tp;
        }
      }
      if (tolerance > 0) {
        const auto& real_eigenvalues = solver.real_eigenvalues();
        EXPECT_EQ(Common::get_matrix_rows(matrix_), real_eigenvalues.size());
        for (const auto& real_ev : real_eigenvalues) {
          EXPECT_TRUE(find_ev(expected_real_eigenvalues_, real_ev, tolerance))
              << "\n\nactual eigenvalue: " << real_ev << "\n\nexpected eigenvalues: " << expected_real_eigenvalues_
              << "\n\ntype: " << tp << "\n\ntolerance: " << tolerance;
        }
      } else {
        // negative tolerance: we expect a failure
        const auto& real_eigenvalues = solver.real_eigenvalues(); /// \todo: Add try/catch around this, too!
        EXPECT_EQ(Common::get_matrix_rows(matrix_), real_eigenvalues.size());
        for (const auto& real_ev : real_eigenvalues) {
          EXPECT_FALSE(find_ev(expected_real_eigenvalues_, real_ev, 1e-15))
              << "\n\nTHIS IS A GOOD THING! UPDATE THE EXPECTATIONS IN tolerances!\n\n"
              << "\n\nactual eigenvalue: " << real_ev << "\n\nexpected eigenvalues: " << expected_real_eigenvalues_
              << "\n\ntype: " << tp;
        }
      }
    }
  } // ... gives_correct_real_eigenvalues(...)

  void gives_correct_max_eigenvalue(const Common::Configuration& tolerances = {})
  {
    ASSERT_TRUE(all_matrices_and_expected_eigenvalues_and_vectors_are_computed_);
    this->make_unit_matrix();
    for (const auto& tp : GeneralizedEigenSolverOpts::types()) {
      const double tolerance = tolerances.get(tp, 1e-15);
      GeneralizedEigenSolverType solver(matrix_, rhs_matrix_, tp);
      const auto actual_max_eigenvalues = solver.max_eigenvalues(1); /// \todo: Add try/catch around this, too!
      ASSERT_GE(1, actual_max_eigenvalues.size());
      if (tolerance > 0)
        EXPECT_TRUE(Common::FloatCmp::eq(expected_max_ev_, actual_max_eigenvalues[0], tolerance))
            << "\n\nactual max eigenvalue: " << actual_max_eigenvalues[0]
            << "\n\nexpected max eigenvalue: " << expected_max_ev_ << "\n\ntolerance: " << tolerance
            << "\n\ntype: " << tp;
      else {
        // negative tolerance: we expect a failure
        EXPECT_NE(expected_max_ev_, actual_max_eigenvalues[0])
            << "\n\nTHIS IS A GOOD THING! UPDATE THE EXPECTATIONS IN tolerances!\n\n"
            << "\n\nactual max eigenvalue: " << actual_max_eigenvalues[0]
            << "\n\nexpected max eigenvalue: " << expected_max_ev_ << "\n\ntype: " << tp;
      }
    }
  }

  void gives_correct_min_eigenvalue(const Common::Configuration& tolerances = {})
  {
    ASSERT_TRUE(all_matrices_and_expected_eigenvalues_and_vectors_are_computed_);
    this->make_unit_matrix();
    for (const auto& tp : GeneralizedEigenSolverOpts::types()) {
      const double tolerance = tolerances.get(tp, 1e-15);
      GeneralizedEigenSolverType solver(matrix_, rhs_matrix_, tp);
      const auto actual_min_eigenvalues = solver.min_eigenvalues(1); /// \todo: Add try/catch around this, too!
      ASSERT_GE(1, actual_min_eigenvalues.size());
      if (tolerance > 0)
        EXPECT_TRUE(Common::FloatCmp::eq(expected_min_ev_, actual_min_eigenvalues[0], tolerance))
            << "\n\nactual min eigenvalue: " << actual_min_eigenvalues[0]
            << "\n\nexpected min eigenvalue: " << expected_min_ev_ << "\n\ntolerance: " << tolerance
            << "\n\ntype: " << tp;
      else {
        // negative tolerance: we expect a failure
        EXPECT_NE(expected_min_ev_, actual_min_eigenvalues[0])
            << "\n\nTHIS IS A GOOD THING! UPDATE THE EXPECTATIONS IN tolerances!\n\n"
            << "\n\nactual min eigenvalue: " << actual_min_eigenvalues[0]
            << "\n\nexpected min eigenvalue: " << expected_min_ev_ << "\n\ntype: " << tp;
      }
    }
  }

  /**
   * \brief min_eigenvalues(k)/max_eigenvalues(k) for k != 1 and for k above the number of eigenvalues.
   *
   * Both accessors sort a copy of the real eigenvalues and truncate it to k, so the interesting cases are k in the
   * middle of the spectrum (does the truncation keep the right end?) and k beyond the spectrum (is the result
   * clamped instead of padded?).
   */
  void gives_correct_extremal_eigenvalues(const Common::Configuration& tolerances = {})
  {
    ASSERT_TRUE(all_matrices_and_expected_eigenvalues_and_vectors_are_computed_);
    this->make_unit_matrix();
    const size_t size = Common::get_matrix_rows(matrix_);
    RealEigenValuesType sorted_expected_eigenvalues = expected_real_eigenvalues_;
    std::sort(sorted_expected_eigenvalues.begin(), sorted_expected_eigenvalues.end());
    // the whole spectrum is indexed below, so an incompletely filled expectation has to fail here rather than run
    // past the end of the vector
    ASSERT_EQ(size, sorted_expected_eigenvalues.size())
        << "expected_real_eigenvalues_ has to hold one entry per row of matrix_!";
    for (const auto& tp : GeneralizedEigenSolverOpts::types()) {
      const double tolerance = tolerances.get(tp, 1e-15);
      if (tolerance <= 0)
        continue; // <- see gives_correct_min_eigenvalue()
      GeneralizedEigenSolverType solver(matrix_, rhs_matrix_, tp);
      for (size_t num_evs = 1; num_evs <= size; ++num_evs) {
        auto actual_min_eigenvalues = solver.min_eigenvalues(num_evs);
        auto actual_max_eigenvalues = solver.max_eigenvalues(num_evs);
        ASSERT_EQ(num_evs, actual_min_eigenvalues.size());
        ASSERT_EQ(num_evs, actual_max_eigenvalues.size());
        std::sort(actual_min_eigenvalues.begin(), actual_min_eigenvalues.end());
        std::sort(actual_max_eigenvalues.begin(), actual_max_eigenvalues.end());
        for (size_t ii = 0; ii < num_evs; ++ii) {
          EXPECT_TRUE(Common::FloatCmp::eq(sorted_expected_eigenvalues[ii], actual_min_eigenvalues[ii], tolerance))
              << "\n\nnum_evs: " << num_evs << "\n\nactual smallest eigenvalues: " << actual_min_eigenvalues
              << "\n\nexpected eigenvalues: " << sorted_expected_eigenvalues << "\n\ntype: " << tp;
          EXPECT_TRUE(Common::FloatCmp::eq(
              sorted_expected_eigenvalues[size - num_evs + ii], actual_max_eigenvalues[ii], tolerance))
              << "\n\nnum_evs: " << num_evs << "\n\nactual largest eigenvalues: " << actual_max_eigenvalues
              << "\n\nexpected eigenvalues: " << sorted_expected_eigenvalues << "\n\ntype: " << tp;
        }
      }
      // asking for more than there are has to yield all of them, and the default is exactly that
      EXPECT_EQ(size, solver.min_eigenvalues(size + 1).size());
      EXPECT_EQ(size, solver.max_eigenvalues(size + 1).size());
      EXPECT_EQ(size, solver.min_eigenvalues().size());
      EXPECT_EQ(size, solver.max_eigenvalues().size());
    }
  } // ... gives_correct_extremal_eigenvalues(...)

  /**
   * \brief Covers the assert_real/positive/negative_eigenvalues post checks, in both directions.
   *
   * Which of the sign assertions has to hold is derived from the expected spectrum, so the same check both confirms
   * the assertion for a matching test case and confirms that a violated assertion is reported for all others.
   */
  void checks_eigenvalue_assertions(const Common::Configuration& tolerances = {})
  {
    ASSERT_TRUE(all_matrices_and_expected_eigenvalues_and_vectors_are_computed_);
    this->make_unit_matrix();
    const std::string assertion_tolerance = "1e-10";
    const bool all_eigenvalues_are_positive = expected_min_ev_ > 1e-10;
    const bool all_eigenvalues_are_negative = expected_max_ev_ < -1e-10;
    for (const auto& tp : GeneralizedEigenSolverOpts::types()) {
      if (tolerances.get(tp, 1e-15) <= 0)
        continue; // <- see gives_correct_real_eigenvalues()
      // the eigenvalues of a symmetric-definite pair are real, so this assertion always holds
      Common::Configuration real_opts = GeneralizedEigenSolverOpts::options(tp);
      real_opts["assert_real_eigenvalues"] = assertion_tolerance;
      GeneralizedEigenSolverType real_solver(matrix_, rhs_matrix_, real_opts);
      EXPECT_EQ(Common::get_matrix_rows(matrix_), real_solver.real_eigenvalues().size());
      for (const bool assert_positive : {true, false}) {
        Common::Configuration opts = GeneralizedEigenSolverOpts::options(tp);
        opts[assert_positive ? "assert_positive_eigenvalues" : "assert_negative_eigenvalues"] = assertion_tolerance;
        // asserting a sign implies asserting real eigenvalues, which pre_checks() has to fill in for us
        GeneralizedEigenSolverType solver(matrix_, rhs_matrix_, opts);
        const bool assertion_holds = assert_positive ? all_eigenvalues_are_positive : all_eigenvalues_are_negative;
        if (assertion_holds) {
          EXPECT_EQ(Common::get_matrix_rows(matrix_), solver.real_eigenvalues().size())
              << "\n\ntype: " << tp << "\n\nassert_positive: " << assert_positive;
        } else if (assert_positive) {
          try {
            solver.eigenvalues();
            FAIL() << "Expected generalized_eigen_solver_failed_bc_eigenvalues_are_not_positive_as_requested"
                   << "\n\ntype: " << tp;
          } catch (const LA::Exceptions::
                       generalized_eigen_solver_failed_bc_eigenvalues_are_not_positive_as_requested& /*ee*/) {
          } catch (...) {
            FAIL() << "Expected generalized_eigen_solver_failed_bc_eigenvalues_are_not_positive_as_requested"
                   << "\n\ntype: " << tp;
          }
        } else {
          try {
            solver.eigenvalues();
            FAIL() << "Expected generalized_eigen_solver_failed_bc_eigenvalues_are_not_negative_as_requested"
                   << "\n\ntype: " << tp;
          } catch (const LA::Exceptions::
                       generalized_eigen_solver_failed_bc_eigenvalues_are_not_negative_as_requested& /*ee*/) {
          } catch (...) {
            FAIL() << "Expected generalized_eigen_solver_failed_bc_eigenvalues_are_not_negative_as_requested"
                   << "\n\ntype: " << tp;
          }
        }
      }
    }
  } // ... checks_eigenvalue_assertions(...)

  /**
   * \brief Any assertion on the eigenvalues has to switch their computation back on.
   *
   * 'compute_eigenvalues' defaults to true, so this only matters when it was explicitly disabled: pre_checks() then
   * has to re-enable it (there is nothing to assert otherwise) and, for the sign assertions, has to derive
   * 'assert_real_eigenvalues' from 'real_tolerance'.
   */
  void computes_eigenvalues_required_for_assertions(const Common::Configuration& tolerances = {})
  {
    ASSERT_TRUE(all_matrices_and_expected_eigenvalues_and_vectors_are_computed_);
    this->make_unit_matrix();
    const bool all_eigenvalues_are_positive = expected_min_ev_ > 1e-10;
    const bool all_eigenvalues_are_negative = expected_max_ev_ < -1e-10;
    for (const auto& tp : GeneralizedEigenSolverOpts::types()) {
      const double tolerance = tolerances.get(tp, 1e-15);
      if (tolerance <= 0)
        continue; // <- see gives_correct_real_eigenvalues()
      std::vector<std::string> assertions{"assert_real_eigenvalues"};
      if (all_eigenvalues_are_positive)
        assertions.emplace_back("assert_positive_eigenvalues");
      if (all_eigenvalues_are_negative)
        assertions.emplace_back("assert_negative_eigenvalues");
      for (const auto& assertion : assertions) {
        Common::Configuration opts = GeneralizedEigenSolverOpts::options(tp);
        opts["compute_eigenvalues"] = "false";
        opts[assertion] = "1e-10";
        GeneralizedEigenSolverType solver(matrix_, rhs_matrix_, opts);
        EXPECT_TRUE(solver.options().template get<bool>("compute_eigenvalues"))
            << "\n\nassertion: " << assertion << "\n\ntype: " << tp;
        EXPECT_GT(solver.options().template get<double>("assert_real_eigenvalues"), 0.)
            << "\n\nassertion: " << assertion << "\n\ntype: " << tp;
        const auto& real_eigenvalues = solver.real_eigenvalues();
        EXPECT_EQ(Common::get_matrix_rows(matrix_), real_eigenvalues.size());
        for (const auto& real_ev : real_eigenvalues)
          EXPECT_TRUE(find_ev(expected_real_eigenvalues_, real_ev, tolerance))
              << "\n\nactual eigenvalue: " << real_ev << "\n\nexpected eigenvalues: " << expected_real_eigenvalues_
              << "\n\nassertion: " << assertion << "\n\ntype: " << tp;
      }
    }
  } // ... computes_eigenvalues_required_for_assertions(...)

  using BaseType::all_matrices_and_expected_eigenvalues_and_vectors_are_computed_;
  using BaseType::expected_eigenvalues_;
  using BaseType::make_unit_matrix;
  using BaseType::matrix_;
  using BaseType::rhs_matrix_;
  using BaseType::rhs_matrix_is_given_;
  using BaseType::unit_matrix_;
  RealEigenValuesType expected_real_eigenvalues_;
  RealType expected_max_ev_;
  RealType expected_min_ev_;
}; // struct GeneralizedEigenSolverTestForMatricesWithRealEigenvaluesAndVectors


#endif // DUNE_XT_LA_TEST_GENERALIZED_EIGENSOLVER_HH
