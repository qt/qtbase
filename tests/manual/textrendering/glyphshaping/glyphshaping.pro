QT += widgets
SOURCES = main.cpp
OTHER_FILES = glyphshaping_data.xml
glyphshaping_data.path = .
glyphshaping_data.files = $$PWD/glyphshaping_data.xml
DEPLOYMENT += glyphshaping_data
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
