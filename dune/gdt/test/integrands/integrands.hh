// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Tobias Leibner (2019)

#ifndef DUNE_GDT_TEST_INTEGRANDS_INTEGRANDS_HH
#define DUNE_GDT_TEST_INTEGRANDS_INTEGRANDS_HH

#include <array>

#include <dune/geometry/quadraturerules.hh>
#include <dune/geometry/type.hh>
#include <dune/grid/common/rangegenerators.hh>

#include <gtest/gtest.h>
#include <dune/xt/common/fvector.hh>

#include <dune/xt/grid/gridprovider.hh>
#include <dune/xt/grid/grids.hh>
#include <dune/xt/grid/type_traits.hh>

#include <dune/xt/functions/generic/element-function.hh>
#include <dune/xt/functions/generic/function.hh>
#include <dune/xt/functions/generic/grid-function.hh>

#include <dune/gdt/operators/matrix.hh>
#include <dune/gdt/spaces/h1/continuous-lagrange.hh>
#include <dune/gdt/tools/sparsity-pattern.hh>

namespace Dune {
namespace GDT {
namespace Test {


template <class G>
struct IntegrandTest : public ::testing::Test
{
  static_assert(XT::Grid::is_grid<G>::value);

  using GV = typename G::LeafGridView;
  using D = typename GV::ctype;
  static constexpr size_t d = GV::dimension;
  using E = XT::Grid::extract_entity_t<GV>;
  using I = XT::Grid::extract_intersection_t<GV>;
  using LocalScalarBasisType = XT::Functions::GenericElementFunctionSet<E, 1, 1>;
  using DomainType = typename LocalScalarBasisType::DomainType;
  using ScalarRangeType = typename LocalScalarBasisType::RangeType;
  using ScalarJacobianType = typename LocalScalarBasisType::DerivativeRangeType;
  using LocalVectorBasisType = XT::Functions::GenericElementFunctionSet<E, 2, 1>;
  using VectorRangeType = typename LocalVectorBasisType::RangeType;
  using VectorJacobianType = typename LocalVectorBasisType::DerivativeRangeType;
  using MatrixType = typename XT::LA::Container<double, XT::LA::default_sparse_backend>::MatrixType;

  std::shared_ptr<XT::Grid::GridProvider<G>> grid_provider_;
  std::shared_ptr<LocalScalarBasisType> scalar_ansatz_;
  std::shared_ptr<LocalScalarBasisType> scalar_test_;
  std::shared_ptr<LocalVectorBasisType> vector_ansatz_;
  std::shared_ptr<LocalVectorBasisType> vector_test_;
  static constexpr bool is_simplex_grid_ = XT::Grid::is_uggrid<G>::value || XT::Grid::is_simplex_alugrid<G>::value;

  virtual std::shared_ptr<XT::Grid::GridProvider<G>> make_grid()
  {
    return std::make_shared<XT::Grid::GridProvider<G>>(
        XT::Grid::make_cube_grid<G>(XT::Common::from_string<FieldVector<double, d>>("[0 0 0 0]"),
                                    XT::Common::from_string<FieldVector<double, d>>("[3 1 1 1]"),
                                    XT::Common::from_string<std::array<unsigned int, d>>("[9 2 2 2]")));
  }

