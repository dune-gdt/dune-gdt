// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)

#include <dune/xt/test/main.hxx> // <- this one has to come first (includes the config.h)!

#include <memory>

#include <dune/xt/functions/constant.hh>
#include <dune/xt/grid/grids.hh>
#include <dune/xt/grid/gridprovider/cube.hh>
#include <dune/xt/grid/type_traits.hh>

#include <dune/gdt/local/bilinear-forms/integrals.hh>
#include <dune/gdt/local/functionals/integrals.hh>
#include <dune/gdt/local/integrands/laplace.hh>
#include <dune/gdt/local/integrands/product.hh>
#include <dune/gdt/spaces/h1/continuous-lagrange.hh>

using namespace Dune;
using namespace Dune::GDT;

using G = YASP_2D_EQUIDISTANT_OFFSET;
using GV = typename G::LeafGridView;
using E = XT::Grid::extract_entity_t<GV>;


/// The statically dispatched integral forms have to yield exactly the results of their type-erased counterparts.
struct StaticIntegralFormsTest : public ::testing::Test
{
  void SetUp() override
  {
    grid_provider_ = std::make_unique<XT::Grid::GridProvider<G>>(XT::Grid::make_cube_grid<G>(0., 1., 4u));
    space_ = std::make_unique<ContinuousLagrangeSpace<GV>>(grid_provider_->leaf_view(), /*order=*/2);
  }

  template <class DynamicVariant, class StaticVariant>
  void expect_same_local_matrices(const DynamicVariant& dynamic_form, const StaticVariant& static_form)
  {
    auto basis = space_->basis().localize();
    const auto max_size = space_->mapper().max_local_size();
    DynamicMatrix<double> expected(max_size, max_size, 0.);
    DynamicMatrix<double> actual(max_size, max_size, 0.);
    for (auto&& element : elements(space_->grid_view())) {
      basis->bind(element);
      dynamic_form.apply2(*basis, *basis, expected);
      static_form.apply2(*basis, *basis, actual);
      for (size_t ii = 0; ii < basis->size(); ++ii)
        for (size_t jj = 0; jj < basis->size(); ++jj)
          EXPECT_DOUBLE_EQ(expected[ii][jj], actual[ii][jj]);
    }
  } // ... expect_same_local_matrices(...)

  std::unique_ptr<XT::Grid::GridProvider<G>> grid_provider_;
  std::unique_ptr<ContinuousLagrangeSpace<GV>> space_;
}; // struct StaticIntegralFormsTest


TEST_F(StaticIntegralFormsTest, laplace_bilinear_form_matches_type_erased_variant)
{
  const LocalElementIntegralBilinearForm<E> dynamic_form(LocalLaplaceIntegrand<E>());
  const auto static_form = make_local_element_integral_bilinear_form(LocalLaplaceIntegrand<E>());
  this->expect_same_local_matrices(dynamic_form, static_form);
}

TEST_F(StaticIntegralFormsTest, product_bilinear_form_matches_type_erased_variant)
{
  const LocalElementIntegralBilinearForm<E> dynamic_form(LocalElementProductIntegrand<E>());
  const auto static_form = make_local_element_integral_bilinear_form(LocalElementProductIntegrand<E>());
  this->expect_same_local_matrices(dynamic_form, static_form);
}

TEST_F(StaticIntegralFormsTest, static_bilinear_form_works_through_the_type_erased_interface)
{
  const LocalElementIntegralBilinearForm<E> dynamic_form(LocalLaplaceIntegrand<E>());
  const auto static_form = make_local_element_integral_bilinear_form(LocalLaplaceIntegrand<E>());
  // erase the type, as operators and assemblers do
  const auto erased_copy = static_form.copy();
  this->expect_same_local_matrices(dynamic_form, *erased_copy);
}

TEST_F(StaticIntegralFormsTest, functional_matches_type_erased_variant)
{
  const auto integrand = LocalElementProductIntegrand<E>().with_ansatz(XT::Functions::ConstantGridFunction<E>(1.));
  const LocalElementIntegralFunctional<E> dynamic_functional(integrand);
  const auto static_functional = make_local_element_integral_functional(integrand);
  auto basis = space_->basis().localize();
  const auto max_size = space_->mapper().max_local_size();
  DynamicVector<double> expected(max_size, 0.);
  DynamicVector<double> actual(max_size, 0.);
  for (auto&& element : elements(space_->grid_view())) {
    basis->bind(element);
    dynamic_functional.apply(*basis, expected);
    static_functional.apply(*basis, actual);
    for (size_t ii = 0; ii < basis->size(); ++ii)
      EXPECT_DOUBLE_EQ(expected[ii], actual[ii]);
  }
} // TEST_F(StaticIntegralFormsTest, functional_matches_type_erased_variant)
