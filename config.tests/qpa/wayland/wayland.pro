SOURCES = wayland.cpp
CONFIG -= qt

for(d, QMAKE_INCDIR_WAYLAND) {
    exists($$d):INCLUDEPATH += $$d
}

for(p, QMAKE_LIBDIR_WAYLAND) {
    exists($$p):LIBS += -L$$p
}

LIBS += $$QMAKE_LIBS_WAYLAND
