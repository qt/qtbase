# Generated from qimagereader.pro.

#####################################################################
## tst_bench_qimagereader Binary:
#####################################################################

add_qt_benchmark(tst_bench_qimagereader
    SOURCES
        tst_qimagereader.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Test
)

#### Keys ignored in scope 1:.:.:qimagereader.pro:<TRUE>:
# QT_FOR_CONFIG = "gui-private"
# TEMPLATE = "app"
# TESTDATA = "images/*"

## Scopes:
#####################################################################

extend_target(tst_bench_qimagereader CONDITION QT_FEATURE_gif
    DEFINES
        QTEST_HAVE_GIF
)

extend_target(tst_bench_qimagereader CONDITION QT_FEATURE_jpeg
    DEFINES
        QTEST_HAVE_JPEG
)
