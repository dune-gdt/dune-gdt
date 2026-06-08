// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2015 - 2018)
//   René Fritze     (2014, 2016 - 2018)
//   Tobias Leibner  (2014, 2016)

/**
 * \file  communication.hh
 * \brief Selects the appropriate DoF communicator for a space depending on whether the grid is parallel.
 **/
#ifndef DUNE_GDT_SPACES_PARALLEL_COMMUNICATION_HH
#define DUNE_GDT_SPACES_PARALLEL_COMMUNICATION_HH

#include <dune/common/parallel/communicator.hh>

#include <dune/istl/owneroverlapcopy.hh>

#include <dune/xt/common/parallel/helper.hh>

#include "helper.hh"

namespace Dune {
namespace GDT {


/**
 * \brief Chooses the DoF communicator type for a grid view and provides its creation/preparation; this sequential
 *        default uses a no-op communicator.
 */
template <class ViewImp,
          bool is_parallel =
              Dune::XT::UseParallelCommunication<typename XT::Grid::extract_grid<ViewImp>::type::Communication>::value>
struct DofCommunicationChooser
{
  using Type = Dune::XT::SequentialCommunication;

  static Type* create(const ViewImp& /*gridView*/)
  {
    return new Type;
  }

  template <class SpaceBackend>
  static bool prepare(const SpaceBackend& /*space_backend*/, Type& /*communicator*/)
  {
    return false;
  }
}; // struct DofCommunicationChooser


#if HAVE_MPI


/**
 * \brief Parallel specialization of DofCommunicationChooser, providing an OwnerOverlapCopyCommunication and setting up
 *        its parallel index set from a space.
 */
template <class ViewImp>
struct DofCommunicationChooser<ViewImp, true>
{
private:
  // this is necessary because alugrid's id is not integral
  using RealGlobalId = typename XT::Grid::extract_grid_t<ViewImp>::GlobalIdSet::IdType;
  using RealLocalId = typename XT::Grid::extract_grid_t<ViewImp>::LocalIdSet::IdType;

public:
  using GlobalId = std::conditional_t<std::is_arithmetic<RealGlobalId>::value, RealGlobalId, uint64_t>;
  using LocalId = std::conditional_t<std::is_arithmetic<RealLocalId>::value, RealLocalId, int>;
  using Type = OwnerOverlapCopyCommunication<GlobalId, LocalId>;
  using type = Type;

  static Type* create(const ViewImp& gridView)
  {
    return new Type(gridView.comm(), SolverCategory::overlapping);
  }

  template <class GV, size_t r, size_t rD, class R>
  static bool prepare(const SpaceInterface<GV, r, rD, R>& space, Type& communicator)
  {
    if (communicator.communicator().size() > 1)
      GDT::GenericParallelHelper<GV, r, rD, R>(space, 1).setup_parallel_indexset(communicator);
    return true;
  } // ... prepare(...)

}; // struct DofCommunicationChooser< ..., true >


#endif // HAVE_MPI

} // namespace GDT
} // namespace Dune

#endif // DUNE_GDT_SPACES_PARALLEL_COMMUNICATION_HH
