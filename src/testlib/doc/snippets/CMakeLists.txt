#! [cmake_use]
find_package(Qt6 REQUIRED COMPONENTS Test)
target_link_libraries(mytarget PRIVATE Qt6::Test)
#! [cmake_use]
