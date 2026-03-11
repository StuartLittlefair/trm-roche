from __future__ import print_function
from distutils.core import setup, Extension
import os, numpy, sys
from Cython.Build import cythonize

""" Setup script for the roche python extension"""

library_dirs = []
include_dirs = []

# need to direct to where includes and  libraries are
if 'TRM_SOFTWARE' in os.environ:
    library_dirs.append(os.path.join(os.environ['TRM_SOFTWARE'], 'lib64'))
    library_dirs.append(os.path.join(os.environ['TRM_SOFTWARE'], 'lib'))
    include_dirs.append(os.path.join(os.environ['TRM_SOFTWARE'], 'include'))
else:
    print ("Environment variable TRM_SOFTWARE pointing to location of shareable libraries and includes not defined!",file=sys.stderr)

include_dirs.append(numpy.get_include())

"""
roche = Extension('trm.roche._roche',
                  define_macros   = [('MAJOR_VERSION', '0'),
                                     ('MINOR_VERSION', '1')],
                  undef_macros    = ['USE_NUMARRAY'],
                  include_dirs    = include_dirs,
                  library_dirs    = library_dirs,
                  runtime_library_dirs = library_dirs,
                  libraries       = ['roche', 'subs'],
                  sources         = [os.path.join('trm', 'roche', 'roche.cc')])

"""
roche = [Extension("trm.roche._roche",
                 [os.path.join('trm','roche','_roche.pyx')],
                 define_macros   = [('MAJOR_VERSION', '0'),
                                    ('MINOR_VERSION', '1')],
                 include_dirs    = include_dirs,
                 library_dirs    = library_dirs,
                 libraries       = ['roche'])]
                 
setup(name='trm.roche',
      version='0.1',
      packages=['trm', 'trm.roche'],
      ext_modules=cythonize(roche),

      author='Tom Marsh',
      description='Python interface to roche geometry routines.',
      author_email='t.r.marsh@warwick.ac.uk',
      url='http://www.astro.warwick.ac.uk/',
      long_description="""
trm.roche provides an interface to a set of C++ routines for computing Roche lobe
shapes, gas streams and the like.
""",

      )

