SOURCES += main.cpp

QWERTY_BUNDLE.version = Bogus.78
QWERTY_BUNDLE.files = some-file "existing file" "non-existing file"
QWERTY_BUNDLE.path = QwertyData

QMAKE_BUNDLE_DATA = QWERTY_BUNDLE
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
