/**
  \file   integration.hh
  **/

#ifndef DUNE_FEM_FUNCTIONALS_DISCRETEFUNCTIONAL_LOCAL_INTEGRATION_HH
#define DUNE_FEM_FUNCTIONALS_DISCRETEFUNCTIONAL_LOCAL_INTEGRATION_HH

// dune fem includes
#include <dune/fem/quadrature/cachingquadrature.hh>

// dune-functionals includes
#include <dune/functionals/common/localvector.hh>

namespace Dune {

namespace Functionals {

namespace DiscreteFunctional {

namespace Local {

template <class LocalEvaluationImp>
class Codim0Integration
{
public:
  typedef LocalEvaluationImp LocalEvaluationType;

  typedef LocalEvaluationType FunctionSpaceType;

  typedef typename FunctionSpaceType::RangeFieldType RangeFieldType;

  typedef typename FunctionSpaceType::DomainType DomainType;

  typedef Dune::Functionals::Common::LocalVector<RangeFieldType> LocalVectorType;

  Codim0Integration(const LocalEvaluationType& localEvaluation)
    : localEvaluation_(localEvaluation)
  {
  }

  const LocalEvaluationType localEvaluation() const
  {
    return localEvaluation_;
  }

  template <class LocalTestBaseFunctionSetType>
  void applyLocal(const LocalTestBaseFunctionSetType& localTestBaseFunctionSet, LocalVectorType& localVector) const
  {
    // some types
    typedef typename LocalTestBaseFunctionSetType::DiscreteFunctionSpaceType DiscreteFunctionSpaceType;

    typedef typename DiscreteFunctionSpaceType::GridPartType GridPartType;

    typedef Dune::CachingQuadrature<GridPartType, 0> VolumeQuadratureType;

    typedef typename LocalTestBaseFunctionSetType::LocalBaseFunctionType LocalTestBaseFunctionType;

    // some stuff
    const unsigned numberOfLocalTestDoFs = localTestBaseFunctionSet.numBaseFunctions();
    const unsigned int quadratureOrder = 1 + localTestBaseFunctionSet.order();
    const VolumeQuadratureType volumeQuadrature(localTestBaseFunctionSet.entity(), quadratureOrder);
    const unsigned int numberOfQuadraturePoints = volumeQuadrature.nop();

    // do loop over all local test DoFs
    for (unsigned int j = 0; j < numberOfLocalTestDoFs; ++j) {
      const LocalTestBaseFunctionType localTestBaseFunction_j = localTestBaseFunctionSet.baseFunction(j);

      // do loop over all quadrature points
      RangeFieldType functional_j(0.0);
      for (unsigned int q = 0; q < numberOfQuadraturePoints; ++q) {
        // local coordinate
        const DomainType x = volumeQuadrature.point(q);

        // integration factors
        const double integrationFactor = localTestBaseFunctionSet.entity().geometry().integrationElement(x);
        const double quadratureWeight  = volumeQuadrature.weight(q);

        // evaluate the local operation
        const RangeFieldType localOperationEvalauted = localEvaluation_.evaluate(localTestBaseFunction_j, x);

        // compute integral
        functional_j += integrationFactor * quadratureWeight * localOperationEvalauted;
      } // done loop over all quadrature points

      // set local vector (the = is important, since we dont assume a clean vector)
      localVector[j] = functional_j;

    } // done loop over all local test DoFs

  } // end method applyLocal

private:
  const LocalEvaluationType& localEvaluation_;

}; // end class Codim0Integration

} // end namespace Local

} // end namespace DiscreteFunctional

} // end namespace Functionals

} // end namespace Dune

#endif // end DUNE_FEM_FUNCTIONALS_DISCRETEFUNCTIONAL_LOCAL_INTEGRATION_HH
