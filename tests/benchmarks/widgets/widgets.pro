TEMPLATE = subdirs
SUBDIRS = \
        graphicsview \
        itemviews \
        kernel \
        styles \
        widgets \

TRUSTED_BENCHMARKS += \
    graphicsview/functional/GraphicsViewBenchmark \
    graphicsview/qgraphicsview

include(../trusted-benchmarks.pri)
