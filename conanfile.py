#############################################################################
##
## Copyright (C) 2021 The Qt Company Ltd.
## Contact: https://www.qt.io/licensing/
##
## This file is part of the release tools of the Qt Toolkit.
##
## $QT_BEGIN_LICENSE:GPL-EXCEPT$
## Commercial License Usage
## Licensees holding valid commercial Qt licenses may use this file in
## accordance with the commercial license agreement provided with the
## Software or, alternatively, in accordance with the terms contained in
## a written agreement between you and The Qt Company. For licensing terms
## and conditions see https://www.qt.io/terms-conditions. For further
## information use the contact form at https://www.qt.io/contact-us.
##
## GNU General Public License Usage
## Alternatively, this file may be used under the terms of the GNU
## General Public License version 3 as published by the Free Software
## Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
## included in the packaging of this file. Please review the following
## information to ensure the GNU General Public License requirements will
## be met: https://www.gnu.org/licenses/gpl-3.0.html.
##
## $QT_END_LICENSE$
##
#############################################################################

from conans import ConanFile, tools, Options
from conans.errors import ConanInvalidConfiguration
import os
import re
import json
import subprocess
from functools import lru_cache
from pathlib import Path
from typing import Dict, List, Any, Optional


class QtConanError(Exception):
    pass


class QtConfigureOption(object):
    def __init__(self, name: str, type: str, values: List[Any], default: Any):
        self.name = name
        self.type = type
        self.conan_option_name = self.convert_to_conan_option_name(name)

        if type == "enum" and set(values) == {"yes", "no"}:
            self._binary_option = True  # matches to Conan option "yes"|"no"
            values = []
            self._prefix = "-"
            self._value_delim = ""
        elif "string" in type.lower() or type in ["enum", "cxxstd", "coverage", "sanitize"]:
            # these options have a value, e.g.
            #  --zlib=qt  (enum type)
            #  --c++std=c++17  (cxxstd type)
            #  --prefix=/foo
            self._binary_option = False
            self._prefix = "--"
            self._value_delim = "="
            # exception to the rule
            if name == "qt-host-path":
                self._prefix = "-"
                self._value_delim = " "
        else:
            # e.g. -debug (void type)
            self._binary_option = True
            self._prefix = "-"
            self._value_delim = ""

        if not self._binary_option and not values:
            self.possible_values = ["ANY"]
        elif type == "addString":
            # -make=libs -make=examples <-> -o make="libs;examples" i.e. the possible values
            # can be randomly selected values in semicolon separated list -> "ANY"
            self.possible_values = ["ANY"]
        else:
            self.possible_values = values

        if self._binary_option and self.possible_values:
            raise QtConanError(
                "A binary option: '{0}' can not contain values: {1}".format(
                    name, self.possible_values
                )
            )

        self.default = default

    @property
    def binary_option(self) -> bool:
        return self._binary_option

    @property
    def incremental_option(self) -> bool:
        return self.type == "addString"

    @property
    def prefix(self) -> str:
        return self._prefix

    @property
    def value_delim(self) -> str:
        return self._value_delim

    def convert_to_conan_option_name(self, qt_configure_option: str) -> str:
        # e.g. '-c++std' -> '-cxxstd' or '-cmake-generator' -> 'cmake_generator'
        return qt_configure_option.lstrip("-").replace("-", "_").replace("+", "x")

    def get_conan_option_values(self) -> Any:
        # The 'None' is added as a possible value. For Conan this means it is not mandatory to pass
        # this option for the build.
        if self._binary_option:
            return ["yes", "no", None]
        if self.possible_values == ["ANY"]:
            # For 'ANY' value it can not be a List type for Conan
            return "ANY"
        return self.possible_values + [None]  # type: ignore

    def get_default_conan_option_value(self) -> Any:
        return self.default


