#!/usr/bin/env python3
# kate: indent-width 2;

from pathlib import Path
import jinja2
from itertools import product

from dotenv import dotenv_values

THIS_DIR = Path(__file__).resolve().absolute().parent
env_path = THIS_DIR / '..' / '..' / '.env'
env = dotenv_values(env_path)
template_filename = THIS_DIR / 'xt_and_gdt_config_template.jinja'
with open(template_filename, 'r') as f:
    tpl = f.read().replace('DUNE_XT_OR_DUNE_GDT', 'dune-xt')
tpl = jinja2.Template(tpl)
# images = ['debian-unstable_gcc_full', 'debian_gcc_full', 'debian_clang_full']
compilers = (('gcc', 'g++'), ('clang', 'clang++'))
images = ('debian',)
compiler_images = product(compilers, images)
subdirs = ['common', 'grid', 'functions', 'functions1', 'functions2', 'la', 'gdt']
kinds = ['cpp', 'headercheck']
matrix = product(compilers, images, subdirs, kinds)
pythons = [f'3.{i}' for i in range(8, 11)]
config = THIS_DIR / 'config.yml'
ml_tag = env['ML_TAG']
with open(config, 'wt') as yml:
    yml.write(tpl.render(**locals()))
