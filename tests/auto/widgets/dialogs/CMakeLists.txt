# Generated from dialogs.pro.

add_subdirectory(qcolordialog)
add_subdirectory(qdialog)
add_subdirectory(qerrormessage)
add_subdirectory(qfiledialog2)
add_subdirectory(qfontdialog)
add_subdirectory(qinputdialog)
add_subdirectory(qprogressdialog)
add_subdirectory(qwizard)
add_subdirectory(qfiledialog)
# QTBUG-101217, qmessagebox hangs on Android
if(NOT ANDROID)
    add_subdirectory(qmessagebox)
endif()
if(QT_FEATURE_private_tests)
    add_subdirectory(qsidebar)
endif()
