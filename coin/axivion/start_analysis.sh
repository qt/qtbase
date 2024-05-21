#!/bin/bash
# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

$HOME/bauhaus-suite/setup.sh --non-interactive
export PATH=/home/qt/bauhaus-suite/bin:$PATH
export BAUHAUS_CONFIG=$(cd $(dirname $(readlink -f $0)) && pwd)
export AXIVION_VERSION_NAME=$(git rev-parse HEAD)
export CAFECC_BASEPATH="/home/qt/work/qt/$TESTED_MODULE_COIN"
gccsetup --cc gcc --cxx g++ --config "$BAUHAUS_CONFIG"
cd "$CAFECC_BASEPATH"
BAUHAUS_IR_COMPRESSION=none COMPILE_ONLY=1 cmake -G Ninja -DAXIVION_ANALYSIS_TOOLCHAIN_FILE=/home/qt/bauhaus-suite/profiles/cmake/axivion-launcher-toolchain.cmake -DCMAKE_PREFIX_PATH=/home/qt/work/qt/qtbase/build -DCMAKE_PROJECT_INCLUDE_BEFORE=/home/qt/bauhaus-suite/profiles/cmake/axivion-before-project-hook.cmake -B build -S . --fresh
cmake --build build -j4
for MODULE in qtconcurrent qtcore qtdbus qtgui qtnetwork qtopengl qtopenglwidgets qtsql qttest qtwidgets qtprintsupport qtxml; do
    export MODULE
    export PLUGINS=""
    export IRNAME=build/$MODULE.ir
    if [ "$MODULE" == "qtconcurrent" ]
    then
        export TARGET_NAME="build/lib/libQt6Concurrent.so.*.ir"
        export EXCLUDE_FILES="build/*:src/3rdparty/*:src/testlib/*:src/corelib/*"
        export PACKAGE="Add-ons"
    elif [ "$MODULE" == "qtcore" ]
    then
        export TARGET_NAME="build/lib/libQt6Core.so.*.ir"
        export EXCLUDE_FILES="build/*:src/3rdparty/*:src/testlib/*"
        export PACKAGE="Essentials"
    elif [ "$MODULE" == "qtdbus" ]
    then
        export TARGET_NAME="build/lib/libQt6DBus.so.*.ir"
        export EXCLUDE_FILES="build/*:src/3rdparty/*:src/testlib/*:src/corelib/*"
        export PACKAGE="Essentials"
    elif [ "$MODULE" == "qtgui" ]
    then
        export TARGET_NAME="build/lib/libQt6Gui.so.*.ir"
        export PLUGINS="build/plugins/platforms/*.so.ir:build/plugins/platforminputcontexts/*.so.ir:build/plugins/platformthemes/*.so.ir:build/plugins/imageformats/*.so.ir:build/plugins/generic/*.so.ir"
        export EXCLUDE_FILES="build/*:src/3rdparty/*:src/testlib/*:src/corelib/*:src/dbus/*:src/opengl/*:src/network/*"
        export PACKAGE="Essentials"
    elif [ "$MODULE" == "qtnetwork" ]
    then
        export TARGET_NAME="build/lib/libQt6Network.so.*.ir"
        export PLUGINS="build/plugins/networkinformation/*.so.ir:build/plugins/tracing/*.so.ir:build/plugins/tls/*.so.ir"
        export EXCLUDE_FILES="build/*:src/3rdparty/*:src/testlib/*:src/corelib/*:src/dbus/*"
        export PACKAGE="Essentials"
    elif [ "$MODULE" == "qtopengl" ]
    then
        export TARGET_NAME="build/lib/libQt6OpenGL.so.*.ir"
        export EXCLUDE_FILES="build/*:src/3rdparty/*:src/corelib/*:src/gui/*:src/dbus/*:src/opengl/qopenglfunctions_*"
        export PACKAGE="Add-ons"
    elif [ "$MODULE" == "qtopenglwidgets" ]
    then
        export TARGET_NAME="build/lib/libQt6OpenGLWidgets.so.*.ir"
        export EXCLUDE_FILES="build/*:src/3rdparty/*:src/corelib/*:src/gui/*:src/dbus/*:src/opengl/qopenglfunctions_*"
        export PACKAGE="Add-ons"
    elif [ "$MODULE" == "qtsql" ]
    then
        export TARGET_NAME="build/lib/libQt6Sql.so.*.ir"
        export PLUGINS="build/plugins/sqldrivers/*.so.ir"
        export EXCLUDE_FILES="build/*:src/3rdparty/*:src/corelib/*:src/gui/*:src/testlib/*"
        export PACKAGE="Add-ons"
    elif [ "$MODULE" == "qttest" ]
    then
        export TARGET_NAME="build/lib/libQt6Test.so.*.ir"
        export EXCLUDE_FILES="build/*:src/3rdparty/*:src/corelib/*:src/gui/*:src/widgets/*:src/testlib/3rdparty/*"
        export PACKAGE="Essentials"
    elif [ "$MODULE" == "qtwidgets" ]
    then
        export TARGET_NAME="build/lib/libQt6Widgets.so.*.ir"
        export EXCLUDE_FILES="build/*:src/3rdparty/*:src/corelib/*:src/dbus/*:src/gui/*:src/testlib/*"
        export PACKAGE="Essentials"
    elif [ "$MODULE" == "qtprintsupport" ]
    then
        export TARGET_NAME="build/lib/libQt6PrintSupport.so.*.ir"
        export PLUGINS="build/plugins/printsupport/*.so.ir"
        export EXCLUDE_FILES="build/*:src/3rdparty/*:src/corelib/*:src/gui/*:src/widgets/*:src/testlib/*:src/dbus/*"
        export PACKAGE="Add-ons"
    elif [ "$MODULE" == "qtxml" ]
    then
        export TARGET_NAME="build/lib/libQt6Xml.so.*.ir"
        export EXCLUDE_FILES="build/*:src/3rdparty/*:src/corelib/*:src/testlib/*"
        export PACKAGE="Add-ons"
    fi
    axivion_ci "$@"
done
