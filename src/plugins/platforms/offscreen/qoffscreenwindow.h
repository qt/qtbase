// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOFFSCREENWINDOW_H
#define QOFFSCREENWINDOW_H

#include <qpa/qplatformbackingstore.h>
#include <qpa/qplatformwindow.h>

#include <qhash.h>

QT_BEGIN_NAMESPACE

class QOffscreenWindow : public QPlatformWindow
{
public:
    QOffscreenWindow(QWindow *window, bool frameMarginsEnabled);
    ~QOffscreenWindow();

    void setGeometry(const QRect &rect) override;
    void setWindowState(Qt::WindowStates states) override;

    QMargins frameMargins() const override;

    void setVisible(bool visible) override;
    void requestActivateWindow() override;

    WId winId() const override;
    QSurfaceFormat format() const override;

    static QOffscreenWindow *windowForWinId(WId id);

private:
    void setFrameMarginsEnabled(bool enabled);
    void setGeometryImpl(const QRect &rect);

    QRect m_normalGeometry;
    QMargins m_margins;
    bool m_positionIncludesFrame;
    bool m_visible;
    bool m_pendingGeometryChangeOnShow;
    bool m_frameMarginsRequested;
    WId m_winId;

    Q_CONSTINIT static QHash<WId, QOffscreenWindow *> m_windowForWinIdHash;
};

QT_END_NAMESPACE

#endif
