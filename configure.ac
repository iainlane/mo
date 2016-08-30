AC_INIT([mo], [0.1])

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
