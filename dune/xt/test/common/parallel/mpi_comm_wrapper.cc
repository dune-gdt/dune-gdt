// This file is part of the dune-gdt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-gdt
// Copyright 2010-2021 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   René Fritze    (2018 - 2019)
//   Tobias Leibner (2020 - 2021)
//
// This file is part of the dune-xt project:

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
