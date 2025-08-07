# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## Widgets Module:
#####################################################################

qt_internal_add_module(Widgets
    QMAKE_MODULE_CONFIG uic
    PLUGIN_TYPES styles
    SOURCES
        compat/removed_api.cpp
        itemviews/qfileiconprovider.cpp itemviews/qfileiconprovider.h itemviews/qfileiconprovider_p.h
        kernel/qapplication.cpp kernel/qapplication.h kernel/qapplication_p.h
        kernel/qboxlayout.cpp kernel/qboxlayout.h
        kernel/qgesture.cpp kernel/qgesture.h kernel/qgesture_p.h
        kernel/qgesturemanager.cpp kernel/qgesturemanager_p.h
        kernel/qgesturerecognizer.cpp kernel/qgesturerecognizer.h
        kernel/qgridlayout.cpp kernel/qgridlayout.h
        kernel/qlayout.cpp kernel/qlayout.h kernel/qlayout_p.h
        kernel/qlayoutengine.cpp kernel/qlayoutengine_p.h
        kernel/qlayoutitem.cpp kernel/qlayoutitem.h
        kernel/qrhiwidget.cpp kernel/qrhiwidget.h kernel/qrhiwidget_p.h
        kernel/qsizepolicy.cpp kernel/qsizepolicy.h
        kernel/qstackedlayout.cpp kernel/qstackedlayout.h
        kernel/qstandardgestures.cpp kernel/qstandardgestures_p.h
        kernel/qtestsupport_widgets.cpp kernel/qtestsupport_widgets.h
        kernel/qtwidgetsglobal.h kernel/qtwidgetsglobal_p.h
        kernel/qwidget.cpp kernel/qwidget.h kernel/qwidget_p.h
        kernel/qwidgetrepaintmanager.cpp kernel/qwidgetrepaintmanager_p.h
        kernel/qwidgetsvariant.cpp
        kernel/qwidgetwindow.cpp kernel/qwidgetwindow_p.h
        kernel/qwindowcontainer.cpp kernel/qwindowcontainer_p.h
        styles/qcommonstyle.cpp styles/qcommonstyle.h styles/qcommonstyle_p.h
        styles/qcommonstylepixmaps_p.h
        styles/qdrawutil.cpp styles/qdrawutil.h
        styles/qpixmapstyle.cpp styles/qpixmapstyle_p.h
        styles/qpixmapstyle_p_p.h
        styles/qproxystyle.cpp styles/qproxystyle.h styles/qproxystyle_p.h
        styles/qstyle.cpp styles/qstyle.h styles/qstyle_p.h
        styles/qstylefactory.cpp styles/qstylefactory.h
        styles/qstylehelper.cpp styles/qstylehelper_p.h
        styles/qstyleoption.cpp styles/qstyleoption.h
        styles/qstylepainter.cpp styles/qstylepainter.h
        styles/qstyleplugin.cpp styles/qstyleplugin.h
        util/qcolormap.cpp util/qcolormap.h
        util/qsystemtrayicon.cpp util/qsystemtrayicon.h util/qsystemtrayicon_p.h
        widgets/qabstractscrollarea.cpp widgets/qabstractscrollarea.h widgets/qabstractscrollarea_p.h
        widgets/qfocusframe.cpp widgets/qfocusframe.h
        widgets/qframe.cpp widgets/qframe.h widgets/qframe_p.h
        widgets/qwidgetanimator.cpp widgets/qwidgetanimator_p.h
    DEFINES
        QT_NO_CONTEXTLESS_CONNECT
        QT_NO_USING_NAMESPACE
        QT_NO_FOREACH
        QT_USE_NODISCARD_FILE_OPEN
    INCLUDE_DIRECTORIES
        dialogs
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
    PRIVATE_MODULE_INTERFACE
        Qt::CorePrivate
        Qt::GuiPrivate
    PRECOMPILED_HEADER
        "kernel/qt_widgets_pch.h"
    NO_PCH_SOURCES
        compat/removed_api.cpp
    ELF_LINKER_DYNAMIC_LIST
        QtWidgets.dynlist.in
)

qt_internal_extend_target(Widgets CONDITION APPLE
    NO_UNITY_BUILD_SOURCES
        kernel/qapplication.cpp
)

