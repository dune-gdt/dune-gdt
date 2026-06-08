// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2020)

#include "config.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <dune/xt/common/string.hh>
#include <dune/xt/grid/type_traits.hh>
#include <dune/xt/grid/gridprovider/provider.hh>
#include <dune/xt/la/type_traits.hh>

#include <dune/gdt/operators/laplace-ipdg-flux-reconstruction.hh>

#include <python/xt/dune/xt/common/configuration.hh>
#include <python/xt/dune/xt/common/fvector.hh>
#include <python/xt/dune/xt/common/parameter.hh>
#include <python/xt/dune/xt/grid/grids.bindings.hh>
#include <python/xt/dune/xt/grid/walker.hh>
#include <python/xt/dune/xt/la/traits.hh>
#include <python/gdt/dune/gdt/operators/interfaces.hh>


namespace Dune {
namespace GDT {
namespace bindings {


template <class GV>
class LaplaceIpdgFluxReconstructionOperator
{
  using G = std::decay_t<XT::Grid::extract_grid_t<GV>>;
  static const size_t d = G::dimension;
  using GP = XT::Grid::GridProvider<G>;

public:
  using type = GDT::LaplaceIpdgFluxReconstructionOperator<GV>;
  // The C++ operator derives from ForwardOperatorInterface, which is not exposed as a Python base
  // class (it carries pure virtual methods that are not bound). We therefore bind the few methods
  // required by the tutorials (assemble, apply) directly onto a base-less pybind11 class.
  using bound_type = pybind11::class_<type>;

private:
  using RS = typename type::RangeSpaceType;
  using V = typename type::VectorType;
  using SF = typename type::SourceFunctionType;
  using E = typename type::E;

public:
  static bound_type bind(pybind11::module& m,
                         const std::string& grid_id,
                         const std::string& layer_id = "",
                         const std::string& class_id = "laplace_ipdg_flux_reconstruction_operator")
  {
    namespace py = pybind11;
    using namespace pybind11::literals;

    std::string class_name = class_id;
    class_name += "_" + grid_id;
    if (!layer_id.empty())
      class_name += "_" + layer_id;
    const auto ClassName = XT::Common::to_camel_case(class_name);
    bound_type c(m, ClassName.c_str(), ClassName.c_str());
    c.def(py::init([](GP& grid,
                      const RS& range_space,
                      const double& symmetry_prefactor,
                      const double& inner_penalty,
                      const double& dirichlet_penalty,
                      XT::Functions::GridFunction<E, d, d> diffusion,
                      XT::Functions::GridFunction<E, d, d> weight_function) {
            return new type(grid.leaf_view(),
                            range_space,
                            symmetry_prefactor,
                            inner_penalty,
                            dirichlet_penalty,
                            diffusion,
                            weight_function);
          }),
          "grid"_a,
          "range_space"_a,
          "symmetry_prefactor"_a,
          "inner_penalty"_a,
          "dirichlet_penalty"_a,
          "diffusion"_a,
          "weight_function"_a,
          py::keep_alive<1, 2>(),
          py::keep_alive<1, 3>());

    c.def("assemble", [](type& self, const bool parallel) { self.assemble(parallel); }, "parallel"_a = false);
    c.def(
        "apply",
        [](type& self, SF source, V& range, const XT::Common::Parameter& param) { self.apply(source, range, param); },
        "source"_a,
        "range"_a,
        "param"_a = XT::Common::Parameter(),
        py::call_guard<py::gil_scoped_release>());
    c.def(
        "apply",
        [](type& self, SF source, const XT::Common::Parameter& param) { return self.apply(source, param); },
        "source"_a,
        "param"_a = XT::Common::Parameter(),
        py::call_guard<py::gil_scoped_release>());

    // factory function with a grid-independent name (the class itself is registered with a
    // grid-specific name); this is the symbol exported as `LaplaceIpdgFluxReconstructionOperator`.
    const auto FactoryName = XT::Common::to_camel_case(class_id);
    m.def(
        FactoryName.c_str(),
        [](GP& grid,
           const RS& range_space,
           const double& symmetry_prefactor,
           const double& inner_penalty,
           const double& dirichlet_penalty,
           XT::Functions::GridFunction<E, d, d> diffusion,
           XT::Functions::GridFunction<E, d, d> weight_function) {
          // NOTE: the operator stores the assembly grid view BY REFERENCE, so it must outlive the
          // operator. grid.leaf_view() returns a temporary (-> dangling ref -> segfault on apply);
          // use the range space's grid view instead (the space is kept alive below).
          return new type(range_space.grid_view(),
                          range_space,
                          symmetry_prefactor,
                          inner_penalty,
                          dirichlet_penalty,
                          diffusion,
                          weight_function);
        },
        "grid"_a,
        "range_space"_a,
        "symmetry_prefactor"_a,
        "inner_penalty"_a,
        "dirichlet_penalty"_a,
        "diffusion"_a,
        "weight_function"_a,
        py::keep_alive<0, 1>(),
        py::keep_alive<0, 2>());

    return c;
  } // ... bind(...)
}; // class LaplaceIpdgFluxReconstructionOperator


} // namespace bindings
} // namespace GDT
} // namespace Dune


template <class GridTypes = Dune::XT::Grid::bindings::AvailableGridTypes>
struct LaplaceIpdgFluxReconstructionOperator_for_all_grids
{
  using G = Dune::XT::Common::tuple_head_t<GridTypes>;
  using GV = typename G::LeafGridView;

  static void bind(pybind11::module& m)
  {
    using Dune::GDT::bindings::LaplaceIpdgFluxReconstructionOperator;
    using Dune::XT::Grid::bindings::grid_name;

    LaplaceIpdgFluxReconstructionOperator<GV>::bind(m, grid_name<G>::value(), "leaf");

    LaplaceIpdgFluxReconstructionOperator_for_all_grids<Dune::XT::Common::tuple_tail_t<GridTypes>>::bind(m);
  }
};

template <>
struct LaplaceIpdgFluxReconstructionOperator_for_all_grids<Dune::XT::Common::tuple_null_type>
{
  static void bind(pybind11::module& /*m*/) {}
};


PYBIND11_MODULE(_operators_laplace_ipdg_flux_reconstruction, m)
{
  namespace py = pybind11;
  using namespace Dune;
  using namespace Dune::XT;
  using namespace Dune::GDT;

  py::module::import("dune.xt.common");
  py::module::import("dune.xt.la");
  py::module::import("dune.xt.grid");
  py::module::import("dune.xt.functions");

  py::module::import("dune.gdt._operators_interfaces_common");
  py::module::import("dune.gdt._operators_interfaces_eigen");
  py::module::import("dune.gdt._operators_interfaces_istl_1d");
  py::module::import("dune.gdt._operators_interfaces_istl_2d");
  py::module::import("dune.gdt._operators_interfaces_istl_3d");
  py::module::import("dune.gdt._spaces_interface");

  LaplaceIpdgFluxReconstructionOperator_for_all_grids<XT::Grid::bindings::AvailableGridTypes>::bind(m);
}
