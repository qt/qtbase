# Generated from sub-attaq.pro.

cmake_minimum_required(VERSION 3.14)
project(sub-attaq LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/animation/sub-attaq")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Widgets)
find_package(Qt6 COMPONENTS OpenGL) # special case

add_qt_gui_executable(sub-attaq
    animationmanager.cpp animationmanager.h
    boat.cpp boat.h boat_p.h
    bomb.cpp bomb.h
    graphicsscene.cpp graphicsscene.h
    main.cpp
    mainwindow.cpp mainwindow.h
    pixmapitem.cpp pixmapitem.h
    progressitem.cpp progressitem.h
    qanimationstate.cpp qanimationstate.h
    states.cpp states.h
    submarine.cpp submarine.h submarine_p.h
    textinformationitem.cpp textinformationitem.h
    torpedo.cpp torpedo.h
)
target_link_libraries(sub-attaq PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
)


# Resources:
set_source_files_properties("pics/big/background.png"
    PROPERTIES QT_RESOURCE_ALIAS "big/background"
)
set_source_files_properties("pics/big/boat.png"
    PROPERTIES QT_RESOURCE_ALIAS "big/boat"
)
set_source_files_properties("pics/big/bomb.png"
    PROPERTIES QT_RESOURCE_ALIAS "big/bomb"
)
set_source_files_properties("pics/big/explosion/boat/step1.png"
    PROPERTIES QT_RESOURCE_ALIAS "big/explosion/boat/step1"
)
set_source_files_properties("pics/big/explosion/boat/step2.png"
    PROPERTIES QT_RESOURCE_ALIAS "big/explosion/boat/step2"
)
set_source_files_properties("pics/big/explosion/boat/step3.png"
    PROPERTIES QT_RESOURCE_ALIAS "big/explosion/boat/step3"
)
set_source_files_properties("pics/big/explosion/boat/step4.png"
    PROPERTIES QT_RESOURCE_ALIAS "big/explosion/boat/step4"
)
set_source_files_properties("pics/big/explosion/submarine/step1.png"
    PROPERTIES QT_RESOURCE_ALIAS "big/explosion/submarine/step1"
)
set_source_files_properties("pics/big/explosion/submarine/step2.png"
    PROPERTIES QT_RESOURCE_ALIAS "big/explosion/submarine/step2"
)
set_source_files_properties("pics/big/explosion/submarine/step3.png"
    PROPERTIES QT_RESOURCE_ALIAS "big/explosion/submarine/step3"
)
set_source_files_properties("pics/big/explosion/submarine/step4.png"
    PROPERTIES QT_RESOURCE_ALIAS "big/explosion/submarine/step4"
)
set_source_files_properties("pics/big/submarine.png"
    PROPERTIES QT_RESOURCE_ALIAS "big/submarine"
)
set_source_files_properties("pics/big/surface.png"
    PROPERTIES QT_RESOURCE_ALIAS "big/surface"
)
set_source_files_properties("pics/big/torpedo.png"
    PROPERTIES QT_RESOURCE_ALIAS "big/torpedo"
)
set_source_files_properties("pics/scalable/boat.svg"
    PROPERTIES QT_RESOURCE_ALIAS "boat"
)
set_source_files_properties("pics/scalable/sub-attaq.svg"
    PROPERTIES QT_RESOURCE_ALIAS "all"
)
set_source_files_properties("pics/scalable/submarine.svg"
    PROPERTIES QT_RESOURCE_ALIAS "submarine"
)
set_source_files_properties("pics/scalable/torpedo.svg"
    PROPERTIES QT_RESOURCE_ALIAS "torpedo"
)
set_source_files_properties("pics/small/background.png"
    PROPERTIES QT_RESOURCE_ALIAS "small/background"
)
set_source_files_properties("pics/small/boat.png"
    PROPERTIES QT_RESOURCE_ALIAS "small/boat"
)
set_source_files_properties("pics/small/bomb.png"
    PROPERTIES QT_RESOURCE_ALIAS "small/bomb"
)
set_source_files_properties("pics/small/submarine.png"
    PROPERTIES QT_RESOURCE_ALIAS "small/submarine"
)
set_source_files_properties("pics/small/surface.png"
    PROPERTIES QT_RESOURCE_ALIAS "small/surface"
)
set_source_files_properties("pics/small/torpedo.png"
    PROPERTIES QT_RESOURCE_ALIAS "small/torpedo"
)
set_source_files_properties("pics/welcome/logo-a.png"
    PROPERTIES QT_RESOURCE_ALIAS "logo-a"
)
set_source_files_properties("pics/welcome/logo-a2.png"
    PROPERTIES QT_RESOURCE_ALIAS "logo-a2"
)
set_source_files_properties("pics/welcome/logo-b.png"
    PROPERTIES QT_RESOURCE_ALIAS "logo-b"
)
set_source_files_properties("pics/welcome/logo-dash.png"
    PROPERTIES QT_RESOURCE_ALIAS "logo-dash"
)
set_source_files_properties("pics/welcome/logo-excl.png"
    PROPERTIES QT_RESOURCE_ALIAS "logo-excl"
)
set_source_files_properties("pics/welcome/logo-q.png"
    PROPERTIES QT_RESOURCE_ALIAS "logo-q"
)
set_source_files_properties("pics/welcome/logo-s.png"
    PROPERTIES QT_RESOURCE_ALIAS "logo-s"
)
set_source_files_properties("pics/welcome/logo-t.png"
    PROPERTIES QT_RESOURCE_ALIAS "logo-t"
)
set_source_files_properties("pics/welcome/logo-t2.png"
    PROPERTIES QT_RESOURCE_ALIAS "logo-t2"
)
set_source_files_properties("pics/welcome/logo-u.png"
    PROPERTIES QT_RESOURCE_ALIAS "logo-u"
)
set(subattaq_resource_files
    "data.xml"
    "pics/big/background.png"
    "pics/big/boat.png"
    "pics/big/bomb.png"
    "pics/big/explosion/boat/step1.png"
    "pics/big/explosion/boat/step2.png"
    "pics/big/explosion/boat/step3.png"
    "pics/big/explosion/boat/step4.png"
    "pics/big/explosion/submarine/step1.png"
    "pics/big/explosion/submarine/step2.png"
    "pics/big/explosion/submarine/step3.png"
    "pics/big/explosion/submarine/step4.png"
    "pics/big/submarine.png"
    "pics/big/surface.png"
    "pics/big/torpedo.png"
    "pics/scalable/boat.svg"
    "pics/scalable/sub-attaq.svg"
    "pics/scalable/submarine.svg"
    "pics/scalable/torpedo.svg"
    "pics/small/background.png"
    "pics/small/boat.png"
    "pics/small/bomb.png"
    "pics/small/submarine.png"
    "pics/small/surface.png"
    "pics/small/torpedo.png"
    "pics/welcome/logo-a.png"
    "pics/welcome/logo-a2.png"
    "pics/welcome/logo-b.png"
    "pics/welcome/logo-dash.png"
    "pics/welcome/logo-excl.png"
    "pics/welcome/logo-q.png"
    "pics/welcome/logo-s.png"
    "pics/welcome/logo-t.png"
    "pics/welcome/logo-t2.png"
    "pics/welcome/logo-u.png"
)

qt6_add_resources(sub-attaq "subattaq"
    PREFIX
        "/"
    FILES
        ${subattaq_resource_files}
)

install(TARGETS sub-attaq
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
