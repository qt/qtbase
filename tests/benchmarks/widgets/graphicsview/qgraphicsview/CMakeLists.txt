# Generated from qgraphicsview.pro.

#####################################################################
## tst_bench_qgraphicsview Binary:
#####################################################################

add_qt_benchmark(tst_bench_qgraphicsview
    SOURCES
        chiptester/chip.cpp chiptester/chip.h
        chiptester/chiptester.cpp chiptester/chiptester.h
        tst_qgraphicsview.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Test
        Qt::Widgets
)

# Resources:
set(qgraphicsview_resource_files
    "images/designer.png"
    "images/wine-big.jpeg"
    "images/wine.jpeg"
    "random.data"
)

add_qt_resource(tst_bench_qgraphicsview "qgraphicsview"
    PREFIX
        "/"
    FILES
        ${qgraphicsview_resource_files}
)
set(images_resource_files
    "qt4logo.png"
)

add_qt_resource(tst_bench_qgraphicsview "images"
    PREFIX
        "/"
    BASE
        "chiptester"
    FILES
        ${images_resource_files}
)


#### Keys ignored in scope 1:.:.:qgraphicsview.pro:<TRUE>:
# TEMPLATE = "app"

## Scopes:
#####################################################################

extend_target(tst_bench_qgraphicsview CONDITION TARGET Qt::OpenGL
    PUBLIC_LIBRARIES
        Qt::OpenGL
)
