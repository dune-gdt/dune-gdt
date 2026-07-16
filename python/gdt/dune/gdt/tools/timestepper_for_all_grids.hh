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

#include <dune/gdt/operators/operator.hh>
#include <dune/gdt/tools/timestepper/adaptive-rungekutta.hh>
#include <dune/gdt/tools/timestepper/explicit-rungekutta.hh>
#include <dune/gdt/tools/timestepper/fractional-step.hh>

#include <python/xt/dune/xt/grid/grids.bindings.hh>

#include <python/gdt/dune/gdt/discretefunction/discretefunction_for_all_grids.hh>

// The time steppers are instantiated over the already-bound GDT::Operator base (the class
// MatrixOperator, AdvectionFvOperator and AdvectionDgOperator all derive from), so ANY bound
// operator can be stepped -- pybind11 upcasts the Python-side operator to the base reference and
// the virtual apply() dispatches to the actual scheme. Their step()/current_solution()/
// current_time() API lives on the TimeStepperInterface binding all steppers derive from; the full
// C++ solve()/EOC-study API is (still) not exposed.
//
// TimeStepperInterface has a pure virtual step(), which is no obstacle to registering it as a
// pybind11 base class -- it only means Python code cannot derive from it (which is not offered,
// there is no trampoline) and cannot construct it (no init is bound).
//
// FractionalTimeStepper/StrangSplittingTimeStepper are instantiated with the interface itself as
// both stepper types, so any combination of bound steppers can be composed without a quadratic
// number of instantiations.

namespace Dune {
namespace GDT {
namespace bindings {


template <class DiscreteFunctionImp>
class TimeStepperInterface
{
public:
  using type = GDT::TimeStepperInterface<DiscreteFunctionImp>;
  using bound_type = pybind11::class_<type>;

  static std::string class_name(const std::string& grid_id, const std::string& class_id = "time_stepper_interface")
  {
    return class_id + "_" + grid_id;
  }

  static bound_type bind(pybind11::module& m, const std::string& grid_id)
  {
    namespace py = pybind11;
    using namespace pybind11::literals;

    const auto ClassName = XT::Common::to_camel_case(class_name(grid_id));
    bound_type c(m, ClassName.c_str(), ClassName.c_str());

    // lets __init__.py's _dim_of/_make_dispatch probe a stepper for the grid dimension (needed by
    // the fractional-step/Strang splitting factories, whose arguments are themselves steppers)
    c.def_property_readonly("dim_domain", [](const type&) { return size_t(DiscreteFunctionImp::d); });

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

    return c;
  } // ... bind(...)
}; // class TimeStepperInterface


template <class OperatorImp, class DiscreteFunctionImp, TimeStepperMethods method>
class ExplicitRungeKuttaTimeStepper
{
  using InterfaceType = TimeStepperInterface<DiscreteFunctionImp>;

public:
  using type = GDT::ExplicitRungeKuttaTimeStepper<OperatorImp, DiscreteFunctionImp, method>;
  using base_type = typename InterfaceType::type;
  using bound_type = pybind11::class_<type, base_type>;

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