# Resources:
set(qstyle_resource_files
    "styles/images/arrow-down-128.png"
    "styles/images/arrow-down-16.png"
    "styles/images/arrow-down-32.png"
    "styles/images/arrow-left-128.png"
    "styles/images/arrow-left-16.png"
    "styles/images/arrow-left-32.png"
    "styles/images/arrow-right-128.png"
    "styles/images/arrow-right-16.png"
    "styles/images/arrow-right-32.png"
    "styles/images/arrow-up-128.png"
    "styles/images/arrow-up-16.png"
    "styles/images/arrow-up-32.png"
    "styles/images/cdr-128.png"
    "styles/images/cdr-16.png"
    "styles/images/cdr-32.png"
    "styles/images/cleartext-128.png"
    "styles/images/cleartext-16.png"
    "styles/images/cleartext-32.png"
    "styles/images/closedock-10.png"
    "styles/images/closedock-16.png"
    "styles/images/closedock-20.png"
    "styles/images/closedock-32.png"
    "styles/images/closedock-48.png"
    "styles/images/closedock-64.png"
    "styles/images/computer-16.png"
    "styles/images/computer-32.png"
    "styles/images/critical-128.png"
    "styles/images/critical-16.png"
    "styles/images/critical-32.png"
    "styles/images/desktop-16.png"
    "styles/images/desktop-32.png"
    "styles/images/dirclosed-128.png"
    "styles/images/dirclosed-16.png"
    "styles/images/dirclosed-32.png"
    "styles/images/dirlink-128.png"
    "styles/images/dirlink-16.png"
    "styles/images/dirlink-32.png"
    "styles/images/diropen-128.png"
    "styles/images/diropen-16.png"
    "styles/images/diropen-32.png"
    "styles/images/dvd-128.png"
    "styles/images/dvd-16.png"
    "styles/images/dvd-32.png"
    "styles/images/file-128.png"
    "styles/images/file-16.png"
    "styles/images/file-32.png"
    "styles/images/filecontents-128.png"
    "styles/images/filecontents-16.png"
    "styles/images/filecontents-32.png"
    "styles/images/filedialog_end-128.png"
    "styles/images/filedialog_end-16.png"
    "styles/images/filedialog_end-32.png"
    "styles/images/filedialog_start-128.png"
    "styles/images/filedialog_start-16.png"
    "styles/images/filedialog_start-32.png"
    "styles/images/fileinfo-128.png"
    "styles/images/fileinfo-16.png"
    "styles/images/fileinfo-32.png"
    "styles/images/filelink-128.png"
    "styles/images/filelink-16.png"
    "styles/images/filelink-32.png"
    "styles/images/floppy-128.png"
    "styles/images/floppy-16.png"
    "styles/images/floppy-32.png"
    "styles/images/fontbitmap-16.png"
    "styles/images/fonttruetype-16.png"
    "styles/images/fusion_groupbox.png"
    "styles/images/harddrive-128.png"
    "styles/images/harddrive-16.png"
    "styles/images/harddrive-32.png"
    "styles/images/information-128.png"
    "styles/images/information-16.png"
    "styles/images/information-32.png"
    "styles/images/media-pause-128.png"
    "styles/images/media-pause-16.png"
    "styles/images/media-pause-32.png"
    "styles/images/media-play-128.png"
    "styles/images/media-play-16.png"
    "styles/images/media-play-32.png"
    "styles/images/media-seek-backward-128.png"
    "styles/images/media-seek-backward-16.png"
    "styles/images/media-seek-backward-32.png"
    "styles/images/media-seek-forward-128.png"
    "styles/images/media-seek-forward-16.png"
    "styles/images/media-seek-forward-32.png"
    "styles/images/media-skip-backward-128.png"
    "styles/images/media-skip-backward-16.png"
    "styles/images/media-skip-backward-32.png"
    "styles/images/media-skip-forward-128.png"
    "styles/images/media-skip-forward-16.png"
    "styles/images/media-skip-forward-32.png"
    "styles/images/media-stop-128.png"
    "styles/images/media-stop-16.png"
    "styles/images/media-stop-32.png"
    "styles/images/media-volume-16.png"
    "styles/images/media-volume-32.png"
    "styles/images/media-volume-128.png"
    "styles/images/media-volume-muted-16.png"
    "styles/images/media-volume-muted-32.png"
    "styles/images/media-volume-muted-128.png"
    "styles/images/networkdrive-128.png"
    "styles/images/networkdrive-16.png"
    "styles/images/networkdrive-32.png"
    "styles/images/newdirectory-128.png"
    "styles/images/newdirectory-16.png"
    "styles/images/newdirectory-32.png"
    "styles/images/normalizedockup-10.png"
    "styles/images/normalizedockup-16.png"
    "styles/images/normalizedockup-20.png"
    "styles/images/normalizedockup-32.png"
    "styles/images/normalizedockup-48.png"
    "styles/images/normalizedockup-64.png"
    "styles/images/parentdir-128.png"
    "styles/images/parentdir-16.png"
    "styles/images/parentdir-32.png"
    "styles/images/question-128.png"
    "styles/images/question-16.png"
    "styles/images/question-32.png"
    "styles/images/refresh-24.png"
    "styles/images/refresh-32.png"
    "styles/images/standardbutton-apply-128.png"
    "styles/images/standardbutton-apply-16.png"
    "styles/images/standardbutton-apply-32.png"
    "styles/images/standardbutton-cancel-128.png"
    "styles/images/standardbutton-cancel-16.png"
    "styles/images/standardbutton-cancel-32.png"
    "styles/images/standardbutton-clear-128.png"
    "styles/images/standardbutton-clear-16.png"
    "styles/images/standardbutton-clear-32.png"
    "styles/images/standardbutton-close-128.png"
    "styles/images/standardbutton-close-16.png"
    "styles/images/standardbutton-close-32.png"
    "styles/images/standardbutton-closetab-128.png"
    "styles/images/standardbutton-closetab-16.png"
    "styles/images/standardbutton-closetab-32.png"
    "styles/images/standardbutton-closetab-down-128.png"
    "styles/images/standardbutton-closetab-down-16.png"
    "styles/images/standardbutton-closetab-down-32.png"
    "styles/images/standardbutton-closetab-hover-128.png"
    "styles/images/standardbutton-closetab-hover-16.png"
    "styles/images/standardbutton-closetab-hover-32.png"
    "styles/images/standardbutton-delete-128.png"
    "styles/images/standardbutton-delete-16.png"
    "styles/images/standardbutton-delete-32.png"
    "styles/images/standardbutton-help-128.png"
    "styles/images/standardbutton-help-16.png"
    "styles/images/standardbutton-help-32.png"
    "styles/images/standardbutton-no-128.png"
    "styles/images/standardbutton-no-16.png"
    "styles/images/standardbutton-no-32.png"
    "styles/images/standardbutton-ok-128.png"
    "styles/images/standardbutton-ok-16.png"
    "styles/images/standardbutton-ok-32.png"
    "styles/images/standardbutton-open-128.png"
    "styles/images/standardbutton-open-16.png"
    "styles/images/standardbutton-open-32.png"
    "styles/images/standardbutton-save-128.png"
    "styles/images/standardbutton-save-16.png"
    "styles/images/standardbutton-save-32.png"
    "styles/images/standardbutton-yes-128.png"
    "styles/images/standardbutton-yes-16.png"
    "styles/images/standardbutton-yes-32.png"
    "styles/images/stop-24.png"
    "styles/images/stop-32.png"
    "styles/images/titlebar-contexthelp-16.png"
    "styles/images/titlebar-contexthelp-32.png"
    "styles/images/titlebar-contexthelp-48.png"
    "styles/images/titlebar-max-16.png"
    "styles/images/titlebar-max-32.png"
    "styles/images/titlebar-max-48.png"
    "styles/images/titlebar-min-16.png"
    "styles/images/titlebar-min-32.png"
    "styles/images/titlebar-min-48.png"
    "styles/images/titlebar-shade-16.png"
    "styles/images/titlebar-shade-32.png"
    "styles/images/titlebar-shade-48.png"
    "styles/images/titlebar-unshade-16.png"
    "styles/images/titlebar-unshade-32.png"
    "styles/images/titlebar-unshade-48.png"
    "styles/images/toolbar-ext-h-128.png"
    "styles/images/toolbar-ext-h-16.png"
    "styles/images/toolbar-ext-h-32.png"
    "styles/images/toolbar-ext-h-8.png"
    "styles/images/toolbar-ext-h-rtl-128.png"
    "styles/images/toolbar-ext-h-rtl-16.png"
    "styles/images/toolbar-ext-h-rtl-32.png"
    "styles/images/toolbar-ext-h-rtl-8.png"
    "styles/images/toolbar-ext-v-10.png"
    "styles/images/toolbar-ext-v-20.png"
    "styles/images/toolbar-ext-v-80.png"
    "styles/images/toolbar-ext-v-5.png"
    "styles/images/trash-128.png"
    "styles/images/trash-16.png"
    "styles/images/trash-32.png"
    "styles/images/viewdetailed-128.png"
    "styles/images/viewdetailed-16.png"
    "styles/images/viewdetailed-32.png"
    "styles/images/viewlist-128.png"
    "styles/images/viewlist-16.png"
    "styles/images/viewlist-32.png"
    "styles/images/warning-128.png"
    "styles/images/warning-16.png"
    "styles/images/warning-32.png"
)

