CONFIG += testcase console
debug_and_release {
    CONFIG(debug, debug|release) {
        TARGET = ../../debug/tst_qobject
    } else {
        TARGET = ../../release/tst_qobject
    }
} else {
    TARGET = ../tst_qobject
}

QT = core-private network testlib
SOURCES = ../tst_qobject.cpp

# Force C++17 if available (needed due to P0012R1)
contains(QT_CONFIG, c++1z): CONFIG += c++1z

!winrt {
    debug_and_release {
        CONFIG(debug, debug|release) {
            TEST_HELPER_INSTALLS = ../debug/signalbug_helper
        } else {
            TEST_HELPER_INSTALLS = ../release/signalbug_helper
        }
    } else {
        TEST_HELPER_INSTALLS = ../signalbug_helper
    }
}
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
