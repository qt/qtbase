# Generated from qguiapplication.pro.

if(NOT QT_FEATURE_private_tests)
    return()
endif()

#####################################################################
## tst_qguiapplication Test:
#####################################################################

# special case begin
if (WIN32)
    set(target_version "1.2.3.4")
else()
    set(target_version "1.2.3")
endif()
# special case end

qt_internal_add_test(tst_qguiapplication
    VERSION ${target_version} # special case
    SOURCES
        ../../../corelib/kernel/qcoreapplication/tst_qcoreapplication.cpp ../../../corelib/kernel/qcoreapplication/tst_qcoreapplication.h # special case
        tst_qguiapplication.cpp
    DEFINES
        QT_DISABLE_DEPRECATED_BEFORE=0x050E00
        QT_QGUIAPPLICATIONTEST=1
    INCLUDE_DIRECTORIES
        ../../../corelib/kernel/qcoreapplication
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
)

# Resources:
set(tst_qguiapplication_resource_files
    "icons/appicon.png"
    "icons/usericon.png"
)

qt_internal_add_resource(tst_qguiapplication "tst_qguiapplication"
    PREFIX
        "/"
    FILES
        ${tst_qguiapplication_resource_files}
)

if (ANDROID)
    set_property(TARGET tst_qguiapplication PROPERTY QT_ANDROID_VERSION_NAME ${target_version})
endif()
# special case begin
if (APPLE)
    set_property(TARGET tst_qguiapplication PROPERTY MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist")
    set_property(TARGET tst_qguiapplication PROPERTY PROPERTY MACOSX_BUNDLE TRUE)
endif()
# special case end

## Scopes:
#####################################################################

#### Keys ignored in scope 3:.:../../../corelib/kernel/qcoreapplication:../../../corelib/kernel/qcoreapplication/qcoreapplication.pro:WIN32:
# VERSION = "1.2.3.4"

#### Keys ignored in scope 4:.:../../../corelib/kernel/qcoreapplication:../../../corelib/kernel/qcoreapplication/qcoreapplication.pro:else:
# VERSION = "1.2.3"
