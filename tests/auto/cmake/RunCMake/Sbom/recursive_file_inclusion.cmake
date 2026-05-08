# Needed to make the sbom functions available.
find_package(Qt6 REQUIRED Core)

include(CommonResultGenIntro)

_qt_internal_setup_sbom(
    GENERATE_SBOM_DEFAULT "TRUE"
)

function(create_sbom_lib_target target)
    add_library(${target} STATIC)
    target_sources(${target} PRIVATE "${CMAKE_SOURCE_DIR}/sources/utils_helper.cpp")
    install(TARGETS ${target}
        EXPORT MainTargets
        ARCHIVE DESTINATION lib
    )
    qt_internal_add_sbom(${target}
        TYPE "LIBRARY"
        RUNTIME_PATH bin
        ARCHIVE_PATH lib
        LIBRARY_PATH lib
    )
endfunction()

# This is used by common_result_gen.cmake.
set(SBOM_VERSION "1.0.0")
set(SBOM_PROJECT_NAME "recursive-file-inclusion")

_qt_internal_sbom_begin_project(
    SUPPLIER "QtProjectTest"
    SUPPLIER_URL "https://qt-project.org/SbomTest"
    VERSION "${SBOM_VERSION}"
    SBOM_PROJECT_NAME "${SBOM_PROJECT_NAME}"
)

# Find BuildInternals which will include QtSbomHelpers for the first time, followed by
# QtPublicSbomHelpers.cmake.
find_package(Qt6 REQUIRED BuildInternals)

# Then include it again a second time. Originally this would have overridden _qt_internal_add_sbom
# to point to qt_internal_add_sbom. With the include guard, it shouldn't anymore.
include(QtSbomHelpers)

# Try to create an sbom target. It shouldn't lead to infinite recursion.
create_sbom_lib_target(Plankton)

_qt_internal_sbom_end_project()

include(CommonResultGen)
