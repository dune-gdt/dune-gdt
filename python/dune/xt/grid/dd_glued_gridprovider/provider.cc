// This file is part of the dune-xt project:
//   https://github.com/dune-community/dune-xt
// Copyright 2009-2018 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:

#include "config.h"

#include <algorithm>

#include <dune/geometry/type.hh>
#include <dune/grid/common/mcmgmapper.hh>

#include <dune/pybindxi/pybind11.h>
#include <dune/pybindxi/stl.h>
#include <dune/xt/common/numeric_cast.hh>
#include <dune/xt/common/parallel/mpi_comm_wrapper.hh>
#include <dune/xt/common/ranges.hh>
#include <dune/xt/la/container/common/vector/dense.hh>
#include <dune/xt/grid/entity.hh>
#include <dune/xt/grid/element.hh>
#include <dune/xt/grid/exceptions.hh>
#include <dune/xt/grid/gridprovider/dgf.hh>
#include <dune/xt/grid/gridprovider/provider.hh>
#include <dune/xt/grid/dd/glued.hh>
#include <dune/xt/grid/filters/intersection.hh>
#include <dune/xt/grid/mapper.hh>
#include <dune/xt/functions/generic/grid-function.hh>

#include <python/dune/xt/common/configuration.hh>
#include <python/dune/xt/common/fvector.hh>
#include <python/dune/xt/grid/grids.bindings.hh>



/**
 * Inherits all types and methods from the coupling intersection, but uses the macro intersection to provide a correctly
 * oriented normal.
 *
 * \attention Presumes that the coupling intersection lies exactly within the macro intersection!
 */
template <class CouplingIntersectionType, class MacroIntersectionType>
class CouplingIntersectionWithCorrectNormal : public CouplingIntersectionType
{
  using BaseType = CouplingIntersectionType;

public:
  using typename BaseType::GlobalCoordinate;
  using typename BaseType::LocalCoordinate;

  enum
  {
    dimension = MacroIntersectionType::dimension
  };

  CouplingIntersectionWithCorrectNormal(const CouplingIntersectionType& coupling_intersection,
                                        const MacroIntersectionType& macro_intersection)
    : BaseType(coupling_intersection)
    , macro_intersection_(macro_intersection)
  {}

  GlobalCoordinate outerNormal(const LocalCoordinate& local) const
  {
    global_ = this->geometry().global(local);
    local_ = macro_intersection_.geometry().local(global_);
    return macro_intersection_.outerNormal(local_);
  }

  GlobalCoordinate integrationOuterNormal(const LocalCoordinate& local) const
  {
    auto normal = this->unitOuterNormal(local);
    const auto integration_element = BaseType::integrationOuterNormal(local).two_norm();
    normal *= integration_element;
    return normal;
  }

  GlobalCoordinate unitOuterNormal(const LocalCoordinate& local) const
  {
    global_ = this->geometry().global(local);
    local_ = macro_intersection_.geometry().local(global_);
    return macro_intersection_.unitOuterNormal(local_);
  }

  GlobalCoordinate centerUnitOuterNormal() const
  {
    global_ = this->geometry().center();
    local_ = macro_intersection_.geometry().local(global_);
    return macro_intersection_.unitOuterNormal(local_);
  }

private:
  const MacroIntersectionType& macro_intersection_;
  mutable GlobalCoordinate global_;
  mutable LocalCoordinate local_;
}; // class CouplingIntersectionWithCorrectNormal



namespace Dune::XT::Grid{

template <class MacroGV, class MicroGV>
class MacroGridBasedBoundaryInfo : public Dune::XT::Grid::BoundaryInfo<Dune::XT::Grid::extract_intersection_t<MicroGV>>
{
  using BaseType = Dune::XT::Grid::BoundaryInfo<Dune::XT::Grid::extract_intersection_t<MicroGV>>;

public:
  using typename BaseType::IntersectionType;
  using MacroBoundaryInfoType = Dune::XT::Grid::BoundaryInfo<Dune::XT::Grid::extract_intersection_t<MacroGV>>;
  using MacroElementType = Dune::XT::Grid::extract_entity_t<MacroGV>;

  MacroGridBasedBoundaryInfo(const MacroGV& macro_grid_view,
                             const MacroElementType& macro_element,
                             const MacroBoundaryInfoType& macro_boundary_info)
    : macro_grid_view_(macro_grid_view)
    , macro_element_(macro_element)
    , macro_boundary_info_(macro_boundary_info)
  {}

