# Generated from tools.pro.

add_subdirectory(qmakelib)
# special case begin
if(NOT WIN32) # @todo: this is broken, see QTBUG-78449
    add_subdirectory(qmake)
endif()
# special case end
#add_subdirectory(moc)
add_subdirectory(rcc)
if(TARGET Qt::Widgets)
    add_subdirectory(uic)
endif()
if(TARGET Qt::DBus)
    add_subdirectory(qdbuscpp2xml)
    add_subdirectory(qdbusxml2cpp)
endif()
