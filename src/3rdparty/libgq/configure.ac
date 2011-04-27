AC_INIT([libgq],[0.2],[marius.vollmer@nokia.com],[libgq])
AC_CONFIG_SRCDIR([Makefile.am])
AM_INIT_AUTOMAKE([-Wall -Werror tar-ustar foreign])

AC_PROG_CXX
AC_PROG_LIBTOOL

PKG_CHECK_MODULES([QT],    [QtCore])
PKG_CHECK_MODULES([QTEST], [QtTest])
PKG_CHECK_MODULES([GLIB],  [glib-2.0])
PKG_CHECK_MODULES([GCONF], [gconf-2.0])

MOC=`pkg-config QtCore --variable=moc_location`
RCC=`pkg-config QtCore --variable=exec_prefix`/bin/rcc

AC_SUBST([MOC])
AC_SUBST([RCC])

AC_CONFIG_FILES([Makefile
                 gconf/Makefile
                 gconf/gq-gconf.pc])

AC_OUTPUT
