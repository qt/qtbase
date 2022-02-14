# Generated from qcoreapplication.pro.

if(NOT QT_FEATURE_private_tests)
    return()
endif()

#####################################################################
## tst_qcoreapplication Test:
#####################################################################

# special case begin
if (WIN32)
    set(target_version "1.2.3.4")
else()
    set(target_version "1.2.3")
endif()
# special case end

qt_internal_add_test(tst_qcoreapplication
    VERSION ${target_version} # special case
    SOURCES
        tst_qcoreapplication.cpp tst_qcoreapplication.h
    PUBLIC_LIBRARIES
        Qt::CorePrivate
)

# special case begin
if (APPLE)
    set_property(TARGET tst_qcoreapplication PROPERTY MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist")
    set_property(TARGET tst_qcoreapplication PROPERTY PROPERTY MACOSX_BUNDLE TRUE)
endif()
# special case end

if (ANDROID)
    set_property(TARGET tst_qcoreapplication PROPERTY QT_ANDROID_VERSION_NAME ${target_version})
endif()


#### Keys ignored in scope 1:.:.:qcoreapplication.pro:<TRUE>:
# QMAKE_INFO_PLIST = "$$PWD/Info.plist"
# _REQUIREMENTS = "qtConfig(private_tests)"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:qcoreapplication.pro:WIN32:
# VERSION = "1.2.3.4"

#### Keys ignored in scope 3:.:.:qcoreapplication.pro:else:
# VERSION = "1.2.3"
