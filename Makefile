GLIB_CFLAGS := $(shell pkg-config --cflags glib-2.0 gobject-2.0)
GLIB_LIBS := $(shell pkg-config --libs glib-2.0 gobject-2.0)

all: sample-query

clean:
	rm sample-query

sample-query: src/mo.c src/mo.h example/sample-query.c
	gcc -Wall -Wpedantic -pedantic -Wextra -I$(CURDIR)/src $(GLIB_CFLAGS) $^ -o $@ $(GLIB_LIBS)
