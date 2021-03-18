/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#ifndef QOFFSCREENWINDOW_H
#define QOFFSCREENWINDOW_H

#include <qpa/qplatformbackingstore.h>
#include <qpa/qplatformwindow.h>

#include <qhash.h>

QT_BEGIN_NAMESPACE

class QOffscreenWindow : public QPlatformWindow
{
public:
    QOffscreenWindow(QWindow *window);
    ~QOffscreenWindow();

    void setGeometry(const QRect &rect) override;
    void setWindowState(Qt::WindowStates states) override;

    QMargins frameMargins() const override;

    void setVisible(bool visible) override;
    void requestActivateWindow() override;

    WId winId() const override;

    static QOffscreenWindow *windowForWinId(WId id);

private:
    void setFrameMarginsEnabled(bool enabled);
    void setGeometryImpl(const QRect &rect);

    QRect m_normalGeometry;
    QMargins m_margins;
    bool m_positionIncludesFrame;
    bool m_visible;
    bool m_pendingGeometryChangeOnShow;
    WId m_winId;

    static QHash<WId, QOffscreenWindow *> m_windowForWinIdHash;
};

QT_END_NAMESPACE

#endif