  const Dune::XT::Grid::BoundaryType& type(const IntersectionType& intersection) const override final
  {
    // find out if this micro intersection lies within the macro element or on a macro intersection
    for (auto&& macro_intersection : intersections(macro_grid_view_, macro_element_)) {
      const size_t num_corners = intersection.geometry().corners();
      size_t num_corners_inside = 0;
      size_t num_corners_outside = 0;
      for (size_t cc = 0; cc < num_corners; ++cc) {
        const auto micro_corner = intersection.geometry().corner(cc);
        if (XT::Grid::contains(macro_intersection, micro_corner))
          ++num_corners_inside;
        else
          ++num_corners_outside;
      }
      if (num_corners_inside == num_corners && num_corners_outside == 0) {
        // we found the macro intersection this micro intersection belongs to
        return macro_boundary_info_.type(macro_intersection);
      }
    }
    // we could not find a macro intersection this micro intersection belongs to
    return no_boundary_;
  } // ... type(...)

  const MacroGV& macro_grid_view_;
  const MacroElementType& macro_element_;
  const MacroBoundaryInfoType& macro_boundary_info_;
  const XT::Grid::NoBoundary no_boundary_;
}; // class MacroGridBasedBoundaryInfo

// Since CouplingIntersectionWithCorrectNormal is not derived from Dune::Intersection, we need this copy here :/
template <class C, class I>
double diameter(const CouplingIntersectionWithCorrectNormal<C, I>& intersection)
{
  auto max_dist = std::numeric_limits<typename I::ctype>::min();
  const auto& geometry = intersection.geometry();
  for (auto i : Common::value_range(geometry.corners())) {
    const auto xi = geometry.corner(i);
    for (auto j : Common::value_range(i + 1, geometry.corners())) {
      auto xj = geometry.corner(j);
      xj -= xi;
      max_dist = std::max(max_dist, xj.two_norm());
    }
  }
  return max_dist;
} // diameter

} // namespace Dune::XT::Grid

namespace Dune::XT::Grid::bindings {


template <class G>
class GluedGridProvider
{
public:
  using type = Grid::DD::Glued<G, G, XT::Grid::Layers::leaf>;
  using bound_type = pybind11::class_<type>;

  using GV = typename type::LocalViewType;

