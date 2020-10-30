#! [cmake_use]
find_package(Qt6 COMPONENTS Test REQUIRED)
target_link_libraries(mytarget Qt6::Test)
#! [cmake_use]
