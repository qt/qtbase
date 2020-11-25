#! [cmake_use]
find_package(Qt6 COMPONENTS Widgets REQUIRED)
target_link_libraries(mytarget PRIVATE Qt6::Widgets)
#! [cmake_use]
