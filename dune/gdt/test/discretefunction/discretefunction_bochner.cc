// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   dune-gdt developers

#ifndef DUNE_XT_COMMON_TEST_MAIN_CATCH_EXCEPTIONS
#  define DUNE_XT_COMMON_TEST_MAIN_CATCH_EXCEPTIONS 1
#endif

#include <dune/xt/test/main.hxx> // <- this one has to come first (includes the config.h)!

#include <array>
#include <string>
#include <vector>

#include <boost/filesystem.hpp>

#include <dune/xt/common/fvector.hh>
#include <dune/xt/common/parameter.hh>
#include <dune/xt/common/string.hh>
#include <dune/xt/grid/grids.hh>
#include <dune/xt/grid/gridprovider/cube.hh>
#include <dune/xt/la/container/istl.hh>
#include <dune/xt/la/container/vector-array/list.hh>

#include <dune/gdt/discretefunction/bochner.hh>
#include <dune/gdt/exceptions.hh>
#include <dune/gdt/spaces/bochner.hh>
#include <dune/gdt/spaces/l2/finite-volume.hh>

using namespace Dune;
using namespace Dune::GDT;

using G = YASP_2D_EQUIDISTANT_OFFSET;
static constexpr size_t d = G::dimension;
using GV = typename G::LeafGridView;
using V = XT::LA::IstlDenseVector<double>;
using BS = BochnerSpace<GV>;


namespace {


XT::Grid::GridProvider<G> make_grid()
{
  return XT::Grid::make_cube_grid<G>(XT::Common::from_string<FieldVector<double, d>>("[0 0]"),
                                     XT::Common::from_string<FieldVector<double, d>>("[1 1]"),
                                     XT::Common::from_string<std::array<unsigned int, d>>("[2 2]"));
}


// The temporal grid of the Bochner space below, two intervals of a P1 Lagrange discretization of [0, 1].
std::vector<double> time_grid()
{
  return {0., 0.5, 1.};
}


/**
 * \brief Fills the DoF vectors such that the function is the constant time_point + 1 at each temporal node.
 *
 * Since the temporal space is P1 and these values depend linearly on time, the resulting Bochner function equals
 * t \mapsto t + 1 exactly, for every t in [0, 1] - which makes evaluate() checkable by hand.
 */
template <class DiscreteBochnerFunctionType>
void fill_dofs(const BS& bochner_space, DiscreteBochnerFunctionType& u)
{
  const auto time_points = bochner_space.time_points();
  for (size_t kk = 0; kk < time_points.size(); ++kk) {
    auto& vec = u.dof_vectors()[kk].vector();
    for (size_t ii = 0; ii < vec.size(); ++ii)
      vec.set_entry(ii, time_points[kk] + 1.);
  }
}


} // namespace


// The freshly allocating factory has to hand out one DoF vector per temporal DoF, each of spatial size.
GTEST_TEST(discretefunction_bochner, freshly_allocated_dof_vectors_match_the_space)
{
  auto grid = make_grid();
  auto grid_view = grid.leaf_view();
  const auto spatial_space = make_finite_volume_space(grid_view);
  const BS bochner_space(spatial_space, time_grid());

  const auto u = make_discrete_bochner_function<V>(bochner_space);

  EXPECT_EQ(&bochner_space, &u.space());
  EXPECT_EQ("DiscreteBochnerFunction", u.name());
  ASSERT_EQ(bochner_space.temporal_space().mapper().size(), u.dof_vectors().length());
  ASSERT_EQ(3u, u.dof_vectors().length());
  for (const auto& annotated_vector : u.dof_vectors()) {
    EXPECT_EQ(spatial_space.mapper().size(), annotated_vector.vector().size());
    EXPECT_DOUBLE_EQ(0., annotated_vector.vector().sup_norm());
  }
}


// A given name has to be kept, an empty one has to be replaced by the documented default.
GTEST_TEST(discretefunction_bochner, keeps_a_given_name)
{
  auto grid = make_grid();
  auto grid_view = grid.leaf_view();
  const auto spatial_space = make_finite_volume_space(grid_view);
  const BS bochner_space(spatial_space, time_grid());

  EXPECT_EQ("u", make_discrete_bochner_function<V>(bochner_space, "u").name());
  EXPECT_EQ("DiscreteBochnerFunction", make_discrete_bochner_function<V>(bochner_space, "").name());
}


