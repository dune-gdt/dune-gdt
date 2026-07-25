// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2019)

/**
 * \file  grid-quality-estimates.hh
 * \brief Numerical estimates of inverse-inequality and element-to-intersection equivalence constants for a grid.
 **/
#ifndef DUNE_GDT_TOOLS_GRID_QUALITY_ESTIMATES_HH
#define DUNE_GDT_TOOLS_GRID_QUALITY_ESTIMATES_HH

#include <limits>
#include <vector>

#include <dune/grid/common/rangegenerators.hh>

#include <dune/xt/common/exceptions.hh>
#include <dune/xt/la/container/common.hh>
#include <dune/xt/la/container/conversion.hh>
#include <dune/xt/la/generalized-eigen-solver.hh>
#include <dune/xt/grid/entity.hh>
#include <dune/xt/grid/intersection.hh>
#include <dune/xt/grid/type_traits.hh>

#include <dune/gdt/exceptions.hh>
#include <dune/gdt/local/bilinear-forms/integrals.hh>
#include <dune/gdt/local/integrands/laplace.hh>
#include <dune/gdt/local/integrands/product.hh>
#include <dune/gdt/spaces/interface.hh>

namespace Dune {
namespace GDT {
namespace internal {


/**
 * \brief The smallest strictly positive eigenvalue of `lhs x = lambda rhs x`, ignoring the numerically zero ones.
 *
 * Shared by both estimators below: each of them is the maximum over all elements of the mesh width times (a power of)
 * this eigenvalue, they only differ in which pair of local product matrices they feed in.
 *
 * \note The lhs of both pairs is only positive *semi*-definite: the constants are in the kernel of the H1 product,
 *       and a cube's order 2 Lagrange basis has an interior node whose basis function vanishes on the whole
 *       element boundary. That is why the zero eigenvalues are filtered out here instead of simply taking the
 *       minimum. Only the rhs has to be positive definite, which is all dsygv requires.
 *
 * \note The cutoff separating "numerically zero" from "genuinely nonzero" is *relative* to the spectrum, and rejects
 *       negative eigenvalues outright. Both matter, because the spectrum scales with the element: the eigenvalues of
 *       the H1/L2 pair are O(h^-2), those of the face/element L2 pair O(h^-1).
 *       - An absolute cutoff is a mesh-size-dependent statement about which modes count. On a 10^5 x 10^5 x 10^5 box
 *         the whole H1/L2 spectrum sits below 1e-7, so *every* mode reads as zero and no constant can be computed at
 *         all -- even though the mesh is perfectly well behaved and the constant is the same 6 as on the unit cube.
 *       - The zero modes come out of lapack as +-eps * max|lambda|, so on a fine mesh (a large spectrum) the negative
 *         ones clear any fixed cutoff. Taking `abs(ev) > cutoff` then lets a negative mode through as the minimum,
 *         and `sqrt` of it is NaN -- which `std::max` silently discards, leaving the estimate at its `min()` seed.
 */
template <class E>
double smallest_nonzero_generalized_eigenvalue(const XT::LA::CommonDenseMatrix<double>& lhs,
                                               const XT::LA::CommonDenseMatrix<double>& rhs,
                                               const E& element,
                                               const std::string& matrix_pair)
{
  const auto evs = XT::LA::make_generalized_eigen_solver(lhs,
                                                         rhs,
                                                         {{"type", XT::LA::generalized_eigen_solver_types(lhs)[0]},
                                                          {"compute_eigenvectors", "false"},
                                                          {"assert_real_eigenvalues", "1e-15"}})
                       .real_eigenvalues();
  double largest_ev = 0.;
  for (auto&& ev : evs)
    largest_ev = std::max(largest_ev, std::abs(ev));
  // Comfortably above the O(eps) = O(1e-16) noise the zero modes carry, and comfortably below the smallest genuine
  // mode, which is the largest one divided by a mesh-independent constant of order 10 to 100.
  const double zero_cutoff = 1e-7 * largest_ev;
  double min_ev = std::numeric_limits<double>::max();
  for (auto&& ev : evs)
    if (ev > zero_cutoff)
      min_ev = std::min(min_ev, ev);
  // Without this the callers would silently hand out h * (max double) = inf for an element with no positive mode at
  // all (a degenerate element, or an lhs that is numerically the zero matrix).
  DUNE_THROW_IF(min_ev == std::numeric_limits<double>::max(),
                Exceptions::tools_error,
                "No eigenvalue of the " << matrix_pair << " matrix pair exceeds the cutoff " << zero_cutoff
                                        << " (largest eigenvalue " << largest_ev << ") on the element at "
                                        << element.geometry().center() << " (diameter " << XT::Grid::diameter(element)
                                        << "), so no constant can be estimated!");
  return min_ev;
} // ... smallest_nonzero_generalized_eigenvalue(...)


} // namespace internal


/**
 * \brief Numerically estimates the constant C_I of the inverse inequality over the given space's grid.
 */
template <class GV, size_t r>
double estimate_inverse_inequality_constant(const SpaceInterface<GV, r>& space)
{
  DUNE_THROW_IF(!XT::Common::Lapacke::available(), XT::Common::Exceptions::dependency_missing, "lapacke");
  using E = XT::Grid::extract_entity_t<GV>;
  double result = std::numeric_limits<double>::min();
  auto basis = space.basis().localize();
  for (auto&& element : elements(space.grid_view())) {
    basis->bind(element);
    const double h = XT::Grid::diameter(element);
    auto H1_product_matrix = XT::LA::convert_to<XT::LA::CommonDenseMatrix<double>>(
        LocalElementIntegralBilinearForm<E, r>(LocalLaplaceIntegrand<E, r>()).apply2(*basis, *basis));
    auto L2_product_matrix = XT::LA::convert_to<XT::LA::CommonDenseMatrix<double>>(
        LocalElementIntegralBilinearForm<E, r>(LocalProductIntegrand<E, r>()).apply2(*basis, *basis));
    // the smalles nonzero eigenvalue is (C_I / h)^2
    const double min_ev = internal::smallest_nonzero_generalized_eigenvalue(
        H1_product_matrix, L2_product_matrix, element, "H1/L2 element product");
    result = std::max(result, h * std::sqrt(min_ev));
  }
  return result;
} // ... estimate_inverse_inequality_constant(...)


/**
 * \brief Numerically estimates the combined inverse trace inequality constant C_M (1 + C_I) over the given space's
 * grid.
 */
template <class GV, size_t r>
double estimate_combined_inverse_trace_inequality_constant(const SpaceInterface<GV, r>& space)
{
  DUNE_THROW_IF(!XT::Common::Lapacke::available(), XT::Common::Exceptions::dependency_missing, "lapacke");
  using E = XT::Grid::extract_entity_t<GV>;
  using I = XT::Grid::extract_intersection_t<GV>;
  double result = std::numeric_limits<double>::min();
  auto basis = space.basis().localize();
  for (auto&& element : elements(space.grid_view())) {
    basis->bind(element);
    const double h = XT::Grid::diameter(element);
    XT::LA::CommonDenseMatrix<double> L2_face_product_matrix(basis->size(), basis->size(), 0.);
    DynamicMatrix<double> tmp_L2_face_product_matrix(basis->size(), basis->size(), 0.);
    for (auto&& intersection : intersections(space.grid_view(), element)) {
      LocalIntersectionIntegralBilinearForm<I, r>(LocalProductIntegrand<I, r>(1.))
          .apply2(intersection, *basis, *basis, tmp_L2_face_product_matrix);
      for (size_t ii = 0; ii < basis->size(); ++ii)
        for (size_t jj = 0; jj < basis->size(); ++jj)
          L2_face_product_matrix.add_to_entry(ii, jj, tmp_L2_face_product_matrix[ii][jj]);
    }
    auto L2_element_product_matrix = XT::LA::convert_to<XT::LA::CommonDenseMatrix<double>>(
        LocalElementIntegralBilinearForm<E, r>(LocalProductIntegrand<E, r>(1.)).apply2(*basis, *basis));
    // the smalles nonzero eigenvalue is (C_M (1 + C_I)) / h
    const double min_ev = internal::smallest_nonzero_generalized_eigenvalue(
        L2_face_product_matrix, L2_element_product_matrix, element, "L2 face/element product");
    result = std::max(result, h * min_ev);
  }
  return result;
} // ... estimate_combined_inverse_trace_inequality_constant(...)


/**
 * \brief Numerically estimates the element-to-intersection equivalence constant (maximal ratio of intersection diameter
 *        to element diameter) over the given grid view.
 */
template <class GV>
double estimate_element_to_intersection_equivalence_constant(
    const GridView<GV>& grid_view,
    const std::function<double(const XT::Grid::extract_intersection_t<GridView<GV>>&)>& intersection_diameter =
        [](const auto& intersection) {
          if (GridView<GV>::dimension == 1) {
            if (intersection.neighbor())
              return 0.5 * (XT::Grid::diameter(intersection.inside()) + XT::Grid::diameter(intersection.outside()));
            else
              return XT::Grid::diameter(intersection.inside());
          } else
            return XT::Grid::diameter(intersection);
        })
{
  auto result = std::numeric_limits<double>::min();
  for (auto&& element : elements(grid_view)) {
    const double h = XT::Grid::diameter(element);
    for (auto&& intersection : intersections(grid_view, element))
      result = std::max(result, intersection_diameter(intersection) / h);
  }
  return result;
} // ... estimate_element_to_intersection_equivalence_constant(...)


} // namespace GDT
} // namespace Dune

#endif // DUNE_GDT_TOOLS_GRID_QUALITY_ESTIMATES_HH
