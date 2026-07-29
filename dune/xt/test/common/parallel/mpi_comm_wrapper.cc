// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   René Fritze    (2018 - 2019)
//   Tobias Leibner (2020)

#include <dune/xt/test/main.hxx>

#include <dune/xt/common/parallel/mpi_comm_wrapper.hh>

using namespace Dune;
using namespace Dune::XT::Common;

MPIHelper::MPICommunicator implicit_convert(MPI_Comm_Wrapper mpi_comm)
{
  return mpi_comm.get();
}

GTEST_TEST(MpiCommWrapper, Global)
{
  const auto orig = MPIHelper::getCommunicator();
  const auto convert = implicit_convert(orig);
  ASSERT_EQ(orig, convert);
}

GTEST_TEST(MpiCommWrapper, Local)
{
  const auto orig = MPIHelper::getLocalCommunicator();
  const auto convert = implicit_convert(orig);
  ASSERT_EQ(orig, convert);
}

GTEST_TEST(MpiCommWrapper, DefaultCtor)
{
  // The default is the global communicator.
  MPI_Comm_Wrapper wrapper;
  ASSERT_EQ(MPIHelper::getCommunicator(), wrapper.get());
}

GTEST_TEST(MpiCommWrapper, AssignmentFromWrappedComm)
{
  MPI_Comm_Wrapper wrapper(MPIHelper::getCommunicator());
  ASSERT_EQ(MPIHelper::getCommunicator(), wrapper.get());

  const auto local = MPIHelper::getLocalCommunicator();
  // operator= returns *this, so assignments can be chained.
  MPI_Comm_Wrapper other;
  other = wrapper = local;
  ASSERT_EQ(local, wrapper.get());
  ASSERT_EQ(local, other.get());

  // Assigning the same communicator again does not change anything.
  wrapper = local;
  ASSERT_EQ(local, wrapper.get());
}
