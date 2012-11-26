TEMPLATE = app
SOURCES = main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/scroller/graphicsview
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS graphicsview.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/widgets/scroller/graphicsview
INSTALLS += target sources
QT += widgets
