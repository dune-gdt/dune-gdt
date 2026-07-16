# ~~~
# This file is part of the dune-gdt project:
#   https://github.com/dune-community/dune-gdt
# Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
# License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
#      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
#          with "runtime exception" (http://www.dune-project.org/license.html)
# Authors:
#   Felix Schindler (2017 - 2018)
#   René Fritze     (2016, 2018)
# ~~~

from importlib import import_module
from tempfile import NamedTemporaryFile

from dune.xt import guarded_import
from dune.xt.common.config import config

from . import _version

__version__ = _version.__version__

for mod_name in (  # order should not matter!
    "_discretefunction_bochner",
    "_discretefunction_discretefunction_1d",
    "_discretefunction_discretefunction_2d",
    "_discretefunction_discretefunction_3d",
    "_discretefunction_dof_vector",
    "_functionals_interfaces_common",
    "_functionals_interfaces_eigen",
    "_functionals_interfaces_istl_1d",
    "_functionals_interfaces_istl_2d",
    "_functionals_interfaces_istl_3d",
    "_functionals_vector_based_1d",
    "_functionals_vector_based_2d",
    "_functionals_vector_based_3d",
    "_interpolations_boundary",
    "_interpolations_default",
    "_interpolations_oswald",
    "_local_bilinear_forms_coupling_intersection_integrals",
    "_local_bilinear_forms_coupling_intersection_interface",
    "_local_bilinear_forms_element_integrals",
    "_local_bilinear_forms_element_interface",
    "_local_bilinear_forms_intersection_integrals",
    "_local_bilinear_forms_intersection_interface",
    "_local_bilinear_forms_restricted_coupling_intersection_integrals",
    "_local_bilinear_forms_restricted_intersection_integrals",
    "_local_bilinear_forms_vectorized_element_integrals",
    "_local_functionals_element_integrals",
    "_local_functionals_element_interface",
    "_local_functionals_intersection_integrals",
    "_local_functionals_intersection_interface",
    "_local_functionals_restricted_intersection_integrals",
    "_local_functionals_vectorized_element_integrals",
    "_local_integrands_binary_element_interface",
    "_local_integrands_binary_intersection_interface",
    "_local_integrands_element_div",
    "_local_integrands_element_product",
    "_local_integrands_intersection_product",
    "_local_integrands_ipdg_boundary_penalty",
    "_local_integrands_ipdg_inner_penalty",
    "_local_integrands_jump_boundary",
    "_local_integrands_jump_inner",
    "_local_integrands_laplace",
    "_local_integrands_laplace_ipdg_dirichlet_coupling",
    "_local_integrands_laplace_ipdg_inner_coupling",
    "_local_integrands_linear_advection",
    "_local_integrands_linear_advection_upwind_dirichlet_coupling",
    "_local_integrands_linear_advection_upwind_inner_coupling",
    "_local_integrands_quaternary_intersection_interface",
    "_local_integrands_unary_element_interface",
    "_local_integrands_unary_intersection_interface",
    "_local_operators_coupling_intersection_indicator",
    "_local_operators_element_indicator",
    "_local_operators_element_interface",
    "_local_operators_intersection_indicator",
    "_local_operators_intersection_interface",
    "_operators_bilinear_form_1d",
    "_operators_bilinear_form_2d",
    "_operators_bilinear_form_3d",
    "_operators_interfaces_common",
    "_operators_interfaces_eigen",
    "_operators_interfaces_istl_1d",
    "_operators_interfaces_istl_2d",
    "_operators_interfaces_istl_3d",
    "_operators_laplace_ipdg_flux_reconstruction",
    "_operators_matrix_based_factory_1d",
    "_operators_matrix_based_factory_2d",
    "_operators_matrix_based_factory_3d",
    "_operators_operator_1d",
    "_operators_operator_2d",
    "_operators_operator_3d",
    "_prolongations",
    "_spaces_bochner",
    "_spaces_h1_continuous_lagrange_1d",
    "_spaces_h1_continuous_lagrange_2d",
    "_spaces_h1_continuous_lagrange_3d",
    "_spaces_hdiv_raviart_thomas",
    "_spaces_interface",
    "_spaces_l2_discontinuous_lagrange_1d",
    "_spaces_l2_discontinuous_lagrange_2d",
    "_spaces_l2_discontinuous_lagrange_3d",
    "_spaces_l2_finite_volume",
    "_spaces_skeleton_finite_volume",
    "_tools_adaptation_helper",
    "_tools_dirichlet_constraints",
    "_tools_grid_quality_estimates",
    "_tools_sparsity_pattern",
):
    guarded_import(globals(), "dune.gdt", mod_name)


