
-------
INSTALL
-------

Mandatory build dependencies:

- glib-2.0
  homepage: http://www.gtk.org/download.html
  debian: libglib2.0-dev
- libevent
  homepage: http://www.monkey.org/~provos/libevent/
  debian: libevent-dev
- json-c
  homepage: http://oss.metaparadigm.com/json-c/
  debian: no package

Optional build dependencies:

- python
  for commands wrappers generator
- doxygen
  for documentation
- check
  for unit tests
- valgrind
  for unit tests analysis

Build steps:

- ./configure
- make
- make install
- **fun**
- make uninstall

Other make targets:

- make doxygen
  build documentation
- make check
  run unit tests
- make check-valgrind
  run unit tests under valgrind



------------
ARCHITECTURE
------------

VDE 3 framework is an event-based library which provides an easy way to build
highly customized virtual network tools. These tools can be built by connecting
basic components within a context. Such components are provided by modules
dynamically loaded at run-time and can be easily developed by others.


Library overview
----------------

As an example let's describe the steps needed to build an ethernet hub capable
of accepting connections from VDE 2 (the code can be found in src/vde_hub.c).

Create and initialize a new context
'''''''''''''''''''''''''''''''''''

The context is the entity responsible for the creation of new components and
the environment in which these will run. There are two important configurable
properties for a context which can be set through ``vde_context_init()``: the
modules path and the event handler. Each module found in the modules path
provides an implementation of a particular component. The event handler
provides to the library an interface where to register callbacks for read/write
events on file descriptors or timeouts.

In this example we can use the default search path of the library and an
event handler based on libevent.

Create new components inside the context
''''''''''''''''''''''''''''''''''''''''

Once the context has been successfully initialized it is possible to create new
components inside it by calling ``vde_context_new_component()``. Within a
context each component is identified by its name, passed to
``vde_context_new_component()`` at creation time, thus component names must be
unique in a context.
There are three kinds of components:

**transport**
  responsible for the creation of communication channels (connections) between
  the context and the outside world;
**engine**
  processes data to/from various connections;
**connection manager**
  ties a transport to an engine so that new connections originating from the
  transport will be associated to the engine.

Different implementations of the same component kind are identified by
different families.

To create a VDE 2 compatible ethernet hub three components are needed: an
engine of the ``hub`` family, a transport of the ``vde2`` family and a
connection manager of the ``default`` family which will tie the two previous
components.

Invoke operations on components
'''''''''''''''''''''''''''''''

After the components are created it is possible to invoke operations on them.
Two of the most common operations are listen and connect issued to the
connection manager, however each component implementation can provide its own
operations.

In this example, to accept connections from VDE 2, we tell the
connection manager to listen using ``vde_conn_manager_listen()``.


Life of a connection
--------------------

A connection is an abstract object used by an engine to receive and send
packets. The engine interacts with the connection by writing packets and
registering callbacks to be informed whenever new packets are available to be
read.

Each connection has an associated backend which implements the mechanisms to
handle packets. When a transport creates a new connection the transport itself
implements the backend, but it is also possible to have local connections
between two engines in the same context. In this case the backend will call the
read callback of an engine as soon as it receives a packet from the other one.

Following the above example, on ``vde_conn_manager_listen()`` the connection
manager will put the transport in listen state.
When the transport has a new connection ready it will call the ``accept_cb()``
callback of connection manager, which will in turn will register the new
connection to the engine associated with the connection manager by calling
``vde_engine_new_conn()``.

Optionally the connection manager before calling ``vde_engine_new_conn()`` can
negotiate authorization data with the remote connection manager, based on this
data and the local engine settings the connection will be either accepted or
rejected.

A connection can be closed either explicitly by the engine or as a consequence
of a fatal error in the backend. In the former case ``vde_conn_fini()`` and
``vde_conn_delete()`` are called to shutdown the connection, in the latter case
the ``error_cb()`` callback of the engine is called to free resources
associated with the connection.


Life of a packet
----------------

A packet is a structured container of data exchanged by engines.

Inside each packet there's a payload plus a VDE-specific header used to
identify the payload type and size. The engine expects a full vde packet to be
received from a connection, however a specific backend might add or remove the
header as needed. For instance connections generated by transport ``vde2`` or
``tap`` will add/remove the header on receive/send because the underlying
protocol expects standard ethernet frames.

When a connection has a vde packet ready to be processed it calls the
``read_cb()`` callback of the engine passing a pointer to the packet. When an
engine wants to send a packet on a connection it calls
``vde_connection_write()`` passing a pointer to the packet as well.

In both cases it is responsability of the caller to free the packet after the
function call returns, thus if the callee wants to preserve the packet it must
perform a copy.

An engine may require additional space when processing a packet, for instance
to tag/untag an ethernet frame with 802.1Q informations or to build a layer 2
packet from a layer 3 packet. In these cases, instead of copying the packet to
a large memory area, the engine can ask the connection to preallocate head and
tail space around the payload.


Remote management
-----------------

In addition to library and modules API it's possible to remotely control a vde
context by setting up a control engine which acts as an RPC interface for the
components. This engine exchanges vde packets using vde connections as any
other engine does, however the content of these packets is not network data, it
is a serialized method call which the control engine is able to decode and
dispatch to the right component. Method calls are serialized into JSON strings
and follow JSON-RPC 1.0 specifications. The method name format of the remote
call is: ``component_name.method_name``.

A component, to remotely expose its functionalities, must dynamically register
them at runtime. These functionalities are divided in two categories:

**commands**
  synchronous method calls from the remote user to a component. From the
  component point of view commands are regular functions which can populate a
  serializable object that will be used as the remote call reply or error.
**signals**
  asynchronous method calls to the remote user done by the component to notify
  changes in its properties. To raise a signal a component calls the function
  ``vde_component_signal_raise()`` passing a signal name and a serializable
  object representing the property changes. The signal will be then propagated
  to all the registered listeners.

Commands and signals can be registered/unregistered at any time via
``vde_component_command_[add|del]()`` and ``vde_component_signal_[add|del]()``
respectively. A listener can be registered/unregistered to a signal via
``vde_component_signal_[attach|detach]()``.

Components implementations must provide a description for their commands and
signals that will be used to generate static checks for the incoming method
calls and at the same time to build an online help that can be requested by the
remote caller.

The following is an example of a command execution on the ``hub`` engine
running in the example application ``src/vde_hub.c``:

::

  --> { "method": "e1.printport", "params": [1], "id": 0 }
  <-- { "id": 0, "result": "please print something useful for port 1", "error": null }

And this is an example of signal registration and signal delivery on the same
engine:

::

  --> { "method": "e2.notify_add", "params": ["e1.port_new"], "id": 0 }
  <-- { "id": 0, "result": "Signal attached", "error": null }
  ... a new connection is added to the hub ...
  <-- { "id": null, "method": "e1.port_new", "params": [ 1 ] }

