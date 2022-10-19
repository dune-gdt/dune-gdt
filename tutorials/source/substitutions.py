# ~~~
# This file is part of the dune-gdt project:
#   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-gdt
# Copyright 2010-2021 dune-gdt developers and contributors. All rights reserved.
# License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
#      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
#          with "runtime exception" (http://www.dune-project.org/license.html)
# Authors:
#
# flake8: noqa
# ~~~

# substitutions for the most important classes and methods
common = """
.. |NumPy| replace:: :mod:`NumPy <numpy>`
.. |NumPy array| replace:: :class:`NumPy array <numpy.ndarray>`
.. |NumPy arrays| replace:: :class:`NumPy arrays <numpy.ndarray>`
.. |Numpy array| replace:: :class:`NumPy array <numpy.ndarray>`
.. |Numpy arrays| replace:: :class:`NumPy arrays <numpy.ndarray>`
.. |array| replace:: :class:`NumPy array <numpy.ndarray>`
.. |Array| replace:: :class:`NumPy array <numpy.ndarray>`

.. |SciPy| replace:: :mod:`SciPy <scipy>`
.. |SciPy spmatrix| replace:: :class:`SciPy spmatrix <scipy.sparse.spmatrix>`
.. |SciPy spmatrices| replace:: :class:`SciPy spmatrices <scipy.sparse.spmatrix>`
.. |Scipy spmatrix| replace:: :class:`SciPy spmatrix <scipy.sparse.spmatrix>`
.. |Scipy spmatrices| replace:: :class:`SciPy spmatrices <scipy.sparse.spmatrix>`

.. |OrderedDict| replace:: :class:`~collections.OrderedDict`
"""

substitutions = common

# list of (key, jinja_safe_key, substitution_value)
jinja_subst = []
for line in substitutions.split("\n"):
    if line == "":
        continue
    key, subst = line.split(" replace:: ")
    key = key.strip()
    key = key.replace(".. |", "").replace("|", "")
    subst = subst.replace(":", "{", 1).replace(":", "}", 2)
    jinja_subst.append((key, key.replace(" ", "_"), subst))

inline_directives = [
    "math",
    "meth",
    "class",
    "ref",
    "mod",
    "attr",
    "doc",
]

if __name__ == "__main__":
    import pprint as pp

    # pp.pprint(jinja_subst)
    with open("rst_to_myst.sed", "wt") as out:
        for dr in inline_directives:
            out.write(f"s;:{dr}:;{{{dr}}};g\n")
        l = "{{\\ "
        r = "\\ }}"
        for key, safe_key, _ in jinja_subst:
            out.write(f"s;|{key}|;{l}{safe_key}{r};g\n")

myst_substitutions = {safe_key: subst for _, safe_key, subst in jinja_subst}
