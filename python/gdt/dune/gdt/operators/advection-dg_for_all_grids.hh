// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   René Fritze (2026)

#ifndef PYTHON_DUNE_GDT_OPERATORS_ADVECTION_DG_FOR_ALL_GRIDS_HH
#define PYTHON_DUNE_GDT_OPERATORS_ADVECTION_DG_FOR_ALL_GRIDS_HH

#include <functional>
#include <memory>

#include <pybind11/functional.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <dune/xt/common/string.hh>
#include <dune/xt/grid/grids.hh>
#include <dune/xt/grid/gridprovider/provider.hh>
#include <dune/xt/grid/type_traits.hh>
#include <dune/xt/la/type_traits.hh>

#include <dune/gdt/operators/advection-dg.hh>

#include <python/xt/dune/xt/grid/grids.bindings.hh>

#include <python/gdt/dune/gdt/module_imports.hh>

#include "numerical-fluxes_for_all_grids.hh"
#include "operator_for_all_grids.hh"

// Only the scalar (m == 1) single-space case is bound, mirroring AdvectionFvOperator (see
// advection-fv_for_all_grids.hh for the WP6 scoping rationale, which applies here unchanged). The
// same simplifications apply: the non-periodic boundary treatment is a single scalar Python
// callable `u_outside = extrapolate(u_inside)`, and no periodicity exception filter is exposed.
//
// Unlike AdvectionFvOperator, AdvectionDgOperator requires assembly (it precomputes the local mass
// matrices it inverts in every apply), so `op.assemble()` (inherited from the OperatorInterface
// bindings) has to be called before the first `apply`/time step -- apply throws a clear
// "You need to call assemble() first" error otherwise, matching the C++ behaviour.

namespace Dune {
namespace GDT {
namespace bindings {


template <class GV, class F = double>
class AdvectionDgOperator
{
  static const constexpr size_t m = 1;
  using G = std::decay_t<XT::Grid::extract_grid_t<GV>>;
  using GP = XT::Grid::GridProvider<G>;

public:
  using type = GDT::AdvectionDgOperator<GV, m, F>;
  using base_type = GDT::Operator<GV, m, 1, m>;
  using bound_type = pybind11::class_<type, base_type>;

private:
  using SpaceType = typename type::SourceSpaceType;
  using NumericalFluxType = typename type::NumericalFluxType;
  using BoundaryTreatmentType = typename type::BoundaryTreatmentByCustomExtrapolationOperatorType;

public:
  static std::string class_name(const std::string& grid_id, const std::string& class_id = "advection_dg_operator")
  {
    return class_id + "_" + grid_id;
  }

  static std::unique_ptr<type> make(const SpaceType& space,
                                    const NumericalFluxType& numerical_flux,
                                    const double artificial_viscosity_nu_1,
                                    const double artificial_viscosity_alpha_1)
  {
    // NOTE: AdvectionDgOperator stores the assembly grid view BY REFERENCE (like the plain
    // Operator, see operator_for_all_grids.hh), so it must outlive the operator; use the
    // space's grid view (kept alive by the callers' keep_alives) rather than a fresh, temporary
    // grid.leaf_view().
    return std::make_unique<type>(space.grid_view(),
                                  numerical_flux,
                                  space,
                                  space,
                                  XT::Grid::ApplyOn::NoIntersections<GV>(),
                                  artificial_viscosity_nu_1,
                                  artificial_viscosity_alpha_1);
  } // ... make(...)

