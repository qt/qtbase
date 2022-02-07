# Generated from tools.pro.

# QTBUG-88538 # special case
if(NOT ANDROID AND NOT IOS)
    add_subdirectory(qmakelib)
    if(QT_FEATURE_qmake)
        add_subdirectory(qmake)
    endif()
    add_subdirectory(moc)
    add_subdirectory(rcc)
endif()
# QTBUG-88538 # special case
if(TARGET Qt::Widgets AND NOT ANDROID AND NOT IOS)
    add_subdirectory(uic)
endif()
if(TARGET Qt::DBus)
    add_subdirectory(qdbuscpp2xml)
    add_subdirectory(qdbusxml2cpp)
endif()
