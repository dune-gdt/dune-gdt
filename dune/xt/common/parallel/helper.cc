// This file is part of the dune-gdt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-gdt
// Copyright 2010-2021 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2012 - 2014, 2016 - 2017)
//   René Fritze     (2011 - 2012, 2014 - 2016, 2018 - 2020)
//   Tobias Leibner  (2020 - 2021)
//
// This file is part of the dune-xt project:

#include <config.h>

#include "helper.hh"
#include <dune/common/parallel/mpihelper.hh>

int Dune::XT::abort_all_mpi_processes()
{
#if HAVE_MPI
  if (MPIHelper::getCollectiveCommunication().size() > 1)
    return MPI_Abort(MPIHelper::getCommunicator(), 1);
#endif
  return 1;
}
