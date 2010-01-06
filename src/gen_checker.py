# Copyright (C) 2009 - Virtualsquare Team
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

import optparse
import os
import re
import sys
import time

import simplejson

# XXX:
# - possible alternatives to declare auto-generated code:
#   + a file compliant with http://json-schema.org/
#   + doxygen tags instead of a json struct

# Schema for input json struct:

# {
#   "basename": "struct_and_files_prefix",
#   "wrappables": [
#     {
#       "name": "the.command.name",
#       "fun": "function_name",
#       "description": "Function help",
#       "parameters":
#         [
#           {
#             "name": "param1"
#             "description": "Parameter description"
#             "type": "int/bool/string"
#           },
#
#           ...,
#
#           {
#             "name": "paramN"
#             "description": "Parameter description"
#             "type": "int/bool/string"
#           }
#         ]
#     }
#     ...
#   ]
# }

COMMANDS_SUFFIX='_commands.h'
WRAPPERS_SUFFIX='_commands.c'

typemap = {'int': ('int', 'vde_sobj_type_int', 'vde_sobj_get_int'),
           'double': ('double', 'vde_sobj_type_double', 'vde_sobj_get_double'),
           'bool': ('bool', 'vde_sobj_type_boolean', 'vde_sobj_get_boolean'),
           'string': ('const char *', 'vde_sobj_type_string', 'vde_sobj_get_string'),
}

# XXX: escape descriptions?

def gen_command(info):
  return ['{ "%(name)s", %(fun)s_wrapper, '
          '  "%(description)s", %(fun)s_wrapper_params },' % info]

def gen_wrapper_declaration(info):
  return ['int %s_wrapper(%s, %s, %s);' % (info['fun'],
                                           'vde_component *component',
                                           'vde_sobj *in', 'vde_sobj **out')]

def gen_params(info):
  res = []
  res.append('static vde_argument %(fun)s_wrapper_params[] = {' % info)

  for p in info['parameters']:
    res.append('  {"%(name)s", "%(description)s", "%(type)s"},' % p)

  res.append('  { NULL, NULL, NULL },')
  res.append('};')
  return res

def gen_wrapper(info):
  params = ''
  num_params = len(info['parameters'])
  # function signature
  args = ['vde_component *component']
  args.extend(['%s %s' % (typemap[p['type']][0], p['name']) for p in info['parameters']])
  args.append('vde_sobj **out')
  wrap = ['int %s(%s);' % (info['fun'], ', '.join(args))]
  wrap.append('')
  wrap.append('int %s_wrapper(%s, %s, %s) {' %
              (info['fun'], 'vde_component *component',
               'vde_sobj *in', 'vde_sobj **out'))

  # declare variables
  for p in info['parameters']:
    var = p['name']
    json_var = 'json_%s' % var
    type = p['type']
    wrap.append('  %s %s; vde_sobj *%s;' % (typemap[type][0], var, json_var))
  # sanity check on received json
  wrap.append('  if (!vde_sobj_is_type(in, vde_sobj_type_array)) {')
  wrap.append('    *out = vde_sobj_new_string("Did not receive an array");')
  wrap.append('    return -1;')
  wrap.append('  }')
  wrap.append('  if (vde_sobj_array_length(in) != %s) {' % num_params)
  wrap.append('    *out = vde_sobj_new_string("Expected %s params");' %
              num_params)
  wrap.append('    return -1;')
  wrap.append('  }')
  # check and convert parameters
  for i, p in enumerate(info['parameters']):
    var = p['name']
    json_var = 'json_%s' % var
    type = p['type']
    wrap.append('  %s = vde_sobj_array_get_idx(in, %s);' % (json_var, i))
    wrap.append('  if (!vde_sobj_is_type(%s, %s)) {' %
                (json_var, typemap[type][1]))
    wrap.append('    *out = vde_sobj_new_string("Param %s not a %s");' %
                (var, type))
    wrap.append('    return -1;')
    wrap.append('  }')
    wrap.append('  %s = %s(%s);' % (var, typemap[type][2], json_var))
    params += '%s, ' % var
  # call function
  wrap.append('  return %s(component, %sout);' % (info['fun'], params))
  wrap.append('}')
  return wrap

