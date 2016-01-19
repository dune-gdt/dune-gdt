#!/usr/bin/env python
#
# This file is part of the dune-xt-common project:
#   https://github.com/dune-community/dune-xt-common
# The copyright lies with the authors of this file (see below).
# License: BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
# Authors:
#   Felix Schindler (2016)
#   Rene Milk       (2012, 2015)

import sys
import subprocess
import sourcetree
import pprint
import networkx as nx

fn = sys.argv[-1]
strip_base = sys.argv[-2]
compiler_cmd = sys.argv[1:-3]

with open(fn,'wb') as out:
    p = subprocess.Popen(compiler_cmd, shell=False, stdout=out, stderr=out)
    errcode = p.wait()
    if errcode != 0:
        sys.exit(errcode)

(root,graph) = sourcetree.parse_file(strip_base, fn, maxdepth=100)

cycle_nodes = nx.simple_cycles(graph)
if cycle_nodes:
    with open(fn+'.cycles','wb') as out:
       pprint.pprint(cycle_nodes, out)

sourcetree.render_graph(sourcetree.parse_file(strip_base, fn, maxdepth=3))
