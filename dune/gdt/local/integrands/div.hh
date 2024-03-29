// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Tobias Leibner  (2019)

#ifndef DUNE_GDT_LOCAL_INTEGRANDS_DIV_HH
#define DUNE_GDT_LOCAL_INTEGRANDS_DIV_HH

#include <dune/xt/functions/grid-function.hh>
#include <dune/xt/functions/interfaces/grid-function.hh>

#include "interfaces.hh"

namespace Dune {
namespace GDT {
namespace internal {


class LocalElementDivProductIntegrandBase
{
protected:
  template <class DivBasis,
            class SecondBasis,
            class Point,
            class Result,
            class LocalFunction,
            class Jacobians,
            class Values>
  void evaluate(const DivBasis& div_basis,
                const SecondBasis& second_basis,
                const Point& reference_point,
                Result& result,
                const XT::Common::Parameter param,
                const LocalFunction& local_function,
                Jacobians& jacobians,
                Values& values) const
  {
    static constexpr size_t d = DivBasis::d;
    // prepare storage
    const size_t rows = div_basis.size(param);
    const size_t cols = second_basis.size(param);
    if (result.rows() < rows || result.cols() < cols)
      result.resize(rows, cols);
    // evaluate
    div_basis.jacobians(reference_point, jacobians, param);
    second_basis.evaluate(reference_point, values, param);
    const auto function_value = local_function.evaluate(reference_point, param);
    // compute product
    for (size_t ii = 0; ii < rows; ++ii) {
      // calculate div(psi_ii(x))
      typename DivBasis::R div_ii = 0;
      for (size_t dd = 0; dd < d; ++dd)
        div_ii += jacobians[ii][dd][dd];
      for (size_t jj = 0; jj < cols; ++jj)
        result[ii][jj] = (function_value * div_ii) * values[jj];
    }
  }
};


} // namespace internal


/**
 * Given an inducing function f, computes `f(x) * phi(x) * div(psi(x))` for all combinations of phi in the ansatz basis
 * and psi in the test basis.
 *
 * \note Note that f can also be given as a scalar value or omitted.
 */
template <class E, class TR = double, class F = double, class AR = TR>
class LocalElementAnsatzValueTestDivProductIntegrand
  : public LocalBinaryElementIntegrandInterface<E, E::dimension, 1, TR, F, 1, 1, AR>
  , public internal::LocalElementDivProductIntegrandBase
{
  using BaseType = LocalBinaryElementIntegrandInterface<E, E::dimension, 1, TR, F, 1, 1, AR>;
  using ThisType = LocalElementAnsatzValueTestDivProductIntegrand;
  using DivBaseType = internal::LocalElementDivProductIntegrandBase;

public:
  using BaseType::d;
  using typename BaseType::DomainType;
  using typename BaseType::ElementType;
  using typename BaseType::LocalAnsatzBasisType;
  using typename BaseType::LocalTestBasisType;

  using GridFunctionType = XT::Functions::GridFunctionInterface<E, 1, 1, F>;

  LocalElementAnsatzValueTestDivProductIntegrand(XT::Functions::GridFunction<E, 1, 1, F> inducing_function = F(1))
    : BaseType(inducing_function.parameter_type())
    , DivBaseType()
    , inducing_function_(inducing_function.copy_as_grid_function())
    , local_function_(inducing_function_->local_function())
  {
  }

  LocalElementAnsatzValueTestDivProductIntegrand(const ThisType& other)
    : BaseType(other)
    , DivBaseType()
    , inducing_function_(other.inducing_function_->copy_as_grid_function())
    , local_function_(inducing_function_->local_function())
  {
  }

  LocalElementAnsatzValueTestDivProductIntegrand(ThisType&& source) = default;

  std::unique_ptr<BaseType> copy_as_binary_element_integrand() const override final
  {
    return std::make_unique<ThisType>(*this);
  }

protected:
  void post_bind(const ElementType& ele) override final
  {
    local_function_->bind(ele);
  }

public:
  int order(const LocalTestBasisType& test_basis,
            const LocalAnsatzBasisType& ansatz_basis,
            const XT::Common::Parameter& param = {}) const override final
  {
    return local_function_->order(param) + test_basis.order(param) + ansatz_basis.order(param);
  }

  using BaseType::evaluate;

  void evaluate(const LocalTestBasisType& test_basis,
                const LocalAnsatzBasisType& ansatz_basis,
                const DomainType& point_in_reference_element,
                DynamicMatrix<F>& result,
                const XT::Common::Parameter& param = {}) const override final
  {
    DivBaseType::evaluate(test_basis,
                          ansatz_basis,
                          point_in_reference_element,
                          result,
                          param,
                          *local_function_,
                          test_basis_jacobians_,
                          ansatz_basis_values_);
  } // ... evaluate(...)

private:
  const std::unique_ptr<GridFunctionType> inducing_function_;
  std::unique_ptr<typename GridFunctionType::LocalFunctionType> local_function_;
  mutable std::vector<typename LocalTestBasisType::DerivativeRangeType> test_basis_jacobians_;
  mutable std::vector<typename LocalAnsatzBasisType::RangeType> ansatz_basis_values_;
}; // class LocalElementAnsatzValueTestDivProductIntegrand


/**
 * Given an inducing function f, computes `f(x) * div(phi(x)) * psi(x)` for all combinations of phi in the ansatz basis
 * and psi in the test basis.
 *
 * \sa LocalElementAnsatzValueTestDivProductIntegrand
 */
template <class E, class TR = double, class F = double, class AR = TR>
class LocalElementAnsatzDivTestValueProductIntegrand
  : public LocalBinaryElementIntegrandInterface<E, 1, 1, TR, F, E::dimension, 1, AR>
  , public internal::LocalElementDivProductIntegrandBase
{
  using BaseType = LocalBinaryElementIntegrandInterface<E, 1, 1, TR, F, E::dimension, 1, AR>;
  using ThisType = LocalElementAnsatzDivTestValueProductIntegrand;
  using DivBaseType = internal::LocalElementDivProductIntegrandBase;

public:
  using BaseType::d;
  using typename BaseType::DomainType;
  using typename BaseType::ElementType;
  using typename BaseType::LocalAnsatzBasisType;
  using typename BaseType::LocalTestBasisType;

  using GridFunctionType = XT::Functions::GridFunctionInterface<E, 1, 1, F>;

  LocalElementAnsatzDivTestValueProductIntegrand(XT::Functions::GridFunction<E, 1, 1, F> inducing_function = F(1))
    : BaseType()
    , inducing_function_(inducing_function.copy_as_grid_function())
    , local_function_(inducing_function_->local_function())
  {
  }

  LocalElementAnsatzDivTestValueProductIntegrand(const ThisType& other)
    : BaseType(other)
    , inducing_function_(other.inducing_function_->copy_as_grid_function())
    , local_function_(inducing_function_->local_function())
  {
  }

  LocalElementAnsatzDivTestValueProductIntegrand(ThisType&& source) = default;

  std::unique_ptr<BaseType> copy_as_binary_element_integrand() const override final
  {
    return std::make_unique<ThisType>(*this);
  }

protected:
  void post_bind(const ElementType& ele) override final
  {
    local_function_->bind(ele);
  }

public:
  int order(const LocalTestBasisType& test_basis,
            const LocalAnsatzBasisType& ansatz_basis,
            const XT::Common::Parameter& param = {}) const override final
  {
    return local_function_->order(param) + test_basis.order(param) + ansatz_basis.order(param);
  }

  using BaseType::evaluate;

  void evaluate(const LocalTestBasisType& test_basis,
                const LocalAnsatzBasisType& ansatz_basis,
                const DomainType& point_in_reference_element,
                DynamicMatrix<F>& result,
                const XT::Common::Parameter& param = {}) const override final
  {
    DivBaseType::evaluate(ansatz_basis,
                          test_basis,
                          point_in_reference_element,
                          result,
                          param,
                          *local_function_,
                          ansatz_basis_jacobians_,
                          test_basis_values_);
    result = XT::Common::transposed(result);
  } // ... evaluate(...)

private:
  const std::unique_ptr<GridFunctionType> inducing_function_;
  std::unique_ptr<typename GridFunctionType::LocalFunctionType> local_function_;
  mutable std::vector<typename LocalTestBasisType::RangeType> test_basis_values_;
  mutable std::vector<typename LocalAnsatzBasisType::DerivativeRangeType> ansatz_basis_jacobians_;
}; // class LocalElementAnsatzDivTestValueProductIntegrand


} // namespace GDT
} // namespace Dune

#endif // DUNE_GDT_LOCAL_INTEGRANDS_DIV_HH
