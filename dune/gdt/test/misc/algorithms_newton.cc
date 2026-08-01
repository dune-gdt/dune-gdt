// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   dune-gdt developers

#ifndef DUNE_XT_COMMON_TEST_MAIN_CATCH_EXCEPTIONS
#  define DUNE_XT_COMMON_TEST_MAIN_CATCH_EXCEPTIONS 1
#endif

#include <dune/xt/test/main.hxx> // <- this one has to come first (includes the config.h)!

#include <array>
#include <cmath>
#include <vector>

#include <dune/xt/common/configuration.hh>
#include <dune/xt/common/fvector.hh>
#include <dune/xt/common/string.hh>
#include <dune/xt/grid/grids.hh>
#include <dune/xt/grid/gridprovider/cube.hh>
#include <dune/xt/la/container/istl.hh>

#include <dune/gdt/algorithms/newton.hh>
#include <dune/gdt/exceptions.hh>
#include <dune/gdt/operators/interfaces.hh>
#include <dune/gdt/spaces/l2/finite-volume.hh>

using namespace Dune;
using namespace Dune::GDT;

using G = YASP_2D_EQUIDISTANT_OFFSET;
static constexpr size_t d = G::dimension;
using GV = typename G::LeafGridView;
using M = XT::LA::IstlRowMajorSparseMatrix<double>;
using V = XT::LA::IstlDenseVector<double>;


namespace {


/**
 * \brief The smallest nonlinear operator we can hand to newton_solve: the DoF-wise square u \mapsto u^2.
 *
 * Solving u^2 = c for c > 0 from a positive initial guess has the known root sqrt(c), and the jacobian diag(2 u) is
 * trivially invertible as long as no DoF is zero. Both counters allow the tests below to check that newton_solve
 * actually iterates (and how often).
 */
class SquareOperator : public OperatorInterface<GV, 1, 1, 1, 1, double, M, GV, GV>
{
  using BaseType = OperatorInterface<GV, 1, 1, 1, 1, double, M, GV, GV>;

public:
  using typename BaseType::AssemblyGridViewType;
  using typename BaseType::MatrixOperatorType;
  using typename BaseType::RangeSpaceType;
  using typename BaseType::SourceSpaceType;
  using typename BaseType::VectorType;

  explicit SquareOperator(const SourceSpaceType& spc)
    : BaseType()
    , space_(spc)
    , num_applications_(0)
    , num_jacobians_(0)
  {
  }

  // pull in methods from various base classes
  using BaseType::apply;
  using BaseType::jacobian;

  const RangeSpaceType& range_space() const override final
  {
    return space_;
  }

  const SourceSpaceType& source_space() const override final
  {
    return space_;
  }

  const AssemblyGridViewType& assembly_grid_view() const override final
  {
    return space_.grid_view();
  }

  bool linear() const override final
  {
    return false;
  }

  void apply(const VectorType& source_vector,
             VectorType& range_vector,
             const XT::Common::Parameter& /*param*/ = {}) const override final
  {
    this->assert_matching_source(source_vector);
    this->assert_matching_range(range_vector);
    for (size_t ii = 0; ii < source_vector.size(); ++ii)
      range_vector.set_entry(ii, source_vector.get_entry(ii) * source_vector.get_entry(ii));
    ++num_applications_;
  } // ... apply(...)

protected:
  // this identifier is operator-private: assert_jacobian_opts() only checks it for membership in this list. It names
  // the shape of the contribution we add below and says nothing about the sparsity pattern empty_jacobian_op()
  // allocates (which, for a finite volume space, also covers the neighbour couplings).
  std::vector<XT::Common::Configuration> all_jacobian_options() const override final
  {
    return {{{"type", "diagonal"}}};
  }

public:
  void jacobian(const VectorType& source_vector,
                MatrixOperatorType& jacobian_op,
                const XT::Common::Configuration& opts,
                const XT::Common::Parameter& /*param*/ = {}) const override final
  {
    this->assert_matching_source(source_vector);
    this->assert_jacobian_opts(opts); // ensures that type diagonal is requested
    for (size_t ii = 0; ii < source_vector.size(); ++ii)
      jacobian_op.matrix().add_to_entry(ii, ii, jacobian_op.scaling * 2. * source_vector.get_entry(ii));
    ++num_jacobians_;
  } // ... jacobian(...)

  size_t num_applications() const
  {
    return num_applications_;
  }

  size_t num_jacobians() const
  {
    return num_jacobians_;
  }

private:
  const SourceSpaceType& space_;
  mutable size_t num_applications_;
  mutable size_t num_jacobians_;
}; // class SquareOperator


// A four element grid keeps the linear systems tiny while still exercising more than a single DoF.
XT::Grid::GridProvider<G> make_grid()
{
  return XT::Grid::make_cube_grid<G>(XT::Common::from_string<FieldVector<double, d>>("[0 0]"),
                                     XT::Common::from_string<FieldVector<double, d>>("[1 1]"),
                                     XT::Common::from_string<std::array<unsigned int, d>>("[2 2]"));
}


} // namespace


// default_newton_solve_options() has to provide the documented precision / iteration keys with their default values.
GTEST_TEST(algorithms_newton, default_options_have_documented_keys)
{
  const auto opts = default_newton_solve_options();
  ASSERT_TRUE(opts.has_key("precision"));
  ASSERT_TRUE(opts.has_key("max_iter"));
  ASSERT_TRUE(opts.has_key("max_dampening_iter"));
  EXPECT_DOUBLE_EQ(1e-7, opts.get<double>("precision"));
  EXPECT_EQ(100u, opts.get<size_t>("max_iter"));
  EXPECT_EQ(1000u, opts.get<size_t>("max_dampening_iter"));
}


