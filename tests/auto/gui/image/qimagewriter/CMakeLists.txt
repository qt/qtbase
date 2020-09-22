# Generated from qimagewriter.pro.

#####################################################################
## tst_qimagewriter Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    images/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qimagewriter
    SOURCES
        tst_qimagewriter.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
    TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:qimagewriter.pro:<TRUE>:
# MOC_DIR = "tmp"

## Scopes:
#####################################################################

if(ANDROID AND NOT ANDROID_EMBEDDED)
    # Resources:
    set(qimagewriter_resource_files
        "images/YCbCr_cmyk.jpg"
        "images/YCbCr_rgb.jpg"
        "images/beavis.jpg"
        "images/colorful.bmp"
        "images/earth.gif"
        "images/font.bmp"
        "images/gnus.xbm"
        "images/kollada.png"
        "images/marble.xpm"
        "images/ship63.pbm"
        "images/teapot.ppm"
        "images/trolltech.gif"
    )

    qt_internal_add_resource(tst_qimagewriter "qimagewriter"
        PREFIX
            "/"
        FILES
            ${qimagewriter_resource_files}
    )
endif()
