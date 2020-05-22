#! [cmake_use]
find_package(Qt6 COMPONENTS Sql REQUIRED)
target_link_libraries(mytarget Qt::Sql)
#! [cmake_use]
