# Needed to make the sbom functions available.
find_package(Qt6 REQUIRED Core)

sbom_test_begin()

_qt_internal_setup_sbom(
    GENERATE_SBOM_DEFAULT "TRUE"
)

set(SBOM_PROJECT_NAME "SystemLibraryAttribution")
_qt_internal_sbom_begin_project(
    SBOM_PROJECT_NAME "${SBOM_PROJECT_NAME}"
    SUPPLIER "QtProjectTest"
    SUPPLIER_URL "https://qt-project.org/SbomTest"
    VERSION "1.0.0"
)
sbom_test_record_project()

# Creates a system library in the shape Qt's FindWrap*.cmake modules produce: an interface target
# in front of the imported target that holds the real library path.
function(add_fake_system_library name library_path)
    cmake_parse_arguments(arg "LINK_ONLY" "" "" ${ARGN})

    add_library(${name}Imported STATIC IMPORTED)
    set_target_properties(${name}Imported PROPERTIES IMPORTED_LOCATION "${library_path}")

    # Qt wraps static dependencies in $<LINK_ONLY:...>, so the link closure walk has to unwrap
    # those rather than skip them.
    set(linked ${name}Imported)
    if(arg_LINK_ONLY)
        set(linked "$<LINK_ONLY:${name}Imported>")
    endif()

    add_library(Wrap${name}_Wrap${name} INTERFACE)
    target_link_libraries(Wrap${name}_Wrap${name} INTERFACE "${linked}")
    add_library(Wrap${name}::Wrap${name} ALIAS Wrap${name}_Wrap${name})

    _qt_internal_sbom_record_system_library_usage(Wrap${name}_Wrap${name}
        SBOM_ENTITY_TYPE SYSTEM_LIBRARY
        FRIENDLY_PACKAGE_NAME "Wrap${name}"
    )
endfunction()

# A prefix that looks like a vcpkg install tree processed by
# coin/provisioning/common/shared/vcpkg_generate_attributions.py. The attribution files and the
# port index are committed fixtures; only the stand-in libraries they point at are generated,
# since a fake archive is not worth carrying in git.
#
# The attribution fixtures are committed with an .in suffix and copied into place, so that no
# file in the source tree is named qt_attribution.json. sanitize-commit flags any such file
# changed without a [ChangeLog][Third-Party Code] entry, which would be wrong for a fixture.
set(fixtures "${CMAKE_CURRENT_SOURCE_DIR}/system_library_attribution")
set(fake_prefix "${CMAKE_CURRENT_BINARY_DIR}/fake_vcpkg_prefix")

configure_file("${fixtures}/qt_vcpkg_ports.json"
    "${fake_prefix}/share/qt_vcpkg_ports.json" COPYONLY)
configure_file("${fixtures}/fakeport/qt_attribution.json.in"
    "${fake_prefix}/share/fakeport/qt_attribution.json" COPYONLY)
configure_file("${fixtures}/genexport/qt_attribution.json.in"
    "${fake_prefix}/share/genexport/qt_attribution.json" COPYONLY)

file(WRITE "${fake_prefix}/lib/libfakeport.a" "not a real archive")
add_fake_system_library(FakePort "${fake_prefix}/lib/libfakeport.a")

file(WRITE "${fake_prefix}/lib/libgenexport.a" "not a real archive")
add_fake_system_library(GenexPort "${fake_prefix}/lib/libgenexport.a" LINK_ONLY)

# The negative cases are deliberately degenerate, so they are generated rather than committed.
# A malformed index or an empty qt_attribution.json in the source tree would be picked up by
# other tooling that walks these files.

# A library that lives outside any vcpkg prefix.
set(bare_prefix "${CMAKE_CURRENT_BINARY_DIR}/bare_prefix")
file(WRITE "${bare_prefix}/lib/libbareport.a" "not a real archive")
add_fake_system_library(BarePort "${bare_prefix}/lib/libbareport.a")

