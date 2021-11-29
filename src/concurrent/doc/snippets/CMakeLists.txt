#! [cmake_use]
find_package(Qt6 REQUIRED COMPONENTS Concurrent)
target_link_libraries(mytarget PRIVATE Qt6::Concurrent)
#! [cmake_use]