class QtOptionParser:
    def __init__(self) -> None:
        self.options: List[QtConfigureOption] = []
        self.load_configure_options()
        self.extra_options: Dict[str, Any] = {"cmake_args_qtbase": "ANY"}
        self.extra_options_default_values = {"cmake_args_qtbase": None}

    def load_configure_options(self) -> None:
        """Read the configure options and features dynamically via configure(.bat).
        There are two contexts where the ConanFile is initialized:
          - 'conan export' i.e. when the conan package is being created from sources (.git)
          - inside conan's cache when invoking: 'conan install, conan info, conan inspect, ..'
        """
        print("QtOptionParser: load configure options ..")
        recipe_folder = Path(__file__).parent.resolve()
        configure_options = recipe_folder / "configure_options.json"
        configure_features = recipe_folder / "configure_features.txt"
        if not configure_options.exists() or not configure_features.exists():
            # This is when the 'conan export' is called
            script = Path("configure.bat") if tools.os_info.is_windows else Path("configure")
            root_path = recipe_folder

            configure = root_path.joinpath(script).resolve()
            if not configure.exists():
                root_path = root_path.joinpath("..").joinpath("export_source").resolve()
                if root_path.exists():
                    configure = root_path.joinpath(script).resolve(strict=True)
                else:
                    raise QtConanError(
                        "Unable to locate 'configure(.bat)' "
                        "from current context: {0}".format(recipe_folder)
                    )

            self.write_configure_options(configure, output_file=configure_options)
            self.write_configure_features(configure, output_file=configure_features)

        opt = self.read_configure_options(configure_options)
        self.set_configure_options(opt["options"])

        features = self.read_configure_features(configure_features)
        self.set_features(feature_name_prefix="feature-", features=features)

    def write_configure_options(self, configure: Path, output_file: Path) -> None:
        print("QtOptionParser: writing Qt configure options to: {0}".format(output_file))
        cmd = [str(configure), "-write-options-for-conan", str(output_file)]
        subprocess.run(cmd, check=True, timeout=60 * 2)

    def read_configure_options(self, input_file: Path) -> Dict[str, Any]:
        print("QtOptionParser: reading Qt configure options from: {0}".format(input_file))
        with open(str(input_file)) as f:
            return json.load(f)

    def write_configure_features(self, configure: Path, output_file: Path) -> None:
        print("QtOptionParser: writing Qt configure features to: {0}".format(output_file))
        cmd = [str(configure), "-list-features"]
        with open(output_file, "w") as f:
            subprocess.run(
                cmd,
                encoding="utf-8",
                check=True,
                timeout=60 * 2,
                stderr=subprocess.STDOUT,
                stdout=f,
            )

    def read_configure_features(self, input_file: Path) -> List[str]:
        print("QtOptionParser: reading Qt configure features from: {0}".format(input_file))
        with open(str(input_file)) as f:
            return f.readlines()

    def set_configure_options(self, configure_options: Dict[str, Any]) -> None:
        for option_name, field in configure_options.items():
            option_type = field.get("type")
            values: List[str] = field.get("values", [])
            # For the moment all Options will get 'None' as the default value
            default = None

            if not option_type:
                raise QtConanError(
                    "Qt 'configure(.bat) -write-options-for-conan' produced output "
                    "that is missing 'type'. Unable to set options dynamically. "
                    "Item: {0}".format(option_name)
                )
            if not isinstance(values, list):
                raise QtConanError("The 'values' field is not a list: {0}".format(option_name))
            if option_type == "enum" and not values:
                raise QtConanError("The enum values are missing for: {0}".format(option_name))

            opt = QtConfigureOption(
                name=option_name, type=option_type, values=values, default=default
            )
            self.options.append(opt)

    def set_features(self, feature_name_prefix: str, features: List[str]) -> None:
        for line in features:
            feature_name = self.parse_feature(line)
            if feature_name:
                opt = QtConfigureOption(
                    name=feature_name_prefix + feature_name, type="void", values=[], default=None
                )
                self.options.append(opt)

    def parse_feature(self, feature_line: str) -> Optional[str]:
        parts = feature_line.split()
        # e.g. 'itemmodel ................ ItemViews: Provides the item model for item views'
        if not len(parts) >= 3:
            return None
        if not parts[1].startswith("."):
            return None
        return parts[0]

    def get_qt_conan_options(self) -> Dict[str, Any]:
        # obtain all the possible configure(.bat) options and map those to
        # Conan options for the recipe
        opt: Dict = {}
        for qt_option in self.options:
            opt[qt_option.conan_option_name] = qt_option.get_conan_option_values()
        opt.update(self.extra_options)
        return opt

    def get_default_qt_conan_options(self) -> Dict[str, Any]:
        # set the default option values for each option in case the user or CI does not pass them
        opt: Dict = {}
        for qt_option in self.options:
            opt[qt_option.conan_option_name] = qt_option.get_default_conan_option_value()
        opt.update(self.extra_options_default_values)
        return opt

    def is_used_option(self, conan_option_value: str) -> bool:
        # conan install ... -o release=no  -> configure(.bat)
        # conan install ...                -> configure(.bat)
        # conan install ... -o release=yes -> configure(.bat) -release
        if not conan_option_value or conan_option_value == "None" or conan_option_value == "no":
            # Conan seems to convert None to literal 'None'?
            return False
        return True

    def convert_conan_option_to_qt_option(self, name: str, value: Any) -> str:
        ret: str = ""

        def _find_qt_option(conan_option_name: str) -> QtConfigureOption:
            for qt_opt in self.options:
                if conan_option_name == qt_opt.conan_option_name:
                    return qt_opt
            else:
                raise QtConanError(
                    "Could not find a matching Qt configure option for: {0}".format(
                        conan_option_name
                    )
                )

        def _is_excluded_from_configure() -> bool:
            # extra options are not Qt configure(.bat) options but those exist as
            # conan recipe options which are treated outside Qt's configure(.bat)
            if name in self.extra_options.keys():
                return True
            return False

        if self.is_used_option(value) and not _is_excluded_from_configure():
            qt_option = _find_qt_option(name)
            if qt_option.incremental_option:
                # e.g. -make=libs -make=examples <-> -o make=libs;examples;foo;bar
                _opt = qt_option.prefix + qt_option.name + qt_option.value_delim
                ret = " ".join(_opt + item.strip() for item in value.split(";") if item.strip())
            else:
                ret = qt_option.prefix + qt_option.name
                if not qt_option.binary_option:
                    ret += qt_option.value_delim + value

        return ret

    def convert_conan_options_to_qt_options(self, conan_options: Options) -> List[str]:
        qt_options: List[str] = []

        def _option_enabled(options: Dict[str, Any], opt: str) -> bool:
            return opt in options and options[opt] == "yes"

        def _option_disabled(options: Dict[str, Any], opt: str) -> bool:
            return opt in options and options[opt] == "no"

        def _filter_overlapping_options(options: Dict[str, Any]) -> None:
            if _option_enabled(options, "shared") or _option_disabled(options, "static"):
                del _options["static"]  # should result only into "-shared"
            if _option_enabled(options, "static") or _option_disabled(options, "shared"):
                del _options["shared"]  # should result only into "-static"

        _options = {key: value for key, value in conan_options.items()}
        _filter_overlapping_options(_options)

        for option_name, option_value in _options.items():
            qt_option = self.convert_conan_option_to_qt_option(
                name=option_name, value=option_value
            )
            if not qt_option:
                continue
            qt_options.append(qt_option)
        return qt_options

    def get_cmake_args_for_configure(self, conan_options: Options) -> List[Optional[str]]:
        ret: List[Optional[str]] = []
        for option_name, option_value in conan_options.items():
            if option_name == "cmake_args_qtbase" and self.is_used_option(option_value):
                ret = [ret for ret in option_value.strip(r" '\"").split()]
        return ret


