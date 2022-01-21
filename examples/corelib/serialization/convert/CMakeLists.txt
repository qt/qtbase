cmake_minimum_required(VERSION 3.16)
project(convert LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/corelib/serialization/convert")

find_package(Qt6 REQUIRED COMPONENTS Core)

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

target_link_libraries(convert PUBLIC
    Qt::Core
)

install(TARGETS convert
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