  static bound_type bind(pybind11::module& m,
                         const std::string& class_id = "glued_grid_provider",
                         const std::string& grid_id = grid_name<G>::value())
  {
    namespace py = pybind11;
    using namespace pybind11::literals;
    constexpr const int dim = type::dimDomain;

    const std::string class_name = class_id + "_" + grid_id;
    const auto ClassName = XT::Common::to_camel_case(class_name);
    bound_type c(m, ClassName.c_str(), (XT::Common::to_camel_case(class_id) + " (" + grid_id + " variant)").c_str());
    c.def_property_readonly("dimension", [](type&) { return dim; });
    c.def(
          "local_grid",
          [](type& self, const size_t macro_entity_index) {
          auto local_grid = self.local_grid(macro_entity_index);
          return local_grid;
          },
          "macro_entity_index"_a);
    c.def_property_readonly("num_subdomains", [](type& self) { return self.num_subdomains(); });
    c.def_property_readonly("boundary_subdomains", [](type& self) {
        std::vector<size_t> boundary_subdomains;
        for (auto&& macro_element : elements(self.macro_grid_view()))
          if (self.boundary(macro_element))
            boundary_subdomains.push_back(self.subdomain(macro_element));
          return boundary_subdomains;
      });
    c.def("neighbors", [](type& self, const size_t ss) {
        DUNE_THROW_IF(ss >= self.num_subdomains(),
                        XT::Common::Exceptions::index_out_of_range,
                        "ss = " << ss << "\n   self.num_subdomains() = " << self.num_subdomains());
         std::vector<size_t> neighboring_subdomains;
         for (auto&& macro_element : elements(self.macro_grid_view())) {
           if (self.subdomain(macro_element) == ss) {
             for (auto&& macro_intersection : intersections(self.macro_grid_view(), macro_element))
               if (macro_intersection.neighbor())
                 neighboring_subdomains.push_back(self.subdomain(macro_intersection.outside()));
             break;
           }
         }
         return neighboring_subdomains;
    },
    "ss"_a);
    c.def("coupling_grid", [](type& self, const size_t ss, const size_t nn){
        for (auto&& inside_macro_element : elements(self.macro_grid_view())) {
          if (self.subdomain(inside_macro_element) == ss) {
            // this is the subdomain we are interested in, create space
//            auto inner_subdomain_grid_view = self.local_grid(inside_macro_element).leaf_view();
//            auto inner_subdomain_space = make_subdomain_space(inner_subdomain_grid_view, space_type);
            bool found_correct_macro_intersection = false;
            for (auto&& macro_intersection :
                 intersections(self.macro_grid_view(), inside_macro_element)) {
              if (macro_intersection.neighbor()) {
                const auto outside_macro_element = macro_intersection.outside();
                if (self.subdomain(outside_macro_element) == nn) {
                  found_correct_macro_intersection = true;
                  // these are the subdomains we are interested in
                  const auto& coupling = self.coupling(inside_macro_element, -1, outside_macro_element, -1, true);
                  using MacroI = std::decay_t<decltype(macro_intersection)>;
                  using CouplingI = typename type::GlueType::Intersection;
                  using I = CouplingIntersectionWithCorrectNormal<CouplingI, MacroI>;
                  const auto coupling_intersection_it_end = coupling.template iend<0>();
                  for (auto coupling_intersection_it = coupling.template ibegin<0>();
                       coupling_intersection_it != coupling_intersection_it_end;
                       ++coupling_intersection_it) {
//                    const auto& coupling_intersection = *coupling_intersection_it;
                    const I coupling_intersection(*coupling_intersection_it, macro_intersection);
                    std::cout << diameter(coupling_intersection) << std::endl;
//                    return *coupling_intersection_it;  // no bindings available !!
                  }
                }
              }
            }
            DUNE_THROW_IF(!found_correct_macro_intersection,
                          XT::Common::Exceptions::index_out_of_range,
                          "ss = " << ss << "\n   nn = " << nn);
          }
        }
    },
    "ss"_a,
    "nn"_a);
    c.def("macro_based_boundary_info", [](type& self, const size_t ss) {
        DUNE_THROW_IF(ss >= self.num_subdomains(),
                      XT::Common::Exceptions::index_out_of_range,
                      "ss = " << ss << "\n   self.num_subdomains() = "
                              << self.num_subdomains());
        using MGV = typename type::MacroGridViewType;
        const XT::Grid::AllDirichletBoundaryInfo<XT::Grid::extract_intersection_t<MGV>> macro_boundary_info;
//        std::unique_ptr<M> subdomain_matrix;
        for (auto&& macro_element : elements(self.macro_grid_view())) {
          if (self.subdomain(macro_element) == ss) {
            // this is the subdomain we are interested in, create space
//            auto subdomain_grid_view = self.local_grid(macro_element).leaf_view();
//            using I = typename GV::Intersection;
//            auto subdomain_space = make_subdomain_space(subdomain_grid_view, space_type);
            const MacroGridBasedBoundaryInfo<MGV, GV> subdomain_boundary_info(
                self.macro_grid_view(), macro_element, macro_boundary_info);
            return subdomain_boundary_info;
          }
        }
    },
    "ss"_a);
    c.def(
        "write_global_visualization",
        [](type& self,
           const std::string& filename_prefix,
           const XT::Functions::FunctionInterface<dim>& func,
           const std::list<int>& subdomains)
            { self.write_global_visualization(filename_prefix, func, subdomains); },
        "filename_prefix"_a,
        "function"_a,
        "subdomains"_a);
    return c;
  } // ... bind(...)
}; // class GridProvider

template <class G>
struct MacroGridBasedBoundaryInfo
{
  using GV = typename G::LeafGridView;
  using I = Dune::XT::Grid::extract_intersection_t<GV>;
  using type = Dune::XT::Grid::MacroGridBasedBoundaryInfo<GV, GV>;
//  using base_type = Dune::XT::Grid::BoundaryInfo<I>;
  using bound_type = pybind11::class_<type>;

