AC_INIT(atque, 1.1.2, [http://sourceforge.net/tracker/?func=add&group_id=235817&atid=1097912] )

AC_PREREQ([2.59])
AC_CONFIG_AUX_DIR(config)
AM_INIT_AUTOMAKE([foreign])

AC_CONFIG_HEADER(config.h)

AC_ARG_ENABLE(gui, [ --enable-gui	build atque GUI [default=yes]], , enable_gui=yes)

AC_LANG(C++)

AC_PROG_CC
AC_PROG_CPP
AC_PROG_CXX
AC_PROG_CXXCPP
AC_PROG_INSTALL
AC_PROG_LIBTOOL

case "$host" in
*-mingw32*)
	AC_CHECK_TOOL(WINDRES, windres, :)
	make_windows=true
	LDFLAGS="$LDFLAGS -Wl,-S"
	;;
*)
	make_windows=false
	;;
esac

AC_CHECK_HEADERS([sndfile.h], , AC_ERROR([Atque requires libsndfile]))
AC_CHECK_LIB(sndfile, sf_open, LIBS="-lsndfile $LIBS", AC_ERROR([Atque requires libsndfile]))

AX_BOOST_BASE([1.33.1])

if [[ "x$enable_gui" = "xyes" ]]; then
AM_OPTIONS_WXCONFIG  
reqwx=3.0.0
AM_PATH_WXCONFIG($reqwx, wxWin=1)
fi

if test "$wxWin" != 1; then
   AC_MSG_WARN([wxWidgets not enabled, building command line tools only])
else
   CPPFLAGS="$CPPFLAGS $WX_CPPFLAGS"
   CXXFLAGS="$CXXFLAGS $WX_CXXFLAGS_ONLY"
   CFLAGS="$CFLAGS $WX_CFLAGS_ONLY"
   LIBS="$LIBS $WX_LIBS"
fi
AM_CONDITIONAL(BUILD_ATQUEGUI, [test "$wxWin" = 1 ])

AM_CONDITIONAL(MAKE_WINDOWS, test x$make_windows = xtrue)

AC_CONFIG_FILES([Makefile ferro/Makefile])
AC_OUTPUT

echo "Configuration done. Now type \"make\"."

