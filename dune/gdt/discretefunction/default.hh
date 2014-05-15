// This file is part of the dune-gdt project:
//   http://users.dune-project.org/projects/dune-gdt
// Copyright holders: Felix Schindler
// License: BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//
// Contributors: Kirsten Weber

#ifndef DUNE_GDT_DISCRETEFUNCTION_DEFAULT_HH
#define DUNE_GDT_DISCRETEFUNCTION_DEFAULT_HH

#include <memory>
#include <type_traits>

#include <dune/common/exceptions.hh>
#include <dune/common/fvector.hh>

#include <dune/grid/io/file/vtk.hh>

#include <dune/stuff/la/container/interfaces.hh>
#include <dune/stuff/functions/interfaces.hh>

#include <dune/gdt/spaces/interface.hh>

#include "local.hh"

namespace Dune {
namespace GDT {


template <class SpaceImp, class VectorImp>
class ConstDiscreteFunction
    : public Stuff::LocalizableFunctionInterface<typename SpaceImp::EntityType, typename SpaceImp::DomainFieldType,
                                                 SpaceImp::dimDomain, typename SpaceImp::RangeFieldType,
                                                 SpaceImp::dimRange, SpaceImp::dimRangeCols>
{
  static_assert(std::is_base_of<SpaceInterface<typename SpaceImp::Traits>, SpaceImp>::value,
                "SpaceImp has to be derived from SpaceInterface!");
  static_assert(std::is_base_of<Dune::Stuff::LA::VectorInterface<typename VectorImp::Traits>, VectorImp>::value,
                "VectorImp has to be derived from Stuff::LA::VectorInterface!");
  static_assert(std::is_same<typename SpaceImp::RangeFieldType, typename VectorImp::ScalarType>::value,
                "Types do not match!");
  typedef Stuff::LocalizableFunctionInterface<typename SpaceImp::EntityType, typename SpaceImp::DomainFieldType,
                                              SpaceImp::dimDomain, typename SpaceImp::RangeFieldType,
                                              SpaceImp::dimRange, SpaceImp::dimRangeCols> BaseType;
  typedef ConstDiscreteFunction<SpaceImp, VectorImp> ThisType;

public:
  typedef SpaceImp SpaceType;
  typedef VectorImp VectorType;
  typedef typename BaseType::EntityType EntityType;
  typedef typename BaseType::LocalfunctionType LocalfunctionType;

  static const unsigned int dimDomain = BaseType::dimDomain;
  typedef typename BaseType::DomainFieldType DomainFieldType;
  typedef typename BaseType::DomainType DomainType;

  static const unsigned int dimRange     = BaseType::dimRange;
  static const unsigned int dimRangeCols = BaseType::dimRangeCols;
  typedef typename BaseType::RangeFieldType RangeFieldType;
  typedef typename BaseType::RangeType RangeType;
  typedef typename BaseType::JacobianRangeType JacobianRangeType;

  typedef ConstLocalDiscreteFunction<SpaceType, VectorType> ConstLocalDiscreteFunctionType;

  ConstDiscreteFunction(const SpaceType& sp, const VectorType& vec,
                        const std::string nm = "dune.gdt.constdiscretefunction")
    : space_(sp)
    , vector_(vec)
    , name_(nm)
  {
    assert(vector_.size() == space_.mapper().size() && "Given vector has wrong size!");
  }

  ConstDiscreteFunction(const ThisType& other)
    : space_(other.space_)
    , vector_(other.vector_)
    , name_(other.name_)
  {
  }

  ConstDiscreteFunction(ThisType&& source)
    : space_(source.space_)
    , vector_(source.vector_)
    , name_(std::move(source.name_))
  {
  }

  virtual ~ConstDiscreteFunction()
  {
  }

  ThisType& operator=(const ThisType& other) = delete;

  virtual ThisType* copy() const DS_OVERRIDE
  {
    return new ThisType(*this);
  }

  virtual std::string name() const DS_OVERRIDE
  {
    return name_;
  }

  const SpaceType& space() const
  {
    return space_;
  }

  const VectorType& vector() const
  {
    return vector_;
  }

  ConstLocalDiscreteFunctionType local_discrete_function(const EntityType& entity) const
  {
    assert(space_.grid_view()->indexSet().contains(entity));
    return ConstLocalDiscreteFunctionType(space_, vector_, entity);
  }

  virtual std::unique_ptr<LocalfunctionType> local_function(const EntityType& entity) const DS_OVERRIDE
  {
    return std::unique_ptr<ConstLocalDiscreteFunctionType>(
        new ConstLocalDiscreteFunctionType(local_discrete_function(entity)));
  }

  void visualize(const std::string filename, const bool subsampling = (SpaceType::polOrder > 1),
                 VTK::OutputType vtk_output_type = VTK::appendedraw) const
  {
    BaseType::template visualize<typename SpaceType::GridViewType>(
        *(space().grid_view()), filename, subsampling, vtk_output_type);
  }

protected:
  const SpaceType& space_;

private:
  const VectorType& vector_;
  const std::string name_;
}; // class ConstDiscreteFunction


template <class SpaceImp, class VectorImp>
class DiscreteFunction : public ConstDiscreteFunction<SpaceImp, VectorImp>
{
  typedef ConstDiscreteFunction<SpaceImp, VectorImp> BaseType;
  typedef DiscreteFunction<SpaceImp, VectorImp> ThisType;

public:
  typedef typename BaseType::SpaceType SpaceType;
  typedef typename BaseType::VectorType VectorType;
  typedef typename BaseType::EntityType EntityType;
  typedef typename BaseType::LocalfunctionType LocalfunctionType;

