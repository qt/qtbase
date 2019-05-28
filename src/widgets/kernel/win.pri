# Qt/Windows only configuration file
# --------------------------------------------------------------------

INCLUDEPATH += ../3rdparty/wintab
!winrt {
    LIBS_PRIVATE *= -luxtheme -ldwmapi
    QMAKE_USE_PRIVATE += shell32
}
