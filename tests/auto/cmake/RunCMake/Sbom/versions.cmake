# Needed to make the sbom functions available.
find_package(Qt6 REQUIRED Core)

_qt_internal_setup_sbom(
    GENERATE_SBOM_DEFAULT "TRUE"
)

# Case: Explicit version
_qt_internal_sbom_begin_project(
    SBOM_PROJECT_NAME "ExplicitVersion"
    SUPPLIER "QtProjectTest"
    SUPPLIER_URL "https://qt-project.org/SbomTest"
    VERSION "1.0.0"
)
set(CORE_HELPER "core_helper_explicit_version")
include(core_helper.cmake)
_qt_internal_sbom_end_project()

# Case: Version from git
_qt_internal_sbom_begin_project(
    SBOM_PROJECT_NAME "VersionFromGit"
    SUPPLIER "QtProjectTest"
    SUPPLIER_URL "https://qt-project.org/SbomTest"
    USE_GIT_VERSION
)
set(CORE_HELPER "core_helper_version_from_git")
include(core_helper.cmake)
_qt_internal_sbom_end_project()

# Case: Version from git with explicit version (explicit overrides the version from git)
_qt_internal_sbom_begin_project(
    SBOM_PROJECT_NAME "VersionFromGitWithExplicit"
    SUPPLIER "QtProjectTest"
    SUPPLIER_URL "https://qt-project.org/SbomTest"
    USE_GIT_VERSION
    VERSION "2.0.0"
)
set(CORE_HELPER "core_helper_version_from_git_with_explicit")
include(core_helper.cmake)
_qt_internal_sbom_end_project()

# Case: variable overrides the explicit version
set(QT_SBOM_VERSION_OVERRIDE "3.0.0-override")
_qt_internal_sbom_begin_project(
    SBOM_PROJECT_NAME "VersionWithOverride"
    SUPPLIER "QtProjectTest"
    SUPPLIER_URL "https://qt-project.org/SbomTest"
    VERSION "3.0.0"
)
set(CORE_HELPER "core_helper_version_with_override")
include(core_helper.cmake)
_qt_internal_sbom_end_project()
unset(QT_SBOM_VERSION_OVERRIDE)

# Case: Qt compat variable
set(QT_REPO_MODULE_VERSION "99")
_qt_internal_sbom_begin_project(
    SBOM_PROJECT_NAME "VersionWithQtRepoOverride"
    SUPPLIER "QtProjectTest"
    SUPPLIER_URL "https://qt-project.org/SbomTest"
)
set(CORE_HELPER "core_helper_version_with_qt_repo_override")
include(core_helper.cmake)
_qt_internal_sbom_end_project()
unset(QT_REPO_MODULE_VERSION)
