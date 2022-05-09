#!/usr/bin/env python3
# kate: indent-width 2;

from pathlib import Path
import jinja2
from itertools import product

THIS_DIR = Path(__file__).resolve().absolute().parent
template_filename = THIS_DIR.parent / 'shared/xt_and_gdt_config_template.txt'
with open(template_filename, 'r') as f:
    tpl = f.read().replace('DUNE_XT_OR_DUNE_GDT', 'dune-xt')
tpl = jinja2.Template(tpl)
# images = ['debian-unstable_gcc_full', 'debian_gcc_full', 'debian_clang_full']
compilers = ('gcc', 'clang')
images = ('debian-full',)
compiler_images = product(compilers, images)
subdirs = ['xt/common', 'xt/grid', 'xt/functions', 'xt/functions1', 'xt/functions2', 'xt/la', 'gdt']
kinds = ['cpp', 'headercheck']
matrix = product(compilers, images, subdirs, kinds)
wheel_pythons = [f'3.{i}' for i in range(7, 10)]
config = THIS_DIR / 'config.yml'
with open(config, 'wt') as yml:
    yml.write(tpl.render(**locals()))