  static bound_type bind(pybind11::module& m_, const std::string& grid_id)
  {
    namespace py = pybind11;
    using namespace pybind11::literals;

    const auto ClassName = XT::Common::to_camel_case(class_name(grid_id));
    bound_type c(m_, ClassName.c_str(), ClassName.c_str());
    c.def(py::init(&AdvectionDgOperator::make),
          "space"_a,
          "numerical_flux"_a,
          "artificial_viscosity_nu_1"_a = GDT::advection_dg_artificial_viscosity_default_nu_1(),
          "artificial_viscosity_alpha_1"_a = GDT::advection_dg_artificial_viscosity_default_alpha_1(),
          py::keep_alive<1, 2>(),
          py::keep_alive<1, 3>());

    // non-periodic boundary treatment: u_outside = extrapolate(u_inside), applied on all boundary
    // intersections not otherwise handled (see the file comment above for why this is simplified)
    c.def(
        "boundary_treatment",
        [](type& self, std::function<double(double)> extrapolate) -> type& {
          using StateRangeType = typename BoundaryTreatmentType::StateRangeType;
          typename BoundaryTreatmentType::LambdaType lambda =
              [extrapolate](const typename BoundaryTreatmentType::IntersectionType& /*intersection*/,
                            const FieldVector<typename BoundaryTreatmentType::D, GV::dimension - 1>& /*x*/,
                            const typename BoundaryTreatmentType::FluxType& /*flux*/,
                            const StateRangeType& u,
                            const XT::Common::Parameter& /*param*/) {
                StateRangeType v;
                v[0] = extrapolate(u[0]);
                return v;
              };
          return self.boundary_treatment(lambda);
        },
        "extrapolate"_a,
        py::return_value_policy::reference_internal,
        // std::function copies the callable, so its Python refcount alone keeps it alive; this
        // additionally makes the edge visible to Python's cyclic GC (e.g. a bound method whose
        // self transitively references this operator would otherwise be an uncollectable cycle).
        py::keep_alive<1, 2>());

    m_.def("advection_dg_operator",
           &AdvectionDgOperator::make,
           "space"_a,
           "numerical_flux"_a,
           "artificial_viscosity_nu_1"_a = GDT::advection_dg_artificial_viscosity_default_nu_1(),
           "artificial_viscosity_alpha_1"_a = GDT::advection_dg_artificial_viscosity_default_alpha_1(),
           py::keep_alive<0, 1>(),
           py::keep_alive<0, 2>());

    return c;
  } // ... bind(...)
}; // class AdvectionDgOperator


} // namespace bindings
} // namespace GDT
} // namespace Dune


template <class GridTypes = Dune::XT::Grid::bindings::AvailableGridTypes>
struct AdvectionDgOperator_for_all_grids
{
  using G = Dune::XT::Common::tuple_head_t<GridTypes>;
  using GV = typename G::LeafGridView;

  static void bind(pybind11::module& m)
  {
    using Dune::GDT::bindings::AdvectionDgOperator;
    using Dune::XT::Grid::bindings::grid_name;

    AdvectionDgOperator<GV>::bind(m, grid_name<G>::value());
    // add your extra dimensions here
    // ...
    AdvectionDgOperator_for_all_grids<Dune::XT::Common::tuple_tail_t<GridTypes>>::bind(m);
  }
};

template <>
struct AdvectionDgOperator_for_all_grids<Dune::XT::Common::tuple_null_type>
{
  static void bind(pybind11::module& /*m*/)
  {
    // recursion base case: no grid types left to bind
  }
};


// Shared by advection-dg_1d.cc/_2d.cc/_3d.cc: see DUNE_GDT_BIND_OPERATOR_MODULE (operator_for_all_grids.hh)
// for the rationale behind the per-dimension split.
#define DUNE_GDT_BIND_ADVECTION_DG_MODULE(dim)                                                                         \
  namespace py = pybind11;                                                                                             \
  using namespace Dune;                                                                                                \
                                                                                                                       \
  DUNE_GDT_BIND_OPERATOR_STACK_IMPORTS;                                                                                \
                                                                                                                       \
  AdvectionDgOperator_for_all_grids<XT::Grid::bindings::Available##dim##dGridTypes>::bind(m);                          \
  m.attr("__all__") = py::make_tuple()


#endif // PYTHON_DUNE_GDT_OPERATORS_ADVECTION_DG_FOR_ALL_GRIDS_HH