qt_internal_add_resource(Widgets "qstyle"
    PREFIX
        "/qt-project.org/styles/commonstyle"
    BASE
        "styles"
    FILES
        ${qstyle_resource_files}
)
set(qstyle1_resource_files
    "styles/images/closedock-down-macstyle-10.png"
    "styles/images/closedock-down-macstyle-16.png"
    "styles/images/closedock-down-macstyle-20.png"
    "styles/images/closedock-down-macstyle-32.png"
    "styles/images/closedock-down-macstyle-48.png"
    "styles/images/closedock-down-macstyle-64.png"
    "styles/images/closedock-macstyle-10.png"
    "styles/images/closedock-macstyle-16.png"
    "styles/images/closedock-macstyle-20.png"
    "styles/images/closedock-macstyle-32.png"
    "styles/images/closedock-macstyle-48.png"
    "styles/images/closedock-macstyle-64.png"
    "styles/images/dockdock-down-macstyle-10.png"
    "styles/images/dockdock-down-macstyle-16.png"
    "styles/images/dockdock-down-macstyle-20.png"
    "styles/images/dockdock-down-macstyle-32.png"
    "styles/images/dockdock-down-macstyle-48.png"
    "styles/images/dockdock-down-macstyle-64.png"
    "styles/images/dockdock-macstyle-10.png"
    "styles/images/dockdock-macstyle-16.png"
    "styles/images/dockdock-macstyle-20.png"
    "styles/images/dockdock-macstyle-32.png"
    "styles/images/dockdock-macstyle-48.png"
    "styles/images/dockdock-macstyle-64.png"
    "styles/images/toolbar-ext-macstyle.png"
    "styles/images/toolbar-ext-macstyle@2x.png"
)

