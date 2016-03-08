# Qt/Windows only configuration file
# --------------------------------------------------------------------

INCLUDEPATH += ../3rdparty/wintab
!winrt: LIBS_PRIVATE *= -lshell32
