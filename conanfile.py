# Copyright (C) 2021 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

from conans import ConanFile, tools
from conans.errors import ConanInvalidConfiguration
import os
import re
import shutil
from functools import lru_cache
from pathlib import Path
from typing import Dict, Union


class QtConanError(Exception):
    pass


def add_cmake_prefix_path(conan_file: ConanFile, dep: str) -> None:
    if dep not in conan_file.deps_cpp_info.deps:
        raise QtConanError("Unable to find dependency: {0}".format(dep))
    dep_cpp_info = conan_file.deps_cpp_info[dep]
    cmake_args_str = str(conan_file.options.get_safe("cmake_args_qtbase", default=""))
    formatted_cmake_args_str = conan_file._shared.append_cmake_arg(
        cmake_args_str, "CMAKE_PREFIX_PATH", dep_cpp_info.rootpath
    )
    print("Adjusted cmake args for qtbase build: {0}".format(formatted_cmake_args_str))
    setattr(conan_file.options, "cmake_args_qtbase", formatted_cmake_args_str)


def _build_qtbase(conan_file: ConanFile):
    # we call the Qt's configure(.bat) directly
    script = Path("configure.bat") if tools.os_info.is_windows else Path("configure")
    configure = Path(conan_file.build_folder).joinpath(script).resolve(strict=True)

    if conan_file.options.get_safe("icu", default=False):
        # we need to tell Qt build system where to find the ICU
        add_cmake_prefix_path(conan_file, dep="icu")

    # convert the Conan options to Qt configure(.bat) arguments
    parser = conan_file._qt_option_parser
    qt_configure_options = parser.convert_conan_options_to_qt_options(conan_file.options)
    cmd = " ".join(
        [str(configure), " ".join(qt_configure_options), "-prefix", conan_file.package_folder]
    )
    cmake_args = parser.get_cmake_args_for_configure(conan_file.options)
    if cmake_args:
        cmd += " -- {0}".format(" ".join(cmake_args))
    conan_file.output.info("Calling: {0}".format(cmd))
    conan_file.run(cmd)

    cmd = " ".join(["cmake", "--build", ".", "--parallel"])
    conan_file.output.info("Calling: {0}".format(cmd))
    conan_file.run(cmd)


@lru_cache(maxsize=8)
def _parse_qt_version_by_key(key: str) -> str:
    with open(Path(__file__).parent.resolve() / ".cmake.conf") as f:
        m = re.search(fr'{key} .*"(.*)"', f.read())
    return m.group(1) if m else ""


def _get_qt_minor_version() -> str:
    return ".".join(_parse_qt_version_by_key("QT_REPO_MODULE_VERSION").split(".")[:2])


