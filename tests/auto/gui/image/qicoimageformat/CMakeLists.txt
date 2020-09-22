# Generated from qicoimageformat.pro.

#####################################################################
## tst_qicoimageformat Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    icons/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qicoimageformat
    SOURCES
        tst_qicoimageformat.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
    TESTDATA ${test_data}
)

## Scopes:
#####################################################################

if(ANDROID)
    # Resources:
    set(qicoimageformat_resource_files
        "icons/invalid/35floppy.ico"
        "icons/valid/35FLOPPY.ICO"
        "icons/valid/AddPerfMon.ico"
        "icons/valid/App.ico"
        "icons/valid/Obj_N2_Internal_Mem.ico"
        "icons/valid/Qt.ico"
        "icons/valid/Status_Play.ico"
        "icons/valid/TIMER01.ICO"
        "icons/valid/WORLD.ico"
        "icons/valid/WORLDH.ico"
        "icons/valid/abcardWindow.ico"
        "icons/valid/semitransparent.ico"
        "icons/valid/trolltechlogo_tiny.ico"
        "icons/valid/yellow.cur"
    )

    qt_internal_add_resource(tst_qicoimageformat "qicoimageformat"
        PREFIX
            "/"
        FILES
            ${qicoimageformat_resource_files}
    )
endif()
