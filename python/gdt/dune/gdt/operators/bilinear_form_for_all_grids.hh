// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2020)

#ifndef PYTHON_DUNE_GDT_OPERATORS_BILINEAR_FORM_FOR_ALL_GRIDS_HH
#define PYTHON_DUNE_GDT_OPERATORS_BILINEAR_FORM_FOR_ALL_GRIDS_HH


#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <dune/xt/grid/type_traits.hh>
#include <dune/xt/grid/grids.hh>
#include <dune/xt/grid/gridprovider/provider.hh>
#include <dune/xt/grid/gridprovider/coupling.hh>
#include <dune/xt/grid/dd/glued.hh>
#include <dune/xt/grid/view/coupling.hh>

#include <dune/xt/la/type_traits.hh>

#include <dune/gdt/operators/bilinear-form.hh>

#include <python/xt/dune/xt/common/configuration.hh>
#include <python/xt/dune/xt/common/fvector.hh>
#include <python/xt/dune/xt/common/parameter.hh>
#include <python/xt/dune/xt/grid/grids.bindings.hh>
#include <python/xt/dune/xt/grid/walker.hh>
#include <python/xt/dune/xt/la/traits.hh>


namespace Dune {
namespace GDT {
namespace bindings {


template <class AGV, size_t s_r = 1, size_t r_r = s_r, class SGV = AGV, class RGV = AGV>
class BilinearForm
{
  using G = std::decay_t<XT::Grid::extract_grid_t<AGV>>;
  static const size_t d = G::dimension;
  using GP = XT::Grid::GridProvider<G>;

public:
  using type = GDT::BilinearForm<AGV, s_r, 1, r_r, 1, double, SGV, RGV>;
  // The base_type cannnot be binded yet because it has pure virtual functions
  //  using base_type = GDT::BilinearFormInterface<SGV, s_r, 1, r_r, 1, double, RGV>; /
  //  using bound_type = pybind11::class_<type, base_type>;
  using bound_type = pybind11::class_<type>;


private:
  using E = typename type::E;
  using I = typename type::I;
  using F = typename type::FieldType;
  using LocalElementBilinearFormType = typename type::LocalElementBilinearFormType;
  using LocalCouplingIntersectionBilinearFormType = typename type::LocalCouplingIntersectionBilinearFormType;
  using LocalIntersectionBilinearFormType = typename type::LocalIntersectionBilinearFormType;

public:
  template <class T, typename... options>
  static void addbind_methods(pybind11::class_<T, options...>& c)
  {
    namespace py = pybind11;
    using namespace pybind11::literals;

    // Append local bilinear forms via "+=" (the finalized idiom). Pass a 2-tuple
    // (local_form, filter) to restrict a form to a sub-set of elements/intersections.
    c.def("__iadd__", // function ptr signature required for the right return type
          (type & (type::*)(const LocalElementBilinearFormType&)) & type::operator+=,
          "local_element_bilinear_form"_a,
          py::is_operator());
    c.def(
        "__iadd__", // function ptr signature required for the right return type
        (type & (type::*)(const std::tuple<const LocalElementBilinearFormType&, const XT::Grid::ElementFilter<AGV>&>&))
            & type::operator+=,
        "tuple_of_localelementbilinearform_elementfilter"_a,
        py::is_operator());
    c.def("__iadd__", // function ptr signature required for the right return type
          (type & (type::*)(const LocalCouplingIntersectionBilinearFormType&)) & type::operator+=,
          "local_coupling_intersection_bilinear_form"_a,
          py::is_operator());
    c.def("__iadd__", // function ptr signature required for the right return type
          (type
           & (type::*)(const std::tuple<const LocalCouplingIntersectionBilinearFormType&,
                                        const XT::Grid::IntersectionFilter<AGV>&>&))
              & type::operator+=,
          "tuple_of_localcouplingintersectionbilinearform_intersectionfilter"_a,
          py::is_operator());
    c.def("__iadd__", // function ptr signature required for the right return type
          (type & (type::*)(const LocalIntersectionBilinearFormType&)) & type::operator+=,
          "local_intersection_bilinear_form"_a,
          py::is_operator());
    c.def("__iadd__", // function ptr signature required for the right return type
          (type
           & (type::*)(const std::tuple<const LocalIntersectionBilinearFormType&,
                                        const XT::Grid::IntersectionFilter<AGV>&>&))
              & type::operator+=,
          "tuple_of_localintersectionbilinearform_intersectionfilter"_a,
          py::is_operator());

    // The supported way to evaluate the assembled bilinear form, e.g. a(u, u) for norms:
    // apply2(range, source) walks the grid and returns the scalar a(range, source);
    // norm(range) returns sqrt(apply2(range, range)). The argument types are the grid
    // functions expected by the C++ apply2 (correct for both leaf and coupling views).
    using RangeFunc = typename type::RangeFunctionType;
    using SourceFunc = typename type::SourceFunctionType;
    c.def(
        "apply2",
        [](type& self, RangeFunc range, SourceFunc source, const XT::Common::Parameter& param) {
          return self.apply2(range, source, param);
        },
        "range"_a,
        "source"_a,
        "param"_a = XT::Common::Parameter(),
        py::call_guard<py::gil_scoped_release>());
    c.def(
        "norm",
        [](type& self, RangeFunc range, const XT::Common::Parameter& param) { return self.norm(range, param); },
        "range"_a,
        "param"_a = XT::Common::Parameter(),
        py::call_guard<py::gil_scoped_release>());
  }

