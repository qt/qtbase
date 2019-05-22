# Generated from sqlbrowser.pro.

#####################################################################
## sqlbrowser Binary:
#####################################################################

add_qt_executable(sqlbrowser
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/sql/sqlbrowser"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/sql/sqlbrowser"
    SOURCES
        browser.cpp browser.h
        browserwidget.ui
        connectionwidget.cpp connectionwidget.h
        main.cpp
        qsqlconnectiondialog.cpp qsqlconnectiondialog.h qsqlconnectiondialog.ui
    PUBLIC_LIBRARIES
        Qt::Sql
        Qt::Widgets
    ENABLE_AUTOGEN_TOOLS
        uic
)

#### Keys ignored in scope 1:.:.:sqlbrowser.pro:<TRUE>:
# TEMPLATE = "app"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:sqlbrowser.pro:build_all AND NOT build_pass:
# CONFIG = "-build_all" "release"
