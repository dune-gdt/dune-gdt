// This file is part of the dune-gdt project:
//   https://github.com/dune-community/dune-gdt
// Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Rene Milk      (2018)
//   Tobias Leibner (2017)

/**
 * \file  slopes.hh
 * \brief Slope limiters used by the linear reconstruction operators for finite volume schemes.
 **/
#ifndef DUNE_GDT_OPERATORS_FV_SLOPES_HH
#define DUNE_GDT_OPERATORS_FV_SLOPES_HH

#include <dune/geometry/quadraturerules.hh>

#include <dune/xt/common/float_cmp.hh>
#include <dune/xt/common/fvector.hh>
#include <dune/xt/common/math.hh>
#include <dune/xt/common/parallel/threadstorage.hh>

namespace Dune {
namespace GDT {


/**
 * \brief Interface for (limited) slopes used to reconstruct a piecewise linear function from cell averages.
 */
template <class E, class EigenVectorWrapperType, size_t stencil_size = 3>
class SlopeBase
{
public:
  virtual ~SlopeBase() = default;

  using VectorType = typename EigenVectorWrapperType::VectorType;
  using MatrixType = typename EigenVectorWrapperType::MatrixType;
  using StencilType = FieldVector<VectorType, stencil_size>;

  virtual SlopeBase* copy() const = 0;

  // at least one of the following two methods has to be implemented
  // returns (limited) slope in ordinary coordinates
  virtual VectorType
  get(const E& entity, const StencilType& stencil, const EigenVectorWrapperType& eigenvectors, const size_t dd) const
  {
    VectorType slope_char = get_char(entity, stencil, eigenvectors, dd);
    VectorType slope;
    eigenvectors.apply_eigenvectors(dd, slope_char, slope);
    return slope;
  }

  // returns (limited) slope in characteristic coordinates
  virtual VectorType get_char(const E& entity,
                              const StencilType& stencil,
                              const EigenVectorWrapperType& eigenvectors,
                              const size_t dd) const
  {
    VectorType slope = get(entity, stencil, eigenvectors, dd);
    // convert to characteristic coordinates
    VectorType slope_char;
    eigenvectors.apply_inverse_eigenvectors(dd, slope, slope_char);
    return slope_char;
  }
};


/**
 * \brief Central slope (centered difference of the neighbouring cell averages) without any limiting.
 */
// Central slope without any limiting
template <class E, class EigenVectorWrapperType, size_t stencil_size>
class CentralSlope : public SlopeBase<E, EigenVectorWrapperType, stencil_size>
{
  using BaseType = SlopeBase<E, EigenVectorWrapperType, stencil_size>;
  using ThisType = CentralSlope;

public:
  using typename BaseType::StencilType;
  using typename BaseType::VectorType;

  BaseType* copy() const override final
  {
    return new ThisType(*this);
  }

  VectorType get(const E& /*entity*/,
                 const StencilType& stencil,
                 const EigenVectorWrapperType& /*eigenvectors*/,
                 const size_t /*dd*/) const override final
  {
    const VectorType& u_left = stencil[0];
    const VectorType& u_right = stencil[2];
    return (u_right - u_left) / 2.;
  }
};


/**
 * \brief Left (backward difference) slope without any limiting.
 */
// Left slope without any limiting
template <class E, class EigenVectorWrapperType, size_t stencil_size>
class LeftSlope : public SlopeBase<E, EigenVectorWrapperType, stencil_size>
{
  using BaseType = SlopeBase<E, EigenVectorWrapperType, stencil_size>;
  using ThisType = LeftSlope;

public:
  using typename BaseType::StencilType;
  using typename BaseType::VectorType;

  BaseType* copy() const override final
  {
    return new ThisType();
  }

  VectorType get(const E& /*entity*/,
                 const StencilType& stencil,
                 const EigenVectorWrapperType& /*eigenvectors*/,
                 const size_t /*dd*/) const override final
  {
    const VectorType& u_left = stencil[0];
    const VectorType& u = stencil[1];
    return u - u_left;
  }
};


/**
 * \brief Right (forward difference) slope without any limiting.
 */
// Right slope without any limiting
template <class E, class EigenVectorWrapperType, size_t stencil_size>
class RightSlope : public SlopeBase<E, EigenVectorWrapperType, stencil_size>
{
  using BaseType = SlopeBase<E, EigenVectorWrapperType, stencil_size>;
  using ThisType = RightSlope;

public:
  using typename BaseType::StencilType;
  using typename BaseType::VectorType;

  BaseType* copy() const override final
  {
    return new ThisType(*this);
  }

  VectorType get(const E& /*entity*/,
                 const StencilType& stencil,
                 const EigenVectorWrapperType& /*eigenvectors*/,
                 const size_t /*dd*/) const override final
  {
    const VectorType& u = stencil[1];
    const VectorType& u_right = stencil[2];
    return u_right - u;
  }
};


/**
 * \brief Zero slope, yielding a piecewise constant (first-order) reconstruction.
 */
// Zero slope
template <class E, class EigenVectorWrapperType, size_t stencil_size>
class NoSlope : public SlopeBase<E, EigenVectorWrapperType, stencil_size>
{
  using BaseType = SlopeBase<E, EigenVectorWrapperType, stencil_size>;
  using ThisType = NoSlope;
  using typename BaseType::VectorType;

public:
  using typename BaseType::StencilType;

