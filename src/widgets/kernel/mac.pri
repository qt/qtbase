!x11:osx {
   LIBS_PRIVATE += -framework Carbon -framework AppKit -lz
   *-mwerks:INCLUDEPATH += compat
}
