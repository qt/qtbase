# Generated from convert.pro.

cmake_minimum_required(VERSION 3.14)
project(convert LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/corelib/serialization/convert")

find_package(Qt6 COMPONENTS Core)

qt_add_executable(convert
    cborconverter.cpp cborconverter.h
    converter.h
    datastreamconverter.cpp datastreamconverter.h
    jsonconverter.cpp jsonconverter.h
    main.cpp
    nullconverter.cpp nullconverter.h
    textconverter.cpp textconverter.h
    xmlconverter.cpp xmlconverter.h
)
set_target_properties(convert PROPERTIES
    WIN32_EXECUTABLE FALSE
    MACOSX_BUNDLE FALSE
)
target_link_libraries(convert PUBLIC
    Qt::Core
)

install(TARGETS convert
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