  void SetUp() override
  {
    grid_provider_ = make_grid();
    // {x, x^2 y}
    scalar_ansatz_ = std::make_shared<LocalScalarBasisType>(
        /*size = */
        2,
        /*ord = */ 3,
        /*evaluate = */
        [](const DomainType& x, std::vector<ScalarRangeType>& ret, const XT::Common::Parameter&) {
          ret = {{x[0]}, {std::pow(x[0], 2) * x[1]}};
        },
        /*param_type = */ XT::Common::ParameterType{},
        /*jacobian = */
        [](const DomainType& x, std::vector<ScalarJacobianType>& ret, const XT::Common::Parameter&) {
          // jacobians both only have a single row
          ret[0][0] = {1., 0.};
          ret[1][0] = {2. * x[0] * x[1], std::pow(x[0], 2)};
        });
    // {y, x y^3}
    scalar_test_ = std::make_shared<LocalScalarBasisType>(
        /*size = */
        2,
        /*ord = */ 4,
        /*evaluate = */
        [](const DomainType& x, std::vector<ScalarRangeType>& ret, const XT::Common::Parameter&) {
          ret = {{x[1]}, {x[0] * std::pow(x[1], 3)}};
        },
        /*param_type = */ XT::Common::ParameterType{},
        /*jacobian = */
        [](const DomainType& x, std::vector<ScalarJacobianType>& ret, const XT::Common::Parameter&) {
          // jacobians both only have a single row
          ret[0][0] = {0., 1.};
          ret[1][0] = {std::pow(x[1], 3), 3 * x[0] * std::pow(x[1], 2)};
        });
    // { (x,y)^T, (x^2, y^2)^T}
    vector_ansatz_ = std::make_shared<LocalVectorBasisType>(
        /*size = */
        2,
        /*ord = */ 2,
        /*evaluate = */
        [](const DomainType& x, std::vector<VectorRangeType>& ret, const XT::Common::Parameter&) {
          ret = {{x[0], x[1]}, {std::pow(x[0], 2), std::pow(x[1], 2)}};
        },
        /*param_type = */ XT::Common::ParameterType{},
        /*jacobian = */
        [](const DomainType& x, std::vector<VectorJacobianType>& ret, const XT::Common::Parameter&) {
          // jacobian of first function
          ret[0][0] = {1., 0.};
          ret[0][1] = {0., 1.};
          // jacobian of second function
          ret[1][0] = {2 * x[0], 0.};
          ret[1][1] = {0., 2 * x[1]};
        });
    // { (x,y)^T, (x^2, y^2)^T}
    vector_test_ = std::make_shared<LocalVectorBasisType>(
        /*size = */
        2,
        /*ord = */ 2,
        /*evaluate = */
        [](const DomainType& x, std::vector<VectorRangeType>& ret, const XT::Common::Parameter&) {
          ret = {{1, 2}, {x[0] * x[1], x[0] + x[1]}};
        },
        /*param_type = */ XT::Common::ParameterType{},
        /*jacobian = */
        [](const DomainType& x, std::vector<VectorJacobianType>& ret, const XT::Common::Parameter&) {
          // jacobian of first function
          ret[0][0] = {0., 0.};
          ret[0][1] = {0., 0.};
          // jacobian of second function
          ret[1][0] = {x[1], x[0]};
          ret[1][1] = {1., 1.};
        });
  } // ... SetUp(...)

  I find_inner_intersection() const
  {
    const auto& gv = grid_provider_->leaf_view();
    for (const auto& element : Dune::elements(gv))
      for (const auto& intersection : Dune::intersections(gv, element))
        if (intersection.neighbor())
          return intersection;
    DUNE_THROW(Dune::InvalidStateException, "No inner intersection found in grid!");
  }

  I find_boundary_intersection() const
  {
    const auto& gv = grid_provider_->leaf_view();
    for (const auto& element : Dune::elements(gv))
      for (const auto& intersection : Dune::intersections(gv, element))
        if (!intersection.neighbor())
          return intersection;
    DUNE_THROW(Dune::InvalidStateException, "No boundary intersection found in grid!");
  }

  // Returns a constant-1 scalar basis function set (size=1, order=0) for use in tests.
  std::shared_ptr<LocalScalarBasisType> make_const_basis() const
  {
    return std::make_shared<LocalScalarBasisType>(
        /*fixed_size=*/1,
        /*ord=*/0,
        [](const DomainType& /*x*/, std::vector<ScalarRangeType>& ret, const XT::Common::Parameter& /*param*/) {
          ret = {{1.0}};
        });
  }

  // Execute callable(gv, el, is) on the first interior (neighbor) intersection found in the
  // grid.  Returns true if such an intersection was found, false if the grid has none.
  template <class Callable>
  bool with_first_interior_intersection(Callable&& callable) const
  {
    const auto& gv = grid_provider_->leaf_view();
    for (auto&& el : elements(gv)) {
      for (auto&& is : intersections(gv, el)) {
        if (is.neighbor()) {
          callable(gv, el, is);
          return true;
        }
      }
    }
    return false;
  }

  // Execute callable(gv, el, is) for every intersection of the first grid element.
  template <class Callable>
  void for_each_intersection_of_first_element(Callable&& callable) const
  {
    const auto& gv = grid_provider_->leaf_view();
    const auto el = *(gv.template begin<0>());
    for (auto&& is : intersections(gv, el))
      callable(gv, el, is);
  }

