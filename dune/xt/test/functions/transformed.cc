// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   dune-xt developers

#ifndef DUNE_XT_COMMON_TEST_MAIN_CATCH_EXCEPTIONS
#  define DUNE_XT_COMMON_TEST_MAIN_CATCH_EXCEPTIONS 1
#endif

#include <dune/xt/test/main.hxx> // <- has to come first, includes the config.h!

#include <dune/geometry/quadraturerules.hh>

#include <dune/xt/grid/grids.hh>
#include <dune/xt/grid/gridprovider/cube.hh>

#include <dune/xt/functions/base/transformed.hh>
#include <dune/xt/functions/exceptions.hh>
#include <dune/xt/functions/generic/grid-function.hh>

using namespace Dune;
using namespace Dune::XT;
using namespace Dune::XT::Functions;


struct TransformedGridFunctionTest : public ::testing::Test
{
  using GridType = CUBEGRID_2D;
  using ElementType = XT::Grid::extract_entity_t<GridType>;
  static constexpr size_t d = GridType::dimension;

  using SourceType = GenericGridFunction<ElementType, 1, 1>;
  using SourceRangeType = typename SourceType::LocalFunctionType::RangeType;
  using TransformedType = TransformedGridFunction<GridFunctionInterface<ElementType, 1, 1>>;

  // transformations reused across the tests below (kept as members to avoid duplicating them per test)
  static SourceRangeType square(const SourceRangeType& u)
  {
    SourceRangeType ret;
    ret[0] = u[0] * u[0];
    return ret;
  }

  static SourceRangeType identity(const SourceRangeType& u)
  {
    return u;
  }

  TransformedGridFunctionTest()
    : grid_(XT::Grid::make_cube_grid<GridType>(0., 1., 4))
    , source_(/*order=*/
              1,
              /*post_bind=*/[](const ElementType&) {},
              // scalar source function u(x) = x[0] + x[1] (in local coordinates of each element)
              /*evaluate=*/
              [](const auto& xx, const auto& /*param*/) {
                SourceRangeType ret;
                ret[0] = xx[0] + xx[1];
                return ret;
              },
              /*param_type=*/{},
              /*name=*/"source")
  {
  }

  const XT::Grid::GridProvider<GridType> grid_;
  const SourceType source_;
}; // struct TransformedGridFunctionTest


TEST_F(TransformedGridFunctionTest, is_constructible)
{
  [[maybe_unused]] TransformedType transformed(source_, &square);
}

TEST_F(TransformedGridFunctionTest, is_constructible_via_factory)
{
  [[maybe_unused]] auto transformed = make_transformed_function<1, 1, double>(source_, &square);
}

TEST_F(TransformedGridFunctionTest, default_name)
{
  TransformedType transformed(source_, &identity);
  EXPECT_EQ(std::string("transformed source"), transformed.name());
}

TEST_F(TransformedGridFunctionTest, custom_name)
{
  TransformedType transformed(source_, &identity, "squared");
  EXPECT_EQ(std::string("squared"), transformed.name());
}

TEST_F(TransformedGridFunctionTest, factory_default_name)
{
  auto transformed = make_transformed_function<1, 1, double>(source_, &identity);
  EXPECT_EQ(std::string("xt.functions.transformed"), transformed.name());
}

TEST_F(TransformedGridFunctionTest, copyable)
{
  TransformedType transformed(source_, &square, "squared");

  // copy constructor
  auto copy = transformed;
  EXPECT_EQ(std::string("squared"), copy.name());

  // copy_as_grid_function
  auto ptr = transformed.copy_as_grid_function();
  ASSERT_NE(ptr, nullptr);
  EXPECT_EQ(std::string("squared"), ptr->name());

  // move constructor
  auto moved = std::move(copy);
  EXPECT_EQ(std::string("squared"), moved.name());
}

TEST_F(TransformedGridFunctionTest, is_bindable)
{
  TransformedType transformed(source_, &square);

  auto local_transformed = transformed.local_function();
  const auto leaf_view = grid_.leaf_view();
  for (auto&& element : elements(leaf_view))
    local_transformed->bind(element);
}

TEST_F(TransformedGridFunctionTest, local_order_matches_source)
{
  TransformedType transformed(source_, &square);

  auto local_source = source_.local_function();
  auto local_transformed = transformed.local_function();
  const auto leaf_view = grid_.leaf_view();
  for (auto&& element : elements(leaf_view)) {
    local_source->bind(element);
    local_transformed->bind(element);
    EXPECT_EQ(local_source->order(), local_transformed->order());
  }
}

TEST_F(TransformedGridFunctionTest, local_evaluate_applies_transformation)
{
  TransformedType transformed(source_, &square);

  auto local_source = source_.local_function();
  auto local_transformed = transformed.local_function();
  const auto leaf_view = grid_.leaf_view();
  for (auto&& element : elements(leaf_view)) {
    local_source->bind(element);
    local_transformed->bind(element);
    for (const auto& quadrature_point : QuadratureRules<double, d>::rule(element.type(), 3)) {
      const auto local_x = quadrature_point.position();
      const auto source_value = local_source->evaluate(local_x);
      const auto transformed_value = local_transformed->evaluate(local_x);
      EXPECT_DOUBLE_EQ(source_value[0] * source_value[0], transformed_value[0]);
    }
  }
}

TEST_F(TransformedGridFunctionTest, local_jacobian_throws)
{
  TransformedType transformed(source_, &identity);

  auto local_transformed = transformed.local_function();
  const auto leaf_view = grid_.leaf_view();
  for (auto&& element : elements(leaf_view)) {
    local_transformed->bind(element);
    for (const auto& quadrature_point : QuadratureRules<double, d>::rule(element.type(), 3)) {
      const auto local_x = quadrature_point.position();
      EXPECT_THROW(local_transformed->jacobian(local_x), Dune::NotImplemented);
    }
  }
}
