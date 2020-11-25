#! [cmake_use]
find_package(Qt6 COMPONENTS DBus REQUIRED)
target_link_libraries(mytarget PRIVATE Qt6::DBus)
#! [cmake_use]
