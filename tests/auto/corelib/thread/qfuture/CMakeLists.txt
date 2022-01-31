# Generated from qfuture.pro.

#####################################################################
## tst_qfuture Test:
#####################################################################

qt_internal_add_test(tst_qfuture
    SOURCES
        tst_qfuture.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
)

qt_internal_extend_target(tst_qfuture CONDITION MSVC
    COMPILE_OPTIONS
        /bigobj
)

qt_internal_undefine_global_definition(tst_qfuture QT_NO_JAVA_STYLE_ITERATORS)
