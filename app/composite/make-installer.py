#!/usr/bin/env python
# -*- mode: python py-indent-offset: 2; -*-
#
# Gimp image compositing
# Copyright (C) 2003  Helvetix Victorinox, <helvetix@gimp.org>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

import sys
import string
import os
import ns
import pprint
import optparse
import copy

#
# This programme creates C code for gluing a collection of compositing
# functions into an array indexed by compositing function, and the
# pixel formats of its arguments.
#
# I make some assuptions about the names of the compositing functions.
#
# I look into the namespace of a set of object files and figure out
# from them what compositing functions are implemented.  This let's me
# build a table with the right cells populated with either the special
# compositing functions, or to use a generically implemented
# compositing function.


# These are in the same order as they appear in the
# ./app/base/base-enums.h GimpLayerModeEffects enumeration, because we
# (probably unwisely) use the value of the enumeration as an index
# into the Big Table.
#
# XXX I'd like some python functions that let me rummage around in C code....
#
composite_modes=[
  "GIMP_COMPOSITE_NORMAL",
  "GIMP_COMPOSITE_DISSOLVE",
  "GIMP_COMPOSITE_BEHIND",
  "GIMP_COMPOSITE_MULTIPLY",
  "GIMP_COMPOSITE_SCREEN",
  "GIMP_COMPOSITE_OVERLAY",
  "GIMP_COMPOSITE_DIFFERENCE",
  "GIMP_COMPOSITE_ADDITION",
  "GIMP_COMPOSITE_SUBTRACT",
  "GIMP_COMPOSITE_DARKEN",
  "GIMP_COMPOSITE_LIGHTEN",
  "GIMP_COMPOSITE_HUE",
  "GIMP_COMPOSITE_SATURATION",
  "GIMP_COMPOSITE_COLOR_ONLY",
  "GIMP_COMPOSITE_VALUE",
  "GIMP_COMPOSITE_DIVIDE",
  "GIMP_COMPOSITE_DODGE",
  "GIMP_COMPOSITE_BURN",
  "GIMP_COMPOSITE_HARDLIGHT",
  "GIMP_COMPOSITE_SOFTLIGHT",
  "GIMP_COMPOSITE_GRAIN_EXTRACT",
  "GIMP_COMPOSITE_GRAIN_MERGE",
  "GIMP_COMPOSITE_COLOR_ERASE",
  "GIMP_COMPOSITE_ERASE" ,
  "GIMP_COMPOSITE_REPLACE" ,
  "GIMP_COMPOSITE_ANTI_ERASE",
  "GIMP_COMPOSITE_BLEND",
  "GIMP_COMPOSITE_SHADE",
  "GIMP_COMPOSITE_SWAP",
  "GIMP_COMPOSITE_SCALE",
  "GIMP_COMPOSITE_CONVERT",
  "GIMP_COMPOSITE_XOR",
  ]

pixel_format=[
  "GIMP_PIXELFORMAT_V8",
  "GIMP_PIXELFORMAT_VA8",
  "GIMP_PIXELFORMAT_RGB8",
  "GIMP_PIXELFORMAT_RGBA8",
#  "GIMP_PIXELFORMAT_V16",
#  "GIMP_PIXELFORMAT_VA16",
#  "GIMP_PIXELFORMAT_RGB16",
#  "GIMP_PIXELFORMAT_RGBA16"
#  "GIMP_PIXELFORMAT_V32",
#  "GIMP_PIXELFORMAT_VA32",
#  "GIMP_PIXELFORMAT_RGB32",
#  "GIMP_PIXELFORMAT_RGBA32"
  "GIMP_PIXELFORMAT_ANY",
  ]


def mode_name(mode):
  s = string.replace(mode.lower(), "gimp_composite_", "")
  return (s)
  
def pixel_depth_name(pixel_format):
  s = string.replace(pixel_format.lower(), "gimp_pixelformat_", "")
  return (s)


pp = pprint.PrettyPrinter(indent=4)