# --- dimension-split binding trampolines --------------------------------------------------
#
# Several heavy binding TUs are split into per-dimension submodules (_foo_1d/_2d/_3d) to keep each
# translation unit's compile-time memory bounded (adding UGGrid pushed the monolithic TUs past the
# 16 GB CI runner). Each split submodule sets `__all__ = ()`, so guarded_import injects nothing into
# this namespace -- in particular the shared, grid-independent factory function it registers would
# otherwise collide across the three modules in guarded_import's duplicate-name check. The canonical
# factory names are restored here by thin Python trampolines that dispatch on the grid dimension of
# one argument (grid providers expose `.dimension`, spaces `.dimDomain`, (grid) functions
# `.dim_domain`). This keeps the public API byte-for-byte identical.


def _dim_of(obj):
    for attr in ("dimension", "dimDomain", "dim_domain"):
        value = getattr(obj, attr, None)
        if value is not None:
            return int(value)
    raise TypeError(
        f"cannot determine the grid dimension from an argument of type "
        f"'{type(obj).__name__}'"
    )


_split_submodule_cache = {}


def _split_submodule(module_base, dim):
    key = (module_base, dim)
    mod = _split_submodule_cache.get(key)
    if mod is None:
        try:
            mod = import_module(f"dune.gdt.{module_base}_{dim}d")
        except ImportError as e:
            raise ImportError(
                f"cannot dispatch to dune.gdt.{module_base}_{dim}d "
                f"(is the {dim}d grid enabled in this build?)"
            ) from e
        _split_submodule_cache[key] = mod
    return mod


def _make_dispatch(module_base, factory_name, dim_arg=0, dim_kwarg=None):
    def _factory(*args, **kwargs):
        if dim_arg < len(args):
            probe = args[dim_arg]
        elif dim_kwarg is not None and dim_kwarg in kwargs:
            probe = kwargs[dim_kwarg]
        else:
            raise TypeError(
                f"{factory_name}(): missing the dimension-carrying argument "
                f"(expected at position {dim_arg})"
            )
        submodule = _split_submodule(module_base, _dim_of(probe))
        return getattr(submodule, factory_name)(*args, **kwargs)

    _factory.__name__ = _factory.__qualname__ = factory_name
    return _factory


# Canonical factory names restored from their per-dimension submodules. All dispatch on the first
# (positional or keyword) argument, which is a grid provider (.dimension) for every factory below
# except DiscreteFunction, whose first argument is a space (.dimDomain).
Operator = _make_dispatch("_operators_operator", "Operator", dim_kwarg="grid")
MatrixOperator = _make_dispatch(
    "_operators_matrix_based_factory", "MatrixOperator", dim_kwarg="grid"
)
BilinearForm = _make_dispatch(
    "_operators_bilinear_form", "BilinearForm", dim_kwarg="grid"
)
ContinuousLagrangeSpace = _make_dispatch(
    "_spaces_h1_continuous_lagrange", "ContinuousLagrangeSpace", dim_kwarg="grid"
)
DiscontinuousLagrangeSpace = _make_dispatch(
    "_spaces_l2_discontinuous_lagrange", "DiscontinuousLagrangeSpace", dim_kwarg="grid"
)
DiscreteFunction = _make_dispatch(
    "_discretefunction_discretefunction", "DiscreteFunction", dim_kwarg="space"
)
VectorFunctional = _make_dispatch(
    "_functionals_vector_based", "VectorFunctional", dim_kwarg="grid"
)
IstlVectorFunctional = _make_dispatch(
    "_functionals_vector_based", "IstlVectorFunctional", dim_kwarg="grid"
)


if config.HAVE_K3D:
    from dune.xt.common.vtk.plot import plot

    def visualize_function(function, grid=None, subsampling=False):
        assert function.dim_domain <= 2, (
            f"Not implemented yet for {function.dim_domain}-dimensional grids!"
        )
        if function.dim_domain == 1:
            import numpy as np
            from matplotlib import pyplot as plt
            from dune.xt.functions import GridFunction
            from dune.gdt import ContinuousLagrangeSpace, default_interpolation

            assert grid  # not optimal
            P1_space = ContinuousLagrangeSpace(grid, order=1)
            interpolation_points = np.array(
                P1_space.interpolation_points(), copy=False
            )[:, 0]
            piecewise_linear_interpolant = default_interpolation(
                GridFunction(grid, function), P1_space
            )
            values = np.array(piecewise_linear_interpolant.dofs.vector, copy=False)

            plt.figure()
            plt.title(f"{function.name}")
            plt.plot(interpolation_points, values)

            return plt.gca()
        elif function.dim_domain == 2:
            assert function.dim_range == 1, (
                f"Not implemented yet for {function.dim_domain}-dimensional functions!"
            )
            tmpfile = NamedTemporaryFile(mode="wb", delete=False, suffix=".vtu").name
            failed = False
            try:  # discrete function
                function.visualize(filename=tmpfile[:-4])
                return plot(tmpfile, color_attribute_name=function.name)
            except TypeError:
                failed = True
            except AttributeError:
                failed = True

            if failed:
                from dune.xt.functions import (
                    visualize_function as visualize_xt_function,
                )

                assert grid
                return visualize_xt_function(function, grid, subsampling=subsampling)