class QtBase(ConanFile):
    name = "qtbase"
    license = "LGPL-3.0, GPL-2.0+, Commercial Qt License Agreement"
    author = "The Qt Company <https://www.qt.io/contact-us>"
    url = "https://code.qt.io/cgit/qt/qtbase.git"
    description = "Qt6 core framework libraries and tools."
    topics = ("qt", "qt6")
    settings = "os", "compiler", "arch", "build_type"
    _qt_option_parser = None
    options = None
    default_options = None
    exports_sources = "*", "!conan*.*"
    # use commit ID as the RREV (recipe revision)
    revision_mode = "scm"
    python_requires = "qt-conan-common/{0}@qt/everywhere".format(_get_qt_minor_version())
    short_paths = True
    _shared = None

    def init(self):
        self._shared = self.python_requires["qt-conan-common"].module
        self._qt_option_parser = self._shared.QtOptionParser(Path(__file__).parent.resolve())
        self.options = self._qt_option_parser.get_qt_conan_options()
        self.default_options = self._qt_option_parser.get_default_qt_conan_options()

    def set_version(self):
        # Executed during "conan export" i.e. in source tree
        _ver = _parse_qt_version_by_key("QT_REPO_MODULE_VERSION")
        _prerelease = _parse_qt_version_by_key("QT_REPO_MODULE_PRERELEASE_VERSION_SEGMENT")
        self.version = _ver + "-" + _prerelease if _prerelease else _ver

    def export(self):
        self.copy("configure_options.json")
        self.copy("configure_features.txt")
        self.copy(".cmake.conf")
        conf = self._shared.qt_sw_versions_config_folder() / self._shared.qt_sw_versions_config_name()
        if not conf.exists():
            # If using "conan export" outside Qt CI provisioned machines
            print("Warning: Couldn't find '{0}'. 3rd party dependencies skipped.".format(conf))
        else:
            shutil.copy2(conf, self.export_folder)

    def requirements(self):
        # list of tuples, (package_name, fallback version)
        optional_requirements = [("icu", "56.1")]
        for req_name, req_ver_fallback in optional_requirements:
            if self.options.get_safe(req_name, default=False) == True:
                # Note! If this conan package is being "conan export"ed outside Qt CI and the
                # sw versions .ini file is not present then it will fall-back to default version
                ver = self._shared.parse_qt_sw_pkg_dependency(
                    config_folder=Path(self.recipe_folder),
                    package_name=req_name,
                    target_os=str(self.settings.os),
                )
                if not ver:
                    print(
                        "Warning: Using fallback version '{0}' for: {1}".format(
                            req_name, req_ver_fallback
                        )
                    )
                    ver = req_ver_fallback
                requirement = "{0}/{1}@qt/everywhere".format(req_name, ver)
                print("Setting 3rd party package requirement: {0}".format(requirement))
                self.requires(requirement)

    def _resolve_qt_host_path(self) -> Union[str, None]:
        """
        Attempt to resolve QT_HOST_PATH.

        When cross-building the user needs to pass 'qt_host_path' which is transformed to
        QT_HOST_PATH later on. Resolve the exact path if possible.

        Returns:
            string: The resolved QT_HOST_PATH or None if unable to determine it.
        """
        _host_p = self.options.get_safe("qt_host_path")
        if _host_p:
            return str(Path(os.path.expandvars(str(_host_p))).expanduser().resolve(strict=True))
        else:
            print("Warning: 'qt_host_path' option was not given in cross-build context")
            return None

    def configure(self):
        if self.settings.compiler == "gcc" and tools.Version(self.settings.compiler.version) < "8":
            raise ConanInvalidConfiguration("Qt6 does not support GCC before 8")

        def _set_default_if_not_set(option_name: str, option_value: bool) -> None:
            # let it fail if option name does not exist, it means the recipe is not up to date
            if self.options.get_safe(option_name) in [None, "None"]:
                setattr(self.options, option_name, option_value)

        def _set_build_type(build_type: str) -> None:
            if self.settings.build_type != build_type:
                msg = (
                    "The build_type '{0}' changed to '{1}'. Please check your Settings and "
                    "Options. The used Qt options enforce '{2}' as a build_type. ".format(
                        self.settings.build_type, build_type, build_type
                    )
                )
                raise QtConanError(msg)
            self.settings.build_type = build_type

        def _check_mutually_exclusive_options(options: Dict[str, bool]) -> None:
            if list(options.values()).count(True) > 1:
                raise QtConanError(
                    "These Qt options are mutually exclusive: {0}"
                    ". Choose only one of them and try again.".format(list(options.keys()))
                )

        default_options = ["shared", "gui", "widgets", "accessibility", "system_proxies", "ico"]

        if self.settings.os == "Macos":
            default_options.append("framework")

        for item in default_options:
            _set_default_if_not_set(item, True)

        release = self.options.get_safe("release", default=False)
        debug = self.options.get_safe("debug", default=False)
        debug_and_release = self.options.get_safe("debug_and_release", default=False)
        force_debug_info = self.options.get_safe("force_debug_info", default=False)
        optimize_size = self.options.get_safe("optimize_size", default=False)

        # these options are mutually exclusive options so do a sanity check
        _check_mutually_exclusive_options(
            {"release": release, "debug": debug, "debug_and_release": debug_and_release}
        )

        # Prioritize Qt's configure options over Settings.build_type
        if debug_and_release == True:
            # Qt build system will build both debug and release binaries
            if force_debug_info == True:
                _set_build_type("RelWithDebInfo")
            else:
                _set_build_type("Release")
        elif release == True:
            _check_mutually_exclusive_options(
                {"force_debug_info": force_debug_info, "optimize_size": optimize_size}
            )
            if force_debug_info == True:
                _set_build_type("RelWithDebInfo")
            elif optimize_size == True:
                _set_build_type("MinSizeRel")
            else:
                _set_build_type("Release")
        elif debug == True:
            _set_build_type("Debug")
        else:
            # As a fallback set the build type for Qt configure based on the 'build_type'
            # defined in the conan build settings
            build_type = self.settings.get_safe("build_type")
            if build_type in [None, "None"]:
                # set default that mirror the configure(.bat) default values
                self.options.release = True
                self.settings.build_type = "Release"
            elif build_type == "Release":
                self.options.release = True
            elif build_type == "Debug":
                self.options.debug = True
            elif build_type == "RelWithDebInfo":
                self.options.release = True
                self.options.force_debug_info = True
            elif build_type == "MinSizeRel":
                self.options.release = True
                self.options.optimize_size = True
            else:
                raise QtConanError("Unknown build_type: {0}".format(self.settings.build_type))

        if self.settings.os == "Android":
            if self.options.get_safe("android_sdk_version") == None:
                cmake_args_qtbase = str(self.options.get_safe("cmake_args_qtbase"))
                sdk_ver = self._shared.parse_android_sdk_version(cmake_args_qtbase)
                if sdk_ver:
                    print("'android_sdk_version' not given. Deduced version: {0}".format(sdk_ver))
                    self.options.android_sdk_version = sdk_ver
                else:
                    # TODO, for now we have no clean means to query the Android SDK version from
                    # Qt build system so we just exclude the "android_sdk" from the package_id.
                    print("Can't deduce 'android_sdk_version'. Excluding it from 'package_id'")
                    delattr(self.info.options, "android_sdk_version")
            if self.options.get_safe("android_ndk_version") == None:
                ndk_ver = str(self.options.get_safe("android_ndk"))
                ndk_ver = self._shared.parse_android_ndk_version(Path(ndk_ver, strict=True))
                print("'android_ndk_version' not given. Deduced version: {0}".format(ndk_ver))
                self.options.android_ndk_version = ndk_ver

    def build(self):
        self._shared.build_env_wrap(self, _build_qtbase)

    def package(self):
        self._shared.call_install(self)

    def package_info(self):
        self._shared.package_info(self)
        if tools.cross_building(conanfile=self):
            qt_host_path = self._resolve_qt_host_path()
            if qt_host_path:
                self.env_info.QT_HOST_PATH.append(qt_host_path)

    def package_id(self):
        # https://docs.conan.io/en/latest/creating_packages/define_abi_compatibility.html

        # The package_revision_mode() is too strict for Qt CI. This mode includes artifacts
        # checksum in package_id which is problematic in Qt CI re-runs (re-run flaky
        # build) which contain different build timestamps (cmake) which end up in library
        # files -> different package_id.
        self.info.requires.recipe_revision_mode()

        # Enable 'qt-conan-common' updates on client side with $conan install .. --update
        self.info.python_requires.recipe_revision_mode()

        # Remove those configure(.bat) options which should not affect package_id.
        # These point to local file system paths and in order to re-use pre-built
        # binaries (by Qt CI) by others these should not affect the 'package_id'
        # as those probably differ on each machine
        rm_list = [
            "sdk",
            "qpa",
            "translationsdir",
            "headersclean",
            "qt_host_path",
            "android_sdk",
            "android_ndk",
        ]
        for item in rm_list:
            if item in self.info.options:
                delattr(self.info.options, item)
        # filter also those cmake options that should not end up in the package_id
        if hasattr(self.info.options, "cmake_args_qtbase"):
            _filter = self._shared.filter_cmake_args_for_package_id
            self.info.options.cmake_args_qtbase = _filter(self.info.options.cmake_args_qtbase)

    def deploy(self):
        self.copy("*")  # copy from current package
        self.copy_deps("*")  # copy from dependencies
