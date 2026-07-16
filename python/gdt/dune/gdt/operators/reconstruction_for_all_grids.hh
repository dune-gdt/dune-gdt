// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   René Fritze (2026)

#ifndef PYTHON_DUNE_GDT_OPERATORS_RECONSTRUCTION_FOR_ALL_GRIDS_HH
#define PYTHON_DUNE_GDT_OPERATORS_RECONSTRUCTION_FOR_ALL_GRIDS_HH

#include <memory>
#include <string>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <dune/xt/common/exceptions.hh>
#include <dune/xt/common/string.hh>
#include <dune/xt/grid/grids.hh>
#include <dune/xt/grid/gridprovider/provider.hh>
#include <dune/xt/grid/type_traits.hh>
#include <dune/xt/la/container.hh>
#include <dune/xt/functions/base/function-as-flux-function.hh>

#include <dune/gdt/operators/reconstruction/linear.hh>

#include <python/xt/dune/xt/grid/grids.bindings.hh>

#include <python/gdt/dune/gdt/module_imports.hh>

// Only the scalar (m == 1) case is bound (see numerical-fluxes_for_all_grids.hh for the WP6 scoping
// rationale). For a scalar conservation law the characteristic transformation is the identity, so
// the reconstruction is instantiated with the DummyEigenVectorWrapper (slopes are limited in
// ordinary coordinates directly) -- no eigendecomposition of the flux Jacobian is ever computed,
// which also means the flux passed in does not need gradient_expressions here.
//
// LinearReconstructionOperator derives from OperatorInterface directly (not from the GDT::Operator
// intermediate MatrixOperator/AdvectionFvOperator derive from); the interface is registered with
// pybind11 (see operators/interfaces.hh), so it is declared as the pybind11 base here and only the
// vector-based `apply` (which the interface bindings do not offer) is added explicitly.

namespace Dune {
namespace GDT {
namespace bindings {
namespace internal {


/// Owns the C++-side objects the reconstruction operator only stores references to (the flux
/// wrapped as a flux function and the slope limiter selected by name). Inherited FIRST, so it is
/// fully constructed before the operator base class receives references into it.
template <class FluxWrapperType, class SlopeType>
struct LinearReconstructionStorage
{
  const FluxWrapperType flux_wrapper;
  const std::unique_ptr<const SlopeType> slope;

  LinearReconstructionStorage(const typename FluxWrapperType::FunctionType& flux,
                              std::unique_ptr<const SlopeType>&& slope_ptr)
    : flux_wrapper(flux)
    , slope(std::move(slope_ptr))
  {
  }
};


} // namespace internal


template <class GV, class F = double>
class LinearReconstructionOperator
{
  static const constexpr size_t d = GV::dimension;
  static const constexpr size_t m = 1;
  using G = std::decay_t<XT::Grid::extract_grid_t<GV>>;
  using E = XT::Grid::extract_entity_t<GV>;
  using M = XT::LA::IstlRowMajorSparseMatrix<F>;

public:
  // f(u), the same x-independent state function the numerical fluxes are constructed from
  using FluxType = XT::Functions::FunctionInterface<m, d, m, F>;
  using FluxWrapperType = XT::Functions::StateFunctionAsFluxFunctionWrapper<E, m, d, m, F>;
  // u(x) on the domain boundary, used to fill the reconstruction stencils of boundary cells
  using BoundaryValueType = XT::Functions::FunctionInterface<d, m, 1, F>;
  using EigenvectorWrapperType = GDT::internal::DummyEigenVectorWrapper<FluxWrapperType, FieldVector<F, m>>;
  using SlopeType = GDT::SlopeBase<E, EigenvectorWrapperType>;
  using OperatorType =
      GDT::LinearReconstructionOperator<FluxWrapperType, BoundaryValueType, GV, M, EigenvectorWrapperType>;
  using StorageType = internal::LinearReconstructionStorage<FluxWrapperType, SlopeType>;
  using SpaceType = typename OperatorType::SourceSpaceType;
  using VectorType = typename OperatorType::VectorType;

  static std::unique_ptr<const SlopeType> make_slope(const std::string& slope)
  {
    if (slope == "minmod")
      return std::make_unique<GDT::MinmodSlope<E, EigenvectorWrapperType>>();
    if (slope == "mc")
      return std::make_unique<GDT::McSlope<E, EigenvectorWrapperType>>();
    if (slope == "superbee")
      return std::make_unique<GDT::SuperbeeSlope<E, EigenvectorWrapperType>>();
    if (slope == "central")
      return std::make_unique<GDT::CentralSlope<E, EigenvectorWrapperType, 3>>();
    if (slope == "no_reconstruction")
      return std::make_unique<GDT::NoSlope<E, EigenvectorWrapperType, 3>>();
    DUNE_THROW(XT::Common::Exceptions::wrong_input_given,
               "slope has to be one of 'minmod', 'mc', 'superbee', 'central' or 'no_reconstruction', is: " << slope);
  } // ... make_slope(...)

