cmake_minimum_required(VERSION 3.16)
project(chat LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/dbus/chat")

find_package(Qt6 REQUIRED COMPONENTS Core DBus Gui Widgets)

set(chat_SRCS)
qt_add_dbus_interface(chat_SRCS
    org.example.chat.xml
    chat_interface
)

qt_add_dbus_adaptor(chat_SRCS
    org.example.chat.xml
    qobject.h
    QObject
    chat_adaptor
)

qt_add_executable(chat
    chat.cpp chat.h
    chatmainwindow.ui
    chatsetnickname.ui
    ${chat_SRCS}
)

set_target_properties(chat PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(chat PUBLIC
    Qt::Core
    Qt::DBus
    Qt::Gui
    Qt::Widgets
)

install(TARGETS chat
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
