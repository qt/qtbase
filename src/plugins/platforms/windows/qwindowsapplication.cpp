/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qwindowsapplication.h"
#include "qwindowsclipboard.h"
#include "qwindowscontext.h"
#include "qwin10helpers.h"


QT_BEGIN_NAMESPACE

void QWindowsApplication::setTouchWindowTouchType(QWindowsApplication::TouchWindowTouchTypes type)
{
    if (m_touchWindowTouchTypes == type)
        return;
    m_touchWindowTouchTypes = type;
    if (auto ctx = QWindowsContext::instance())
        ctx->registerTouchWindows();
}

QWindowsApplication::TouchWindowTouchTypes QWindowsApplication::touchWindowTouchType() const
{
    return m_touchWindowTouchTypes;
}

QWindowsApplication::WindowActivationBehavior QWindowsApplication::windowActivationBehavior() const
{
   return m_windowActivationBehavior;
}

void QWindowsApplication::setWindowActivationBehavior(WindowActivationBehavior behavior)
{
    m_windowActivationBehavior = behavior;
}

bool QWindowsApplication::isTabletMode() const
{
#if QT_CONFIG(clipboard)
    if (const QWindowsClipboard *clipboard = QWindowsClipboard::instance())
        return qt_windowsIsTabletMode(clipboard->clipboardViewer());
#endif
    return false;
}

bool QWindowsApplication::isWinTabEnabled() const
{
    auto ctx = QWindowsContext::instance();
    return ctx != nullptr && ctx->tabletSupport() != nullptr;
}

bool QWindowsApplication::setWinTabEnabled(bool enabled)
{
    if (enabled == isWinTabEnabled())
        return true;
    auto ctx = QWindowsContext::instance();
    if (!ctx)
        return false;
    return enabled ? ctx->initTablet() : ctx->disposeTablet();
}

QT_END_NAMESPACE
