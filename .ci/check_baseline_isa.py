#!/usr/bin/env python3
"""Exercise the wheel's vendored BLAS/LAPACK under a restricted CPU model.

Run from .ci/make_docker_wheels.bash under `qemu-x86_64-static -cpu <model>`,
which fixes what CPUID reports for this process. A dependency whose kernels
were pinned to the *build* host's ISA -- openblas without the "dynamic-arch"
feature was exactly that -- then executes an instruction the emulated CPU does
not have and dies here with SIGILL, rather than a few weeks later in an
unrelated job on whichever hosted runner happened to be older. See #456/#467.

The eigensolver is pinned to the "lapack" type on purpose: "shifted_qr", the
other type dune.xt.la offers, is a pure-C++ fallback compiled from our own
sources with no -march flags, so it would never reach the vendored libraries
this check exists to test.
"""

import numpy as np

import dune.gdt  # noqa: F401  -- loads its shared objects too, not just dune.xt's
import dune.xt.la as la

N = 64
h = 1.0 / (N + 1)

# The 1d Dirichlet-Laplace stiffness matrix on (0, 1): symmetric tridiagonal,
# eigenvalues (4/h^2) sin^2(k pi h / 2), the smallest approximating pi^2.
matrix = la.CommonDenseMatrix(N, N, 0.0)
for i in range(N):
    matrix.set_entry(i, i, 2.0 / h**2)
    if i:
        matrix.set_entry(i, i - 1, -1.0 / h**2)
        matrix.set_entry(i - 1, i, -1.0 / h**2)

options = dict(la.CommonDenseMatrixEigenSolver.options("lapack"))
# We only want eigenvalues. Asking for none also has to disable the
# "assert_eigendecomposition" post-check, which would otherwise dereference the
# eigenvectors we did not ask for (dune/xt/la/eigen-solver/internal/base.hh).
options["compute_eigenvectors"] = "false"
options["assert_eigendecomposition"] = "-1"
eigenvalues = np.sort(
    [ev.real for ev in la.CommonDenseMatrixEigenSolver(matrix, options).eigenvalues()]
)

# Not just "did not crash": a wheel that survives the restricted CPU but computes
# garbage fails here too.
smallest, expected = eigenvalues[0], np.pi**2
assert abs(smallest - expected) < 0.01 * expected, (
    f"smallest eigenvalue {smallest} != {expected}"
)
print(
    f"baseline-ISA check ok: smallest eigenvalue {smallest:.6f} (expected {expected:.6f})"
)
