TEMPLATE = app
INCLUDEPATH += .

HEADERS     = bookdelegate.h bookwindow.h initdb.h
RESOURCES   = books.qrc
SOURCES     = bookdelegate.cpp main.cpp bookwindow.cpp
FORMS       = bookwindow.ui

QT += sql

target.path = $$[QT_INSTALL_DEMOS]/qtbase/books
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS *.pro images
sources.path = $$[QT_INSTALL_DEMOS]/qtbase/books
INSTALLS += target sources

symbian: include($$QT_SOURCE_TREE/demos/symbianpkgrules.pri)

wince*: {
    CONFIG(debug, debug|release):sqlPlugins.files = $$QT_BUILD_TREE/plugins/sqldrivers/*d4.dll
    CONFIG(release, debug|release):sqlPlugins.files = $$QT_BUILD_TREE/plugins/sqldrivers/*[^d]4.dll
    sqlPlugins.path = sqldrivers
    DEPLOYMENT += sqlPlugins
}