# A library in a prefix whose index is not readable as JSON.
set(broken_prefix "${CMAKE_CURRENT_BINARY_DIR}/broken_vcpkg_prefix")
file(WRITE "${broken_prefix}/lib/libbrokenport.a" "not a real archive")
file(WRITE "${broken_prefix}/share/qt_vcpkg_ports.json" "{ not json at all\n")
add_fake_system_library(BrokenPort "${broken_prefix}/lib/libbrokenport.a")

# A library whose attribution file exists but is empty, e.g. because provisioning was
# interrupted between truncating the file and writing its contents. This must not abort
# the configure with the empty-file FATAL_ERROR in _qt_internal_sbom_read_qt_attribution.
set(empty_attribution_prefix "${CMAKE_CURRENT_BINARY_DIR}/empty_attribution_vcpkg_prefix")
file(WRITE "${empty_attribution_prefix}/lib/libemptyattributionport.a" "not a real archive")
file(WRITE "${empty_attribution_prefix}/share/qt_vcpkg_ports.json"
    "{\n"
    "    \"lib/libemptyattributionport.a\": \"emptyattributionport\"\n"
    "}\n")
file(WRITE
    "${empty_attribution_prefix}/share/emptyattributionport/qt_attribution.json" "")
add_fake_system_library(EmptyAttributionPort
    "${empty_attribution_prefix}/lib/libemptyattributionport.a")

# Only consumed system libraries get a package, so link them into something.
add_library(consumer STATIC)
target_sources(consumer PRIVATE sources/utils_helper.cpp)
target_link_libraries(consumer PRIVATE
    WrapFakePort::WrapFakePort
    WrapGenexPort::WrapGenexPort
    WrapBarePort::WrapBarePort
    WrapBrokenPort::WrapBrokenPort
    WrapEmptyAttributionPort::WrapEmptyAttributionPort
)
install(TARGETS consumer
    ARCHIVE DESTINATION lib
)
_qt_internal_add_sbom(consumer
    SBOM_ENTITY_TYPE QT_MODULE
    ARCHIVE_PATH lib
    LIBRARY_PATH lib
    RUNTIME_PATH bin
)

# The attribution file's values must reach the system library's package.
# Backslash escapes do not survive the round trip through result.cmake, so the literal '?' of the
# PURL query string is matched with a character class instead of '\?'.
add_assert_str_exists_in_spdx_v2_3_doc("PackageVersion: 7\\.8\\.9")
add_assert_str_exists_in_spdx_v2_3_doc(
    "ExternalRef: PACKAGE-MANAGER purl pkg:vcpkg/fakeport@7\\.8\\.9[?]triplet=fake-triplet")
add_assert_str_exists_in_spdx_v2_3_doc("GPL-2\\.0-or-later")

# Bind the version to the package name, so this cannot pass on the other port's values.
string(CONCAT genex_needle
    "PackageName: WrapGenexPort\n"
    "SPDXID:[^\n]*\n"
    "PackageDownloadLocation: https://qt-project\\.org/SbomTest/genexport\n"
    "PackageVersion: 4\\.5\\.6\n"
)
add_assert_str_exists_in_spdx_v2_3_doc("${genex_needle}")
add_assert_str_exists_in_spdx_v2_3_doc(
    "ExternalRef: PACKAGE-MANAGER purl pkg:vcpkg/genexport@4\\.5\\.6[?]triplet=fake-triplet")

# A library with no resolvable attribution file must still get its plain package.
function(add_assert_package_without_attribution package_name)
    string(CONCAT needle
        "PackageName: ${package_name}\n"
        "SPDXID:[^\n]*\n"
        "PackageDownloadLocation: NOASSERTION\n"
        "PackageVersion: unknown\n"
    )
    add_assert_str_exists_in_spdx_v2_3_doc("${needle}")
endfunction()

add_assert_package_without_attribution(WrapBarePort)
add_assert_package_without_attribution(WrapBrokenPort)
add_assert_package_without_attribution(WrapEmptyAttributionPort)

_qt_internal_sbom_end_project()

sbom_test_end()
