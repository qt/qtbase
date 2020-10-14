# Qt kernel module

# Only used on platforms with CONFIG += precompile_header
PRECOMPILED_HEADER = kernel/qt_widgets_pch.h

KERNEL_P= kernel
HEADERS += \
        kernel/qtwidgetsglobal.h \
        kernel/qtwidgetsglobal_p.h \
	kernel/qapplication.h \
	kernel/qapplication_p.h \
        kernel/qwidgetrepaintmanager_p.h \
	kernel/qboxlayout.h \
	kernel/qgridlayout.h \
        kernel/qlayout.h \
	kernel/qlayout_p.h \
	kernel/qlayoutengine_p.h \
	kernel/qlayoutitem.h \
        kernel/qsizepolicy.h \
        kernel/qstackedlayout.h \
        kernel/qwidget.h \
        kernel/qwidget_p.h \
	kernel/qgesture.h \
	kernel/qgesture_p.h \
	kernel/qstandardgestures_p.h \
	kernel/qgesturerecognizer.h \
	kernel/qgesturemanager_p.h \
        kernel/qwidgetwindow_p.h \
        kernel/qwindowcontainer_p.h \
        kernel/qtestsupport_widgets.h

SOURCES += \
	kernel/qapplication.cpp \
        kernel/qwidgetrepaintmanager.cpp \
        kernel/qboxlayout.cpp \
	kernel/qgridlayout.cpp \
        kernel/qlayout.cpp \
	kernel/qlayoutengine.cpp \
	kernel/qlayoutitem.cpp \
        kernel/qsizepolicy.cpp \
        kernel/qstackedlayout.cpp \
	kernel/qwidget.cpp \
	kernel/qgesture.cpp \
	kernel/qstandardgestures.cpp \
	kernel/qgesturerecognizer.cpp \
	kernel/qgesturemanager.cpp \
        kernel/qwidgetsvariant.cpp \
        kernel/qwidgetwindow.cpp \
        kernel/qwindowcontainer.cpp \
        kernel/qtestsupport_widgets.cpp \
        kernel/qwidgetstatemachine.cpp

macx: {
    HEADERS += kernel/qmacgesturerecognizer_p.h
    SOURCES += kernel/qmacgesturerecognizer.cpp
}

qtConfig(action) {
    HEADERS += kernel/qaction_widgets_p.h \
               kernel/qwidgetaction.h \
               kernel/qwidgetaction_p.h
    SOURCES += kernel/qaction_widgets.cpp \
               kernel/qwidgetaction.cpp
}

qtConfig(formlayout) {
    HEADERS += kernel/qformlayout.h
    SOURCES += kernel/qformlayout.cpp
}

qtConfig(shortcut) {
    SOURCES += kernel/qshortcut_widgets.cpp
}

qtConfig(tooltip) {
    HEADERS += kernel/qtooltip.h
    SOURCES += kernel/qtooltip.cpp
}

qtConfig(whatsthis) {
    HEADERS += kernel/qwhatsthis.h
    SOURCES += kernel/qwhatsthis.cpp
}
