/****************************************************************************
**
** Copyright (C) 2014-2015 Canonical, Ltd.
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


#ifndef QMIRCLIENTWINDOW_H
#define QMIRCLIENTWINDOW_H

#include <qpa/qplatformwindow.h>
#include <QSharedPointer>
#include <QMutex>

#include <memory>

class QMirClientClipboard;
class QMirClientInput;
class QMirClientScreen;
class QMirClientSurface;
struct MirConnection;
struct MirSurface;

class QMirClientWindow : public QObject, public QPlatformWindow
{
    Q_OBJECT
public:
    QMirClientWindow(QWindow *w, const QSharedPointer<QMirClientClipboard> &clipboard, QMirClientScreen *screen,
                 QMirClientInput *input, MirConnection *mirConnection);
    virtual ~QMirClientWindow();

    // QPlatformWindow methods.
    WId winId() const override;
    void setGeometry(const QRect&) override;
    void setWindowState(Qt::WindowState state) override;
    void setVisible(bool visible) override;
    void setWindowTitle(const QString &title) override;
    void propagateSizeHints() override;

    // New methods.
    void *eglSurface() const;
    MirSurface *mirSurface() const;
    void handleSurfaceResized(int width, int height);
    void handleSurfaceFocused();
    void onSwapBuffersDone();

private:
    void updatePanelHeightHack(Qt::WindowState);
    mutable QMutex mMutex;
    const WId mId;
    const QSharedPointer<QMirClientClipboard> mClipboard;
    std::unique_ptr<QMirClientSurface> mSurface;
};

#endif // QMIRCLIENTWINDOW_H