qt_internal_add_resource(Widgets "qstyle1"
    PREFIX
        "/qt-project.org/styles/macstyle"
    BASE
        "styles"
    FILES
        ${qstyle1_resource_files}
)

set(qstyle_resource_fusion_files
    "styles/images/fusion_closedock-10.png"
    "styles/images/fusion_closedock-16.png"
    "styles/images/fusion_closedock-20.png"
    "styles/images/fusion_closedock-32.png"
    "styles/images/fusion_closedock-48.png"
    "styles/images/fusion_closedock-64.png"
    "styles/images/fusion_normalizedockup_10.png"
    "styles/images/fusion_normalizedockup-16.png"
    "styles/images/fusion_normalizedockup_20.png"
    "styles/images/fusion_normalizedockup-32.png"
    "styles/images/fusion_normalizedockup_48.png"
    "styles/images/fusion_normalizedockup_64.png"
    "styles/images/fusion_titlebar-min-10.png"
    "styles/images/fusion_titlebar-min-16.png"
    "styles/images/fusion_titlebar-min-20.png"
    "styles/images/fusion_titlebar-min-32.png"
    "styles/images/fusion_titlebar-min-48.png"
    "styles/images/fusion_titlebar-min-64.png"
)

qt_internal_add_resource(Widgets "qstyle_fusion"
    PREFIX
        "/qt-project.org/styles/fusionstyle"
    BASE
        "styles"
    FILES
        ${qstyle_resource_fusion_files}
)

## Scopes:
#####################################################################

qt_internal_extend_target(Widgets CONDITION MSVC AND (TEST_architecture_arch STREQUAL "i386")
    LINK_OPTIONS
        "/BASE:0x65000000"
)

qt_internal_extend_target(Widgets CONDITION MACOS
    SOURCES
        kernel/qmacgesturerecognizer.cpp kernel/qmacgesturerecognizer_p.h
    LIBRARIES
        ${FWAppKit}
        z
)

