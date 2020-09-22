# Generated from qmovie.pro.

#####################################################################
## tst_qmovie Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    animations/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qmovie
    SOURCES
        tst_qmovie.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
    TESTDATA ${test_data}
)

# Resources:
set(resources_resource_files
    "animations/comicsecard.gif"
    "animations/corrupt.gif"
    "animations/trolltech.gif"
)

qt_internal_add_resource(tst_qmovie "resources"
    PREFIX
        "/"
    FILES
        ${resources_resource_files}
)


#### Keys ignored in scope 1:.:.:qmovie.pro:<TRUE>:
# MOC_DIR = "tmp"
# QT_FOR_CONFIG = "gui-private"

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qmovie CONDITION TARGET Qt::Widgets
    PUBLIC_LIBRARIES
        Qt::Widgets
)

qt_internal_extend_target(tst_qmovie CONDITION QT_FEATURE_gif
    DEFINES
        QTEST_HAVE_GIF
)

qt_internal_extend_target(tst_qmovie CONDITION QT_FEATURE_jpeg
    DEFINES
        QTEST_HAVE_JPEG
)
