set(resource_files
    sources/res1.txt
)
set(resources_source_files
    sources/res1_source.txt
)
set(resource_install_path "resources")

_qt_internal_add_sbom(ResourceFiles
    TYPE RESOURCES
)
_qt_internal_sbom_add_files(ResourceFiles
    FILES "${resource_files}"
    SOURCE_FILES "${resource_source_files}"
    FILE_TYPE "RESOURCE"
    INSTALL_PATH "${resource_install_path}"
    INSTALL_PATH_DEBUG "${resource_install_path}/debug"
    INSTALL_PATH_RELEASE "${resource_install_path}/release"
)

install(FILES
    ${resource_files}
    DESTINATION "${resource_install_path}"
)

set(custom_files
    sources/custom1.txt
)
set(custom_source_files
    sources/custom1_source.txt
)
set(custom_install_path "custom")

_qt_internal_add_sbom(CustomFiles
    TYPE CUSTOM
)

_qt_internal_sbom_add_files(CustomFiles
    FILES "${custom_files}"
    SOURCE_FILES "${custom_source_files}"
    FILE_TYPE "CUSTOM"
    INSTALL_PATH "${custom_install_path}"
    INSTALL_PATH_DEBUG "${custom_install_path}/debug"
    INSTALL_PATH_RELEASE "${custom_install_path}/release"
)

install(FILES
    ${custom_files}
    DESTINATION "${custom_install_path}"
)