  // Find the first interior intersection, construct two const-1 bases bound to the inside/outside
  // elements, and invoke callable(is, basis_in, basis_out).  Returns true if such an intersection
  // was found.  Avoids repeating the el_out / basis setup boilerplate in coupling tests.
  template <class Callable>
  bool with_first_coupling_intersection(Callable&& callable) const
  {
    return with_first_interior_intersection([&](const GV& /*gv*/, const E& el_in, const I& is) {
      auto el_out = is.outside();
      auto basis_in = make_const_basis();
      auto basis_out = make_const_basis();
      basis_in->bind(el_in);
      basis_out->bind(el_out);
      callable(is, *basis_in, *basis_out);
    });
  }

  template <class UnaryIntegrandType, class BasisType>
  void check_unary_clone_matches(UnaryIntegrandType& integrand, const BasisType& basis)
  {
    const auto element = *(grid_provider_->leaf_view().template begin<0>());
    integrand.bind(element);
    auto clone = integrand.copy_as_unary_element_integrand();
    clone->bind(element);
    const auto order = integrand.order(basis);
    const size_t n = basis.size();
    DynamicVector<double> result_orig(n, 0.), result_clone(n, 0.);
    const auto quadrature_rule = Dune::QuadratureRules<D, d>::rule(element.type(), order);
    for (const auto& qp : quadrature_rule) {
      const auto& x = qp.position();
      integrand.evaluate(basis, x, result_orig);
      clone->evaluate(basis, x, result_clone);
      for (size_t ii = 0; ii < n; ++ii)
        EXPECT_DOUBLE_EQ(result_orig[ii], result_clone[ii]);
    }
  }

  template <class BinaryIntegrandType>
  void check_binary_clone_matches(BinaryIntegrandType& integrand)
  {
    const auto element = *(grid_provider_->leaf_view().template begin<0>());
    integrand.bind(element);
    auto clone = integrand.copy_as_binary_element_integrand();
    clone->bind(element);
    const auto order = integrand.order(*scalar_test_, *scalar_ansatz_);
    DynamicMatrix<double> result_orig(2, 2, 0.), result_clone(2, 2, 0.);
    const auto quadrature_rule = Dune::QuadratureRules<D, d>::rule(element.type(), order);
    for (const auto& qp : quadrature_rule) {
      const auto& x = qp.position();
      integrand.evaluate(*scalar_test_, *scalar_ansatz_, x, result_orig);
      clone->evaluate(*scalar_test_, *scalar_ansatz_, x, result_clone);
      for (size_t ii = 0; ii < 2; ++ii)
        for (size_t jj = 0; jj < 2; ++jj)
          EXPECT_DOUBLE_EQ(result_orig[ii][jj], result_clone[ii][jj]);
    }
  }

  virtual void is_constructable() = 0;
}; // struct IntegrandTest


} // namespace Test
} // namespace GDT
} // namespace Dune


using Grids2D = ::testing::Types<YASP_2D_EQUIDISTANT_OFFSET
#if HAVE_DUNE_ALUGRID
                                 ,
                                 ALU_2D_SIMPLEX_CONFORMING,
                                 ALU_2D_SIMPLEX_NONCONFORMING,
                                 ALU_2D_CUBE
#endif
#if HAVE_DUNE_UGGRID || HAVE_UG
                                 ,
                                 UG_2D
#endif
#if HAVE_ALBERTA
                                 ,
                                 ALBERTA_2D
#endif
                                 >;

DUNE_XT_COMMON_TYPENAME(YASP_2D_EQUIDISTANT_OFFSET)
#if HAVE_DUNE_ALUGRID
DUNE_XT_COMMON_TYPENAME(ALU_2D_SIMPLEX_CONFORMING)
DUNE_XT_COMMON_TYPENAME(ALU_2D_SIMPLEX_NONCONFORMING)
DUNE_XT_COMMON_TYPENAME(ALU_2D_CUBE)
#endif
#if HAVE_DUNE_UGGRID || HAVE_UG
DUNE_XT_COMMON_TYPENAME(UG_2D)
#endif
#if HAVE_ALBERTA
DUNE_XT_COMMON_TYPENAME(ALBERTA_2D)
#endif


#endif // DUNE_GDT_TEST_INTEGRANDS_INTEGRANDS_HH
