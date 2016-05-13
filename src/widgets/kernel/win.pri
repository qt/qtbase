# Qt/Windows only configuration file
# --------------------------------------------------------------------

INCLUDEPATH += ../3rdparty/wintab
!winrt: LIBS_PRIVATE *= -lshell32 -luxtheme -ldwmapi
# Override MinGW's definition in _mingw.h
mingw: DEFINES += WINVER=0x600 _WIN32_WINNT=0x0600