def functionnameify(filename):
  f = os.path.basename(filename)
  f = string.replace(f, ".o", "")
  f = string.replace(f, ".c", "")
  f = string.replace(f, ".h", "")
  f = string.replace(f, "-", "_")
  return (f)

def filenameify(filename):
  f = os.path.basename(filename)
  f = string.replace(f, ".o", "")
  f = string.replace(f, ".c", "")
  f = string.replace(f, ".h", "")
  return (f)

def print_function_table(fpout, name, function_table, requirements=[]):

  if len(function_table) < 1:
    return;
 
  print >>fpout, 'static struct install_table {'
  print >>fpout, '  GimpCompositeOperation mode;'
  print >>fpout, '  GimpPixelFormat A;'
  print >>fpout, '  GimpPixelFormat B;'
  print >>fpout, '  GimpPixelFormat D;'
  print >>fpout, '  void (*function)(GimpCompositeContext *);'
  #print >>fpout, '  char *name;'
  print >>fpout, '} _%s[] = {' % (functionnameify(name))

  for r in requirements:
    print >>fpout, '#if %s' % (r)
    pass
  
  for mode in composite_modes:
    for A in filter(lambda pf: pf != "GIMP_PIXELFORMAT_ANY", pixel_format):
      for B in filter(lambda pf: pf != "GIMP_PIXELFORMAT_ANY", pixel_format):
        for D in filter(lambda pf: pf != "GIMP_PIXELFORMAT_ANY", pixel_format):
          key = "%s_%s_%s_%s" % (string.lower(mode), pixel_depth_name(A), pixel_depth_name(B), pixel_depth_name(D))
          if function_table.has_key(key):
            print >>fpout, ' { %s, %s, %s, %s, %s }, ' % (mode, A, B, D, function_table[key][0])
            pass
          pass
        pass
      pass
    pass

  for r in requirements:
    print >>fpout, '#endif'
    pass

  print >>fpout, ' { 0, 0, 0, 0, NULL }'
  print >>fpout, '};'
  
  return
  
def print_function_table_name(fpout, name, function_table):

  print >>fpout, ''
  print >>fpout, 'char *%s_name[%s][%s][%s][%s] = {' % (functionnameify(name), "GIMP_COMPOSITE_N", "GIMP_PIXELFORMAT_N", "GIMP_PIXELFORMAT_N", "GIMP_PIXELFORMAT_N")
  for mode in composite_modes:
    print >>fpout, ' { /* %s */' % (mode)
    for A in filter(lambda pf: pf != "GIMP_PIXELFORMAT_ANY", pixel_format):
      print >>fpout, '  { /* A = %s */' % (pixel_depth_name(A))
      for B in filter(lambda pf: pf != "GIMP_PIXELFORMAT_ANY", pixel_format):
        print >>fpout, '   /* %-6s */ {' % (pixel_depth_name(B)),
        for D in filter(lambda pf: pf != "GIMP_PIXELFORMAT_ANY", pixel_format):
          key = "%s_%s_%s_%s" % (string.lower(mode), pixel_depth_name(A), pixel_depth_name(B), pixel_depth_name(D))
          if function_table.has_key(key):
            print >>fpout, '"%s", ' % (function_table[key][0]),
          else:
            print >>fpout, '"%s", ' % (""),
            pass
          pass
        print >>fpout, '},'
        pass
      print >>fpout, '  },'
      pass
    print >>fpout, ' },'
    pass

  print >>fpout, '};\n'
  
  return
  
