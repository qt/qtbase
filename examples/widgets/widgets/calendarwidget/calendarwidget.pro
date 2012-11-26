HEADERS =   window.h
SOURCES =   main.cpp \ 
	    window.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/widgets/calendarwidget
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS calendarwidget.pro resources
sources.path = $$[QT_INSTALL_EXAMPLES]/widgets/widgets/calendarwidget
INSTALLS += target sources

QT += widgets
