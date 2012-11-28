#ifndef DUNE_STUFF_GRID_PROVIDER_GMSH_HH
#define DUNE_STUFF_GRID_PROVIDER_GMSH_HH

#ifdef HAVE_CMAKE_CONFIG
#include "cmake_config.h"
#else
#include "config.h"
#endif // ifdef HAVE_CMAKE_CONFIG

// system
#include <sstream>
#include <type_traits>
#include <boost/assign/list_of.hpp>

// dune-common
#include <dune/common/parametertree.hh>
#include <dune/common/shared_ptr.hh>
#include <dune/common/exceptions.hh>
#include <dune/common/fvector.hh>

// dune-grid
#include <dune/grid/utility/structuredgridfactory.hh>
#include <dune/grid/yaspgrid.hh>
#ifdef HAVE_ALUGRID
#include <dune/grid/alugrid.hh>
#endif
#include <dune/grid/sgrid.hh>
//#include <dune/grid/common/mcmgmapper.hh>
//#include <dune/grid/io/file/vtk/vtkwriter.hh>
#include <dune/grid/io/file/gmshreader.hh>

// dune-stuff
#include <dune/stuff/common/parameter/tree.hh>

// local
#include "interface.hh"

namespace Dune {
namespace Stuff {
namespace Grid {
namespace Provider {

/**
 * \brief   Gmsh grid provider
 */
#if defined HAVE_CONFIG_H || defined HAVE_CMAKE_CONFIG
template <class GridImp = Dune::GridSelector::GridType>
#else // defined HAVE_CONFIG_H || defined HAVE_CMAKE_CONFIG
template <class GridImp = Dune::SGrid<2, 2>>
#endif // defined HAVE_CONFIG_H || defined HAVE_CMAKE_CONFIG
class Gmsh : public Interface<GridImp>
{
public:
  //! Type of the provided grid.
  typedef GridImp GridType;

  typedef Interface<GridType> BaseType;

  typedef Gmsh<GridType> ThisType;

  //! Dimension of the provided grid.
  static const unsigned int dim = BaseType::dim;

  //! Type of the grids coordinates.
  typedef typename BaseType::CoordinateType CoordinateType;

  Gmsh(const Dune::Stuff::Common::ExtendedParameterTree& paramTree)
  {
    dune_static_assert(!(Dune::is_same<GridType, Dune::YaspGrid<dim>>::value), "GmshReader doesn't work with YaspGrid");
    dune_static_assert(!(Dune::is_same<GridType, Dune::SGrid<2, 2>>::value), "GmshReader doesn't work with SGrid");
    paramTree.assert("mshfile");
    const std::string filename = paramTree.get("mshfile", "sample.msh");
    // read gmshfile
    grid_ = Dune::shared_ptr<GridType>(GmshReader<GridType>::read(filename));
  }

  //! Unique identifier: \c stuff.grid.provider.gmsh
  static const std::string id()
  {
    return BaseType::id() + ".gmsh";
  }

  /**
    \brief  Provides access to the created grid.
    \return Reference to the grid.
    **/
  virtual GridType& grid()
  {
    return *grid_;
  }

  /**
   *  \brief  Provides const access to the created grid.
   *  \return Reference to the grid.
   **/
  virtual const GridType& grid() const
  {
    return *grid_;
  }

  //! access to shared ptr
  virtual Dune::shared_ptr<GridType> gridPtr()
  {
    return grid_;
  }

  //! const access to shared ptr
  virtual const Dune::shared_ptr<const GridType> gridPtr() const
  {
    return grid_;
  }

private:
  const std::string filename_;
  Dune::shared_ptr<GridType> grid_;
}; // class Gmsh


} // namespace Provider
} // namespace Grid
} // namespace Stuff
} // namespace Dune

#endif // DUNE_STUFF_GRID_PROVIDER_GMSH_HH
