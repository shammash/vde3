VDE_SRC = context.c component.c conn_manager.c logging.c module.c transport_vde2.c connection.c

VDE_OBJ = context.o component.o conn_manager.o logging.o module.o transport_vde2.o connection.o

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

%.o: %.c
	$(CC) $(EXT_CFLAGS) $(VDE_DEF) -Wall -c -Iinclude -o $@ $<


all: $(VDE_OBJ)

clean:
	-rm $(VDE_OBJ)
