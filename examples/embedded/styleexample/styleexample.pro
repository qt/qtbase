QT += widgets

HEADERS += stylewidget.h
FORMS += stylewidget.ui
SOURCES += main.cpp stylewidget.cpp
RESOURCES += styleexample.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/embedded/styleexample
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS *.pro *.html
sources.path = $$[QT_INSTALL_EXAMPLES]/embedded/styleexample
INSTALLS += target sources
