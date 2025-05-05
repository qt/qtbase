if(CMP0156-OLD)
     cmake_policy(SET CMP0156 OLD)
endif()
if(FORCE-CMP0156-NEW)
    set(QT_FORCE_CMP0156_TO_VALUE NEW)
endif()

find_package(Qt6 REQUIRED COMPONENTS Gui)

qt_add_library(core_helper STATIC)
target_sources(core_helper PRIVATE core_helper.mm)
set_target_properties(core_helper PROPERTIES FRAMEWORK TRUE)

qt_add_library(gui_helper STATIC)
target_sources(gui_helper PRIVATE gui_helper.cpp)
set_target_properties(gui_helper PROPERTIES FRAMEWORK TRUE)
target_link_libraries(gui_helper PRIVATE core_helper)

qt_add_executable(app)
target_sources(app PRIVATE main.cpp)
target_link_libraries(app PRIVATE Qt6::Gui)
target_link_options(app PRIVATE -ObjC)

# This will cause core_helper to be linked into the app twice if
# policy CMP0156 is not set to NEW, and this will cause duplicate symbol errors.
target_link_libraries(app PRIVATE core_helper gui_helper)
