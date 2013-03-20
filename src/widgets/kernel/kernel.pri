# Qt kernel module

# Only used on platforms with CONFIG += precompile_header
PRECOMPILED_HEADER = kernel/qt_widgets_pch.h

KERNEL_P= kernel
HEADERS += \
	kernel/qaction.h \
    kernel/qaction_p.h \
	kernel/qactiongroup.h \
	kernel/qapplication.h \
	kernel/qapplication_p.h \
        kernel/qwidgetbackingstore_p.h \
	kernel/qboxlayout.h \
	kernel/qdesktopwidget.h \
	kernel/qformlayout.h \
	kernel/qgridlayout.h \
        kernel/qlayout.h \
	kernel/qlayout_p.h \
	kernel/qlayoutengine_p.h \
	kernel/qlayoutitem.h \
        kernel/qshortcut.h \
	kernel/qsizepolicy.h \
        kernel/qstackedlayout.h \
	kernel/qtooltip.h \
	kernel/qwhatsthis.h \
        kernel/qwidget.h \
        kernel/qwidget_p.h \
	kernel/qwidgetaction.h \
	kernel/qwidgetaction_p.h \
	kernel/qgesture.h \
	kernel/qgesture_p.h \
	kernel/qstandardgestures_p.h \
	kernel/qgesturerecognizer.h \
	kernel/qgesturemanager_p.h \
        kernel/qdesktopwidget_qpa_p.h \
        kernel/qwidgetwindow_qpa_p.h \
        kernel/qwindowcontainer_p.h

SOURCES += \
	kernel/qaction.cpp \
	kernel/qactiongroup.cpp \
	kernel/qapplication.cpp \
        kernel/qwidgetbackingstore.cpp \
        kernel/qboxlayout.cpp \
	kernel/qformlayout.cpp \
	kernel/qgridlayout.cpp \
        kernel/qlayout.cpp \
	kernel/qlayoutengine.cpp \
	kernel/qlayoutitem.cpp \
        kernel/qshortcut.cpp \
        kernel/qstackedlayout.cpp \
	kernel/qtooltip.cpp \
	kernel/qwhatsthis.cpp \
	kernel/qwidget.cpp \
	kernel/qwidgetaction.cpp \
	kernel/qgesture.cpp \
	kernel/qstandardgestures.cpp \
	kernel/qgesturerecognizer.cpp \
	kernel/qgesturemanager.cpp \
        kernel/qdesktopwidget.cpp \
        kernel/qwidgetsvariant.cpp \
        kernel/qapplication_qpa.cpp \
        kernel/qdesktopwidget_qpa.cpp \
        kernel/qwidget_qpa.cpp \
        kernel/qwidgetwindow.cpp \
        kernel/qwindowcontainer.cpp


# TODO
false:!x11:mac {
	SOURCES += \
		kernel/qclipboard_mac.cpp \
		kernel/qmime_mac.cpp \
		kernel/qt_mac.cpp \
                kernel/qkeymapper_mac.cpp

        OBJECTIVE_HEADERS += \
                qcocoawindow_mac_p.h \
                qcocoapanel_mac_p.h \
                qcocoawindowdelegate_mac_p.h \
                qcocoaview_mac_p.h \
                qcocoaapplication_mac_p.h \
                qcocoaapplicationdelegate_mac_p.h \
                qmacgesturerecognizer_mac_p.h \
                qmultitouch_mac_p.h \
                qcocoasharedwindowmethods_mac_p.h \
                qcocoaintrospection_p.h

        OBJECTIVE_SOURCES += \
                kernel/qcursor_mac.mm \
                kernel/qdnd_mac.mm \
                kernel/qapplication_mac.mm \
		        kernel/qwidget_mac.mm \
		        kernel/qcocoapanel_mac.mm \
                kernel/qcocoaview_mac.mm \
                kernel/qcocoawindow_mac.mm \
                kernel/qcocoawindowdelegate_mac.mm \
                kernel/qcocoaapplication_mac.mm \
                kernel/qcocoaapplicationdelegate_mac.mm \
                kernel/qt_cocoa_helpers_mac.mm \
                kernel/qdesktopwidget_mac.mm \
                kernel/qeventdispatcher_mac.mm \
                kernel/qcocoawindowcustomthemeframe_mac.mm \
                kernel/qmacgesturerecognizer_mac.mm \
                kernel/qmultitouch_mac.mm \
                kernel/qcocoaintrospection_mac.mm

        HEADERS += \
                kernel/qt_cocoa_helpers_mac_p.h \
                kernel/qcocoaapplication_mac_p.h \
                kernel/qcocoaapplicationdelegate_mac_p.h \
                kernel/qeventdispatcher_mac_p.h

        MENU_NIB.files = mac/qt_menu.nib
        MENU_NIB.path = Resources
        MENU_NIB.version = Versions
        QMAKE_BUNDLE_DATA += MENU_NIB
        RESOURCES += mac/macresources.qrc

        LIBS_PRIVATE += -framework AppKit
}

wince*: {
        HEADERS += \
                ../corelib/kernel/qfunctions_wince.h \
                kernel/qwidgetsfunctions_wince.h

        SOURCES += \
                kernel/qwidgetsfunctions_wince.cpp
}
