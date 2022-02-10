# Generated from qpainter.pro.

#####################################################################
## tst_qpainter Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    drawEllipse/*)
list(APPEND test_data ${test_data_glob})
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    drawLine_rop_bitmap/*)
list(APPEND test_data ${test_data_glob})
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    drawPixmap_rop/*)
list(APPEND test_data ${test_data_glob})
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    drawPixmap_rop_bitmap/*)
list(APPEND test_data ${test_data_glob})
list(APPEND test_data "task217400.png")

qt_internal_add_test(tst_qpainter
    SOURCES
        tst_qpainter.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
    TESTDATA ${test_data}
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qpainter CONDITION TARGET Qt::Widgets
    PUBLIC_LIBRARIES
        Qt::Widgets
        Qt::WidgetsPrivate
)
