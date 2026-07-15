// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   René Fritze (2026)

#ifndef PYTHON_DUNE_GDT_TOOLS_TIMESTEPPER_FOR_ALL_GRIDS_HH
#define PYTHON_DUNE_GDT_TOOLS_TIMESTEPPER_FOR_ALL_GRIDS_HH

#include <limits>
#include <string>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <dune/xt/common/string.hh>
#include <dune/xt/grid/grids.hh>
#include <dune/xt/grid/type_traits.hh>
#include <dune/xt/la/container.hh>

#include <dune/gdt/tools/timestepper/explicit-rungekutta.hh>

#include <python/xt/dune/xt/grid/grids.bindings.hh>

#include <python/gdt/dune/gdt/discretefunction/discretefunction_for_all_grids.hh>
#include <python/gdt/dune/gdt/operators/advection-fv_for_all_grids.hh>

// Only ExplicitRungeKuttaTimeStepper is bound (explicit Euler is its default method template
// argument), instantiated over AdvectionFvOperator on every already-bound grid type -- enough to
// drive the WP6 example notebook's explicit time loop (#320). AdaptiveRungeKuttaTimeStepper and the
// fractional-step/Strang splitting time steppers are left to a follow-up.
//
// TimeStepperInterface has a pure virtual `step`, so (like OperatorInterface's commented-out
// trampoline, see python/gdt/dune/gdt/operators/interfaces.hh) it is not bound as a pybind11 base;
// ExplicitRungeKuttaTimeStepper is bound standalone instead.

namespace Dune {
namespace GDT {
namespace bindings {


template <class OperatorImp, class DiscreteFunctionImp, TimeStepperMethods method>
class ExplicitRungeKuttaTimeStepper
{
public:
  using type = GDT::ExplicitRungeKuttaTimeStepper<OperatorImp, DiscreteFunctionImp, method>;
  using bound_type = pybind11::class_<type>;

  static bound_type bind(pybind11::module& m,
                         const std::string& grid_id,
                         const std::string& method_id,
                         const std::string& class_id = "explicit_runge_kutta_time_stepper")
  {
    namespace py = pybind11;
    using namespace pybind11::literals;

    const auto ClassName = XT::Common::to_camel_case(class_id + "_" + method_id + "_" + grid_id);
    bound_type c(m, ClassName.c_str(), ClassName.c_str());

    c.def(py::init([](const OperatorImp& op, DiscreteFunctionImp& initial_values, const double r, const double t_0) {
            return new type(op, initial_values, r, t_0);
          }),
          "op"_a,
          "initial_values"_a,
          "r"_a = 1.0,
          "t_0"_a = 0.0,
          py::keep_alive<1, 2>(),
          py::keep_alive<1, 3>());

    c.def(
        "step",
        [](type& self, const double dt, const double max_dt) { return self.step(dt, max_dt); },
        "dt"_a,
        "max_dt"_a = std::numeric_limits<double>::max(),
        py::call_guard<py::gil_scoped_release>());

    c.def(
        "current_solution",
        [](type& self) -> DiscreteFunctionImp& { return self.current_solution(); },
        py::return_value_policy::reference_internal);
    c.def("current_time", [](const type& self) { return self.current_time(); });

    // canonical, method-named factory function (e.g. "explicit_euler_time_stepper"); overloaded
    // across grid types within one dimension submodule (like the "operator" factory in
    // operator_for_all_grids.hh) and dispatched across dimensions from Python (see __init__.py)
    m.def((method_id + "_time_stepper").c_str(),
          [](const OperatorImp& op, DiscreteFunctionImp& initial_values, const double r, const double t_0) {
            // NOTE: unlike AdvectionFvOperator, ExplicitRungeKuttaTimeStepper is neither copyable nor
            // movable (TimeStepperInterface explicitly deletes both), so this must return a pointer.
            return new type(op, initial_values, r, t_0);
          },
          "op"_a,
          "initial_values"_a,
          "r"_a = 1.0,
          "t_0"_a = 0.0,
          py::keep_alive<0, 1>(),
          py::keep_alive<0, 2>());

    return c;
  } // ... bind(...)
}; // class ExplicitRungeKuttaTimeStepper


} // namespace bindings
} // namespace GDT
} // namespace Dune