def load_function_table(filename):
  nmx = ns.nmx(filename)

  gimp_composite_function = dict()

  for mode in composite_modes:
    for A in filter(lambda pf: pf != "GIMP_PIXELFORMAT_ANY", pixel_format):
      for B in filter(lambda pf: pf != "GIMP_PIXELFORMAT_ANY", pixel_format):
        for D in filter(lambda pf: pf != "GIMP_PIXELFORMAT_ANY", pixel_format):
          key = "%s_%s_%s_%s" % (string.lower(mode), pixel_depth_name(A), pixel_depth_name(B), pixel_depth_name(D))
            
          for a in ["GIMP_PIXELFORMAT_ANY", A]:
            for b in ["GIMP_PIXELFORMAT_ANY", B]:
              for d in ["GIMP_PIXELFORMAT_ANY", D]:
                key = "%s_%s_%s_%s" % (string.lower(mode), pixel_depth_name(a), pixel_depth_name(b), pixel_depth_name(d))
                  
                f = nmx.exports_re(key + ".*")
                if f != None: gimp_composite_function["%s_%s_%s_%s" % (string.lower(mode), pixel_depth_name(A), pixel_depth_name(B), pixel_depth_name(D))] =  [f]
                pass
              pass
            pass
          pass
        pass
      pass
    pass

  return (gimp_composite_function)


def merge_function_tables(tables):
  main_table = copy.deepcopy(tables[0][1])
  
  for t in tables[1:]:
    #print >>sys.stderr, t[0]
    for mode in composite_modes:
      for A in filter(lambda pf: pf != "GIMP_PIXELFORMAT_ANY", pixel_format):
        for B in filter(lambda pf: pf != "GIMP_PIXELFORMAT_ANY", pixel_format):
          for D in filter(lambda pf: pf != "GIMP_PIXELFORMAT_ANY", pixel_format):
            key = "%s_%s_%s_%s" % (string.lower(mode), pixel_depth_name(A), pixel_depth_name(B), pixel_depth_name(D))
            if t[1].has_key(key):
              #print >>sys.stderr, "%s = %s::%s" % (key, t[0], t[1][key])
              main_table[key] = t[1][key]
              pass
            pass
          pass
        pass
      pass
    pass
            
  return (main_table)


