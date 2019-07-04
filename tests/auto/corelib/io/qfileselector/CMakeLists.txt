# Generated from qfileselector.pro.

#####################################################################
## tst_qfileselectors Test:
#####################################################################

add_qt_test(tst_qfileselectors
    SOURCES
        tst_qfileselector.cpp
    LIBRARIES
        Qt::CorePrivate
)

# Resources:
add_qt_resource(tst_qfileselectors "qfileselector" PREFIX "/" FILES
    extras/+custom1/test
    extras/+custom1/test3
    extras/+custom2/test
    extras/+custom3/+custom2/test
    extras/+custom3/+custom4/test
    extras/+custom3/+custom5/test
    extras/+custom3/test
    extras/+custom5/+custom3/test
    extras/test
    extras/test2
    platforms/+android/test
    platforms/+android/test2
    platforms/+darwin/test
    platforms/+haiku/test
    platforms/+haiku/test2
    platforms/+ios/test
    platforms/+ios/test2
    platforms/+linux/test
    platforms/+linux/test2
    platforms/+macos/test
    platforms/+macos/test2
    platforms/+qnx/test
    platforms/+qnx/test2
    platforms/+unix/+android/test
    platforms/+unix/+darwin/+ios/test
    platforms/+unix/+darwin/+macos/test
    platforms/+unix/+darwin/test
    platforms/+unix/+haiku/test
    platforms/+unix/+linux/test
    platforms/+unix/+qnx/test
    platforms/+unix/test
    platforms/+unix/test3
    platforms/+wince/test
    platforms/+wince/test2
    platforms/+windows/+wince/test
    platforms/+windows/+winnt/test
    platforms/+windows/+winrt/test
    platforms/+windows/test
    platforms/+windows/test3
    platforms/+winnt/test2
    platforms/+winrt/test
    platforms/+winrt/test2
    platforms/test
    platforms/test2
    platforms/test3
    platforms/test4
    platforms/test5)


#### Keys ignored in scope 1:.:.:qfileselector.pro:<TRUE>:
# CONFIG = "testcase"
