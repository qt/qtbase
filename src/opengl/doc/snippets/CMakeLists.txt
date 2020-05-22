#! [cmake_use]
find_package(Qt6 COMPONENTS OpenGL REQUIRED)
target_link_libraries(mytarget Qt::OpenGL)
#! [cmake_use]
