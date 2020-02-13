# Generated from qfuture.pro.

#####################################################################
## tst_qfuture Test:
#####################################################################

add_qt_test(tst_qfuture
    SOURCES
        tst_qfuture.cpp
    DEFINES
        # -QT_NO_JAVA_STYLE_ITERATORS # special case
        QT_STRICT_ITERATORS
    PUBLIC_LIBRARIES
        Qt::CorePrivate
)
