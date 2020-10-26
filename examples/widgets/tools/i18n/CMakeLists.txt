# Generated from i18n.pro.

cmake_minimum_required(VERSION 3.14)
project(i18n LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/tools/i18n")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Widgets)

qt_add_executable(i18n
    languagechooser.cpp languagechooser.h
    main.cpp
    mainwindow.cpp mainwindow.h
)
set_target_properties(i18n PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(i18n PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
)


# Resources:
set(i18n_resource_files
    "translations/i18n_ar.qm"
    "translations/i18n_cs.qm"
    "translations/i18n_de.qm"
    "translations/i18n_el.qm"
    "translations/i18n_en.qm"
    "translations/i18n_eo.qm"
    "translations/i18n_fr.qm"
    "translations/i18n_it.qm"
    "translations/i18n_jp.qm"
    "translations/i18n_ko.qm"
    "translations/i18n_no.qm"
    "translations/i18n_ru.qm"
    "translations/i18n_sv.qm"
    "translations/i18n_zh.qm"
)

qt6_add_resources(i18n "i18n"
    PREFIX
        "/"
    FILES
        ${i18n_resource_files}
)

install(TARGETS i18n
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
