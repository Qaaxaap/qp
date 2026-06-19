AC_INIT([qp], [0.2.4], [github.com/Qaaxaap/qp/issues])

AC_CONFIG_SRCDIR([src/main.cpp])
AM_INIT_AUTOMAKE([foreign subdir-objects])

AC_PROG_CC
AC_PROG_CXX

AC_CONFIG_HEADERS([config.h])

AC_CHECK_HEADER([libintl.h], [], [AC_MSG_ERROR([libintl.h not found; install gettext])])
AC_CHECK_FUNC([gettext], [], [AC_MSG_ERROR([gettext() not found])])

AC_ARG_WITH([lfspkg],
    [AS_HELP_STRING([--with-lfspkg=DIR],
        [path to lfspkg source tree (default: ../lfspkg)])],
    [LFSPKG_DIR="$withval"],
    [LFSPKG_DIR="../lfspkg"])

LFSPKG_CPPFLAGS="-I$LFSPKG_DIR/include"
LFSPKG_LIBS="$LFSPKG_DIR/src/liblfspkg.a"

AC_LANG([C++])
AC_MSG_CHECKING([for lfspkg headers])
save_CXXFLAGS="$CXXFLAGS"
CXXFLAGS="-std=c++17 $LFSPKG_CPPFLAGS"
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <lfspkg/db.hpp>]], [[]])],
    [AC_MSG_RESULT([yes])],
    [AC_MSG_ERROR([lfspkg headers not found in $LFSPKG_DIR/include])])
CXXFLAGS="$save_CXXFLAGS"

AC_SUBST([LFSPKG_CPPFLAGS])
AC_SUBST([LFSPKG_LIBS])

AC_CONFIG_FILES([
    Makefile
    po/Makefile
    src/Makefile
    tests/Makefile
])

AC_OUTPUT
