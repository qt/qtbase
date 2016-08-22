# The library is expected to be the same as in ES 2.0 (libGLESv2).
# The difference is the header and the presence of the functions in
# the library.

SOURCES = opengles3.cpp

CONFIG -= qt

mac {
    DEFINES += BUILD_ON_MAC
}