def _build_qtbase(conan_file: ConanFile):
    # we call the Qt's configure(.bat) directly
    script = Path("configure.bat") if tools.os_info.is_windows else Path("configure")
    configure = Path(conan_file.build_folder).joinpath(script).resolve(strict=True)

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
    _qt_option_parser = QtOptionParser()
    options = _qt_option_parser.get_qt_conan_options()
    default_options = _qt_option_parser.get_default_qt_conan_options()
    exports = "configure_options.json", "configure_features.txt", ".cmake.conf"
    exports_sources = "*", "!conan*.*"
    # use commit ID as the RREV (recipe revision)
    revision_mode = "scm"
    python_requires = "qt-conan-common/{0}@qt/everywhere".format(_get_qt_minor_version())

    def set_version(self):
        # Executed during "conan export" i.e. in source tree
        _ver = _parse_qt_version_by_key("QT_REPO_MODULE_VERSION")
        _prerelease = _parse_qt_version_by_key("QT_REPO_MODULE_PRERELEASE_VERSION_SEGMENT")
        self.version = _ver + "-" + _prerelease if _prerelease else _ver

    def configure(self):
        if self.settings.compiler == "gcc" and tools.Version(self.settings.compiler.version) < "8":
            raise ConanInvalidConfiguration("Qt6 does not support GCC before 8")

        def _set_default_if_not_set(option_name: str, option_value: str) -> None:
            # let it fail if option name does not exist, it means the recipe is not up to date
            value = getattr(self.options, option_name)
            if not value or value == "None":
                setattr(self.options, option_name, option_value)

        default_all_os_options = [
            "release",
            "shared",
            "gui",
            "widgets",
            "accessibility",
            "system_proxies",
            "ico",
        ]
        for item in default_all_os_options:
            _set_default_if_not_set(item, "yes")

        if self.settings.os == "Windows":
            self.options.debug_and_release = "yes"
        elif self.settings.os == "Linux":
            pass
        elif self.settings.os == "Macos":
            self.options.debug_and_release = "yes"
            self.options.framework = "yes"

        if self.options.get_safe("debug_and_release", default="no") == "yes":
            # Qt build system will build both debug and release binaries
            del self.settings.build_type

        # Map Conan build_type to Qt configure(.bat) options
        if self.settings.get_safe("build_type", default="None") == "Release":
            self.options.release = "yes"
            self.options.debug = "no"
        elif self.settings.get_safe("build_type", default="None") == "Debug":
            self.options.release = "no"
            self.options.debug = "yes"
        elif self.settings.get_safe("build_type", default="None") == "MinSizeRel":
            self.options.optimize_size = "yes"
        elif self.settings.get_safe("build_type", default="None") == "RelWithDebInfo":
            self.options.force_debug_info = "yes"

    def build(self):
        self.python_requires["qt-conan-common"].module.build_env_wrap(self, _build_qtbase)

    def package(self):
        cmd = ["cmake", "--install", "."]
        self.run(" ".join(cmd))

        with open(os.path.join(self.package_folder, "bin", "qt.conf"), "w") as f:
            f.write("[Paths]\nPrefix = ..\n")

    def package_info(self):
        self.python_requires["qt-conan-common"].module.package_info(self)

    def package_id(self):
        # https://docs.conan.io/en/latest/creating_packages/define_abi_compatibility.html
        self.info.requires.package_revision_mode()
        # Enable 'qt-conan-common' updates on client side with $conan install .. --update
        self.info.python_requires.recipe_revision_mode()

        # Remove those configure(.bat) options which should not affect package_id.
        # These point to local file system paths and in order to re-use pre-built
        # binaries (by Qt CI) by others these should not affect the 'package_id'
        # as those probably differ on each machine
        rm_list = [
            "sdk",
            "android_sdk",
            "android_ndk",
            "android_ndk_platform",
            "android_abis",
            "android_javac_target",
            "android_javac_source",
            "qpa",
            "translationsdir",
            "headersclean",
        ]
        for item in rm_list:
            if item in self.info.options:
                delattr(self.info.options, item)
        # filter also those cmake options that should not end up in the package_id
        if hasattr(self.info.options, "cmake_args_qtbase"):
            _filter = self.python_requires[
                "qt-conan-common"
            ].module.filter_cmake_args_for_package_id

            self.info.options.cmake_args_qtbase = _filter(self.info.options.cmake_args_qtbase)

    def deploy(self):
        self.copy("*")  # copy from current package
        self.copy_deps("*")  # copy from dependencies
