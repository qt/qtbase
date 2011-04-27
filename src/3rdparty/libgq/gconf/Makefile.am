AM_CXXFLAGS = $(QT_CFLAGS) $(GLIB_CFLAGS) $(GCONF_CFLAGS)
LIBS += $(QT_LIBS) $(GLIB_LIBS) $(GCONF_LIBS)

lib_LTLIBRARIES = libgq-gconf.la

libgq_gconf_la_SOURCES = GConfItem \
                         gconfitem.h \
                         gconfitem.cpp \
                         gconfitem_moc.cpp

gqincludedir=$(includedir)/gq
gqinclude_HEADERS = gconfitem.h GConfItem

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = gq-gconf.pc

# Tests

check_PROGRAMS = test-gconf

test_gconf_SOURCES = test-gconf.h test-gconf.cpp test-gconf_moc.cpp
test_gconf_CFLAGS = $(QTEST_CFLAGS)
test_gconf_LDADD = libgq-gconf.la $(QTEST_LIBS)

TESTS = run-test-gconf

EXTRA_DIST = run-test-gconf

# Moccing

%_moc.cpp: %.h
	$(MOC) -o "$@" "$<"

clean-moc:
	rm -f *_moc.cpp *_moc.lo

clean-local: clean-moc
