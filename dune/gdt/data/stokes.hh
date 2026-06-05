// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Tobias Leibner (2019)

#ifndef DUNE_GDT_DATA_STOKES_HH
#define DUNE_GDT_DATA_STOKES_HH

#include <cmath>
#include <memory>

#include <dune/common/fmatrix.hh>

#include <dune/xt/common/exceptions.hh>
#include <dune/xt/common/parameter.hh>
#include <dune/xt/la/container/eye-matrix.hh>

#include <dune/xt/grid/boundaryinfo/alldirichlet.hh>
#include <dune/xt/grid/gridprovider/cube.hh>
#include <dune/xt/grid/type_traits.hh>

#include <dune/xt/functions/base/function-as-grid-function.hh>
#include <dune/xt/functions/constant.hh>
#include <dune/xt/functions/generic/function.hh>
#include <dune/xt/functions/interfaces/grid-function.hh>

namespace Dune {
namespace GDT {
namespace Data {


/**
 * \brief Problem data for a stationary Stokes problem with Dirichlet boundary values.
 *
 * gtest-free problem definition shared by the tests in dune/gdt/test/stokes/ and the
 * benchmarks in benchmarks/.
 */
template <class GV>
struct StokesDirichletProblem
{
  static_assert(XT::Grid::is_view<GV>::value, "");
  static_assert(GV::dimension == 2, "");

  static constexpr size_t d = GV::dimension;
  using E = XT::Grid::extract_entity_t<GV>;
  using I = XT::Grid::extract_intersection_t<GV>;
  using G = typename GV::Grid;
  using RangeField = double;
  using ScalarGridFunction = XT::Functions::GridFunctionInterface<E, 1, 1>;
  using VectorGridFunction = XT::Functions::GridFunctionInterface<E, d, 1>;
  using DomainType = typename ScalarGridFunction::LocalFunctionType::DomainType;
  using DiffusionTensor = XT::Functions::GridFunctionInterface<E, d, d, RangeField>;

  StokesDirichletProblem(std::shared_ptr<const DiffusionTensor> diffusion_in = default_diffusion(),
                         std::shared_ptr<const VectorGridFunction> rhs_f_in = default_rhs_f(),
                         std::shared_ptr<const VectorGridFunction> rhs_g_in = default_rhs_g(),
                         std::shared_ptr<const VectorGridFunction> dirichlet_in = default_dirichlet_values(),
                         std::shared_ptr<const VectorGridFunction> reference_sol_u = nullptr,
                         std::shared_ptr<const ScalarGridFunction> reference_sol_p = nullptr)
    : diffusion_(diffusion_in)
    , rhs_f_(rhs_f_in)
    , rhs_g_(rhs_g_in)
    , dirichlet_(dirichlet_in)
    , reference_sol_u_(reference_sol_u)
    , reference_sol_p_(reference_sol_p)
    , boundary_info_()
    , grid_(XT::Grid::make_cube_grid<G>(-1, 1, 20))
    , grid_view_(grid_.leaf_view())
  {
  }

  static std::shared_ptr<const DiffusionTensor> default_diffusion()
  {
    return std::make_shared<const XT::Functions::ConstantGridFunction<E, d, d>>(
        XT::LA::eye_matrix<FieldMatrix<double, d, d>>(d, d), "isotropic_diffusion");
  }

  static std::shared_ptr<const VectorGridFunction> default_rhs_f()
  {
    return std::make_shared<const XT::Functions::ConstantGridFunction<E, d, 1>>(0., "zero rhs");
  }

  static std::shared_ptr<const VectorGridFunction> default_rhs_g()
  {
    return std::make_shared<const XT::Functions::ConstantGridFunction<E, d, 1>>(0., "zero rhs");
  }

  static std::shared_ptr<const VectorGridFunction> default_dirichlet_values()
  {
    return std::make_shared<const XT::Functions::ConstantGridFunction<E, d, 1>>(0., "dirichlet zero boundary values");
  }

  const GV& grid_view()
  {
    return grid_view_;
  }

  const XT::Functions::GridFunctionInterface<E, d, d>& diffusion()
  {
    return *diffusion_;
  }