    // canonical, method-named factory function (e.g. "explicit_euler_time_stepper"); overloaded
    // across grid types within one dimension submodule (like the "operator" factory in
    // operator_for_all_grids.hh) and dispatched across dimensions from Python (see __init__.py)
    m.def((method_id + "_time_stepper").c_str(),
          [](const OperatorImp& op, DiscreteFunctionImp& initial_values, const double r, const double t_0) {
            // NOTE: no time stepper is copyable or movable (TimeStepperInterface explicitly deletes
            // both), so this must return a pointer.
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


template <class OperatorImp, class DiscreteFunctionImp, TimeStepperMethods method>
class AdaptiveRungeKuttaTimeStepper
{
  using InterfaceType = TimeStepperInterface<DiscreteFunctionImp>;

public:
  using type = GDT::AdaptiveRungeKuttaTimeStepper<OperatorImp, DiscreteFunctionImp, method>;
  using base_type = typename InterfaceType::type;
  using bound_type = pybind11::class_<type, base_type>;

  static bound_type bind(pybind11::module& m,
                         const std::string& grid_id,
                         const std::string& method_id,
                         const std::string& class_id = "adaptive_runge_kutta_time_stepper")
  {
    namespace py = pybind11;
    using namespace pybind11::literals;

    const auto ClassName = XT::Common::to_camel_case(class_id + "_" + method_id + "_" + grid_id);
    bound_type c(m, ClassName.c_str(), ClassName.c_str());

    c.def(py::init([](const OperatorImp& op,
                      DiscreteFunctionImp& initial_values,
                      const double r,
                      const double t_0,
                      const double tol,
                      const double scale_factor_min,
                      const double scale_factor_max) {
            return new type(op, initial_values, r, t_0, tol, scale_factor_min, scale_factor_max);
          }),
          "op"_a,
          "initial_values"_a,
          "r"_a = 1.0,
          "t_0"_a = 0.0,
          "tol"_a = 1e-4,
          "scale_factor_min"_a = 0.2,
          "scale_factor_max"_a = 5.0,
          py::keep_alive<1, 2>(),
          py::keep_alive<1, 3>());

    // NOTE: see the analogous comment in ExplicitRungeKuttaTimeStepper::bind above.
    m.def((method_id + "_time_stepper").c_str(),
          [](const OperatorImp& op,
             DiscreteFunctionImp& initial_values,
             const double r,
             const double t_0,
             const double tol,
             const double scale_factor_min,
             const double scale_factor_max) {
            return new type(op, initial_values, r, t_0, tol, scale_factor_min, scale_factor_max);
          },
          "op"_a,
          "initial_values"_a,
          "r"_a = 1.0,
          "t_0"_a = 0.0,
          "tol"_a = 1e-4,
          "scale_factor_min"_a = 0.2,
          "scale_factor_max"_a = 5.0,
          py::keep_alive<0, 1>(),
          py::keep_alive<0, 2>());

    return c;
  } // ... bind(...)
}; // class AdaptiveRungeKuttaTimeStepper


/// Binds both FractionalTimeStepper (Godunov/first-order splitting) and StrangSplittingTimeStepper,
/// which share the same two-stepper constructor signature.
template <template <class, class> class StepperImp, class DiscreteFunctionImp>
class SplittingTimeStepper
{
  using InterfaceType = TimeStepperInterface<DiscreteFunctionImp>;

public:
  using base_type = typename InterfaceType::type;
  // instantiated with the type-erased interface as both stepper types, see the file comment above
  using type = StepperImp<base_type, base_type>;
  using bound_type = pybind11::class_<type, base_type>;

  static bound_type bind(pybind11::module& m, const std::string& grid_id, const std::string& class_id)
  {
    namespace py = pybind11;
    using namespace pybind11::literals;

    const auto ClassName = XT::Common::to_camel_case(class_id + "_" + grid_id);
    bound_type c(m, ClassName.c_str(), ClassName.c_str());

    c.def(py::init([](base_type& first_stepper, base_type& second_stepper) {
            return new type(first_stepper, second_stepper);
          }),
          "first_stepper"_a,
          "second_stepper"_a,
          py::keep_alive<1, 2>(),
          py::keep_alive<1, 3>());

    // NOTE: see the analogous comment in ExplicitRungeKuttaTimeStepper::bind above.
    m.def((class_id + "_time_stepper").c_str(),
          [](base_type& first_stepper, base_type& second_stepper) { return new type(first_stepper, second_stepper); },
          "first_stepper"_a,
          "second_stepper"_a,
          py::keep_alive<0, 1>(),
          py::keep_alive<0, 2>());

    return c;
  } // ... bind(...)
}; // class SplittingTimeStepper


} // namespace bindings
} // namespace GDT
} // namespace Dune


template <class GridTypes = Dune::XT::Grid::bindings::AvailableGridTypes>
struct TimeSteppers_for_all_grids
{
  using G = Dune::XT::Common::tuple_head_t<GridTypes>;
  using GV = typename G::LeafGridView;
  using V = Dune::XT::LA::IstlDenseVector<double>;
  // the bound base of every (matrix-, FV- and DG-) operator, see the file comment above
  using OperatorType = Dune::GDT::Operator<GV, 1, 1, 1>;
  using DiscreteFunctionType = Dune::GDT::DiscreteFunction<V, GV, 1>;

  static void bind(pybind11::module& m)
  {
    using Dune::GDT::FractionalTimeStepper;
    using Dune::GDT::StrangSplittingTimeStepper;
    using Dune::GDT::TimeStepperMethods;
    using Dune::GDT::bindings::AdaptiveRungeKuttaTimeStepper;
    using Dune::GDT::bindings::ExplicitRungeKuttaTimeStepper;
    using Dune::GDT::bindings::SplittingTimeStepper;
    using Dune::GDT::bindings::TimeStepperInterface;
    using Dune::XT::Grid::bindings::grid_name;

    TimeStepperInterface<DiscreteFunctionType>::bind(m, grid_name<G>::value());

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

    AdaptiveRungeKuttaTimeStepper<OperatorType, DiscreteFunctionType, TimeStepperMethods::bogacki_shampine>::bind(
        m, grid_name<G>::value(), "bogacki_shampine");
    AdaptiveRungeKuttaTimeStepper<OperatorType, DiscreteFunctionType, TimeStepperMethods::dormand_prince>::bind(
        m, grid_name<G>::value(), "dormand_prince");

    SplittingTimeStepper<FractionalTimeStepper, DiscreteFunctionType>::bind(
        m, grid_name<G>::value(), "fractional_step");
    SplittingTimeStepper<StrangSplittingTimeStepper, DiscreteFunctionType>::bind(
        m, grid_name<G>::value(), "strang_splitting");

    TimeSteppers_for_all_grids<Dune::XT::Common::tuple_tail_t<GridTypes>>::bind(m);
  }
};

template <>
struct TimeSteppers_for_all_grids<Dune::XT::Common::tuple_null_type>
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
                                                                                                                       \
  TimeSteppers_for_all_grids<XT::Grid::bindings::Available##dim##dGridTypes>::bind(m);                                 \
  m.attr("__all__") = py::make_tuple()


#endif // PYTHON_DUNE_GDT_TOOLS_TIMESTEPPER_FOR_ALL_GRIDS_HH
