// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2012 - 2020)
//   René Fritze     (2012 - 2020)
//   Sven Kaulmann   (2014)
//   Tobias Leibner  (2014, 2016, 2020)

/// \file
/// \brief Free functions to print, measure and obtain reference elements of grid entities.

#ifndef DUNE_XT_GRID_ENTITY_HH
#define DUNE_XT_GRID_ENTITY_HH

#include <dune/geometry/referenceelements.hh>

#include <dune/grid/common/entity.hh>
#include <dune/grid/common/gridview.hh>

#include <dune/xt/common/print.hh>
#include <dune/xt/common/ranges.hh>
#include <dune/xt/common/string.hh>
#include <dune/xt/common/type_traits.hh>

#include <python/xt/dune/xt/grid/grids.bindings.hh>

namespace Dune {


/// \brief Streams a textual representation of an entity (including its corners) to an output stream.
template <int cd, int dim, class GridImp, template <int, int, class> class EntityImp>
std::ostream& operator<<(std::ostream& out, const Entity<cd, dim, GridImp, EntityImp>& entity)
{
  out << "Entity<" << cd << ", " << dim << ", " << XT::Common::Typename<GridImp>::value() << ">(";
  out << "{0: [" << entity.geometry().corner(0) << "]";
  for (int ii = 1; ii < entity.geometry().corners(); ++ii)
    out << ", " << ii << ": [" << entity.geometry().corner(ii) << "]";
  out << "})";
  return out;
}


namespace XT::Grid {


/// \brief Prints an entity and the coordinates of its corners to an output stream.
template <class EntityType>
void print_entity(const EntityType& entity,
                  const std::string& name = Common::Typename<EntityType>::value(),
                  std::ostream& out = std::cout,
                  const std::string& prefix = "")
{
  if (!name.empty())
    out << prefix << name << ":\n";
  const auto& geometry = entity.geometry();
  for (auto ii : Common::value_range(geometry.corners()))
    out << prefix + "  " << "corner " + Common::to_string(ii) << " = " << geometry.corner(ii) << "\n";
} // ... print_entity(...)


/// \brief Computes the diameter of an entity as the maximum distance between any two of its corners.
template <int codim, int worlddim, class GridImp, template <int, int, class> class EntityImp>
double diameter(const Dune::Entity<codim, worlddim, GridImp, EntityImp>& entity)
{
  auto max_dist = std::numeric_limits<typename GridImp::ctype>::min();
  const auto& geometry = entity.geometry();
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


/// \brief Computes the diameter of an entity (alias for diameter()).
template <int codim, int worlddim, class GridImp, template <int, int, class> class EntityImp>
double entity_diameter(const Dune::Entity<codim, worlddim, GridImp, EntityImp>& entity)
{
  return diameter(entity);
}


/// \brief Returns the reference element associated with the given entity.
template <int codim, int worlddim, class GridImp, template <int, int, class> class EntityImp>
auto reference_element(const Dune::Entity<codim, worlddim, GridImp, EntityImp>& entity)
    -> decltype(ReferenceElements<typename GridImp::ctype, worlddim>::general(entity.type()))
{
  return ReferenceElements<typename GridImp::ctype, worlddim>::general(entity.type());
}

/// \brief Returns the reference element associated with the given geometry.
template <int mydim, int cdim, class GridImp, template <int, int, class> class GeometryImp>
auto reference_element(const Dune::Geometry<mydim, cdim, GridImp, GeometryImp>& geometry)
    -> decltype(ReferenceElements<typename GridImp::ctype, mydim>::general(geometry.type()))
{
  return ReferenceElements<typename GridImp::ctype, mydim>::general(geometry.type());
}


} // namespace XT::Grid
} // namespace Dune

#endif // DUNE_XT_GRID_ENTITY_HH
