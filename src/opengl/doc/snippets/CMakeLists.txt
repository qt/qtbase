#! [cmake_use]
find_package(Qt6 REQUIRED COMPONENTS OpenGL)
target_link_libraries(mytarget PRIVATE Qt6::OpenGL)
#! [cmake_use]
