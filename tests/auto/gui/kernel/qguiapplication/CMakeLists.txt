# Generated from qguiapplication.pro.

if(NOT QT_FEATURE_private_tests)
    return()
endif()

#####################################################################
## tst_qguiapplication Test:
#####################################################################

add_qt_test(tst_qguiapplication
    SOURCES
        ../../../corelib/kernel/qcoreapplication/tst_qcoreapplication.cpp ../../../corelib/kernel/qcoreapplication/tst_qcoreapplication.h # special case
        tst_qguiapplication.cpp
    DEFINES
        QT_DISABLE_DEPRECATED_BEFORE=0x050E00
    INCLUDE_DIRECTORIES
        ../../../corelib/kernel/qcoreapplication
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Threads::Threads # special case
)

# Resources:
set(tst_qguiapplication_resource_files
    "icons/appicon.png"
    "icons/usericon.png"
)

add_qt_resource(tst_qguiapplication "tst_qguiapplication"
    PREFIX
        "/"
    FILES
        ${tst_qguiapplication_resource_files}
)


## Scopes:
#####################################################################

#### Keys ignored in scope 3:.:../../../corelib/kernel/qcoreapplication:../../../corelib/kernel/qcoreapplication/qcoreapplication.pro:WIN32:
# VERSION = "1.2.3.4"

#### Keys ignored in scope 4:.:../../../corelib/kernel/qcoreapplication:../../../corelib/kernel/qcoreapplication/qcoreapplication.pro:else:
# VERSION = "1.2.3"
