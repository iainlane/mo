m4_define([mo_version_major],[0])
m4_define([mo_version_minor],[1])
m4_define([mo_version_micro],[0])

AC_INIT([mo], [mo_version_major.mo_version_minor.mo_version_micro])

AM_INIT_AUTOMAKE([foreign no-dist-gzip dist-xz subdir-objects])
AM_SILENT_RULES([yes])
AC_CONFIG_MACRO_DIR([m4])

# Required version
GLIB_REQUIRED_VERSION=2.43.4

LT_INIT

AC_PROG_CC

AX_CFLAGS_WARN_ALL
AX_APPEND_COMPILE_FLAGS([-std=c99 \
                         -Wall \
                         -Wextra \
                         -pedantic \
                         -Werror=implicit-function-declaration \
                         -Wshadow \
                         -Wstrict-overflow \
                         -fno-strict-aliasing],
                         [CFLAGS])

AC_ARG_ENABLE([werror],
              AS_HELP_STRING([--enable-werror], [Turn on -Werror and some other
                              compile flags that are useful during development
                              but annoying for release builds.
]))

AS_IF([test "x$enable_werror" = "xyes"], [
# format-extra-args seems messed up on gcc with g_set_error
       AX_APPEND_COMPILE_FLAGS([-Werror -Wno-error=format-extra-args])
])

PKG_CHECK_MODULES(GLIB, gobject-2.0 >= $GLIB_REQUIRED_VERSION \
                        glib-2.0 >= $GLIB_REQUIRED_VERSION \
                        gio-2.0 >= $GLIB_REQUIRED_VERSION)

# Before making a release, the MO_LT_VERSION string should be modified. The string is of the form c:r:a. Follow these instructions sequentially:
#
#  1. If the library source code has changed at all since the last update, then increment revision (‘c:r:a’ becomes ‘c:r+1:a’).
#  2. If any interfaces have been added, removed, or changed since the last update, increment current, and set revision to 0.
#  3. If any interfaces have been added since the last public release, then increment age.
#  4. If any interfaces have been removed or changed since the last public release, then set age to 0.
MO_LT_VERSION=0:0:0
AC_SUBST([MO_LT_VERSION])

GTK_DOC_CHECK([1.14],[--flavour no-tmpl])

MO_VERSION_MAJOR=mo_version_major
MO_VERSION_MINOR=mo_version_minor
MO_VERSION_MICRO=mo_version_micro

AC_SUBST([MO_VERSION_MAJOR])
AC_SUBST([MO_VERSION_MINOR])
AC_SUBST([MO_VERSION_MICRO])

AC_CONFIG_HEADERS(config.h)
AC_CONFIG_FILES([
    Makefile
    docs/Makefile
    docs/reference/Makefile
    docs/reference/libmo/Makefile
])

AC_ENABLE_SHARED
AC_DISABLE_STATIC

#GETTEXT_PACKAGE=mo
#AC_SUBST(GETTEXT_PACKAGE)
#AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE",
#                                      [the gettext translation domain])

#AM_GNU_GETTEXT
#GLIB_DEFINE_LOCALEDIR(GNOMELOCALEDIR)

AC_OUTPUT
