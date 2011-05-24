HEADERS       = mainwindow.h \
                stylesheeteditor.h
FORMS         = mainwindow.ui \
                stylesheeteditor.ui
RESOURCES     = stylesheet.qrc
SOURCES       = main.cpp \
                mainwindow.cpp \
                stylesheeteditor.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/stylesheet
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS *.pro images layouts qss
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/stylesheet
INSTALLS += target sources

symbian: CONFIG += qt_example
maemo5: CONFIG += qt_example

symbian: warning(This example might not fully work on Symbian platform)
maemo5: warning(This example might not fully work on Maemo platform)
simulator: warning(This example might not fully work on Simulator platform)
