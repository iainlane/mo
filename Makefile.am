SUBDIRS = . docs

ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_AMFLAGS}
CLEANFILES =
DISTCLEANFILES =
DISTCHECK_CONFIGURE_FLAGS = --disable-silent-rules
EXTRA_DIST =
MAINTAINERCLEANFILES =

libmo_sources = libmo/mofile.c \
                libmo/mogroup.c
libmo_public_headers = libmo/mo.h \
                       libmo/mofile.h \
                       libmo/mogroup.h

lib_LTLIBRARIES = libmo/libmo.la

libmo_libmo_la_SOURCES = $(libmo_sources)

libmo_libmo_la_CPPFLAGS = -DMO_COMPILATION \
                          $(AM_CPPFLAGS)

libmo_libmo_la_CFLAGS = $(GLIB_CFLAGS) \
                        $(WARN_CFLAGS) \
                        $(AM_CFLAGS)

libmo_libmo_la_LIBADD = $(GLIB_LIBS) \
                        $(AM_LIBADD)

libmo_libmo_la_LDFLAGS = -Wl,--version-script=libmo/mo.map \
                       -version-info $(MO_LT_VERSION) \
                       $(WARN_LDFLAGS) \
                       $(AM_LDFLAGS)

libmoincdir = $(includedir)/libmo
libmoinc_HEADERS = $(libmo_public_headers)

# Example program

noinst_PROGRAMS = example/sample-query

example_sample_query_SOURCES = example/sample-query.c
example_sample_query_CFLAGS = -I$(top_srcdir) \
                              $(GLIB_CFLAGS) \
                              $(WARN_CFLAGS) \
                              $(AM_CFLAGS)

example_sample_query_LDADD = $(GLIB_LIBS) \
                             $(top_builddir)/libmo/libmo.la

example_sample_query_LDFLAGS = $(WARN_LDFLAGS) \
                               $(AM_LDFLAGS)

# introspection
-include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_GIRS =
INTROSPECTION_SCANNER_ARGS = --warn-all
INTROSPECTION_COMPILER_ARGS =
DISTCHECK_CONFIGURE_FLAGS += --enable-introspection
EXTRA_DIST += m4/introspection.m4

if HAVE_INTROSPECTION
libmo/Mo-@MO_API_VERSION_MAJOR@.@MO_API_VERSION_MINOR@.gir: libmo/libmo.la
libmo_Mo_@MO_API_VERSION_MAJOR@_@MO_API_VERSION_MINOR@_gir_INCLUDES = GObject-2.0 GLib-2.0
libmo_Mo_@MO_API_VERSION_MAJOR@_@MO_API_VERSION_MINOR@_gir_CFLAGS = $(MO_CFLAGS) $(GLIB_CFLAGS) $(gdata_libgdata_la_CPPFLAGS)
libmo_Mo_@MO_API_VERSION_MAJOR@_@MO_API_VERSION_MINOR@_gir_LIBS = libmo/libmo.la
libmo_Mo_@MO_API_VERSION_MAJOR@_@MO_API_VERSION_MINOR@_gir_FILES = $(libmo_sources) $(libmo_public_headers)
libmo_Mo_@MO_API_VERSION_MAJOR@_@MO_API_VERSION_MINOR@_gir_NAMESPACE = Mo
libmo_Mo_@MO_API_VERSION_MAJOR@_@MO_API_VERSION_MINOR@_gir_EXPORT_PACKAGES = libmo
libmo_Mo_@MO_API_VERSION_MAJOR@_@MO_API_VERSION_MINOR@_gir_SCANNERFLAGS = \
        --nsversion=$(MO_API_VERSION) \
        --symbol-prefix=mo \
        --c-include="libmo/mo.h" \
        $(WARN_SCANNERFLAGS) \
        -DMO_COMPILATION \
        $(NULL)

INTROSPECTION_GIRS += libmo/Mo-$(MO_API_VERSION).gir

girdir = $(datadir)/gir-1.0
gir_DATA = $(INTROSPECTION_GIRS)

typelibdir = $(libdir)/girepository-1.0
typelib_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

CLEANFILES += $(gir_DATA) $(typelib_DATA)
endif

# gtk-doc
EXTRA_DIST += gtk-doc.make

MAINTAINERCLEANFILES += gtk-doc.make

DISTCHECK_CONFIGURE_FLAGS += --enable-gtk-doc
