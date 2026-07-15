// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   René Fritze (2026)

#ifndef PYTHON_DUNE_GDT_OPERATORS_ADVECTION_FV_FOR_ALL_GRIDS_HH
#define PYTHON_DUNE_GDT_OPERATORS_ADVECTION_FV_FOR_ALL_GRIDS_HH

#include <functional>

#include <pybind11/functional.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <dune/xt/common/string.hh>
#include <dune/xt/grid/grids.hh>
#include <dune/xt/grid/gridprovider/provider.hh>
#include <dune/xt/grid/type_traits.hh>
#include <dune/xt/la/type_traits.hh>

#include <dune/gdt/operators/advection-fv.hh>

#include <python/xt/dune/xt/grid/grids.bindings.hh>

#include "numerical-fluxes_for_all_grids.hh"
#include "operator_for_all_grids.hh"

// Only the scalar (m == 1) single-space case is bound, i.e. the same slice that the linear-transport
// and Burgers C++ test setups exercise (#320 WP6's acceptance criterion); non-conforming coupling
// (separate source/range spaces, RGV != SGV) and vector-valued systems are left to a follow-up.
//
// The non-periodic boundary treatment is simplified to a single scalar Python callable
// `u_outside = extrapolate(u_inside)` (see AdvectionFvOperator::boundary_treatment below), rather
// than exposing the full C++ lambda signature (intersection, local coordinates, flux, param) --
// sufficient for the outflow/Neumann-type boundaries used by the WP6 example notebook. Periodic
// domains are not bound either (dune.xt.grid has no Python-facing periodic grid view yet), so
// problems that rely on periodicity should keep the support of their initial data away from the
// domain boundary for the duration of the simulation instead.

namespace Dune {
namespace GDT {
namespace bindings {


template <class GV, class F = double>
class AdvectionFvOperator
{
  static const constexpr size_t m = 1;
  using G = std::decay_t<XT::Grid::extract_grid_t<GV>>;
  using GP = XT::Grid::GridProvider<G>;

public:
  using type = GDT::AdvectionFvOperator<GV, m, F>;
  using base_type = GDT::Operator<GV, m, 1, m>;
  using bound_type = pybind11::class_<type, base_type>;

private:
  using SpaceType = typename type::SourceSpaceType;
  using NumericalFluxType = typename type::NumericalFluxType;
  using BoundaryTreatmentType = typename type::BoundaryTreatmentByCustomExtrapolationOperatorType;

public:
  static std::string class_name(const std::string& grid_id, const std::string& class_id = "advection_fv_operator")
  {
    return class_id + "_" + grid_id;
  }

  static bound_type bind(pybind11::module& m_, const std::string& grid_id)
  {
    namespace py = pybind11;
    using namespace pybind11::literals;

    const auto ClassName = XT::Common::to_camel_case(class_name(grid_id));
    bound_type c(m_, ClassName.c_str(), ClassName.c_str());
    c.def(py::init([](const SpaceType& space, const NumericalFluxType& numerical_flux) {
            // NOTE: AdvectionFvOperator stores the assembly grid view BY REFERENCE (like the plain
            // Operator, see operator_for_all_grids.hh), so it must outlive the operator; use the
            // space's grid view (kept alive below) rather than a fresh, temporary grid.leaf_view().
            return new type(space.grid_view(), numerical_flux, space, space);
          }),
          "space"_a,
          "numerical_flux"_a,
          py::keep_alive<1, 2>(),
          py::keep_alive<1, 3>());

    // non-periodic boundary treatment: u_outside = extrapolate(u_inside), applied on all boundary
    // intersections not otherwise handled (see the file comment above for why this is simplified)
    c.def(
        "boundary_treatment",
        [](type& self, std::function<double(double)> extrapolate) -> type& {
          typename BoundaryTreatmentType::LambdaType lambda =
              [extrapolate](const typename BoundaryTreatmentType::IntersectionType& /*intersection*/,
                            const typename BoundaryTreatmentType::LocalIntersectionCoords& /*x*/,
                            const typename BoundaryTreatmentType::FluxType& /*flux*/,
                            const typename BoundaryTreatmentType::DynamicStateType& u,
                            typename BoundaryTreatmentType::DynamicStateType& v,
                            const XT::Common::Parameter& /*param*/) { v[0] = extrapolate(u[0]); };
          return self.boundary_treatment(lambda);
        },
        "extrapolate"_a,
        py::return_value_policy::reference_internal,
        // std::function copies the callable, so its Python refcount alone keeps it alive; this
        // additionally makes the edge visible to Python's cyclic GC (e.g. a bound method whose
        // self transitively references this operator would otherwise be an uncollectable cycle).
        py::keep_alive<1, 2>());

    m_.def(
        "advection_fv_operator",
        [](const SpaceType& space, const NumericalFluxType& numerical_flux) {
          return new type(space.grid_view(), numerical_flux, space, space);
        },
        "space"_a,
        "numerical_flux"_a,
        py::keep_alive<0, 1>(),
        py::keep_alive<0, 2>());

    return c;
  } // ... bind(...)
}; // class AdvectionFvOperator


} // namespace bindings
} // namespace GDT
} // namespace Dune


