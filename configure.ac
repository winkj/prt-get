AC_INIT([prt-get], [5.19.1])
AC_CONFIG_SRCDIR([src/prtget.cpp])
AM_INIT_AUTOMAKE

AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL

AC_PREFIX_DEFAULT(/usr)

CFLAGS="$CFLAGS $X_CFLAGS"
CXXFLAGS="$CXXFLAGS $X_CFLAGS"
LIBS="$LIBS $X_LIBS"
LDFLAGS="$LDFLAGS $LIBS $X_PRE_LIBS"

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST

# I hate you autotools
sysconfdir="/etc"
localstatedir="/var"

# Checks for library functions.
AM_CONFIG_HEADER(config.h)
AC_CONFIG_FILES([Makefile
	src/Makefile
	doc/Makefile
	misc/Makefile])

AC_OUTPUT
