VDE_SRC = context.c component.c conn_manager.c logging.c module.c \
  transport_vde2.c connection.c libevent_handler.c engine_hub.c \
  localconnection.c

VDE_OBJ = context.o component.o conn_manager.o logging.o module.o \
  transport_vde2.o connection.o libevent_handler.o engine_hub.o \
  localconnection.o engine_hub_commands.o

LOTSA_WARNINGS=-Winline -Wformat=2 -Wswitch-default -Wswitch-enum \
-Wunused-parameter -Wfloat-equal -Wundef -Wshadow -Wunsafe-loop-optimizations \
-Wtype-limits -Wcast-qual -Wcast-align -Wconversion -Wno-sign-conversion \
-Wempty-body -Wsign-compare -Wlogical-op -Waggregate-return -Wstrict-prototypes \
-Wold-style-declaration -Wold-style-definition -Wmissing-parameter-type \
-Wmissing-prototypes -Wmissing-declarations -Wmissing-noreturn \
-Wmissing-format-attribute -Wpacked -Wunreachable-code -Wnested-externs \
-Wdisabled-optimization -Wunused -Wunused-parameter

ANNOYING_WARNINGS=-Wpadded # warn when adding pad to structs

EXT_CFLAGS=$(shell pkg-config --cflags json glib-2.0)
EXT_LIBS=$(shell pkg-config --libs json glib-2.0) -levent
VDE_DEF=-ggdb -Iinclude/ -I.

%.o: %.c
	$(CC) $(CFLAGS) $(EXT_CFLAGS) $(VDE_DEF) -Wall -c -o $@ $<

all: $(VDE_OBJ) vde_hub vde_hub2hub

vde_hub: $(VDE_OBJ) vde_hub.o
	$(CC) $(CFLAGS) $(EXT_LIBS) -o $@ $^

vde_hub2hub: $(VDE_OBJ) vde_hub2hub.o
	$(CC) $(CFLAGS) $(EXT_LIBS) -o $@ $^

engine_hub_commands.c: engine_hub_wrappable.json
	python gen_checker.py engine_hub_wrappable.json

engine_hub.c: engine_hub_commands.c

clean:
	-rm $(VDE_OBJ)
	-rm vde_hub vde_hub2hub
	-rm engine_hub_commands.c engine_hub_commands.h

tags:
	cscope -b -R -Iinclude/
