/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
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
