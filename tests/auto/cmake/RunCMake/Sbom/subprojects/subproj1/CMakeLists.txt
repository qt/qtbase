project(subproj1)

# This is used by common_result_gen.cmake.
set(SBOM_VERSION "1.0.1")
set(SBOM_PROJECT_NAME "${PROJECT_NAME}")

_qt_internal_sbom_begin_project(
    SBOM_PROJECT_NAME "${SBOM_PROJECT_NAME}"
    INSTALL_SBOM_DIR "${SBOM_INSTALL_DIR}"
    SUPPLIER "QtProjectTest"
    SUPPLIER_URL "https://qt-project.org/SbomTest"
    VERSION "${SBOM_VERSION}"
)

add_library(subproj1_helper STATIC)
target_sources(subproj1_helper PRIVATE subproj1_helper.cpp)
target_link_libraries(subproj1_helper)
install(TARGETS subproj1_helper
    RUNTIME DESTINATION bin
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)
_qt_internal_add_sbom(subproj1_helper
    TYPE "LIBRARY"
    RUNTIME_PATH bin
    ARCHIVE_PATH lib
    LIBRARY_PATH lib
)


# This will actually refer to the root project Threads, that gets brought in via Qt6::Core
# dependency.
find_package(Threads)
if(TARGET Threads::Threads)
    _qt_internal_add_sbom(Threads::Threads
        TYPE SYSTEM_LIBRARY
    )
    target_link_libraries(subproj1_helper PRIVATE Threads::Threads)
endif()

# Create another IMPORTED target that is meant to simulate a system library, so we don't refer to
# a target that exists in the root scope.
add_library(FancySystemLib IMPORTED INTERFACE)
_qt_internal_add_sbom(FancySystemLib
    TYPE SYSTEM_LIBRARY
)
target_link_libraries(subproj1_helper PRIVATE FancySystemLib)

_qt_internal_sbom_end_project()

include(CommonResultGen)
