#  -*- coding: utf-8 -*-

#
# ~~~
# This file is part of the dune-gdt project:
#   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-gdt
# Copyright 2010-2021 dune-gdt developers and contributors. All rights reserved.
# License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
#      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
#          with "runtime exception" (http://www.dune-project.org/license.html)
# Authors:
#   Felix Schindler (2017)
#   René Fritze     (2020)
#   Tobias Leibner  (2021)
# ~~~

from widget_module import Widget


class DerivedWidget(Widget):

    def __init__(self, message):
        super(DerivedWidget, self).__init__(message)

    def the_answer(self):
        return 42
