// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Barbara Verfürth (2015)
//   Felix Schindler  (2013 - 2020)
//   René Fritze      (2014 - 2020)
//   Tobias Leibner   (2014 - 2015, 2018 - 2020)

#ifndef DUNE_XT_LA_SOLVER_ISTL_HH
#define DUNE_XT_LA_SOLVER_ISTL_HH

#include <type_traits>
#include <cmath>

#include <dune/istl/operators.hh>
#include <dune/istl/preconditioners.hh>
#include <dune/istl/solvers.hh>
#include <dune/istl/schwarz.hh>
#include <dune/istl/umfpack.hh>
#include <dune/istl/superlu.hh>

#include <dune/xt/common/exceptions.hh>
#include <dune/xt/common/configuration.hh>

#include <dune/xt/la/container/istl.hh>

#include "istl/amg.hh"
#include "istl/preconditioners.hh"
#include "../solver.hh"

namespace Dune::XT::LA {
namespace internal {


/**
 * \not
 **/
template <class S, class CommunicatorType>
struct IstlSolverTraits
{
  using IstlVectorType = typename IstlDenseVector<S>::BackendType;
  using IstlMatrixType = typename IstlRowMajorSparseMatrix<S>::BackendType;
  using MatrixOperatorType =
      OverlappingSchwarzOperator<IstlMatrixType, IstlVectorType, IstlVectorType, CommunicatorType>;
  using ScalarproductType = OverlappingSchwarzScalarProduct<IstlVectorType, CommunicatorType>;

  static MatrixOperatorType make_operator(const IstlMatrixType& matrix, const CommunicatorType& communicator)
  {
    return MatrixOperatorType(matrix, communicator);
  }

  static ScalarproductType make_scalarproduct(const CommunicatorType& communicator)
  {
    return ScalarproductType(communicator);
  }

  template <class SequentialPreconditionerType>
  static std::shared_ptr<
      BlockPreconditioner<IstlVectorType, IstlVectorType, CommunicatorType, SequentialPreconditionerType>>
  make_preconditioner(const std::shared_ptr<SequentialPreconditionerType>& seq_preconditioner,
                      const CommunicatorType& communicator)
  {
    return std::make_shared<
        BlockPreconditioner<IstlVectorType, IstlVectorType, CommunicatorType, SequentialPreconditionerType>>(
        *seq_preconditioner, communicator);
  }
};


template <class S>
struct IstlSolverTraits<S, SequentialCommunication>
{
  using IstlVectorType = typename IstlDenseVector<S>::BackendType;
  using IstlMatrixType = typename IstlRowMajorSparseMatrix<S>::BackendType;
  using MatrixOperatorType = MatrixAdapter<IstlMatrixType, IstlVectorType, IstlVectorType>;
  using ScalarproductType = SeqScalarProduct<IstlVectorType>;

  static MatrixOperatorType make_operator(const IstlMatrixType& matrix, const SequentialCommunication& /*communicator*/)
  {
    return MatrixOperatorType(matrix);
  }

  static ScalarproductType make_scalarproduct(const SequentialCommunication& /*communicator*/)
  {
    return ScalarproductType();
  }

  template <class SequentialPreconditionerType>
  static std::shared_ptr<SequentialPreconditionerType>
  make_preconditioner(const std::shared_ptr<SequentialPreconditionerType>& seq_preconditioner,
                      const SequentialCommunication& /*communicator*/)
  {
    return seq_preconditioner;
  }
};


} // namespace internal


template <class S, class CommunicatorType>
class SolverOptions<IstlRowMajorSparseMatrix<S>, CommunicatorType> : protected internal::SolverUtils
{
public:
  using MatrixType = IstlRowMajorSparseMatrix<S>;

  static std::vector<std::string> types()
  {
    std::vector<std::string> ret{
        "bicgstab.ssor", "bicgstab.amg.ssor", "bicgstab.amg.ilu0", "bicgstab.ilut", "bicgstab", "cg"};

    if (std::is_same<CommunicatorType, XT::SequentialCommunication>::value) {
#if HAVE_SUPERLU
      ret.insert(ret.begin(), "superlu");
#endif
#if HAVE_UMFPACK
      ret.emplace_back("umfpack");
#endif
    }
    return ret;
  } // ... types(...)

