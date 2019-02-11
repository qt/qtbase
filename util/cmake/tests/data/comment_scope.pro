# QtCore can't be compiled with -Wl,-no-undefined because it uses the "environ"
# variable and on FreeBSD and OpenBSD, this variable is in the final executable itself.
# OpenBSD 6.0 will include environ in libc.
freebsd|openbsd: QMAKE_LFLAGS_NOUNDEF =

include(animation/animation.pri)