  static bound_type bind_leaf(pybind11::module& m,
                              const std::string& grid_id,
                              const std::string& layer_id = "",
                              const std::string& class_id = "bilinear_form")
  {
    namespace py = pybind11;
    using namespace pybind11::literals;

    std::string class_name = class_id;
    class_name += "_" + grid_id;
    if (!layer_id.empty())
      class_name += "_" + layer_id;
    class_name += "_" + XT::Common::to_string(r_r) + "d_range";
    class_name += "_" + XT::Common::to_string(s_r) + "d_source";
    const auto ClassName = XT::Common::to_camel_case(class_name);

    bound_type c(m, ClassName.c_str(), ClassName.c_str());
    c.def(py::init(
              [](GP& grid, const std::string& logging_prefix) { return new type(grid.leaf_view(), logging_prefix); }),
          "grid"_a,
          "logging_prefix"_a = "",
          py::keep_alive<0, 1>());

    addbind_methods(c);

    // factories
    const auto FactoryName = XT::Common::to_camel_case(class_id);
    m.def(
        FactoryName.c_str(),
        [](GP& grid, const std::string& logging_prefix) { return new type(grid.leaf_view(), logging_prefix); },
        "grid"_a,
        "logging_prefix"_a = "",
        py::keep_alive<0, 1>());

    return c;

  } // ... bind_leaf(...)

  static bound_type bind_coupling(pybind11::module& m,
                                  const std::string& grid_id,
                                  const std::string& layer_id = "",
                                  const std::string& class_id = "bilinear_form")
  {
    namespace py = pybind11;
    using namespace pybind11::literals;

    std::string class_name = class_id;
    class_name += "_" + grid_id;
    if (!layer_id.empty())
      class_name += "_" + layer_id;
    class_name += "_" + XT::Common::to_string(r_r) + "d_range";
    class_name += "_" + XT::Common::to_string(s_r) + "d_source";
    const auto ClassName = XT::Common::to_camel_case(class_name);

    bound_type c(m, ClassName.c_str(), ClassName.c_str());
    c.def(py::init([](XT::Grid::CouplingGridProvider<AGV>& grid, const std::string& logging_prefix) {
            return new type(grid.coupling_view(), logging_prefix);
          }),
          "grid"_a,
          "logging_prefix"_a = "",
          py::keep_alive<0, 1>());

    addbind_methods(c);

    // factories
    const auto FactoryName = XT::Common::to_camel_case(class_id);
    m.def(
        FactoryName.c_str(),
        [](XT::Grid::CouplingGridProvider<AGV>& grid, const std::string& logging_prefix) {
          return new type(grid.coupling_view(), logging_prefix);
        },
        "grid"_a,
        "logging_prefix"_a = "",
        py::keep_alive<0, 1>());

    return c;

  } // ... bind(...)

}; // class BilinearForm


} // namespace bindings
} // namespace GDT
} // namespace Dune


template <class GridTypes = Dune::XT::Grid::bindings::AvailableGridTypes>
struct BilinearForm_for_all_grids
{
  using G = Dune::XT::Common::tuple_head_t<GridTypes>;
  using LGV = typename G::LeafGridView;
  static const constexpr size_t d = G::dimension;

  static void bind(pybind11::module& m)
  {
    using Dune::GDT::bindings::BilinearForm;
    using Dune::XT::Grid::bindings::grid_name;

    BilinearForm<LGV>::bind_leaf(m, grid_name<G>::value(), "leaf");
    if (d > 1) {
      BilinearForm<LGV, d, 1>::bind_leaf(m, grid_name<G>::value(), "leaf");
      BilinearForm<LGV, 1, d>::bind_leaf(m, grid_name<G>::value(), "leaf");
      BilinearForm<LGV, d, d>::bind_leaf(m, grid_name<G>::value(), "leaf");
    }

#if HAVE_DUNE_GRID_GLUE
    if constexpr (d == 2) {
      using GridGlueType = Dune::XT::Grid::DD::Glued<G, G, Dune::XT::Grid::Layers::leaf>;
      using CGV = Dune::XT::Grid::CouplingGridView<GridGlueType>;
      BilinearForm<CGV, 1, 1, LGV, LGV>::bind_coupling(m, grid_name<G>::value(), "coupling");
      if (d > 1) {
        BilinearForm<CGV, d, 1, LGV, LGV>::bind_coupling(m, grid_name<G>::value(), "coupling");
        BilinearForm<CGV, 1, d, LGV, LGV>::bind_coupling(m, grid_name<G>::value(), "coupling");
        BilinearForm<CGV, d, d, LGV, LGV>::bind_coupling(m, grid_name<G>::value(), "coupling");
      }
    }
#endif
    // add your extra dimensions here
    // ...

    BilinearForm_for_all_grids<Dune::XT::Common::tuple_tail_t<GridTypes>>::bind(m);
  }
};

template <>
struct BilinearForm_for_all_grids<Dune::XT::Common::tuple_null_type>
{
  static void bind(pybind11::module& /*m*/) {}
};


#endif // PYTHON_DUNE_GDT_OPERATORS_BILINEAR_FORM_FOR_ALL_GRIDS_HH
