# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## QIosOptionalPlugin_NSPhotoLibrary Plugin:
#####################################################################

qt_internal_add_plugin(QIosOptionalPlugin_NSPhotoLibraryPlugin
    OUTPUT_NAME qiosnsphotolibrarysupport
    STATIC # Force static, even in shared builds
    PLUGIN_TYPE platforms/darwin
    CLASS_NAME QIosOptionalPlugin_NSPhotoLibrary
    DEFAULT_IF FALSE
    SOURCES
        plugin.mm
        qiosfileengineassetslibrary.h qiosfileengineassetslibrary.mm
        qiosfileenginefactory.h
        qiosimagepickercontroller.h qiosimagepickercontroller.mm
    LIBRARIES
        ${FWAssetsLibrary}
        ${FWFoundation}
        ${FWUIKit}
        Qt::Core
        Qt::Gui
        Qt::GuiPrivate
)

set_target_properties(QIosOptionalPlugin_NSPhotoLibraryPlugin
    PROPERTIES
        DISABLE_PRECOMPILE_HEADERS ON
)
