#! [qt5_wrap_cpp]
set(SOURCES myapp.cpp main.cpp)
qt5_wrap_cpp(SOURCES myapp.h)
add_executable(myapp ${SOURCES})
#! [qt5_wrap_cpp]

#! [qt5_add_resources]
set(SOURCES main.cpp)
qt5_add_resources(SOURCES example.qrc)
add_executable(myapp ${SOURCES})
#! [qt5_add_resources]

#! [qt5_add_big_resources]
set(SOURCES main.cpp)
qt5_add_big_resources(SOURCES big_resource.qrc)
add_executable(myapp ${SOURCES})
#! [qt5_add_big_resources]

#! [qt5_add_binary_resources]
qt5_add_binary_resources(resources project.qrc OPTIONS -no-compress)
add_dependencies(myapp resources)
#! [qt5_add_binary_resources]

#! [qt5_generate_moc]
qt5_generate_moc(main.cpp main.moc TARGET myapp)
#! [qt5_generate_moc]