template <class GridTypes = Dune::XT::Grid::bindings::AvailableGridTypes>
struct AdvectionFvOperator_for_all_grids
{
  using G = Dune::XT::Common::tuple_head_t<GridTypes>;
  using GV = typename G::LeafGridView;

  static void bind(pybind11::module& m)
  {
    using Dune::GDT::bindings::AdvectionFvOperator;
    using Dune::XT::Grid::bindings::grid_name;

    AdvectionFvOperator<GV>::bind(m, grid_name<G>::value());
    // add your extra dimensions here
    // ...
    AdvectionFvOperator_for_all_grids<Dune::XT::Common::tuple_tail_t<GridTypes>>::bind(m);
  }
};

template <>
struct AdvectionFvOperator_for_all_grids<Dune::XT::Common::tuple_null_type>
{
  static void bind(pybind11::module& /*m*/) {} // recursion base case: no grid types left to bind
};


// Shared by advection-fv_1d.cc/_2d.cc/_3d.cc: see DUNE_GDT_BIND_OPERATOR_MODULE (operator_for_all_grids.hh)
// for the rationale behind the per-dimension split.
#define DUNE_GDT_BIND_ADVECTION_FV_MODULE(dim)                                                                         \
  namespace py = pybind11;                                                                                             \
  using namespace Dune;                                                                                                \
  using namespace Dune::XT;                                                                                            \
  using namespace Dune::GDT;                                                                                           \
                                                                                                                       \
  py::module::import("dune.xt.common");                                                                                \
  py::module::import("dune.xt.la");                                                                                    \
  py::module::import("dune.xt.grid");                                                                                  \
  py::module::import("dune.xt.functions");                                                                             \
                                                                                                                       \
  py::module::import("dune.gdt._local_operators_element_interface");                                                   \
  py::module::import("dune.gdt._local_operators_intersection_interface");                                              \
  py::module::import("dune.gdt._operators_interfaces_common");                                                         \
  py::module::import("dune.gdt._operators_interfaces_eigen");                                                          \
  py::module::import("dune.gdt._operators_interfaces_istl_1d");                                                        \
  py::module::import("dune.gdt._operators_interfaces_istl_2d");                                                        \
  py::module::import("dune.gdt._operators_interfaces_istl_3d");                                                        \
  py::module::import("dune.gdt._operators_operator_1d");                                                               \
  py::module::import("dune.gdt._operators_operator_2d");                                                               \
  py::module::import("dune.gdt._operators_operator_3d");                                                               \
  py::module::import("dune.gdt._operators_numerical_fluxes_1d");                                                       \
  py::module::import("dune.gdt._operators_numerical_fluxes_2d");                                                       \
  py::module::import("dune.gdt._operators_numerical_fluxes_3d");                                                       \
  py::module::import("dune.gdt._spaces_interface");                                                                    \
                                                                                                                       \
  AdvectionFvOperator_for_all_grids<XT::Grid::bindings::Available##dim##dGridTypes>::bind(m);                          \
  m.attr("__all__") = py::make_tuple()


#endif // PYTHON_DUNE_GDT_OPERATORS_ADVECTION_FV_FOR_ALL_GRIDS_HH