qt_internal_extend_target(Widgets CONDITION WIN32
    LIBRARIES
        dwmapi
        shell32
        uxtheme
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_graphicseffect
    SOURCES
        effects/qgraphicseffect.cpp effects/qgraphicseffect.h effects/qgraphicseffect_p.h
        effects/qpixmapfilter.cpp effects/qpixmapfilter_p.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_action
    SOURCES
        kernel/qaction.h
        kernel/qaction_widgets.cpp kernel/qaction_widgets_p.h
        kernel/qactiongroup.h
        kernel/qwidgetaction.cpp kernel/qwidgetaction.h kernel/qwidgetaction_p.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_formlayout
    SOURCES
        kernel/qformlayout.cpp kernel/qformlayout.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_shortcut
    SOURCES
        kernel/qshortcut.h
        kernel/qshortcut_widgets.cpp
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_tooltip
    SOURCES
        kernel/qtooltip.cpp kernel/qtooltip.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_whatsthis
    SOURCES
        kernel/qwhatsthis.cpp kernel/qwhatsthis.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_animation
    SOURCES
        styles/qstyleanimation.cpp styles/qstyleanimation_p.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_style_stylesheet
    SOURCES
        styles/qstylesheetstyle.cpp styles/qstylesheetstyle_p.h
        styles/qstylesheetstyle_default.cpp
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_style_windows
    SOURCES
        styles/qwindowsstyle.cpp styles/qwindowsstyle_p.h
        styles/qwindowsstyle_p_p.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_style_fusion
    SOURCES
        styles/qfusionstyle.cpp styles/qfusionstyle_p.h
        styles/qfusionstyle_p_p.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_abstractbutton
    SOURCES
        widgets/qabstractbutton.cpp widgets/qabstractbutton.h widgets/qabstractbutton_p.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_abstractslider
    SOURCES
        widgets/qabstractslider.cpp widgets/qabstractslider.h widgets/qabstractslider_p.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_buttongroup
    SOURCES
        widgets/qbuttongroup.cpp widgets/qbuttongroup.h widgets/qbuttongroup_p.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_calendarwidget
    SOURCES
        widgets/qcalendarwidget.cpp widgets/qcalendarwidget.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_checkbox
    SOURCES
        widgets/qcheckbox.cpp widgets/qcheckbox.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_combobox
    SOURCES
        widgets/qcombobox.cpp widgets/qcombobox.h widgets/qcombobox_p.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_commandlinkbutton
    SOURCES
        widgets/qcommandlinkbutton.cpp widgets/qcommandlinkbutton.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_datetimeedit
    SOURCES
        widgets/qdatetimeedit.cpp widgets/qdatetimeedit.h widgets/qdatetimeedit_p.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_dial
    SOURCES
        widgets/qdial.cpp widgets/qdial.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_dockwidget
    SOURCES
        widgets/qdockarealayout.cpp widgets/qdockarealayout_p.h
        widgets/qdockwidget.cpp widgets/qdockwidget.h widgets/qdockwidget_p.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_effects
    SOURCES
        widgets/qeffects.cpp widgets/qeffects_p.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_fontcombobox
    SOURCES
        widgets/qfontcombobox.cpp widgets/qfontcombobox.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_groupbox
    SOURCES
        widgets/qgroupbox.cpp widgets/qgroupbox.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_keysequenceedit
    SOURCES
        widgets/qkeysequenceedit.cpp widgets/qkeysequenceedit.h widgets/qkeysequenceedit_p.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_label
    SOURCES
        widgets/qlabel.cpp widgets/qlabel.h widgets/qlabel_p.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_lcdnumber
    SOURCES
        widgets/qlcdnumber.cpp widgets/qlcdnumber.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_lineedit
    SOURCES
        widgets/qlineedit.cpp widgets/qlineedit.h widgets/qlineedit_p.cpp widgets/qlineedit_p.h
        widgets/qwidgetlinecontrol.cpp widgets/qwidgetlinecontrol_p.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_mainwindow
    SOURCES
        widgets/qmainwindow.cpp widgets/qmainwindow.h
        widgets/qmainwindowlayout.cpp widgets/qmainwindowlayout_p.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_mdiarea
    SOURCES
        widgets/qmdiarea.cpp widgets/qmdiarea.h widgets/qmdiarea_p.h
        widgets/qmdisubwindow.cpp widgets/qmdisubwindow.h widgets/qmdisubwindow_p.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_menu
    SOURCES
        widgets/qmenu.cpp widgets/qmenu.h widgets/qmenu_p.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_menubar
    SOURCES
        widgets/qmenubar.cpp widgets/qmenubar.h widgets/qmenubar_p.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_progressbar
    SOURCES
        widgets/qprogressbar.cpp widgets/qprogressbar.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_pushbutton
    SOURCES
        widgets/qpushbutton.cpp widgets/qpushbutton.h widgets/qpushbutton_p.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_radiobutton
    SOURCES
        widgets/qradiobutton.cpp widgets/qradiobutton.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_resizehandler
    SOURCES
        widgets/qwidgetresizehandler.cpp widgets/qwidgetresizehandler_p.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_dialogbuttonbox
    SOURCES
        widgets/qdialogbuttonbox.cpp widgets/qdialogbuttonbox.h widgets/qdialogbuttonbox_p.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_rubberband
    SOURCES
        widgets/qrubberband.cpp widgets/qrubberband.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_scrollarea
    SOURCES
        widgets/qscrollarea.cpp widgets/qscrollarea.h widgets/qscrollarea_p.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_scrollbar
    SOURCES
        widgets/qscrollbar.cpp widgets/qscrollbar.h widgets/qscrollbar_p.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_sizegrip
    SOURCES
        widgets/qsizegrip.cpp widgets/qsizegrip.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_slider
    SOURCES
        widgets/qslider.cpp widgets/qslider.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_spinbox
    SOURCES
        widgets/qabstractspinbox.cpp widgets/qabstractspinbox.h widgets/qabstractspinbox_p.h
        widgets/qspinbox.cpp widgets/qspinbox.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_splashscreen
    SOURCES
        widgets/qsplashscreen.cpp widgets/qsplashscreen.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_splitter
    SOURCES
        widgets/qsplitter.cpp widgets/qsplitter.h widgets/qsplitter_p.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_stackedwidget
    SOURCES
        widgets/qstackedwidget.cpp widgets/qstackedwidget.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_statusbar
    SOURCES
        widgets/qstatusbar.cpp widgets/qstatusbar.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_tabbar
    SOURCES
        widgets/qtabbar.cpp widgets/qtabbar.h widgets/qtabbar_p.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_textedit
    SOURCES
        widgets/qplaintextedit.cpp widgets/qplaintextedit.h widgets/qplaintextedit_p.h
        widgets/qtextedit.cpp widgets/qtextedit.h widgets/qtextedit_p.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_textbrowser
    SOURCES
        widgets/qtextbrowser.cpp widgets/qtextbrowser.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_tabwidget
    SOURCES
        widgets/qtabwidget.cpp widgets/qtabwidget.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_toolbar
    SOURCES
        widgets/qtoolbar.cpp widgets/qtoolbar.h widgets/qtoolbar_p.h
        widgets/qtoolbararealayout.cpp widgets/qtoolbararealayout_p.h
        widgets/qtoolbarlayout.cpp widgets/qtoolbarlayout_p.h
        widgets/qtoolbarseparator.cpp widgets/qtoolbarseparator_p.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_toolbox
    SOURCES
        widgets/qtoolbox.cpp widgets/qtoolbox.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_toolbutton
    SOURCES
        widgets/qtoolbarextension.cpp widgets/qtoolbarextension_p.h
        widgets/qtoolbutton.cpp widgets/qtoolbutton.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_widgettextcontrol
    SOURCES
        widgets/qwidgettextcontrol.cpp widgets/qwidgettextcontrol_p.h
        widgets/qwidgettextcontrol_p_p.h
)

