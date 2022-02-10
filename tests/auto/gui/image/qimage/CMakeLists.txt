# Generated from qimage.pro.

#####################################################################
## tst_qimage Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    images/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qimage
    SOURCES
        tst_qimage.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
    TESTDATA ${test_data}
)

qt_internal_extend_target(tst_qimage CONDITION WIN32
    PUBLIC_LIBRARIES
        gdi32
        user32
)

qt_internal_extend_target(tst_qimage CONDITION APPLE
    PUBLIC_LIBRARIES
        ${FWCoreGraphics}
)
