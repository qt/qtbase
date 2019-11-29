# Qt/Windows only configuration file
# --------------------------------------------------------------------

!winrt {
    LIBS_PRIVATE *= -luxtheme -ldwmapi
    QMAKE_USE_PRIVATE += shell32
}