  typedef LocalDiscreteFunction<SpaceType, VectorType> LocalDiscreteFunctionType;

  DiscreteFunction(const SpaceType& sp, VectorType& vec, const std::string nm = "dune.gdt.discretefunction")
    : BaseType(sp, vec, nm)
    , vector_(vec)
  {
  }

  DiscreteFunction(const ThisType& other)
    : BaseType(other)
    , vector_(other.vector_)
  {
  }

  DiscreteFunction(ThisType&& source)
    : BaseType(std::move(source))
    , vector_(source.vector_)
  {
  }

  ~DiscreteFunction()
  {
  }

  ThisType& operator=(const ThisType& other) = delete;

  virtual ThisType* copy() const DS_OVERRIDE
  {
    return new ThisType(*this);
  }

  VectorType& vector()
  {
    return vector_;
  }

  using BaseType::local_discrete_function;

  LocalDiscreteFunctionType local_discrete_function(const EntityType& entity)
  {
    assert(space_.grid_view()->indexSet().contains(entity));
    return LocalDiscreteFunctionType(space_, vector_, entity);
  }

private:
  using BaseType::space_;
  VectorType& vector_;
}; // class DiscreteFunction

template <class SpaceImp, class VectorImp>
class StoredDiscreteFunction : public DiscreteFunction<SpaceImp, VectorImp>
{
  typedef DiscreteFunction<SpaceImp, VectorImp> BaseType;
  typedef StoredDiscreteFunction<SpaceImp, VectorImp> ThisType;

public:
  typedef typename BaseType::SpaceType SpaceType;
  typedef typename BaseType::VectorType VectorType;
  typedef typename BaseType::EntityType EntityType;
  typedef typename BaseType::LocalfunctionType LocalfunctionType;

  typedef LocalDiscreteFunction<SpaceType, VectorType> LocalDiscreteFunctionType;

  StoredDiscreteFunction(const SpaceType& sp, const std::string nm = "dune.gdt.discretefunction")
    : BaseType(sp, vector_, nm)
    , vector_(sp.mapper().size())
  {
  }


  ~StoredDiscreteFunction()
  {
  }

  ThisType& operator=(const ThisType& other) = delete;

  virtual ThisType* copy() const DS_OVERRIDE
  {
    return new ThisType(*this);
  }

  VectorType& vector()
  {
    return vector_;
  }

  LocalDiscreteFunctionType local_discrete_function(const EntityType& entity)
  {
    return LocalDiscreteFunctionType(BaseType::space(), vector_, entity);
  }

  bool dofsValid() const
  {
    for (const auto& val : vector_) {
      if (std::isnan(val) || std::isinf(val))
        return false;
    }
    return true;
  }

private:
  VectorType vector_;
}; // class DiscreteFunction

} // namespace GDT
} // namespace Dune

#endif // DUNE_GDT_DISCRETEFUNCTION_DEFAULT_HH
