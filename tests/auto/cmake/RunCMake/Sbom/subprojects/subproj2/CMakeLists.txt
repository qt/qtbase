project(subproj2)

# This is used by common_result_gen.cmake.
set(SBOM_VERSION "1.0.2")
set(SBOM_PROJECT_NAME "${PROJECT_NAME}")

if(NOT CREATE_SINGLE_SBOM)
    _qt_internal_sbom_begin_project(
        SBOM_PROJECT_NAME "${SBOM_PROJECT_NAME}"
        INSTALL_SBOM_DIR "${SBOM_INSTALL_DIR}"
        SUPPLIER "QtProjectTest"
        SUPPLIER_URL "https://qt-project.org/SbomTest"
        VERSION "${SBOM_VERSION}"
    )
endif()

add_library(subproj2_helper STATIC)
target_sources(subproj2_helper PRIVATE subproj2_helper.cpp)
target_link_libraries(subproj2_helper)
install(TARGETS subproj2_helper
    RUNTIME DESTINATION bin
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)
_qt_internal_add_sbom(subproj2_helper
    TYPE "LIBRARY"
    RUNTIME_PATH bin
    ARCHIVE_PATH lib
    LIBRARY_PATH lib
)

find_package(Threads)
if(TARGET Threads::Threads)
    _qt_internal_add_sbom(Threads::Threads
        TYPE SYSTEM_LIBRARY
    )
    target_link_libraries(subproj2_helper PRIVATE Threads::Threads)
endif()

if(NOT CREATE_SINGLE_SBOM)
    add_library(FancySystemLib IMPORTED INTERFACE)
    set_target_properties(FancySystemLib PROPERTIES IMPORTED_GLOBAL TRUE)
    _qt_internal_add_sbom(FancySystemLib
        TYPE SYSTEM_LIBRARY
    )
endif()
target_link_libraries(subproj2_helper PRIVATE FancySystemLib)

if(NOT CREATE_SINGLE_SBOM)
    _qt_internal_sbom_end_project()

    include(CommonResultGen)
endif()
