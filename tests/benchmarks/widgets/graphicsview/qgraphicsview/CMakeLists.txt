# Generated from qgraphicsview.pro.

#####################################################################
## tst_bench_qgraphicsview Binary:
#####################################################################

qt_internal_add_benchmark(tst_bench_qgraphicsview
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

qt_internal_add_resource(tst_bench_qgraphicsview "qgraphicsview"
    PREFIX
        "/"
    FILES
        ${qgraphicsview_resource_files}
)
set(images_resource_files
    "chiptester/qt4logo.png"
)

qt_internal_add_resource(tst_bench_qgraphicsview "images"
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

qt_internal_extend_target(tst_bench_qgraphicsview CONDITION TARGET Qt::OpenGL
    PUBLIC_LIBRARIES
        Qt::OpenGL
)
