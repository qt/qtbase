# Generated from app.pro.

#####################################################################
## plugandpaint Binary:
#####################################################################

add_qt_executable(plugandpaint
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/tools/plugandpaint"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/tools/plugandpaint"
    SOURCES
        interfaces.h
        main.cpp
        mainwindow.cpp mainwindow.h
        paintarea.cpp paintarea.h
        plugindialog.cpp plugindialog.h
    LIBRARIES
        Qt::Widgets
        pnp_basictools # special case
)

#### Keys ignored in scope 1:.:app.pro:<NONE>:
# CONFIG = "install_ok"
# DESTDIR = ".."

## Scopes:
#####################################################################

# special case: Remove the rest
