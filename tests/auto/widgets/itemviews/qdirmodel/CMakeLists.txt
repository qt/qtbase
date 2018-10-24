add_qt_test("tst_qdirmodel"
    SOURCES
        ../../../../shared/emulationdetector.h
        tst_qdirmodel.cpp
    INCLUDE_DIRECTORIES
        ../../../../shared/
    LIBRARIES
        Qt::Widgets
)

extend_target("tst_qdirmodel" CONDITION ANDROID DEFINES SRCDIR="./")

extend_target("tst_qdirmodel" CONDITION NOT ANDROID
    DEFINES SRCDIR="${CMAKE_CURRENT_SOURCE_DIR}/"
)

extend_target("tst_qdirmodel" CONDITION ANDROID AND NOT ANDROID_EMBEDDED
    SOURCES testdata.qrc
)