qt_internal_extend_target(Widgets CONDITION MACOS AND (QT_FEATURE_menu OR QT_FEATURE_menubar)
    SOURCES
        widgets/qmenu_mac.mm
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_colordialog
    SOURCES
        dialogs/qcolordialog.cpp dialogs/qcolordialog.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_dialog
    SOURCES
        dialogs/qdialog.cpp dialogs/qdialog.h dialogs/qdialog_p.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_errormessage
    SOURCES
        dialogs/qerrormessage.cpp dialogs/qerrormessage.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_filedialog
    SOURCES
        dialogs/qfiledialog.cpp dialogs/qfiledialog.h dialogs/qfiledialog.ui dialogs/qfiledialog_p.h
        dialogs/qsidebar.cpp dialogs/qsidebar_p.h
    ENABLE_AUTOGEN_TOOLS
        uic
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_filesystemmodel
    SOURCES
        dialogs/qfilesystemmodel.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_fontdialog
    SOURCES
        dialogs/qfontdialog.cpp dialogs/qfontdialog.h dialogs/qfontdialog_p.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_fscompleter
    SOURCES
        dialogs/qfscompleter_p.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_inputdialog
    SOURCES
        dialogs/qinputdialog.cpp dialogs/qinputdialog.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_messagebox
    SOURCES
        dialogs/qmessagebox.cpp dialogs/qmessagebox.h
    NO_UNITY_BUILD_SOURCES
        dialogs/qmessagebox.cpp # error: ‘Old_Ok’ [etc] was not declared in this scope
)

