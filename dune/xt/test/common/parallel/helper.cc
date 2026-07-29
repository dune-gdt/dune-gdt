// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   René Fritze    (2026)

// Covers dune/xt/common/parallel/helper.hh: SequentialCommunication, UseParallelCommunication and
// abort_all_mpi_processes().
//
// This test is deliberately not named "*mpi*", so cmake/modules/DuneXTTesting.cmake runs it on a single rank only.
// That matters for abort_all_mpi_processes(), whose whole point is to call MPI_Abort as soon as there is more than
// one rank -- with a single rank it is the documented no-op returning 1, which is what is checked below.

#include <dune/xt/test/main.hxx> // <- This one has to come first, includes config.h!

#include <type_traits>

#include <dune/common/parallel/mpihelper.hh>

#include <dune/xt/common/parallel/helper.hh>

using namespace Dune::XT;


GTEST_TEST(parallel_helper, SequentialCommunication_is_a_dune_amg_SequentialInformation)
{
  static_assert(std::is_base_of_v<Dune::Amg::SequentialInformation, SequentialCommunication>);
  SequentialCommunication comm;
  EXPECT_EQ(1, comm.communicator().size());
  EXPECT_EQ(0, comm.communicator().rank());
}


GTEST_TEST(parallel_helper, UseParallelCommunication)
{
  // A sequential communication never counts as parallel ...
  EXPECT_FALSE(UseParallelCommunication<Dune::Communication<Dune::No_Comm>>::value);
  EXPECT_FALSE(UseParallelCommunication<int>::value);
#if HAVE_MPI
  // ... whereas an MPI one does, but only in a build which has MPI at all.
  EXPECT_TRUE(UseParallelCommunication<Dune::Communication<MPI_Comm>>::value);
#endif
}


GTEST_TEST(parallel_helper, abort_all_mpi_processes_is_a_noop_on_a_single_rank)
{
  ASSERT_EQ(1, Dune::MPIHelper::getCommunication().size())
      << "This test must be run on a single rank, see the comment at the top of this file!";
  EXPECT_EQ(1, abort_all_mpi_processes());
  // Calling it again is just as harmless.
  EXPECT_EQ(1, abort_all_mpi_processes());
}
