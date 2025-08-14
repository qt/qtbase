add_library(core_helper STATIC)
target_sources(core_helper PRIVATE sources/core_helper.cpp)
install(TARGETS core_helper
    ARCHIVE DESTINATION lib
)
_qt_internal_add_sbom(core_helper
    TYPE "LIBRARY"
    RUNTIME_PATH bin
    ARCHIVE_PATH lib
    LIBRARY_PATH lib
)

add_library(utils_helper STATIC)
target_sources(utils_helper PRIVATE sources/utils_helper.cpp)
install(TARGETS utils_helper
    ARCHIVE DESTINATION lib
)
_qt_internal_add_sbom(utils_helper
    TYPE "LIBRARY"
    RUNTIME_PATH bin
    ARCHIVE_PATH lib
    LIBRARY_PATH lib
)

add_library(gui_helper SHARED)
target_sources(gui_helper PRIVATE sources/gui_helper.cpp)
target_link_libraries(gui_helper PRIVATE core_helper Qt6::Core)
install(TARGETS gui_helper
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)
_qt_internal_add_sbom(gui_helper
    TYPE "LIBRARY"
    RUNTIME_PATH bin
    ARCHIVE_PATH lib
    LIBRARY_PATH lib
)

add_executable(app)
target_sources(app PRIVATE sources/main.cpp)
target_link_libraries(app PRIVATE gui_helper utils_helper)
install(TARGETS app
    BUNDLE DESTINATION bin
)
_qt_internal_add_sbom(app
    TYPE "EXECUTABLE"
    RUNTIME_PATH bin
)

find_package(ZLIB)
if(ZLIB_FOUND)
    _qt_internal_add_sbom(ZLIB::ZLIB
        TYPE SYSTEM_LIBRARY
    )
    _qt_internal_extend_sbom_dependencies(app
        SBOM_DEPENDENCIES ZLIB::ZLIB
    )
endif()

add_subdirectory(custom_files)

