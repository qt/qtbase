TEMPLATE = app
SOURCES = raycasting.cpp
RESOURCES += raycasting.qrc

symbian {
    TARGET.UID3 = 0xA000CF76
    CONFIG += qt_demo
}

target.path = $$[QT_INSTALL_DEMOS]/qtbase/embedded/raycasting
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS *.pro
sources.path = $$[QT_INSTALL_DEMOS]/qtbase/embedded/raycasting
INSTALLS += target sources
