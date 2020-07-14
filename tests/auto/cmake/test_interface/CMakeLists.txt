
cmake_minimum_required(VERSION 3.14)

project(test_interface)

find_package(Qt6Widgets)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_executable(test_interface_exe WIN32 main.cpp mainwindow.cpp)

# No need to specify include directories, compile definitions, the PIC flag, or to
# link explicitly to Qt::WinMain.
target_link_libraries(test_interface_exe Qt::Widgets)

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/try_compile-test.cpp"
    "
#include <QString>
#include <QWidget>

int main(int,char**) { QWidget w; w.show(); return 0; }
"
)

# Fix try_compile to inherit the parent configuration.
set(CMAKE_TRY_COMPILE_CONFIGURATION "${CMAKE_BUILD_TYPE}")

# The try_compile works because Qt::Widgets is listed in the LINK_LIBRARIES,
# which causes the includes, defines and appropriate PIC flag to be used.
try_compile(_TRY_COMPILE_RES "${CMAKE_CURRENT_BINARY_DIR}/try_compile-test"
    "${CMAKE_CURRENT_BINARY_DIR}/try_compile-test.cpp"
    LINK_LIBRARIES Qt::Widgets
    OUTPUT_VARIABLE TC_OV
)

if (NOT _TRY_COMPILE_RES)
    message(SEND_ERROR "The use of try_compile with Qt::Widgets failed. The output was :\n${TC_OV}")
endif()
