#!/usr/bin/env python
# -*- coding: utf-8 -*-

from setuptools import setup, Extension
import commands

chash_ext = Extension(
    name = 'chash',
    sources = ['chash.c'],
    libraries = ['chash']
)

setup (
    name = 'CHash',
    version = '1.0.1',
    description = 'Consistent hashing library Python extension',
    author = 'Sebastien Estienne',
    author_email = 'sebastien.estienne@dailymotion.com',
    ext_modules = [chash_ext],
    zip_safe=False
)
