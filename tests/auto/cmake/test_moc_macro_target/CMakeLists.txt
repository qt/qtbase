
cmake_minimum_required(VERSION 2.8)

project(test_moc_macro_target)

find_package(Qt5Core REQUIRED)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

qt5_generate_moc(main_gen_test.cpp
  "${CMAKE_CURRENT_BINARY_DIR}/main_gen_test.moc"
  TARGET Qt5GenerateMacroTest
)
add_executable(Qt5GenerateMacroTest main_gen_test.cpp "${CMAKE_CURRENT_BINARY_DIR}/main_gen_test.moc")
target_include_directories(Qt5GenerateMacroTest PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/interface")
target_link_libraries(Qt5GenerateMacroTest Qt5::Core)

qt5_wrap_cpp(moc_file mywrapobject.h
  TARGET Qt5WrapMacroTest
)
add_executable(Qt5WrapMacroTest main_wrap_test.cpp ${moc_file})
target_include_directories(Qt5WrapMacroTest PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/interface")
target_link_libraries(Qt5WrapMacroTest Qt5::Core)