def gimp_composite_regression(fpout, function_tables, options):

  # XXX move all this out to C code, instead of here.
  print >>fpout, '#include "config.h"'
  print >>fpout, ''
  print >>fpout, '#include <stdio.h>'
  print >>fpout, '#include <stdlib.h>'
  print >>fpout, '#include <string.h>'
  print >>fpout, ''
  print >>fpout, '#include <sys/time.h>'
  print >>fpout, ''
  print >>fpout, '#include <glib-object.h>'
  print >>fpout, ''
  print >>fpout, '#include "base/base-types.h"'
  print >>fpout, ''
  print >>fpout, '#include "gimp-composite.h"'
  print >>fpout, '#include "gimp-composite-dispatch.h"'
  print >>fpout, '#include "gimp-composite-regression.h"'
  print >>fpout, '#include "gimp-composite-util.h"'
  print >>fpout, '#include "gimp-composite-generic.h"'
  print >>fpout, '#include "%s.h"' % (filenameify(options.file))
  print >>fpout, ''
  print >>fpout, 'int'
  print >>fpout, '%s_test(int iterations, int n_pixels)' % (functionnameify(options.file))
  print >>fpout, '{'
  print >>fpout, '  GimpCompositeContext generic_ctx;'
  print >>fpout, '  GimpCompositeContext special_ctx;'
  print >>fpout, '  double ft0;'
  print >>fpout, '  double ft1;'
  print >>fpout, '  gimp_rgba8_t *rgba8D1;'
  print >>fpout, '  gimp_rgba8_t *rgba8D2;'
  print >>fpout, '  gimp_rgba8_t *rgba8A;'
  print >>fpout, '  gimp_rgba8_t *rgba8B;'
  print >>fpout, '  gimp_rgba8_t *rgba8M;'
  print >>fpout, '  gimp_va8_t *va8A;'
  print >>fpout, '  gimp_va8_t *va8B;'
  print >>fpout, '  gimp_va8_t *va8M;'
  print >>fpout, '  gimp_va8_t *va8D1;'
  print >>fpout, '  gimp_va8_t *va8D2;'
  print >>fpout, '  int i;'
  print >>fpout, ''
  print >>fpout, '  rgba8A =  gimp_composite_regression_fixed_rgba8(n_pixels+1);'
  print >>fpout, '  rgba8B =  gimp_composite_regression_fixed_rgba8(n_pixels+1);'
  print >>fpout, '  rgba8M =  gimp_composite_regression_fixed_rgba8(n_pixels+1);'
  print >>fpout, '  rgba8D1 = (gimp_rgba8_t *) calloc(sizeof(gimp_rgba8_t), n_pixels+1);'
  print >>fpout, '  rgba8D2 = (gimp_rgba8_t *) calloc(sizeof(gimp_rgba8_t), n_pixels+1);'
  print >>fpout, '  va8A =    (gimp_va8_t *)   calloc(sizeof(gimp_va8_t), n_pixels+1);'
  print >>fpout, '  va8B =    (gimp_va8_t *)   calloc(sizeof(gimp_va8_t), n_pixels+1);'
  print >>fpout, '  va8M =    (gimp_va8_t *)   calloc(sizeof(gimp_va8_t), n_pixels+1);'
  print >>fpout, '  va8D1 =   (gimp_va8_t *)   calloc(sizeof(gimp_va8_t), n_pixels+1);'
  print >>fpout, '  va8D2 =   (gimp_va8_t *)   calloc(sizeof(gimp_va8_t), n_pixels+1);'
  print >>fpout, ''
  print >>fpout, '  for (i = 0; i < n_pixels; i++) {'
  print >>fpout, '    va8A[i].v = i;'
  print >>fpout, '    va8A[i].a = 255-i;'
  print >>fpout, '    va8B[i].v = i;'
  print >>fpout, '    va8B[i].a = i;'
  print >>fpout, '    va8M[i].v = i;'
  print >>fpout, '    va8M[i].a = i;'
  print >>fpout, '  }'
  print >>fpout, ''

  #pp.pprint(function_tables)

  generic_table = function_tables
  
  for mode in composite_modes:
    for A in filter(lambda pf: pf != "GIMP_PIXELFORMAT_ANY", pixel_format):
      for B in filter(lambda pf: pf != "GIMP_PIXELFORMAT_ANY", pixel_format):
        for D in filter(lambda pf: pf != "GIMP_PIXELFORMAT_ANY", pixel_format):
          key = "%s_%s_%s_%s" % (string.lower(mode), pixel_depth_name(A), pixel_depth_name(B), pixel_depth_name(D))
          if function_tables.has_key(key):
            #print key
            print >>fpout, ''
            print >>fpout, '  /* %s */' % (key)
            print >>fpout, '  memset((void *) &special_ctx, 0, sizeof(special_ctx));'
            print >>fpout, '  special_ctx.op = %s;' % (mode)
            print >>fpout, '  special_ctx.n_pixels = n_pixels;'
            print >>fpout, '  special_ctx.scale.scale = 2;'
            print >>fpout, '  special_ctx.pixelformat_A = %s;' % (A)
            print >>fpout, '  special_ctx.pixelformat_B = %s;' % (B)
            print >>fpout, '  special_ctx.pixelformat_D = %s;' % (D)
            print >>fpout, '  special_ctx.pixelformat_M = %s;' % (D)
            print >>fpout, '  special_ctx.A = (unsigned char *) %sA;' % (pixel_depth_name(A))
            print >>fpout, '  special_ctx.B = (unsigned char *) %sB;' % (pixel_depth_name(B))
            print >>fpout, '  special_ctx.M = (unsigned char *) %sB;' % (pixel_depth_name(D))
            print >>fpout, '  special_ctx.D = (unsigned char *) %sD1;' % (pixel_depth_name(D))
            print >>fpout, '  memset(special_ctx.D, 0, special_ctx.n_pixels * gimp_composite_pixel_bpp[special_ctx.pixelformat_D]);'
            
            print >>fpout, '  memset((void *) &generic_ctx, 0, sizeof(special_ctx));'
            print >>fpout, '  generic_ctx.op = %s;' % (mode)
            print >>fpout, '  generic_ctx.n_pixels = n_pixels;'
            print >>fpout, '  generic_ctx.scale.scale = 2;'
            print >>fpout, '  generic_ctx.pixelformat_A = %s;' % (A)
            print >>fpout, '  generic_ctx.pixelformat_B = %s;' % (B)
            print >>fpout, '  generic_ctx.pixelformat_D = %s;' % (D)
            print >>fpout, '  generic_ctx.pixelformat_M = %s;' % (D)
            print >>fpout, '  generic_ctx.A = (unsigned char *) %sA;' % (pixel_depth_name(A))
            print >>fpout, '  generic_ctx.B = (unsigned char *) %sB;' % (pixel_depth_name(B))
            print >>fpout, '  generic_ctx.M = (unsigned char *) %sB;' % (pixel_depth_name(D))
            print >>fpout, '  generic_ctx.D = (unsigned char *) %sD2;' % (pixel_depth_name(D))
            print >>fpout, '  memset(generic_ctx.D, 0, generic_ctx.n_pixels * gimp_composite_pixel_bpp[generic_ctx.pixelformat_D]);'

            #print >>fpout, '  gimp_composite_context_print(&special_ctx);'
            #print >>fpout, '  gimp_composite_context_print(&generic_ctx);'
            
            print >>fpout, '  ft0 = gimp_composite_regression_time_function(iterations, %s, &generic_ctx);' % ("gimp_composite_dispatch")
            print >>fpout, '  ft1 = gimp_composite_regression_time_function(iterations, %s, &special_ctx);' % (generic_table[key][0])
            print >>fpout, '  gimp_composite_regression_compare_contexts("%s", &generic_ctx, &special_ctx);' % (mode_name(mode))
            print >>fpout, '  gimp_composite_regression_timer_report("%s", ft0, ft1);' % (mode_name(mode))
            pass
          pass
        pass
      pass
    pass
  
  print >>fpout, '  return (0);'
  print >>fpout, '}'

  print >>fpout, ''
  print >>fpout, 'int'
  print >>fpout, 'main(int argc, char *argv[])'
  print >>fpout, '{'
  print >>fpout, '  int iterations;'
  print >>fpout, '  int n_pixels;'
  print >>fpout, ''
  print >>fpout, '  srand(314159);'
  print >>fpout, ''
  print >>fpout, '  putenv("GIMP_COMPOSITE=0x1");'
  print >>fpout, ''
  print >>fpout, '  iterations = %d;' % options.iterations
  print >>fpout, '  n_pixels = %d;' % options.n_pixels
  print >>fpout, ''
  print >>fpout, '  gimp_composite_generic_install();'
  print >>fpout, ''
  print >>fpout, '  return (%s_test(iterations, n_pixels));' % (functionnameify(options.file))
  print >>fpout, '}'

  return


