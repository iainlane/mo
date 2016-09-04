SUBDIRS = . docs

ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_AMFLAGS}
CLEANFILES =
DISTCLEANFILES =
DISTCHECK_CONFIGURE_FLAGS = --disable-silent-rules
EXTRA_DIST =
MAINTAINERCLEANFILES =

libmo_sources = src/mo.c
libmo_public_headers = src/mo.h

lib_LTLIBRARIES = src/libmo.la

src_libmo_la_SOURCES = $(libmo_sources)

src_libmo_la_CFLAGS = $(GLIB_CFLAGS)

src_libmo_la_LIBADD = $(GLIB_LIBS)

src_libmo_la_LDFLAGS = -export-symbols-regex '^mo_' \
                       -version-info $(MO_LT_VERSION)

pkginclude_HEADERS = $(libmo_public_headers)

# Example program

noinst_PROGRAMS = example/sample-query

example_sample_query_SOURCES = example/sample-query.c
example_sample_query_CFLAGS = -I$(top_srcdir)/src \
                              $(GLIB_CFLAGS) \
                              $(AM_CFLAGS)

example_sample_query_LDADD = $(GLIB_LIBS) \
                             $(top_builddir)/src/libmo.la

# introspection
-include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_GIRS =
INTROSPECTION_SCANNER_ARGS = --warn-all
INTROSPECTION_COMPILER_ARGS =
DISTCHECK_CONFIGURE_FLAGS += --enable-introspection
EXTRA_DIST += m4/introspection.m4

if HAVE_INTROSPECTION
src/Mo-@MO_API_VERSION_MAJOR@.@MO_API_VERSION_MINOR@.gir: src/libmo.la
src_Mo_@MO_API_VERSION_MAJOR@_@MO_API_VERSION_MINOR@_gir_INCLUDES = GObject-2.0 GLib-2.0
src_Mo_@MO_API_VERSION_MAJOR@_@MO_API_VERSION_MINOR@_gir_CFLAGS = $(MO_CFLAGS) $(GLIB_CFLAGS) $(gdata_libgdata_la_CPPFLAGS)
src_Mo_@MO_API_VERSION_MAJOR@_@MO_API_VERSION_MINOR@_gir_LIBS = src/libmo.la
src_Mo_@MO_API_VERSION_MAJOR@_@MO_API_VERSION_MINOR@_gir_FILES = $(libmo_sources) $(libmo_public_headers)
src_Mo_@MO_API_VERSION_MAJOR@_@MO_API_VERSION_MINOR@_gir_NAMESPACE = Mo
src_Mo_@MO_API_VERSION_MAJOR@_@MO_API_VERSION_MINOR@_gir_EXPORT_PACKAGES = libmo
src_Mo_@MO_API_VERSION_MAJOR@_@MO_API_VERSION_MINOR@_gir_SCANNERFLAGS = \
        --nsversion=$(MO_API_VERSION) \
        --symbol-prefix=mo \
        --c-include="src/mo.h" \
        $(WARN_SCANNERFLAGS) \
        $(NULL)

INTROSPECTION_GIRS += src/Mo-$(MO_API_VERSION).gir

girdir = $(datadir)/gir-1.0
gir_DATA = $(INTROSPECTION_GIRS)

typelibdir = $(libdir)/girepository-1.0
typelib_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

CLEANFILES += $(gir_DATA) $(typelib_DATA)
endif

# gtk-doc
EXTRA_DIST += gtk-doc.make

DISTCLEANFILES += gtk-doc.make

MAINTAINERCLEANFILES += gtk-doc.make

DISTCHECK_CONFIGURE_FLAGS += --enable-gtk-doc
