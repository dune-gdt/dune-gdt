dune-gdt
========

dune-gdt is a [DUNE](http://www.dune-project.org/) module which provides a generic
discretization toolbox for grid-based numerical methods. It contains building blocks - like
local operators, local evaluations, local assemblers - for discretization methods and suitable
discrete function spaces.

[![pre-commit](https://img.shields.io/badge/pre--commit-enabled-brightgreen?logo=pre-commit&logoColor=white)](https://github.com/pre-commit/pre-commit)
[![pipeline status](https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt/badges/master/pipeline.svg)](https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt/-/commits/master)


License
-------

dune-gdt is dual licensed as [BSD 2](http://opensource.org/licenses/BSD-2-Clause) or ([GPL-2.0+](http://opensource.org/licenses/gpl-license) with [runtime exception](https://dune-project.org/about/license/)), see [LICENSE](LICENSE) for details.


Citing
------

dune-gdt also contains dune-xt, which is an eXTensions module for [DUNE](https://www.dune-project.org).
There is a paper describing some of the concepts behind dune-xt.
While already dated (in particular, the four modules dune-xt-common, dune-xt-grid, dune-xt-la and dune-xt-functions have been merged into the single dune-gdt module), most ideas still apply:

    T. Leibner and R. Milk and F. Schindler: "Extending DUNE: The dune-xt modules"
    Archive of Numerical Software, 5:193-216, 2017
    https://www.doi.org/10.11588/ans.2017.1.27720