  const VectorGridFunction& rhs_f()
  {
    return *rhs_f_;
  }

  const VectorGridFunction& rhs_g()
  {
    return *rhs_g_;
  }

  const VectorGridFunction& dirichlet()
  {
    return *dirichlet_;
  }

  const VectorGridFunction& reference_solution_u()
  {
    DUNE_THROW_IF(!reference_sol_u_, Dune::InvalidStateException, "No reference solution provided!");
    return *reference_sol_u_;
  }

  const ScalarGridFunction& reference_solution_p()
  {
    DUNE_THROW_IF(!reference_sol_p_, Dune::InvalidStateException, "No reference solution provided!");
    return *reference_sol_p_;
  }

  const XT::Grid::BoundaryInfo<I>& boundary_info()
  {
    return boundary_info_;
  }

  std::shared_ptr<const DiffusionTensor> diffusion_;
  std::shared_ptr<const VectorGridFunction> rhs_f_;
  std::shared_ptr<const VectorGridFunction> rhs_g_;
  std::shared_ptr<const VectorGridFunction> dirichlet_;
  std::shared_ptr<const VectorGridFunction> reference_sol_u_;
  std::shared_ptr<const ScalarGridFunction> reference_sol_p_;
  const XT::Grid::AllDirichletBoundaryInfo<I> boundary_info_;
  const XT::Grid::GridProvider<G> grid_;
  const GV grid_view_;
}; // struct StokesDirichletProblem


/**
 * \brief Analytic Dirichlet boundary values for the Stokes "testcase 1" (also the exact velocity).
 */
template <class GV>
std::shared_ptr<const typename StokesDirichletProblem<GV>::VectorGridFunction> stokes_testcase1_dirichlet()
{
  using Problem = StokesDirichletProblem<GV>;
  static constexpr size_t d = Problem::d;
  using E = typename Problem::E;
  using DomainType = typename Problem::DomainType;
  using VectorGridFunction = typename Problem::VectorGridFunction;
  static const XT::Functions::GenericFunction<d, d, 1> dirichlet_values(
      50, [](const DomainType& xx, const XT::Common::Parameter&) {
        typename VectorGridFunction::LocalFunctionType::RangeReturnType ret;
        auto x = xx[0];
        auto y = xx[1];
        ret[0] = -std::exp(x) * (y * std::cos(y) + std::sin(y));
        ret[1] = std::exp(x) * y * std::sin(y);
        return ret;
      });
  return std::make_shared<XT::Functions::FunctionAsGridFunctionWrapper<E, d, 1, double>>(dirichlet_values);
}

/// \brief Exact velocity for the Stokes "testcase 1" (identical to the Dirichlet values).
template <class GV>
std::shared_ptr<const typename StokesDirichletProblem<GV>::VectorGridFunction> stokes_testcase1_exact_solution_u()
{
  return stokes_testcase1_dirichlet<GV>();
}

/// \brief Exact pressure for the Stokes "testcase 1".
template <class GV>
std::shared_ptr<const typename StokesDirichletProblem<GV>::ScalarGridFunction> stokes_testcase1_exact_solution_p()
{
  using Problem = StokesDirichletProblem<GV>;
  static constexpr size_t d = Problem::d;
  using E = typename Problem::E;
  using DomainType = typename Problem::DomainType;
  using ScalarGridFunction = typename Problem::ScalarGridFunction;
  static const XT::Functions::GenericFunction<d, 1, 1> exact_sol_p(
      50, [](const DomainType& xx, const XT::Common::Parameter&) {
        typename ScalarGridFunction::LocalFunctionType::RangeReturnType ret;
        auto x = xx[0];
        auto y = xx[1];
        ret[0] = 2. * std::exp(x) * std::sin(y);
        return ret;
      });
  return std::make_shared<XT::Functions::FunctionAsGridFunctionWrapper<E, 1, 1, double>>(exact_sol_p);
}


} // namespace Data
} // namespace GDT
} // namespace Dune

#endif // DUNE_GDT_DATA_STOKES_HH
