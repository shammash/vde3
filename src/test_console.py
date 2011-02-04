#!/usr/bin/python

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

import sys
import readline
import socket
import tempfile
import os
import struct
import atexit
import termios
import threading
import select

PROMPT='vde> '
_MAXPATH=1024
_MAXRECV=4096

pack_unpack = { 'darwin': ('<IIIh104s128s', '<h104s'),
                'linux2': ('<IIIh108s128s', '<h108s'),
              }

PACK_STR, UNPACK_STR = pack_unpack[sys.platform]

data = None
ctl = None
quit = False

def get_from_data():
  global quit

  while not quit:
    rlist, wlist, xlist = select.select([data, ctl], [], [], 1.0)
    if data in rlist:
      read = data.recv(_MAXRECV)
      print repr(read)
      sys.stdout.write(PROMPT)
      sys.stdout.flush()
    if ctl in rlist and ctl.recv(0) == '':
      print "Connection closed by remote side"
      quit = True

def setterm():
  old = termios.tcgetattr(sys.stdin)
  new = termios.tcgetattr(sys.stdin)
  #new[3] = new[3] & ~termios.ICANON & ~termios.ECHO
  new[3] = new[3] & ~termios.ICANON
  new[6] [termios.VMIN] = 1
  new[6] [termios.VTIME] = 0

  termios.tcsetattr(sys.stdin, termios.TCSANOW, new)
  termios.tcsendbreak(sys.stdin,0)

  atexit.register(termios.tcsetattr, sys.stdin, termios.TCSAFLUSH, old)


def main():
  global data, ctl
  global quit
  ctl, datasock, peername = vde2_connect('/tmp/vde3_test_ctrl/ctl')
  print 'connected to %s.' % peername
  data = datasock

  atexit.register(os.remove, data.getsockname())

  #setterm()

  #sys.stdout.write(PROMPT)
  #sys.stdout.flush()

  th = threading.Thread(target = get_from_data, name = 'getter')
  th.start()
  while not quit:
    try:
      cmd = raw_input(PROMPT)
    except (EOFError, KeyboardInterrupt):
      print "Got EOF"
      quit = True
      break
    if cmd:
      data.sendto(cmd + '\x00', peername)

  print 'Quitting.'
  th.join(1)

  return 0

def vde2_connect(remote_address, local_address=None):
  # connect to vde2 socket on remote_address
  # return (ctl_socket, data_socket, peer_address)

  # XXX error checking

  ctl_sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
  ctl_sock.connect(remote_address)

  data_sock = socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM)
  if local_address is None:
    (fd, name) = tempfile.mkstemp()
    os.close(fd) # yes, this is racy
    local_address = name

  try:
    os.remove(local_address)
  except OSError:
    pass

  data_sock.bind(local_address)
  data = struct.pack(PACK_STR, 0xfeedface, 3, 0, 1, local_address,
                     "vde minimal console")
  ctl_sock.send(data)

  data = ctl_sock.recv(_MAXPATH)
  peer_address = struct.unpack(UNPACK_STR, data)[1].split('\x00')[0]

  return (ctl_sock, data_sock, peer_address)

if __name__ == '__main__':
  histfile = os.path.join(".vde_console_hist")
  try:
      readline.read_history_file(histfile)
  except IOError:
      pass
  atexit.register(readline.write_history_file, histfile)

  sys.exit(main())
