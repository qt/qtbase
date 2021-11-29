#! [cmake_use]
find_package(Qt6 REQUIRED COMPONENTS DBus)
target_link_libraries(mytarget PRIVATE Qt6::DBus)
#! [cmake_use]
