# Generated from qimageconversion.pro.

#####################################################################
## tst_bench_imageConversion Binary:
#####################################################################

add_qt_benchmark(tst_bench_imageConversion
    SOURCES
        tst_qimageconversion.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Test
)

#### Keys ignored in scope 1:.:.:qimageconversion.pro:<TRUE>:
# QT_FOR_CONFIG = "gui-private"
# TEMPLATE = "app"

## Scopes:
#####################################################################

extend_target(tst_bench_imageConversion CONDITION QT_FEATURE_gif
    DEFINES
        QTEST_HAVE_GIF
)

extend_target(tst_bench_imageConversion CONDITION QT_FEATURE_jpeg
    DEFINES
        QTEST_HAVE_JPEG
)