// Wrapping existing DoF vectors must not copy them: writing through the function has to be visible in the original.
GTEST_TEST(discretefunction_bochner, wraps_existing_dof_vectors)
{
  auto grid = make_grid();
  auto grid_view = grid.leaf_view();
  const auto spatial_space = make_finite_volume_space(grid_view);
  const BS bochner_space(spatial_space, time_grid());

  XT::LA::ListVectorArray<V> dof_vectors(spatial_space.mapper().size(), bochner_space.temporal_space().mapper().size());
  auto u = make_discrete_bochner_function(bochner_space, dof_vectors, "u");
  EXPECT_EQ("u", u.name());
  ASSERT_EQ(dof_vectors.length(), u.dof_vectors().length());

  u.dof_vectors()[0].vector().set_entry(0, 42.);
  EXPECT_DOUBLE_EQ(42., dof_vectors[0].vector().get_entry(0));
}


// DoF vectors which do not match the Bochner space have to be rejected on construction.
GTEST_TEST(discretefunction_bochner, rejects_mismatching_dof_vectors)
{
  auto grid = make_grid();
  auto grid_view = grid.leaf_view();
  const auto spatial_space = make_finite_volume_space(grid_view);
  const BS bochner_space(spatial_space, time_grid());
  const size_t spatial_size = spatial_space.mapper().size();
  const size_t temporal_size = bochner_space.temporal_space().mapper().size();

  XT::LA::ListVectorArray<V> too_few_vectors(spatial_size, temporal_size - 1);
  EXPECT_THROW(make_discrete_bochner_function(bochner_space, too_few_vectors), Exceptions::space_error);

  XT::LA::ListVectorArray<V> too_large_vectors(spatial_size + 1, temporal_size);
  EXPECT_THROW(make_discrete_bochner_function(bochner_space, too_large_vectors), Exceptions::space_error);
}


// Evaluating in time has to reproduce the DoF vectors at the temporal nodes ...
GTEST_TEST(discretefunction_bochner, evaluate_reproduces_the_temporal_nodes)
{
  auto grid = make_grid();
  auto grid_view = grid.leaf_view();
  const auto spatial_space = make_finite_volume_space(grid_view);
  const BS bochner_space(spatial_space, time_grid());

  auto u = make_discrete_bochner_function<V>(bochner_space);
  fill_dofs(bochner_space, u);

  const auto time_points = bochner_space.time_points();
  ASSERT_EQ(3u, time_points.size());
  for (size_t kk = 0; kk < time_points.size(); ++kk) {
    const auto u_t = u.evaluate(time_points[kk]);
    ASSERT_EQ(spatial_space.mapper().size(), u_t.dofs().vector().size());
    for (size_t ii = 0; ii < u_t.dofs().vector().size(); ++ii)
      EXPECT_NEAR(time_points[kk] + 1., u_t.dofs().vector().get_entry(ii), 1e-12);
  }
}


// ... and, since the data is linear in time and the temporal space is P1, in between them as well.
GTEST_TEST(discretefunction_bochner, evaluate_interpolates_between_the_temporal_nodes)
{
  auto grid = make_grid();
  auto grid_view = grid.leaf_view();
  const auto spatial_space = make_finite_volume_space(grid_view);
  const BS bochner_space(spatial_space, time_grid());

  auto u = make_discrete_bochner_function<V>(bochner_space);
  fill_dofs(bochner_space, u);

  for (const double t : {0.125, 0.25, 0.5, 0.75, 0.9}) {
    const auto u_t = u.evaluate(t);
    for (size_t ii = 0; ii < u_t.dofs().vector().size(); ++ii)
      EXPECT_NEAR(t + 1., u_t.dofs().vector().get_entry(ii), 1e-12);
  }
}


// Times outside of the temporal interval are documented to be clamped to it.
GTEST_TEST(discretefunction_bochner, evaluate_clamps_times_outside_of_the_time_interval)
{
  auto grid = make_grid();
  auto grid_view = grid.leaf_view();
  const auto spatial_space = make_finite_volume_space(grid_view);
  const BS bochner_space(spatial_space, time_grid());
  EXPECT_DOUBLE_EQ(0., bochner_space.time_interval().first);
  EXPECT_DOUBLE_EQ(1., bochner_space.time_interval().second);

  auto u = make_discrete_bochner_function<V>(bochner_space);
  fill_dofs(bochner_space, u);

  const auto before = u.evaluate(-1.);
  const auto after = u.evaluate(2.);
  for (size_t ii = 0; ii < before.dofs().vector().size(); ++ii) {
    EXPECT_NEAR(1., before.dofs().vector().get_entry(ii), 1e-12);
    EXPECT_NEAR(2., after.dofs().vector().get_entry(ii), 1e-12);
  }
}


