// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   René Fritze (2026)

#include "config.h"

#include <array>
#include <vector>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <dune/xt/common/exceptions.hh>
#include <dune/xt/common/string.hh>

#include <dune/gdt/tools/euler.hh>

#include <python/xt/dune/xt/common/fmatrix.hh>
#include <python/xt/dune/xt/common/fvector.hh>

// EulerTools is a purely algebraic helper (conversions between conservative and primitive
// variables, the Euler flux f, its Jacobian and the eigendecomposition of f' * n) with no grid
// dependence, so all three dimensions fit into this single translation unit. Conservative states w
// (of size m = d + 2) and unit normals n (of size d) are plain Python sequences of floats; the
// visualize() helper (which is templated on grid layers) is not bound.
//
// NOTE: the flux Jacobian and its eigendecomposition are implemented in C++ for d in {1, 2} only
// (they raise NotImplemented for d == 3, matching the DUNE_THROW in dune/gdt/tools/euler.hh).

namespace {


template <size_t d>
struct EulerTools_binder
{
  using type = Dune::GDT::EulerTools<d>;
  static const constexpr size_t m = d + 2;
  using R = double;
  using StateType = Dune::FieldVector<R, m>;
  using NormalType = Dune::FieldVector<double, d>;

  static void bind(pybind11::module& m_)
  {
    namespace py = pybind11;
    using namespace pybind11::literals;

    const auto ClassName = Dune::XT::Common::to_camel_case("euler_tools_" + Dune::XT::Common::to_string(d) + "d");
    py::class_<type> c(m_, ClassName.c_str(), ClassName.c_str());

    c.def(py::init([](const double gamma) { return new type(gamma); }), "gamma"_a);

    c.def_property_readonly("dim_domain", [](const type&) { return d; });
    c.def_property_readonly("dim_range", [](const type&) { return m; });
    c.def_property_readonly_static("density_index", [](py::object /*cls*/) { return type::density_index(); });
    c.def_property_readonly_static("velocity_indices", [](py::object /*cls*/) { return type::velocity_indices(); });
    c.def_property_readonly_static("energy_index", [](py::object /*cls*/) { return type::energy_index(); });
    c.def_property_readonly_static("pressure_index", [](py::object /*cls*/) { return type::pressure_index(); });

    c.def("density", [](const type& self, const StateType& w) { return self.density(w)[0]; }, "w"_a);
    c.def("velocity", [](const type& self, const StateType& w) { return self.velocity(w); }, "w"_a);
    c.def("energy", [](const type& self, const StateType& w) { return self.energy(w)[0]; }, "w"_a);
    c.def("pressure", [](const type& self, const StateType& w) { return self.pressure(w)[0]; }, "w"_a);
    c.def("speed_of_sound", [](const type& self, const StateType& w) { return self.speed_of_sound(w); }, "w"_a);
    c.def("mach_number", [](const type& self, const StateType& w) { return self.mach_number(w); }, "w"_a);
    c.def("enthalpy", [](const type& self, const StateType& w) { return self.enthalpy(w); }, "w"_a);

    c.def("to_primitive", [](const type& self, const StateType& w) { return self.primitive(w); }, "w"_a);
    c.def(
        "to_conservative",
        [](const type& self, const double density, const NormalType& velocity, const double pressure) {
          return self.conservative(Dune::FieldVector<R, 1>(density),
                                   Dune::XT::Common::FieldVector<R, d>(velocity),
                                   Dune::FieldVector<R, 1>(pressure));
        },
        "density"_a,
        "velocity"_a,
        "pressure"_a);

    c.def("flux", [](const type& self, const StateType& w) { return self.flux(w); }, "w"_a);
    c.def(
        "flux_at_impermeable_walls",
        [](const type& self, const StateType& w, const NormalType& n) {
          return self.flux_at_impermeable_walls(w, Dune::XT::Common::FieldVector<double, d>(n));
        },
        "w"_a,
        "n"_a);
    c.def(
        "flux_jacobian",
        [](const type& self, const StateType& w) {
          const auto jacobians = self.flux_jacobian(w);
          std::vector<Dune::XT::Common::FieldMatrix<R, m, m>> ret;
          ret.reserve(d);
          for (size_t ss = 0; ss < d; ++ss)
            ret.push_back(jacobians[ss]);
          return ret;
        },
        "w"_a);
    c.def(
        "eigenvalues_flux_jacobian",
        [](const type& self, const StateType& w, const NormalType& n) { return self.eigenvalues_flux_jacobian(w, n); },
        "w"_a,
        "n"_a);
    c.def(
        "eigenvectors_flux_jacobian",
        [](const type& self, const StateType& w, const NormalType& n) { return self.eigenvectors_flux_jacobian(w, n); },
        "w"_a,
        "n"_a);
    c.def(
        "eigenvectors_inv_flux_jacobian",
        [](const type& self, const StateType& w, const NormalType& n) {
          return self.eigenvectors_inv_flux_jacobian(w, n);
        },
        "w"_a,
        "n"_a);
  } // ... bind(...)
}; // struct EulerTools_binder


} // namespace


PYBIND11_MODULE(_tools_euler, m)
{
  namespace py = pybind11;
  using namespace pybind11::literals;

  py::module::import("dune.xt.common");

  EulerTools_binder<1>::bind(m);
  EulerTools_binder<2>::bind(m);
  EulerTools_binder<3>::bind(m);

  // canonical factory, dispatching on the (physical) dimension like __init__.py's trampolines do
  // on the grid dimension for the grid-bound factories
  m.def(
      "EulerTools",
      [](const size_t dim, const double gamma) -> py::object {
        if (dim == 1)
          return py::cast(new Dune::GDT::EulerTools<1>(gamma));
        if (dim == 2)
          return py::cast(new Dune::GDT::EulerTools<2>(gamma));
        if (dim == 3)
          return py::cast(new Dune::GDT::EulerTools<3>(gamma));
        DUNE_THROW(Dune::XT::Common::Exceptions::wrong_input_given, "dim has to be 1, 2 or 3, is: " << dim);
        return py::none(); // unreachable, but some compilers cannot tell
      },
      "dim"_a,
      "gamma"_a = 1.4); // air (or water) at roughly 20 degrees Celsius, see dune/gdt/tools/euler.hh
}
