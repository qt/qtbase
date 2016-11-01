HEADERS += $$PWD/qcoretextfontdatabase_p.h $$PWD/qfontengine_coretext_p.h
OBJECTIVE_SOURCES += $$PWD/qfontengine_coretext.mm $$PWD/qcoretextfontdatabase.mm

qtConfig(freetype) {
    QMAKE_USE += freetype
    HEADERS += $$QT_SOURCE_TREE/src/gui/text/qfontengine_ft_p.h
    SOURCES += $$QT_SOURCE_TREE/src/gui/text/qfontengine_ft.cpp
}

uikit: \
    # On iOS/tvOS/watchOS CoreText and CoreGraphics are stand-alone frameworks
    LIBS_PRIVATE += -framework CoreText -framework CoreGraphics
else: \
    # On Mac OS they are part of the ApplicationServices umbrella framework,
    # even in 10.8 where they were also made available stand-alone.
    LIBS_PRIVATE += -framework ApplicationServices

# CoreText is documented to be available on watchOS, but the headers aren't present
# in the watchOS Simulator SDK like they are supposed to be. Work around the problem
# by adding the device SDK's headers to the search path as a fallback.
# rdar://25314492, rdar://27844864
watchos:CONFIG(simulator, simulator|device) {
    QMAKE_CXXFLAGS += \
        -F$$xcodeSDKInfo(Path, $${simulator.sdk})/System/Library/Frameworks \
        -F$$xcodeSDKInfo(Path, $${device.sdk})/System/Library/Frameworks
}