  static bound_type bind(pybind11::module& m,
                   const std::string& class_id = "macro_grid_based_boundary_info",
                   const std::string& grid_id = grid_name<G>::value())
  {
    namespace py = pybind11;
    using namespace pybind11::literals;
    const std::string class_name = class_id + "_" + grid_id;
    const auto ClassName = XT::Common::to_camel_case(class_name);
    bound_type c(m, ClassName.c_str(), (XT::Common::to_camel_case(class_id) + " (" + grid_id + " variant)").c_str());
    c.def(
        "type",
        [](type& self,
           const I& intersection)    // no bindings available !!
            { self.type(intersection); },
        "intersection"_a);
//    c.def(
//        py::init([](const Grid::BoundaryType& default_boundary_type, const D& tol, const std::string& logging_prefix) {
//          return std::make_unique<type>(tol, default_boundary_type.copy(), logging_prefix);
//        }),
//        "default_boundary_type"_a,
//        "tolerance"_a = 1e-10,
//        "logging_prefix"_a = "");
//    c.def(
//        "register_new_normal",
//        [](type& self, const typename type::WorldType& normal, const Grid::BoundaryType& boundary_type) {
//          self.register_new_normal(normal, boundary_type.copy());
//        },
//        "normal"_a,
//        "boundary_type"_a);

//    m.def(
//        Common::to_camel_case(class_id).c_str(),
//        [](const Grid::GridProvider<G>&,
//           const Grid::BoundaryType& default_boundary_type,
//           const D& tol,
//           const std::string& logging_prefix) {
//          return std::make_unique<type>(tol, default_boundary_type.copy(), logging_prefix);
//        },
//        "grid_provider"_a,
//        "default_boundary_type"_a,
//        "tolerance"_a = 1e-10,
//        "logging_prefix"_a = "");
    return c;
  } // ... bind(...)
}; // struct NormalBasedBoundaryInfo_for_all_grids

} // namespace Dune::XT::Grid::bindings


#include <dune/xt/grid/grids.hh>

template <class GridTypes = Dune::XT::Grid::Available2dGridTypes>  // grid-glue only working 2d
struct GluedGridProvider_for_all_grids
{
  static void bind(pybind11::module& m)
  {
    Dune::XT::Grid::bindings::GluedGridProvider<Dune::XT::Common::tuple_head_t<GridTypes>>::bind(m);
    GluedGridProvider_for_all_grids<Dune::XT::Common::tuple_tail_t<GridTypes>>::bind(m);
  }
};

template <>
struct GluedGridProvider_for_all_grids<Dune::XT::Common::tuple_null_type>
{
  static void bind(pybind11::module& /*m*/) {}
};

template <class GridTypes = Dune::XT::Grid::AvailableGridTypes>  // grid-glue only working 2d
struct MacroGridBasedBoundaryInfo_for_all_grids
{
  static void bind(pybind11::module& m)
  {
    Dune::XT::Grid::bindings::MacroGridBasedBoundaryInfo<Dune::XT::Common::tuple_head_t<GridTypes>>::bind(m);
    MacroGridBasedBoundaryInfo_for_all_grids<Dune::XT::Common::tuple_tail_t<GridTypes>>::bind(m);
  }
};

template <>
struct MacroGridBasedBoundaryInfo_for_all_grids<Dune::XT::Common::tuple_null_type>
{
  static void bind(pybind11::module& /*m*/) {}
};


PYBIND11_MODULE(_grid_dd_glued_gridprovider_provider, m)
{
  namespace py = pybind11;
  using namespace Dune::XT::Grid;

  py::module::import("dune.xt.common");
  py::module::import("dune.xt.la");
  py::module::import("dune.xt.grid._grid_boundaryinfo_interfaces");
  py::module::import("dune.xt.grid._grid_boundaryinfo_types");
  py::module::import("dune.xt.grid._grid_filters_base");

//#define BIND_(NAME) InitlessIntersectionFilter_for_all_grids<ApplyOn::NAME>::bind(m, std::string("ApplyOn") + #NAME)

//  BIND_(AllIntersections);
//  BIND_(AllIntersectionsOnce);
//  BIND_(NoIntersections);
//  BIND_(InnerIntersections);
//  BIND_(InnerIntersectionsOnce);
//  //  BIND_(PartitionSetInnerIntersectionsOnce); <- requires partition set as template argument
//  BIND_(BoundaryIntersections);
//  BIND_(NonPeriodicBoundaryIntersections);
//  BIND_(PeriodicBoundaryIntersections);
//  BIND_(PeriodicBoundaryIntersectionsOnce);
//  //  BIND_(GenericFilteredIntersections); <- requires lambda in init
//  //  BIND_(CustomBoundaryAndProcessIntersections); <- requires boundary type and info in init
//  BIND_(ProcessIntersections);

//#undef BIND_

//  CustomBoundaryIntersectionFilter_for_all_grids<>::bind(m);

  GluedGridProvider_for_all_grids<>::bind(m);
  MacroGridBasedBoundaryInfo_for_all_grids<>::bind(m);
}