// visualize() writes one file per temporal snapshot, numbered consecutively if the DoF vectors carry no time note.
GTEST_TEST(discretefunction_bochner, visualize_uses_a_counter_without_time_notes)
{
  auto grid = make_grid();
  auto grid_view = grid.leaf_view();
  const auto spatial_space = make_finite_volume_space(grid_view);
  const BS bochner_space(spatial_space, time_grid());

  auto u = make_discrete_bochner_function<V>(bochner_space);
  fill_dofs(bochner_space, u);

  const std::string prefix = "discretefunction_bochner__visualize_counter";
  // only in a serial run do we know the file names the VTKWriter produces
  std::vector<std::string> expected_files;
  if (grid_view.comm().size() == 1)
    for (const std::string& suffix : {"0", "1", "2"})
      expected_files.push_back(prefix + "_" + suffix + ".vtu");
  // a leftover file from an earlier run would let the assertions below pass without visualize() writing anything
  for (const auto& expected_file : expected_files)
    boost::filesystem::remove(expected_file);

  EXPECT_NO_THROW(visualize(u, prefix, /*subsampling=*/false));
  for (const auto& expected_file : expected_files)
    EXPECT_TRUE(boost::filesystem::exists(expected_file)) << "missing " << expected_file;

  EXPECT_THROW(visualize(u, ""), XT::Common::Exceptions::wrong_input_given);
}


// A time note which carries no value cannot name a file, so it has to fall back to the counter just like a missing
// one does (a single such vector switches the whole sequence over).
GTEST_TEST(discretefunction_bochner, visualize_uses_a_counter_for_an_empty_time_note)
{
  auto grid = make_grid();
  auto grid_view = grid.leaf_view();
  const auto spatial_space = make_finite_volume_space(grid_view);
  const BS bochner_space(spatial_space, time_grid());

  XT::LA::ListVectorArray<V> dof_vectors(spatial_space.mapper().size(), 0);
  const auto time_points = bochner_space.time_points();
  const XT::Common::Parameter valueless_note("_t", std::vector<double>{});
  ASSERT_TRUE(valueless_note.has_key("_t"));
  ASSERT_TRUE(valueless_note.get("_t").empty());
  for (size_t kk = 0; kk < time_points.size(); ++kk)
    dof_vectors.append(V(spatial_space.mapper().size(), time_points[kk] + 1.),
                       kk == 1 ? valueless_note : XT::Common::Parameter("_t", time_points[kk]));
  auto u = make_discrete_bochner_function(bochner_space, dof_vectors);

  const std::string prefix = "discretefunction_bochner__visualize_empty_note";
  // only in a serial run do we know the file names the VTKWriter produces
  std::vector<std::string> expected_files;
  if (grid_view.comm().size() == 1)
    for (const std::string& suffix : {"0", "1", "2"})
      expected_files.push_back(prefix + "_" + suffix + ".vtu");
  // a leftover file from an earlier run would let the assertions below pass without visualize() writing anything
  for (const auto& expected_file : expected_files)
    boost::filesystem::remove(expected_file);

  EXPECT_NO_THROW(visualize(u, prefix, /*subsampling=*/false));
  for (const auto& expected_file : expected_files)
    EXPECT_TRUE(boost::filesystem::exists(expected_file)) << "missing " << expected_file;
}


// If every DoF vector is annotated with its time, that time is used in the file name instead of the counter.
GTEST_TEST(discretefunction_bochner, visualize_uses_the_time_note_if_present)
{
  auto grid = make_grid();
  auto grid_view = grid.leaf_view();
  const auto spatial_space = make_finite_volume_space(grid_view);
  const BS bochner_space(spatial_space, time_grid());

  XT::LA::ListVectorArray<V> dof_vectors(spatial_space.mapper().size(), 0);
  const auto time_points = bochner_space.time_points();
  for (const auto& time_point : time_points)
    dof_vectors.append(V(spatial_space.mapper().size(), time_point + 1.), {"_t", time_point});
  auto u = make_discrete_bochner_function(bochner_space, dof_vectors);

  const std::string prefix = "discretefunction_bochner__visualize_time";
  // only in a serial run do we know the file names the VTKWriter produces
  std::vector<std::string> expected_files;
  if (grid_view.comm().size() == 1)
    for (const auto& time_point : time_points)
      expected_files.push_back(prefix + "_" + XT::Common::to_string(time_point) + ".vtu");
  // a leftover file from an earlier run would let the assertions below pass without visualize() writing anything
  for (const auto& expected_file : expected_files)
    boost::filesystem::remove(expected_file);

  EXPECT_NO_THROW(visualize(u, prefix, /*subsampling=*/false));
  for (const auto& expected_file : expected_files)
    EXPECT_TRUE(boost::filesystem::exists(expected_file)) << "missing " << expected_file;
}
