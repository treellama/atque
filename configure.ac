AC_INIT(atque, 0.1)

AC_PREREQ([2.59])
AC_CONFIG_AUX_DIR(config)
AM_INIT_AUTOMAKE([foreign])

AC_CONFIG_HEADER(config.h)

AC_LANG(C++)


AC_PROG_CC
AC_PROG_CPP
AC_PROG_CXX
AC_PROG_CXXCPP
AC_PROG_INSTALL
AC_PROG_LIBTOOL

dnl Check for ferro!
AC_CHECK_HEADERS(ferro/Wadfile.h, , AC_ERROR([You must first install ferro]))
AC_CHECK_LIB(ferro, main, LIBS="-lferro $LIBS", AC_ERROR([You must first install ferro]))

AX_BOOST_BASE([1.33.1])
AX_BOOST_FILESYSTEM

AC_CONFIG_FILES([Makefile])
AC_OUTPUT

echo "Configuration done. Now type \"make\"."

