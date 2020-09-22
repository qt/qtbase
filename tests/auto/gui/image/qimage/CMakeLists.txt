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

## Scopes:
#####################################################################

if(ANDROID AND NOT ANDROID_EMBEDDED)
    # Resources:
    set(qimage_resource_files
        "images/image.bmp"
        "images/image.gif"
        "images/image.ico"
        "images/image.jpg"
        "images/image.pbm"
        "images/image.pgm"
        "images/image.png"
        "images/image.ppm"
        "images/image.xbm"
        "images/image.xpm"
        "images/jpeg_exif_orientation_value_1.jpg"
        "images/jpeg_exif_orientation_value_2.jpg"
        "images/jpeg_exif_orientation_value_3.jpg"
        "images/jpeg_exif_orientation_value_4.jpg"
        "images/jpeg_exif_orientation_value_5.jpg"
        "images/jpeg_exif_orientation_value_6.jpg"
        "images/jpeg_exif_orientation_value_7.jpg"
        "images/jpeg_exif_orientation_value_8.jpg"
    )

    qt_internal_add_resource(tst_qimage "qimage"
        PREFIX
            "/"
        FILES
            ${qimage_resource_files}
    )
endif()

qt_internal_extend_target(tst_qimage CONDITION WIN32
    PUBLIC_LIBRARIES
        gdi32
        user32
)

qt_internal_extend_target(tst_qimage CONDITION APPLE
    PUBLIC_LIBRARIES
        ${FWCoreGraphics}
)