// newton_solve() does not differentiate the operator it is given, but the residual operator lhs - rhs. That composed
// operator therefore has to report jacobian options (newton_solve takes the first one, unconditionally) and has to
// delegate the actual assembly to its summands.
GTEST_TEST(algorithms_newton, the_residual_operator_delegates_its_jacobian)
{
  auto grid = make_grid();
  auto grid_view = grid.leaf_view();
  const auto space = make_finite_volume_space(grid_view);
  const SquareOperator op(space);

  const size_t n = space.mapper().size();
  const V rhs(n, 2.);
  const auto residual_op = op - rhs;

  const auto types = residual_op.jacobian_options();
  ASSERT_FALSE(types.empty()); // <- newton_solve calls .at(0) on this
  EXPECT_EQ("lincomb", types.at(0));

  const V u(n, 3.);
  auto jacobian_op = residual_op.jacobian(u, types.at(0));
  jacobian_op.assemble();

  EXPECT_EQ(1u, op.num_jacobians()); // <- the composed operator delegated to us
  // the constant summand contributes a zero jacobian, so this is exactly our diag(2 u)
  for (size_t ii = 0; ii < n; ++ii)
    EXPECT_DOUBLE_EQ(6., jacobian_op.matrix().get_entry(ii, ii));
}


// Solving u^2 = 2 from the initial guess 1 has to yield sqrt(2) in every DoF, within a handful of iterations.
GTEST_TEST(algorithms_newton, converges_to_the_known_root)
{
  auto grid = make_grid();
  auto grid_view = grid.leaf_view();
  const auto space = make_finite_volume_space(grid_view);
  const SquareOperator op(space);

  const size_t n = space.mapper().size();
  ASSERT_EQ(4u, n);
  const V rhs(n, 2.);
  V u(n, 1.);

  newton_solve(op, rhs, u);

  for (size_t ii = 0; ii < n; ++ii)
    EXPECT_NEAR(std::sqrt(2.), u.get_entry(ii), 1e-6);
  // newton has to have iterated, but the quadratic convergence of this problem must not require many iterations
  EXPECT_GE(op.num_jacobians(), 1u);
  EXPECT_LE(op.num_jacobians(), 10u);
  EXPECT_GT(op.num_applications(), op.num_jacobians());
}


// The initial guess is used as such: starting at the root, newton has to succeed without computing a single jacobian.
GTEST_TEST(algorithms_newton, accepts_an_initial_guess_which_already_solves_the_problem)
{
  auto grid = make_grid();
  auto grid_view = grid.leaf_view();
  const auto space = make_finite_volume_space(grid_view);
  const SquareOperator op(space);

  const size_t n = space.mapper().size();
  const V rhs(n, 2.);
  V u(n, std::sqrt(2.));

  newton_solve(op, rhs, u);

  for (size_t ii = 0; ii < n; ++ii)
    EXPECT_NEAR(std::sqrt(2.), u.get_entry(ii), 1e-12);
  EXPECT_EQ(0u, op.num_jacobians());
  EXPECT_EQ(1u, op.num_applications());
}


// A non-default precision has to be honoured: a very coarse one has to stop the iteration earlier.
GTEST_TEST(algorithms_newton, honours_a_given_precision)
{
  auto grid = make_grid();
  auto grid_view = grid.leaf_view();
  const auto space = make_finite_volume_space(grid_view);
  const SquareOperator accurate_op(space);
  const SquareOperator coarse_op(space);

  const size_t n = space.mapper().size();
  const V rhs(n, 2.);

  V accurate_u(n, 1.);
  newton_solve(accurate_op, rhs, accurate_u);

  auto coarse_opts = default_newton_solve_options();
  coarse_opts["precision"] = "1e-1";
  V coarse_u(n, 1.);
  newton_solve(coarse_op, rhs, coarse_u, {}, coarse_opts);

  EXPECT_LT(coarse_op.num_jacobians(), accurate_op.num_jacobians());
  for (size_t ii = 0; ii < n; ++ii)
    EXPECT_NEAR(std::sqrt(2.), coarse_u.get_entry(ii), 1e-1);
}


// The documented failure behaviour of the non-convergence path is an Exceptions::newton_error ...
GTEST_TEST(algorithms_newton, throws_if_the_maximum_number_of_iterations_is_reached)
{
  auto grid = make_grid();
  auto grid_view = grid.leaf_view();
  const auto space = make_finite_volume_space(grid_view);
  const SquareOperator op(space);

  const size_t n = space.mapper().size();
  const V rhs(n, 2.);
  V u(n, 1.);

  auto opts = default_newton_solve_options();
  opts["max_iter"] = "0"; // <- the initial guess is not a root, so this can never succeed
  EXPECT_THROW(newton_solve(op, rhs, u, {}, opts), Exceptions::newton_error);
}


// ... which is also thrown if the automatic dampening does not manage to reduce the residual.
GTEST_TEST(algorithms_newton, throws_if_the_dampening_does_not_converge)
{
  auto grid = make_grid();
  auto grid_view = grid.leaf_view();
  const auto space = make_finite_volume_space(grid_view);
  const SquareOperator op(space);

  const size_t n = space.mapper().size();
  const V rhs(n, 2.);
  V u(n, 1.);

  auto opts = default_newton_solve_options();
  opts["max_dampening_iter"] = "0"; // <- not even the undampened candidate may be tried
  EXPECT_THROW(newton_solve(op, rhs, u, {}, opts), Exceptions::newton_error);
  // the jacobian was required to obtain the update whose dampening then failed
  EXPECT_EQ(1u, op.num_jacobians());
}