if(QT_FEATURE_messagebox)
    # Resources:
    set(qmessagebox_resource_files
        "dialogs/images/qtlogo-64.png"
    )

    qt_internal_add_resource(Widgets "qmessagebox"
        PREFIX
            "/qt-project.org/qmessagebox"
        BASE
            "dialogs"
        FILES
            ${qmessagebox_resource_files}
    )
endif()

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_progressdialog
    SOURCES
        dialogs/qprogressdialog.cpp dialogs/qprogressdialog.h
)

if(QT_FEATURE_wizard)
    qt_internal_extend_target(Widgets CONDITION
        SOURCES
            dialogs/qwizard.cpp dialogs/qwizard.h
    )

    if(APPLE)
        set_source_files_properties(dialogs/qwizard.cpp
            PROPERTIES
                LANGUAGE OBJCXX
                SKIP_PRECOMPILE_HEADERS ON
        )
    endif()

    qt_internal_extend_target(Widgets CONDITION WIN32
        SOURCES
            dialogs/qwizard_win.cpp dialogs/qwizard_win_p.h
    )
endif()

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_accessibility
    SOURCES
        accessible/complexwidgets.cpp accessible/complexwidgets_p.h
        accessible/qaccessiblemenu.cpp accessible/qaccessiblemenu_p.h
        accessible/qaccessiblewidget.cpp accessible/qaccessiblewidget.h
        accessible/qaccessiblewidgetfactory.cpp accessible/qaccessiblewidgetfactory_p.h
        accessible/qaccessiblewidgets.cpp accessible/qaccessiblewidgets_p.h
        accessible/rangecontrols.cpp accessible/rangecontrols_p.h
        accessible/simplewidgets.cpp accessible/simplewidgets_p.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_accessibility AND QT_FEATURE_itemviews
    SOURCES
        accessible/itemviews.cpp accessible/itemviews_p.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_itemviews
    SOURCES
        itemviews/qabstractitemdelegate.cpp itemviews/qabstractitemdelegate.h itemviews/qabstractitemdelegate_p.h
        itemviews/qabstractitemview.cpp itemviews/qabstractitemview.h itemviews/qabstractitemview_p.h
        itemviews/qbsptree.cpp itemviews/qbsptree_p.h
        itemviews/qheaderview.cpp itemviews/qheaderview.h itemviews/qheaderview_p.h
        itemviews/qitemdelegate.cpp itemviews/qitemdelegate.h
        itemviews/qitemeditorfactory.cpp itemviews/qitemeditorfactory.h itemviews/qitemeditorfactory_p.h
        itemviews/qstyleditemdelegate.cpp itemviews/qstyleditemdelegate.h
        itemviews/qwidgetitemdata_p.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_columnview
    SOURCES
        itemviews/qcolumnview.cpp itemviews/qcolumnview.h itemviews/qcolumnview_p.h
        itemviews/qcolumnviewgrip.cpp itemviews/qcolumnviewgrip_p.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_datawidgetmapper
    SOURCES
        itemviews/qdatawidgetmapper.cpp itemviews/qdatawidgetmapper.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_listview
    SOURCES
        itemviews/qlistview.cpp itemviews/qlistview.h itemviews/qlistview_p.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_listwidget
    SOURCES
        itemviews/qlistwidget.cpp itemviews/qlistwidget.h itemviews/qlistwidget_p.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_tableview
    SOURCES
        itemviews/qtableview.cpp itemviews/qtableview.h itemviews/qtableview_p.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_tablewidget
    SOURCES
        itemviews/qtablewidget.cpp itemviews/qtablewidget.h itemviews/qtablewidget_p.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_treeview
    SOURCES
        itemviews/qtreeview.cpp itemviews/qtreeview.h itemviews/qtreeview_p.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_treewidget
    SOURCES
        itemviews/qtreewidget.cpp itemviews/qtreewidget.h itemviews/qtreewidget_p.h
        itemviews/qtreewidgetitemiterator.cpp itemviews/qtreewidgetitemiterator.h
        itemviews/qtreewidgetitemiterator_p.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_graphicsview
    SOURCES
        graphicsview/qgraph_p.h
        graphicsview/qgraphicsanchorlayout.cpp graphicsview/qgraphicsanchorlayout.h graphicsview/qgraphicsanchorlayout_p.cpp graphicsview/qgraphicsanchorlayout_p.h
        graphicsview/qgraphicsgridlayout.cpp graphicsview/qgraphicsgridlayout.h
        graphicsview/qgraphicsgridlayoutengine.cpp graphicsview/qgraphicsgridlayoutengine_p.h
        graphicsview/qgraphicsitem.cpp graphicsview/qgraphicsitem.h graphicsview/qgraphicsitem_p.h
        graphicsview/qgraphicslayout.cpp graphicsview/qgraphicslayout.h graphicsview/qgraphicslayout_p.cpp graphicsview/qgraphicslayout_p.h
        graphicsview/qgraphicslayoutitem.cpp graphicsview/qgraphicslayoutitem.h graphicsview/qgraphicslayoutitem_p.h
        graphicsview/qgraphicslayoutstyleinfo.cpp graphicsview/qgraphicslayoutstyleinfo_p.h
        graphicsview/qgraphicslinearlayout.cpp graphicsview/qgraphicslinearlayout.h
        graphicsview/qgraphicsproxywidget.cpp graphicsview/qgraphicsproxywidget.h graphicsview/qgraphicsproxywidget_p.h
        graphicsview/qgraphicsscene.cpp graphicsview/qgraphicsscene.h graphicsview/qgraphicsscene_p.h
        graphicsview/qgraphicsscene_bsp.cpp graphicsview/qgraphicsscene_bsp_p.h
        graphicsview/qgraphicsscenebsptreeindex.cpp graphicsview/qgraphicsscenebsptreeindex_p.h
        graphicsview/qgraphicssceneevent.cpp graphicsview/qgraphicssceneevent.h
        graphicsview/qgraphicssceneindex.cpp graphicsview/qgraphicssceneindex_p.h
        graphicsview/qgraphicsscenelinearindex.cpp graphicsview/qgraphicsscenelinearindex_p.h
        graphicsview/qgraphicstransform.cpp graphicsview/qgraphicstransform.h graphicsview/qgraphicstransform_p.h
        graphicsview/qgraphicsview.cpp graphicsview/qgraphicsview.h graphicsview/qgraphicsview_p.h
        graphicsview/qgraphicswidget.cpp graphicsview/qgraphicswidget.h graphicsview/qgraphicswidget_p.cpp graphicsview/qgraphicswidget_p.h
        graphicsview/qsimplex_p.cpp graphicsview/qsimplex_p.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_easingcurve AND QT_FEATURE_graphicsview
    SOURCES
        graphicsview/qgraphicsitemanimation.cpp graphicsview/qgraphicsitemanimation.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_completer
    SOURCES
        util/qcompleter.cpp util/qcompleter.h util/qcompleter_p.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_scroller
    SOURCES
        util/qflickgesture.cpp util/qflickgesture_p.h
        util/qscroller.cpp util/qscroller.h util/qscroller_p.h
        util/qscrollerproperties.cpp util/qscrollerproperties.h util/qscrollerproperties_p.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_undocommand
    SOURCES
        util/qundostack.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_undogroup
    SOURCES
        util/qundogroup.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_undoview
    SOURCES
        util/qundoview.cpp util/qundoview.h
)

