QT += widgets core-private gui-private
SOURCES += main.cpp
CONFIG -= app_bundle
darwin {
    QMAKE_CXXFLAGS += -x objective-c++
    LIBS += -framework Foundation -framework CoreGraphics -framework AppKit
}