  BaseType* copy() const override final
  {
    return new ThisType(*this);
  }

  VectorType get(const E& /*entity*/,
                 const StencilType& /*stencil*/,
                 const EigenVectorWrapperType& /*eigenvectors*/,
                 const size_t /*dd*/) const override final
  {
    return VectorType(0.);
  }
};


/**
 * \brief Slope limited by the minmod limiter applied in characteristic coordinates.
 */
template <class E, class EigenVectorWrapperType>
class MinmodSlope : public SlopeBase<E, EigenVectorWrapperType, 3>
{
  using BaseType = SlopeBase<E, EigenVectorWrapperType, 3>;
  using ThisType = MinmodSlope;
  using typename BaseType::VectorType;

public:
  using typename BaseType::StencilType;

  BaseType* copy() const override final
  {
    return new ThisType(*this);
  }

  VectorType get_char(const E& /*entity*/,
                      const StencilType& stencil,
                      const EigenVectorWrapperType& eigenvectors,
                      const size_t dd) const override final
  {
    const VectorType slope_left = stencil[1] - stencil[0];
    const VectorType slope_right = stencil[2] - stencil[1];
    VectorType slope_left_char, slope_right_char;
    eigenvectors.apply_inverse_eigenvectors(dd, slope_left, slope_left_char);
    eigenvectors.apply_inverse_eigenvectors(dd, slope_right, slope_right_char);
    return minmod(slope_left_char, slope_right_char);
  }

  static VectorType minmod(const VectorType& first_slope, const VectorType& second_slope)
  {
    VectorType ret;
    for (size_t ii = 0; ii < first_slope.size(); ++ii)
      ret[ii] = XT::Common::minmod(first_slope[ii], second_slope[ii]);
    return ret;
  }
};


/**
 * \brief Slope limited by the monotonized central (MC) limiter applied in characteristic coordinates.
 */
template <class E, class EigenVectorWrapperType>
class McSlope : public SlopeBase<E, EigenVectorWrapperType, 3>
{
  using BaseType = SlopeBase<E, EigenVectorWrapperType, 3>;
  using ThisType = McSlope;
  using MinmodType = MinmodSlope<E, EigenVectorWrapperType>;

public:
  using typename BaseType::StencilType;
  using typename BaseType::VectorType;

  BaseType* copy() const override final
  {
    return new ThisType(*this);
  }

  VectorType get_char(const E& /*entity*/,
                      const StencilType& stencil,
                      const EigenVectorWrapperType& eigenvectors,
                      const size_t dd) const override final
  {
    const VectorType& u_left = stencil[0];
    const VectorType& u = stencil[1];
    const VectorType& u_right = stencil[2];
    const VectorType slope_left = (u - u_left) * 2.;
    const VectorType slope_right = (u_right - u) * 2.;
    const VectorType slope_center = (u_right - u_left) / 2.;
    VectorType slope_left_char, slope_right_char, slope_center_char;
    eigenvectors.apply_inverse_eigenvectors(dd, slope_left, slope_left_char);
    eigenvectors.apply_inverse_eigenvectors(dd, slope_right, slope_right_char);
    eigenvectors.apply_inverse_eigenvectors(dd, slope_center, slope_center_char);
    const VectorType first_slope = MinmodType::minmod(slope_left_char, slope_right_char);
    return MinmodType::minmod(first_slope, slope_center_char);
  }
};


/**
 * \brief Slope limited by the superbee limiter applied in characteristic coordinates.
 */
template <class E, class EigenVectorWrapperType>
class SuperbeeSlope : public SlopeBase<E, EigenVectorWrapperType, 3>
{
  using BaseType = SlopeBase<E, EigenVectorWrapperType, 3>;
  using ThisType = SuperbeeSlope;
  using MinmodType = MinmodSlope<E, EigenVectorWrapperType>;

public:
  using typename BaseType::StencilType;
  using typename BaseType::VectorType;

  BaseType* copy() const override final
  {
    return new ThisType(*this);
  }

  VectorType get_char(const E& /*entity*/,
                      const StencilType& stencil,
                      const EigenVectorWrapperType& eigenvectors,
                      const size_t dd) const override final
  {
    const VectorType slope_left = stencil[1] - stencil[0];
    const VectorType slope_right = stencil[2] - stencil[1];
    VectorType slope_left_char, slope_right_char;
    eigenvectors.apply_inverse_eigenvectors(dd, slope_left, slope_left_char);
    eigenvectors.apply_inverse_eigenvectors(dd, slope_right, slope_right_char);
    return superbee(slope_left_char, slope_right_char);
  }

  static VectorType superbee(const VectorType& first_slope, const VectorType& second_slope)
  {
    VectorType ret(0.);
    for (size_t ii = 0; ii < first_slope.size(); ++ii)
      // superbee limiter: maxmod(minmod(a, 2b), minmod(2a, b)). The per-component call used to read
      // superbee(first_slope[ii], second_slope[ii]) -- with no scalar overload the arguments converted
      // back to VectorType and it recursed into itself until the stack overflowed (segfault).
      ret[ii] = XT::Common::maxmod(XT::Common::minmod(first_slope[ii], 2. * second_slope[ii]),
                                   XT::Common::minmod(2. * first_slope[ii], second_slope[ii]));
    return ret;
  }
}; // class SuperbeeSlope


} // namespace GDT
} // namespace Dune

#endif // DUNE_GDT_OPERATORS_FV_SLOPES_HH