# this function returns a tuple of lists, each containing a line
def do_wrap(wrappable):
  declarations = []
  params = []
  commands = []
  wrappers = []

  for info in wrappable:
    # TODO: check info is a dict, parameters is an array, type is in typemap
    declarations.extend(gen_wrapper_declaration(info))
    params.extend(gen_params(info))
    commands.extend(gen_command(info))
    wrappers.extend(gen_wrapper(info))
    wrappers.append('')  # separate wrappers with and empty line

  return declarations, params, commands, wrappers

def main():
  usage = 'usage: %prog [options] <filename>'
  parser = optparse.OptionParser(usage=usage)

  parser.add_option('-o', '--outdir', dest='outdir',
                    help='output files to OUTDIR (default: .)',
                    metavar='OUTDIR', default='.', type="string")
  parser.add_option('-b', '--basename', dest='basename',
                    help='override basename in wrappable', type="string")
  parser.add_option('-q', '--quiet',
                    help="Output only errors (default: no)",
                    action='store_false', dest='verbose')
  parser.add_option('-v', '--verbose',
                    help="Output messages on stdout (default: yes)",
                    action='store_true', dest='verbose', default=True)

  (options, args) = parser.parse_args()
  if len(args) < 1:
    parser.error('provide one filename to operate on')

  wrappable_path = args[0]
  me = os.path.basename(sys.argv[0])

  try:
    data = simplejson.load(open(wrappable_path, 'r'))
  except IOError, e:
    print >>sys.stderr, 'error opening: %s' % e
    return 1
  except ValueError, e:
    print >>sys.stderr, 'error parsing %s: %s' % (wrappable_path, e)
    return 3

  if not ('basename' in data and 'wrappables' in data):
    print >>sys.stderr, 'invalid wrappable, data or basename keys not found'
    return 3

  basename = data['basename']
  if options.basename:
    basename = options.basename

  outdir = options.outdir

  header_name = '%s%s' % (basename, COMMANDS_SUFFIX)
  wrapper_name = '%s%s' % (basename, WRAPPERS_SUFFIX)

  header_path = os.path.join(outdir, header_name)
  wrapper_path = os.path.join(outdir, wrapper_name)

  try:
    cmd_out = open(header_path, 'w')
  except IOError, e:
    print >>sys.stderr, 'error writing: %s' % e
    return 1

  try:
    wrap_out = open(wrapper_path, 'w')
  except IOError, e:
    print >>sys.stderr, 'error writing: %s' % e
    return 1

  # start real processing
  declarations, params, commands, wrappers = do_wrap(data['wrappables'])

  # write C declarations / header
  header_guard = '__%s__' % re.sub('[^_A-Z]', '_', header_name.upper())
  cmd_out.write('/* Autogenerated by %s\n * on %s\n * do not edit!!\n */\n' %
                (me, time.ctime()))
  cmd_out.write('\n')
  cmd_out.write('#ifndef %s\n' % header_guard)
  cmd_out.write('#define %s\n' % header_guard)
  cmd_out.write('\n')
  cmd_out.write('#include <stdbool.h>\n')
  cmd_out.write('#include <vde3.h>\n')
  cmd_out.write('#include <vde3/common.h>\n')
  cmd_out.write('#include <vde3/command.h>\n')
  cmd_out.write('\n')
  for el in declarations:
    cmd_out.write(el + '\n')
  cmd_out.write('\n')
  for el in params:
    cmd_out.write(el + '\n')
  cmd_out.write('\n')

  cmd_out.write('static vde_command %s_commands [] = {\n' % basename)
  commands.append('{ NULL, NULL, NULL, NULL },')
  for c in commands:
    cmd_out.write('  %s\n' % c)
  cmd_out.write('};\n')
  cmd_out.write('\n')
  cmd_out.write('#endif /* %s */\n' % header_guard)
  cmd_out.write('\n')

  cmd_out.close()
  if options.verbose:
    print "Wrote %s" % cmd_out.name

  # write C definitions
  wrap_out.write('/* Autogenerated by %s\n * on %s\n * do not edit!!\n */\n' %
                (me, time.ctime()))
  wrap_out.write('\n')
  wrap_out.write('#include "%s"\n' % header_name)
  wrap_out.write('\n')
  for w in wrappers:
    wrap_out.write(w + '\n')
  wrap_out.close()

  if options.verbose:
    print "Wrote %s" % wrap_out.name

  return 0

if __name__ == '__main__':
  sys.exit(main())

# vim:et:sw=2:ts=2
