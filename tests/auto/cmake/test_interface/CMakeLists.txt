
cmake_minimum_required(VERSION 2.8.11)

project(test_interface)

find_package(Qt5Widgets)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Core_EXECUTABLE_COMPILE_FLAGS}")

add_executable(test_interface_exe WIN32 main.cpp mainwindow.cpp)

# No need to specify include directories, compile definitions, the PIC flag, or to
# link explicitly to Qt5::WinMain.
target_link_libraries(test_interface_exe Qt5::Widgets)

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/try_compile-test.cpp"
    "
#include <QString>
#include <QWidget>

int main(int,char**) { QWidget w; w.show(); return 0; }
"
)

# Fix try_compile to inherit the parent configuration.
set(CMAKE_TRY_COMPILE_CONFIGURATION "${CMAKE_BUILD_TYPE}")

# The try_compile works because Qt5::Widgets is listed in the LINK_LIBRARIES,
# which causes the includes, defines and appropriate PIC flag to be used.
try_compile(_TRY_COMPILE_RES "${CMAKE_CURRENT_BINARY_DIR}/try_compile-test"
    "${CMAKE_CURRENT_BINARY_DIR}/try_compile-test.cpp"
    LINK_LIBRARIES Qt5::Widgets
    OUTPUT_VARIABLE TC_OV
)

if (NOT _TRY_COMPILE_RES)
    message(SEND_ERROR "The use of try_compile with Qt5::Widgets failed. The output was :\n${TC_OV}")
endif()
