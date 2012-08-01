TEMPLATE = app
SOURCES  = main.cpp

extratarget.commands = @echo extra target worked OK
QMAKE_EXTRA_TARGETS += extratarget
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
