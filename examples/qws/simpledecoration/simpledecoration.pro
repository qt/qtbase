TEMPLATE  = app
HEADERS   = analogclock.h \
            mydecoration.h
SOURCES   = analogclock.cpp \
            main.cpp \
            mydecoration.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/qws/simpledecoration
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/qws/simpledecoration
INSTALLS += target sources