  // StorageType is inherited first, see its documentation
  class type
    : StorageType
    , public OperatorType
  {
  public:
    type(const SpaceType& source_space,
         const FluxType& flux,
         const BoundaryValueType& boundary_values,
         const std::string& slope)
      : StorageType(flux, make_slope(slope))
      , OperatorType(this->flux_wrapper, boundary_values, source_space, *this->slope)
    {
    }
  };

  // the pybind11-registered interface instantiation, see _operators_interfaces_istl_{1,2,3}d
  using base_type = GDT::OperatorInterface<GV, m, 1, m>;
  using bound_type = pybind11::class_<type, base_type>;

  static std::string class_name(const std::string& grid_id,
                                const std::string& class_id = "linear_reconstruction_operator")
  {
    return class_id + "_" + grid_id;
  }

  static bound_type bind(pybind11::module& m_, const std::string& grid_id)
  {
    namespace py = pybind11;
    using namespace pybind11::literals;

    const auto ClassName = XT::Common::to_camel_case(class_name(grid_id));
    bound_type c(m_, ClassName.c_str(), ClassName.c_str());
    c.def(py::init([](const SpaceType& source_space,
                      const FluxType& flux,
                      const BoundaryValueType& boundary_values,
                      const std::string& slope) {
            return std::make_unique<type>(source_space, flux, boundary_values, slope);
          }),
          "space"_a,
          "flux"_a,
          "boundary_values"_a,
          "slope"_a = "minmod",
          py::keep_alive<1, 2>(),
          py::keep_alive<1, 3>(),
          py::keep_alive<1, 4>());

    // The reconstructed function lives in the operator's own order-1 discontinuous Lagrange range
    // space; range_vector has to match it in size (create it from your own
    // DiscontinuousLagrangeSpace(grid, order=1) of matching size, or use the creating overload below).
    c.def(
        "apply",
        [](type& self, const VectorType& source_vector, VectorType& range_vector, const XT::Common::Parameter& param) {
          self.apply(source_vector, range_vector, param);
        },
        "source"_a,
        "range"_a,
        "param"_a = XT::Common::Parameter(),
        py::call_guard<py::gil_scoped_release>());
    c.def(
        "apply",
        [](type& self, const VectorType& source_vector, const XT::Common::Parameter& param) {
          VectorType range_vector(self.range_space().mapper().size(), 0.);
          self.apply(source_vector, range_vector, param);
          return range_vector;
        },
        "source"_a,
        "param"_a = XT::Common::Parameter(),
        py::call_guard<py::gil_scoped_release>());

    m_.def(
        "linear_reconstruction_operator",
        [](const SpaceType& source_space,
           const FluxType& flux,
           const BoundaryValueType& boundary_values,
           const std::string& slope) { return std::make_unique<type>(source_space, flux, boundary_values, slope); },
        "space"_a,
        "flux"_a,
        "boundary_values"_a,
        "slope"_a = "minmod",
        py::keep_alive<0, 1>(),
        py::keep_alive<0, 2>(),
        py::keep_alive<0, 3>());

    return c;
  } // ... bind(...)
}; // class LinearReconstructionOperator


} // namespace bindings
} // namespace GDT
} // namespace Dune


template <class GridTypes = Dune::XT::Grid::bindings::AvailableGridTypes>
struct LinearReconstructionOperator_for_all_grids
{
  using G = Dune::XT::Common::tuple_head_t<GridTypes>;
  using GV = typename G::LeafGridView;

  static void bind(pybind11::module& m)
  {
    using Dune::GDT::bindings::LinearReconstructionOperator;
    using Dune::XT::Grid::bindings::grid_name;

    LinearReconstructionOperator<GV>::bind(m, grid_name<G>::value());
    // add your extra dimensions here
    // ...
    LinearReconstructionOperator_for_all_grids<Dune::XT::Common::tuple_tail_t<GridTypes>>::bind(m);
  }
};

template <>
struct LinearReconstructionOperator_for_all_grids<Dune::XT::Common::tuple_null_type>
{
  static void bind(pybind11::module& /*m*/)
  {
    // recursion base case: no grid types left to bind
  }
};


// Shared by reconstruction_1d.cc/_2d.cc/_3d.cc: see DUNE_GDT_BIND_OPERATOR_MODULE (operator_for_all_grids.hh)
// for the rationale behind the per-dimension split.
#define DUNE_GDT_BIND_RECONSTRUCTION_MODULE(dim)                                                                       \
  namespace py = pybind11;                                                                                             \
  using namespace Dune;                                                                                                \
                                                                                                                       \
  DUNE_GDT_BIND_OPERATOR_STACK_IMPORTS;                                                                                \
                                                                                                                       \
  LinearReconstructionOperator_for_all_grids<XT::Grid::bindings::Available##dim##dGridTypes>::bind(m);                 \
  m.attr("__all__") = py::make_tuple()


#endif // PYTHON_DUNE_GDT_OPERATORS_RECONSTRUCTION_FOR_ALL_GRIDS_HH