def gimp_composite_installer_install(fpout, name, function_table, requirements=[]):
  print >>fpout, ''
  print >>fpout, 'void'
  print >>fpout, '%s_install (void)' % (functionnameify(name))
  print >>fpout, '{'

  for r in requirements:
    print >>fpout, '#if %s' % (r)
    pass

  if len(function_table) > 1:
    print >>fpout, '  int mode, a, b, d;'
    print >>fpout, ''
    print >>fpout, '  for (mode = 0; mode < GIMP_COMPOSITE_N; mode++) {' # Assumes no "holes" in the enumeration
    print >>fpout, '    for (a = 0; a < GIMP_PIXELFORMAT_N; a++) {'
    print >>fpout, '      for (b = 0; b < GIMP_PIXELFORMAT_N; b++) {'
    print >>fpout, '        for (d = 0; d < GIMP_PIXELFORMAT_N; d++) {'
    print >>fpout, '          if (%s[mode][a][b][d]) {' % (functionnameify(name))
    print >>fpout, '            gimp_composite_function[mode][a][b][d] = %s[mode][a][b][d];' % (functionnameify(name))
    print >>fpout, '            if (gimp_composite_options.bits & GIMP_COMPOSITE_OPTION_VERBOSE) {'
    print >>fpout, '              printf("gimp_composite_install: %s %s %s %s: %p\\n", gimp_composite_mode_astext(mode), gimp_composite_pixelformat_astext(a),  gimp_composite_pixelformat_astext(b), gimp_composite_pixelformat_astext(d), gimp_composite_function[mode][a][b][d]);'
    print >>fpout, '            }'
    print >>fpout, '          }'
    print >>fpout, '        }'
    print >>fpout, '      }'
    print >>fpout, '    }'
    print >>fpout, '  }'
  else:
    print >>fpout, '  /* nothing to do */'
    pass
  
  print >>fpout, ''
  print >>fpout, '  %s_init();' % functionnameify(name)
  for r in requirements:
    print >>fpout, '#endif'
    pass
  print >>fpout, '}'
  pass

