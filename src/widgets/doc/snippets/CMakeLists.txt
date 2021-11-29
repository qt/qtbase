#! [cmake_use]
find_package(Qt6 REQUIRED COMPONENTS Widgets)
target_link_libraries(mytarget PRIVATE Qt6::Widgets)
#! [cmake_use]
