#! [cmake_use]
find_package(Qt6 REQUIRED COMPONENTS Xml)
target_link_libraries(mytarget PRIVATE Qt6::Xml)
#! [cmake_use]
