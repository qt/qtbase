#! [cmake_use]
find_package(Qt6 COMPONENTS Concurrent REQUIRED)
target_link_libraries(mytarget PRIVATE Qt6::Concurrent)
#! [cmake_use]
