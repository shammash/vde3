VDE_SRC = context.c component.c conn_manager.c logging.c module.c transport_vde2.c

VDE_OBJ = context.o component.o conn_manager.o logging.o module.o transport_vde2.o

%.o: %.c
	gcc-4.3 `pkg-config --cflags json glib-2.0` $(VDE_DEF) -c -Iinclude -o $@ $<


all: $(VDE_OBJ)

clean:
	-rm $(VDE_OBJ)