  static Common::Configuration options(const std::string& type = "")
  {
    const std::string tp = !type.empty() ? type : types()[0];
    internal::SolverUtils::check_given(tp, types());
    Common::Configuration general_opts({"type", "post_check_solves_system", "verbose"}, {tp.c_str(), "1e-5", "0"});
    Common::Configuration iterative_options({"max_iter", "precision"}, {"10000", "1e-10"});
    iterative_options += general_opts;
    if (tp.substr(0, 13) == "bicgstab.amg." || tp == "bicgstab" || tp == "cg") {
      iterative_options.set("smoother.iterations", "1");
      iterative_options.set("smoother.relaxation_factor", "1");
      iterative_options.set("smoother.verbose", "0");
      iterative_options.set("preconditioner.max_level", "100");
      iterative_options.set("preconditioner.coarse_target", "1000");
      iterative_options.set("preconditioner.min_coarse_rate", "1.2");
      iterative_options.set("preconditioner.prolong_damp", "1.6");
      iterative_options.set("preconditioner.anisotropy_dim", "2"); // <- this should be the dimDomain of the problem!
      iterative_options.set("preconditioner.isotropy_dim", "2"); // <- this as well
      iterative_options.set("preconditioner.verbose", "0");
      return iterative_options;
    }
    if (tp == "bicgstab.ilut" || tp == "bicgstab.ssor") {
      iterative_options.set("preconditioner.iterations", "2");
      iterative_options.set("preconditioner.relaxation_factor", "1.0");
      return iterative_options;
#if HAVE_UMFPACK
    }
    if (tp == "umfpack") {
      return general_opts;
#endif
#if HAVE_SUPERLU
    } else if (tp == "superlu") {
      return general_opts;
#endif
    } else
      DUNE_THROW(Common::Exceptions::internal_error, "Given solver type '" << tp << "' has no default options");
    return Common::Configuration();
  }
}; // class SolverOptions


template <class S, class CommunicatorType>
class Solver<IstlRowMajorSparseMatrix<S>, CommunicatorType> : protected internal::SolverUtils
{
public:
  using MatrixType = IstlRowMajorSparseMatrix<S>;
  using R = typename MatrixType::RealType;

  Solver(const MatrixType& matrix)
    : matrix_(matrix)
    , communicator_(CommunicatorType())
  {
  }

  Solver(const MatrixType& matrix, const CommunicatorType& communicator)
    : matrix_(matrix)
    , communicator_(communicator)
  {
  }

  Solver(Solver&& source) = default;

  static std::vector<std::string> types()
  {
    return SolverOptions<MatrixType, CommunicatorType>::types();
  } // ... types()

  static XT::Common::Configuration options(const std::string& type = "")
  {
    return SolverOptions<MatrixType, CommunicatorType>::options(type);
  } // ... options(...)

  void apply(const IstlDenseVector<S>& rhs, IstlDenseVector<S>& solution) const
  {
    apply(rhs, solution, types()[0]);
  }

  void apply(const IstlDenseVector<S>& rhs, IstlDenseVector<S>& solution, const std::string& type) const
  {
    apply(rhs, solution, options(type));
  }

  int verbosity(const Common::Configuration& opts, const Common::Configuration& default_opts) const
  {
    const auto actual_value = opts.get("verbose", default_opts.get<int>("verbose"));
    return
#if HAVE_MPI
        (communicator_.access().communicator().rank() == 0) ? actual_value : 0;
#else
        actual_value;
#endif
  }

