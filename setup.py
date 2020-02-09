from distutils.core import setup, Extension

module1 = Extension('pycirphys',
                    sources = ['object.c'])

setup (name = 'pycirphys',
       version = '0',
       description = 'Circle physics engine for CPython.',
       ext_modules = [module1])
