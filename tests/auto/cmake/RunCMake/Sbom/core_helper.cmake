
add_library(${CORE_HELPER} STATIC)
target_sources(${CORE_HELPER} PRIVATE sources/core_helper.cpp)
install(TARGETS ${CORE_HELPER}
    ARCHIVE DESTINATION lib
)
_qt_internal_add_sbom(${CORE_HELPER}
    TYPE "LIBRARY"
    RUNTIME_PATH bin
    ARCHIVE_PATH lib
    LIBRARY_PATH lib
)