  /**
   *  \note does a copy of the rhs
   */
  void apply(const IstlDenseVector<S>& rhs, IstlDenseVector<S>& solution, const Common::Configuration& opts) const
  {
    using Traits = internal::IstlSolverTraits<S, CommunicatorType>;
    using IstlVectorType = typename Traits::IstlVectorType;
    using MatrixOperatorType = typename Traits::MatrixOperatorType;
    using BiCgSolverType = BiCGSTABSolver<IstlVectorType>;
    using CgSolverType = CGSolver<IstlVectorType>;

    InverseOperatorResult solver_result;
    auto scalar_product = Traits::make_scalarproduct(communicator_.access());

    try {
      if (!opts.has_key("type"))
        DUNE_THROW(Common::Exceptions::configuration_error,
                   "Given options (see below) need to have at least the key 'type' set!\n\n"
                       << opts);
      const auto type = opts.get<std::string>("type");
      internal::SolverUtils::check_given(type, types());
      const Common::Configuration default_opts = options(type);
      IstlDenseVector<S> writable_rhs = rhs.copy();

      if (type.substr(0, 13) == "bicgstab.amg.") {
        solver_result = AmgApplicator<S, CommunicatorType>(matrix_, communicator_.access())
                            .call(writable_rhs, solution, opts, default_opts, type.substr(13));
      } else if (type == "bicgstab.ilut") {
        auto matrix_operator = Traits::make_operator(matrix_.backend(), communicator_.access());
        using SequentialPreconditionerType = SeqILU<typename MatrixType::BackendType, IstlVectorType, IstlVectorType>;
        auto seq_preconditioner = std::make_shared<SequentialPreconditionerType>(
            matrix_.backend(),
            opts.get("preconditioner.iterations", default_opts.get<int>("preconditioner.iterations")),
            opts.get("preconditioner.relaxation_factor", default_opts.get<S>("preconditioner.relaxation_factor")));
        auto preconditioner = Traits::make_preconditioner(seq_preconditioner, communicator_.access());
        BiCgSolverType solver(matrix_operator,
                              scalar_product,
                              *preconditioner,
                              opts.get("precision", default_opts.get<R>("precision")),
                              opts.get("max_iter", default_opts.get<int>("max_iter")),
                              verbosity(opts, default_opts));
        solver.apply(solution.backend(), writable_rhs.backend(), solver_result);
      } else if (type == "bicgstab.ssor") {
        auto matrix_operator = Traits::make_operator(matrix_.backend(), communicator_.access());
        using SequentialPreconditionerType = SeqSSOR<typename MatrixType::BackendType, IstlVectorType, IstlVectorType>;
        auto seq_preconditioner = std::make_shared<SequentialPreconditionerType>(
            matrix_.backend(),
            opts.get("preconditioner.iterations", default_opts.get<int>("preconditioner.iterations")),
            opts.get("preconditioner.relaxation_factor", default_opts.get<S>("preconditioner.relaxation_factor")));
        auto preconditioner = Traits::make_preconditioner(seq_preconditioner, communicator_.access());
        BiCgSolverType solver(matrix_operator,
                              scalar_product,
                              *preconditioner,
                              opts.get("precision", default_opts.get<S>("precision")),
                              opts.get("max_iter", default_opts.get<int>("max_iter")),
                              verbosity(opts, default_opts));
        solver.apply(solution.backend(), writable_rhs.backend(), solver_result);
      } else if (type == "bicgstab") {
        auto matrix_operator = Traits::make_operator(matrix_.backend(), communicator_.access());
        const auto cat = matrix_operator.category();
        using SequentialPreconditioner = IdentityPreconditioner<MatrixOperatorType>;
        auto seq_preconditioner = std::make_shared<SequentialPreconditioner>(cat);
        auto preconditioner = Traits::make_preconditioner(seq_preconditioner, communicator_.access());
        // define the BiCGStab as the actual solver
        BiCgSolverType solver(matrix_operator,
                              scalar_product,
                              *preconditioner,
                              opts.get("precision", default_opts.get<S>("precision")),
                              opts.get("max_iter", default_opts.get<int>("max_iter")),
                              verbosity(opts, default_opts));
        solver.apply(solution.backend(), writable_rhs.backend(), solver_result);
      } else if (type == "cg") {
        auto matrix_operator = Traits::make_operator(matrix_.backend(), communicator_.access());
        const auto cat = matrix_operator.category();
        using SequentialPreconditioner = IdentityPreconditioner<MatrixOperatorType>;
        auto seq_preconditioner = std::make_shared<SequentialPreconditioner>(cat);
        auto preconditioner = Traits::make_preconditioner(seq_preconditioner, communicator_.access());
        // define the CG as the actual solver
        CgSolverType solver(matrix_operator,
                            scalar_product,
                            *preconditioner,
                            opts.get("precision", default_opts.get<S>("precision")),
                            opts.get("max_iter", default_opts.get<int>("max_iter")),
                            verbosity(opts, default_opts),
                            false);
        solver.apply(solution.backend(), writable_rhs.backend(), solver_result);
#if HAVE_UMFPACK
      } else if (type == "umfpack") {
        UMFPack<typename MatrixType::BackendType> solver(matrix_.backend(),
                                                         opts.get("verbose", default_opts.get<int>("verbose")));
        solver.apply(solution.backend(), writable_rhs.backend(), solver_result);
#endif // HAVE_UMFPACK
#if HAVE_SUPERLU
      } else if (type == "superlu") {
        SuperLU<typename MatrixType::BackendType> solver(matrix_.backend(),
                                                         opts.get("verbose", default_opts.get<int>("verbose")));

        solver.apply(solution.backend(), writable_rhs.backend(), solver_result);
#endif // HAVE_SUPERLU
      } else
        DUNE_THROW(Common::Exceptions::internal_error,
                   "Given type '" << type << "' is not supported, although it was reported by types()!");
      if (!solver_result.converged)
        DUNE_THROW(Exceptions::linear_solver_failed_bc_it_did_not_converge,
                   "The dune-istl backend reported 'InverseOperatorResult.converged == false'!\n"
                       << "Those were the given options:\n\n"
                       << opts);

      // check (use writable_rhs as tmp)
      const R post_check_solves_system_threshold =
          opts.get("post_check_solves_system", default_opts.get<R>("post_check_solves_system"));
      if (post_check_solves_system_threshold > 0) {
        matrix_.mv(solution, writable_rhs);
        //! TODO the additional copy to make the original rhs consistent is only necessary in parallel setups
        auto tmp_rhs = rhs;
        communicator_.access().copyOwnerToAll(writable_rhs.backend(), writable_rhs.backend());
        communicator_.access().copyOwnerToAll(tmp_rhs.backend(), tmp_rhs.backend());
        writable_rhs -= tmp_rhs;
        const R sup_norm = writable_rhs.sup_norm();
        if (sup_norm > post_check_solves_system_threshold || Common::isnan(sup_norm) || Common::isinf(sup_norm))
          DUNE_THROW(Exceptions::linear_solver_failed_bc_the_solution_does_not_solve_the_system,
                     "The computed solution does not solve the system (although the dune-istl backend "
                         << "reported no error) and you requested checking (see options below)!\n"
                         << "If you want to disable this check, set 'post_check_solves_system = 0' in the options."
                         << "\n\n"
                         << "  (A * x - b).sup_norm() = " << sup_norm << "\n\n"
                         << "Those were the given options:\n\n"
                         << opts);
      }
    } catch (ISTLError& e) {
      DUNE_THROW(Exceptions::linear_solver_failed,
                 "The dune-istl backend reported: " << e.what() << "\nThose were the given options:\n\n"
                                                    << opts);
    } catch (FMatrixError& e) {
      DUNE_THROW(Exceptions::linear_solver_failed,
                 "The dune-istl backend reported: " << e.what() << "Those were the given options:\n\n"
                                                    << opts);
    }
  } // ... apply(...)

private:
  const MatrixType& matrix_;
  const Common::ConstStorageProvider<CommunicatorType> communicator_;
}; // class Solver

} // namespace Dune::XT::LA


// begin: this is what we need for the lib
#if DUNE_XT_WITH_PYTHON_BINDINGS


extern template class Dune::XT::LA::Solver<Dune::XT::LA::IstlRowMajorSparseMatrix<double>>;


#endif // DUNE_XT_WITH_PYTHON_BINDINGS
// end: this is what we need for the lib


#endif // DUNE_XT_LA_SOLVER_ISTL_HH
