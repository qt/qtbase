find_package(Qt6 REQUIRED COMPONENTS Core)

qt_standard_project_setup(
    I18N_SOURCE_LANGUAGE de
    I18N_TRANSLATED_LANGUAGES pl
)

qt_add_executable(app MACOSX_BUNDLE main.cpp)
target_link_libraries(app PRIVATE Qt6::Core)
