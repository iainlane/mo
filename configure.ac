AC_INIT([mo], [0.1])

AM_INIT_AUTOMAKE([foreign no-dist-gzip dist-xz subdir-objects])
AM_SILENT_RULES([yes])
AC_CONFIG_MACRO_DIR([m4])

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
       AX_APPEND_COMPILE_FLAGS([-Werror])
])

PKG_CHECK_MODULES(GLIB, gobject-2.0 glib-2.0 gio-2.0)

AC_CONFIG_HEADERS(config.h)
AC_CONFIG_FILES([
    Makefile
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
