// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qwindowswindowclassdescription.h"

#include <QtGui/qwindow.h>
#include <QtCore/qvariant.h>

#include "qwindowswindowclassregistry.h"

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

QString QWindowsWindowClassDescription::classNameSuffix(Qt::WindowFlags type, unsigned int style, bool hasIcon)
{
    QString suffix;

    switch (type) {
        case Qt::Popup:
            suffix += "Popup"_L1;
            break;
        case Qt::Tool:
            suffix += "Tool"_L1;
            break;
        case Qt::ToolTip:
            suffix += "ToolTip"_L1;
            break;
        default:
            break;
    }

    if (style & CS_DROPSHADOW)
        suffix += "DropShadow"_L1;
    if (style & CS_SAVEBITS)
        suffix += "SaveBits"_L1;
    if (style & CS_OWNDC)
        suffix += "OwnDC"_L1;
    if (hasIcon)
        suffix += "Icon"_L1;

    return suffix;
}

bool QWindowsWindowClassDescription::computeHasIcon(Qt::WindowFlags flags, Qt::WindowFlags type)
{
    bool hasIcon = true;

    switch (type) {
        case Qt::Tool:
        case Qt::ToolTip:
        case Qt::Popup:
            hasIcon = false;
            break;
        case Qt::Dialog:
            if (!(flags & Qt::WindowSystemMenuHint))
                hasIcon = false; // QTBUG-2027, dialogs without system menu.
            break;
    }

    return hasIcon;
}

unsigned int QWindowsWindowClassDescription::computeWindowStyles(Qt::WindowFlags flags, Qt::WindowFlags type, WindowStyleOptions options)
{
    unsigned int style = CS_DBLCLKS;

    // The following will not set CS_OWNDC for any widget window, even if it contains a
    // QOpenGLWidget or QQuickWidget later on. That cannot be detected at this stage.
    if (options.testFlag(WindowStyleOption::GLSurface) || (flags & Qt::MSWindowsOwnDC))
        style |= CS_OWNDC;
    if (!(flags & Qt::NoDropShadowWindowHint) && (type == Qt::Popup || options.testFlag(WindowStyleOption::DropShadow)))
        style |= CS_DROPSHADOW;

    switch (type) {
        case Qt::Tool:
        case Qt::ToolTip:
        case Qt::Popup:
            style |= CS_SAVEBITS; // Save/restore background
            break;
    }

    return style;
}

QWindowsWindowClassDescription QWindowsWindowClassDescription::fromName(QString name, WNDPROC procedure)
{
    return { std::move(name), procedure };
}

QWindowsWindowClassDescription QWindowsWindowClassDescription::fromWindow(const QWindow *window, WNDPROC procedure)
{
    Q_ASSERT(window);

    const Qt::WindowFlags flags = window->flags();
    const Qt::WindowFlags type = flags & Qt::WindowType_Mask;

    WindowStyleOptions options = WindowStyleOption::None;
    if (window->surfaceType() == QSurface::OpenGLSurface)
        options |= WindowStyleOption::GLSurface;
    if (window->property("_q_windowsDropShadow").toBool())
        options |= WindowStyleOption::DropShadow;

    QWindowsWindowClassDescription description;
    description.procedure = procedure;
    description.style = computeWindowStyles(flags, type, options);
    description.hasIcon = computeHasIcon(flags, type);
    description.name = "QWindow"_L1 + classNameSuffix(type, description.style, description.hasIcon);

    return description;
}

QDebug operator<<(QDebug dbg, const QWindowsWindowClassDescription &description)
{
    dbg << description.name
        << " style=0x" << Qt::hex << description.style << Qt::dec
        << " brush=" << description.brush
        << " hasIcon=" << description.hasIcon;

    return dbg;
}

QT_END_NAMESPACE
