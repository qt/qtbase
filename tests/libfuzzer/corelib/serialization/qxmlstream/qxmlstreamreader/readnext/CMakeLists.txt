cmake_minimum_required(VERSION 3.16)
project(readnext LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core)

qt_add_executable(readnext
    main.cpp
)

target_link_libraries(readnext PUBLIC
    Qt::Core
)
if(DEFINED ENV{LIB_FUZZING_ENGINE})
    target_link_libraries(readnext PRIVATE
        $ENV{LIB_FUZZING_ENGINE}
    )
else()
    target_link_libraries(readnext PRIVATE
        -fsanitize=fuzzer
    )
endif()
