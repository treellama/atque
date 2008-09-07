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

AC_CHECK_HEADERS([sndfile.h], , AC_ERROR([Atque requires libsndfile]))
AC_CHECK_LIB(sndfile, sf_open, LIBS="-lsndfile $LIBS", AC_ERROR([Atque requires libsndfile]))

AX_BOOST_BASE([1.33.1])
AX_BOOST_FILESYSTEM

AM_OPTIONS_WXCONFIG
reqwx=2.8.0
AM_PATH_WXCONFIG($reqwx, wxWin=1)
if test "$wxWin" != 1; then
   AC_MSG_ERROR([Atque requires wxWidgets])
fi

CPPFLAGS="$CPPFLAGS $WX_CPPFLAGS"
CXXFLAGS="$CXXFLAGS $WX_CXXFLAGS_ONLY"
CFLAGS="$CFLAGS $WX_CFLAGS_ONLY"
LIBS="$LIBS $WX_LIBS"

AC_CONFIG_FILES([Makefile])
AC_OUTPUT

echo "Configuration done. Now type \"make\"."

