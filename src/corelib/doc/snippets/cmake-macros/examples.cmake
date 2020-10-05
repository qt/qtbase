#! [qt5_wrap_cpp]
set(SOURCES myapp.cpp main.cpp)
qt5_wrap_cpp(SOURCES myapp.h)
add_executable(myapp ${SOURCES})
#! [qt5_wrap_cpp]

#! [qt_wrap_cpp]
set(SOURCES myapp.cpp main.cpp)
qt_wrap_cpp(SOURCES myapp.h)
add_executable(myapp ${SOURCES})
#! [qt_wrap_cpp]

#! [qt5_add_resources]
set(SOURCES main.cpp)
qt5_add_resources(SOURCES example.qrc)
add_executable(myapp ${SOURCES})
#! [qt5_add_resources]

#! [qt_add_resources]
set(SOURCES main.cpp)
qt_add_resources(SOURCES example.qrc)
add_executable(myapp ${SOURCES})
#! [qt_add_resources]

#! [qt5_add_big_resources]
set(SOURCES main.cpp)
qt5_add_big_resources(SOURCES big_resource.qrc)
add_executable(myapp ${SOURCES})
#! [qt5_add_big_resources]

#! [qt_add_big_resources]
set(SOURCES main.cpp)
qt_add_big_resources(SOURCES big_resource.qrc)
add_executable(myapp ${SOURCES})
#! [qt_add_big_resources]

#! [qt5_add_binary_resources]
qt5_add_binary_resources(resources project.qrc OPTIONS -no-compress)
add_dependencies(myapp resources)
#! [qt5_add_binary_resources]

#! [qt_add_binary_resources]
qt_add_binary_resources(resources project.qrc OPTIONS -no-compress)
add_dependencies(myapp resources)
#! [qt_add_binary_resources]

#! [qt5_generate_moc]
qt5_generate_moc(main.cpp main.moc TARGET myapp)
#! [qt5_generate_moc]

#! [qt_generate_moc]
qt_generate_moc(main.cpp main.moc TARGET myapp)
#! [qt_generate_moc]

#! [qt5_import_plugins]
add_executable(myapp main.cpp)
target_link_libraries(myapp Qt5::Gui Qt5::Sql)
qt5_import_plugins(myapp
    INCLUDE Qt5::QCocoaIntegrationPlugin
    EXCLUDE Qt5::QMinimalIntegrationPlugin
    INCLUDE_BY_TYPE imageformats Qt5::QGifPlugin Qt5::QJpegPlugin
    EXCLUDE_BY_TYPE sqldrivers
)
#! [qt5_import_plugins]

#! [qt_import_plugins]
add_executable(myapp main.cpp)
target_link_libraries(myapp Qt5::Gui Qt5::Sql)
qt_import_plugins(myapp
    INCLUDE Qt5::QCocoaIntegrationPlugin
    EXCLUDE Qt5::QMinimalIntegrationPlugin
    INCLUDE_BY_TYPE imageformats Qt5::QGifPlugin Qt5::QJpegPlugin
    EXCLUDE_BY_TYPE sqldrivers
)
#! [qt_import_plugins]