qt_internal_extend_target(Widgets CONDITION QT_FEATURE_xcb
    SOURCES
        util/qsystemtrayicon_x11.cpp
)

qt_internal_extend_target(Widgets CONDITION NOT QT_FEATURE_xcb
    SOURCES
        util/qsystemtrayicon_qpa.cpp
)


qt_internal_generate_tracepoints(Widgets widgets
    SOURCES
        kernel/qapplication.cpp
)

qt_internal_add_docs(Widgets
    doc/qtwidgets.qdocconf
)

if(IOS)
    qt_internal_set_apple_privacy_manifest(Widgets
        "${CMAKE_CURRENT_SOURCE_DIR}/platform/ios/PrivacyInfo.xcprivacy")
endif()

if(QT_FEATURE_doc_snippets)
    add_subdirectory(doc/snippets/customviewstyle)
    if (QT_FEATURE_filedialog)
        add_subdirectory(doc/snippets/filedialogurls)
    endif()
    if (QT_FEATURE_graphicsview)
        add_subdirectory(doc/snippets/graphicssceneadditem)
    endif()
    if(QT_FEATURE_opengl AND QT_FEATURE_printdialog)
        add_subdirectory(doc/snippets/graphicsview)
    endif()
    if (QT_FEATURE_mdiarea)
        add_subdirectory(doc/snippets/mdiarea)
    endif()
    if (QT_FEATURE_scrollarea)
        add_subdirectory(doc/snippets/myscrollarea)
    endif()
endif()
