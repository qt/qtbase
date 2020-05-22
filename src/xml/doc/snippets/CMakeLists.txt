#! [cmake_use]
find_package(Qt6 COMPONENTS Xml REQUIRED)
target_link_libraries(mytarget Qt::Xml)
#! [cmake_use]