def gimp_composite_installer_install2(fpout, name, function_table, requirements=[]):
  print >>fpout, ''
  print >>fpout, 'void'
  print >>fpout, '%s_install (void)' % (functionnameify(name))
  print >>fpout, '{'

  if len(function_table) > 1:
    print >>fpout, '  static struct install_table *t = _%s;' % (functionnameify(name))
    print >>fpout, ''
    print >>fpout, '  for (t = &_%s[0]; t->function != NULL; t++) {' % (functionnameify(name))
    print >>fpout, '    gimp_composite_function[t->mode][t->A][t->B][t->D] = t->function;'
    print >>fpout, '  }'
  else:
    print >>fpout, '  /* nothing to do */'
    pass
  
  print >>fpout, ''
  print >>fpout, '  %s_init();' % functionnameify(name)
  print >>fpout, '}'
  pass

def gimp_composite_hfile(fpout, name, function_table):
  print >>fpout, '/* THIS FILE IS AUTOMATICALLY GENERATED.  DO NOT EDIT */'
  print >>fpout, '/* REGENERATE BY USING make-installer.py */'
  print >>fpout, ''
  print >>fpout, 'void %s_install (void);' % (functionnameify(name))
  print >>fpout, ''
  print >>fpout, 'typedef void (*%s_table[%s][%s][%s][%s]);' % (functionnameify(name), "GIMP_COMPOSITE_N", "GIMP_PIXELFORMAT_N", "GIMP_PIXELFORMAT_N", "GIMP_PIXELFORMAT_N")
  
  return

def gimp_composite_cfile(fpout, name, function_table, requirements=[]):
  print >>fpout, '/* THIS FILE IS AUTOMATICALLY GENERATED.  DO NOT EDIT */'
  print >>fpout, '/* REGENERATE BY USING make-installer.py */'
  print >>fpout, '#include "config.h"'
  print >>fpout, '#include <glib-object.h>'
  print >>fpout, '#include <stdlib.h>'
  print >>fpout, '#include <stdio.h>'
  print >>fpout, '#include "base/base-types.h"'
  print >>fpout, '#include "gimp-composite.h"'
  print >>fpout, ''
  print >>fpout, '#include "%s.h"' % (filenameify(name))
  print >>fpout, ''

  print_function_table(fpout, name, function_table, requirements)

  gimp_composite_installer_install2(fpout, name, function_table, requirements)

  return

###########################################

op = optparse.OptionParser(version="$Revision$")
op.add_option('-f', '--file', action='store',      type='string', dest='file',        default=None,
              help='the input object file')
op.add_option('-t', '--test', action='store_true',                 dest='test',       default=False,
              help='generate regression testing code')
op.add_option('-i', '--iterations', action='store', type='int',    dest='iterations', default=1,
              help='number of iterations in regression tests')
op.add_option('-n', '--n_pixels', action='store',  type="int",     dest='n_pixels',   default=512*512+1,
              help='number of pixels in each regression test iteration')
op.add_option('-r', '--requires', action='append', type='string',  dest='requires',   default=[],
              help='cpp #if conditionals')
options, args = op.parse_args()

table = load_function_table(options.file)

gimp_composite_cfile(open(filenameify(options.file) + "-installer.c", "w"), options.file, table, options.requires)

if options.test == True:
  gimp_composite_regression(open(filenameify(options.file) + "-test.c", "w"), table, options)
  pass

sys.exit(0)
