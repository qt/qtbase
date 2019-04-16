TEMPLATE = aux

# Needs explicit load()ing due to aux template. Relies on QT being non-empty.
load(qt)

goodlib.target = good.$${QMAKE_APPLE_DEVICE_ARCHS}.dylib
goodlib.commands = $(CXX) $(CXXFLAGS) -shared -o $@ -I$(INCPATH) $<
goodlib.depends += $$PWD/../fakeplugin.cpp

all.depends += goodlib

QMAKE_EXTRA_TARGETS += goodlib all
QMAKE_CLEAN += $$goodlib.target