template <class GridTypes = Dune::XT::Grid::bindings::AvailableGridTypes>
struct ExplicitRungeKuttaTimeStepper_for_all_grids
{
  using G = Dune::XT::Common::tuple_head_t<GridTypes>;
  using GV = typename G::LeafGridView;
  using M = Dune::XT::LA::IstlRowMajorSparseMatrix<double>;
  using V = Dune::XT::LA::IstlDenseVector<double>;
  using OperatorType = Dune::GDT::AdvectionFvOperator<GV, 1, double, M>;
  using DiscreteFunctionType = Dune::GDT::DiscreteFunction<V, GV, 1>;

  static void bind(pybind11::module& m)
  {
    using Dune::GDT::TimeStepperMethods;
    using Dune::GDT::bindings::ExplicitRungeKuttaTimeStepper;
    using Dune::XT::Grid::bindings::grid_name;

    ExplicitRungeKuttaTimeStepper<OperatorType, DiscreteFunctionType, TimeStepperMethods::explicit_euler>::bind(
        m, grid_name<G>::value(), "explicit_euler");
    ExplicitRungeKuttaTimeStepper<
        OperatorType,
        DiscreteFunctionType,
        TimeStepperMethods::explicit_rungekutta_second_order_ssp>::bind(m,
                                                                        grid_name<G>::value(),
                                                                        "explicit_rungekutta_second_order_ssp");
    ExplicitRungeKuttaTimeStepper<
        OperatorType,
        DiscreteFunctionType,
        TimeStepperMethods::explicit_rungekutta_third_order_ssp>::bind(m,
                                                                       grid_name<G>::value(),
                                                                       "explicit_rungekutta_third_order_ssp");
    ExplicitRungeKuttaTimeStepper<
        OperatorType,
        DiscreteFunctionType,
        TimeStepperMethods::explicit_rungekutta_classic_fourth_order>::bind(m,
                                                                            grid_name<G>::value(),
                                                                            "explicit_rungekutta_classic_fourth_order");

    ExplicitRungeKuttaTimeStepper_for_all_grids<Dune::XT::Common::tuple_tail_t<GridTypes>>::bind(m);
  }
};

template <>
struct ExplicitRungeKuttaTimeStepper_for_all_grids<Dune::XT::Common::tuple_null_type>
{
  static void bind(pybind11::module& /*m*/) {} // recursion base case: no grid types left to bind
};


// Shared by timestepper_1d.cc/_2d.cc/_3d.cc: see DUNE_GDT_BIND_OPERATOR_MODULE (operator_for_all_grids.hh)
// for the rationale behind the per-dimension split.
#define DUNE_GDT_BIND_TIMESTEPPER_MODULE(dim)                                                                          \
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
  py::module::import("dune.gdt._spaces_interface");                                                                    \
  py::module::import("dune.gdt._discretefunction_dof_vector");                                                         \
  py::module::import("dune.gdt._discretefunction_discretefunction_1d");                                                \
  py::module::import("dune.gdt._discretefunction_discretefunction_2d");                                                \
  py::module::import("dune.gdt._discretefunction_discretefunction_3d");                                                \
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
  py::module::import("dune.gdt._operators_advection_fv_1d");                                                           \
  py::module::import("dune.gdt._operators_advection_fv_2d");                                                           \
  py::module::import("dune.gdt._operators_advection_fv_3d");                                                           \
                                                                                                                       \
  ExplicitRungeKuttaTimeStepper_for_all_grids<XT::Grid::bindings::Available##dim##dGridTypes>::bind(m);                \
  m.attr("__all__") = py::make_tuple()


#endif // PYTHON_DUNE_GDT_TOOLS_TIMESTEPPER_FOR_ALL_GRIDS_HH
