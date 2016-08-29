ACLOCAL_AMFLAGS = -I m4

lib_LTLIBRARIES = src/libmo.la

src_libmo_la_SOURCES = src/mo.c

src_libmo_la_CFLAGS = $(GLIB_CFLAGS)

src_libmo_la_LIBADD = $(GLIB_LIBS)

src_libmo_la_LDFLAGS = -export-symbols-regex '^mo_' \
                       -version-info 0:0:0

pkginclude_HEADERS = src/mo.h

# Example program

noinst_PROGRAMS = example/sample-query

example_sample_query_SOURCES = example/sample-query.c
example_sample_query_CFLAGS = -I$(top_srcdir)/src \
                              $(GLIB_CFLAGS) \
                              $(AM_CFLAGS)

example_sample_query_LDADD = $(GLIB_LIBS) \
                             $(top_builddir)/src/libmo.la
