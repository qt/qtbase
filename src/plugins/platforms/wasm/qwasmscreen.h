/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QWASMSCREEN_H
#define QWASMSCREEN_H

#include "qwasmcursor.h"

#include <qpa/qplatformscreen.h>

#include <QtCore/qscopedpointer.h>
#include <QtCore/qtextstream.h>

QT_BEGIN_NAMESPACE

class QPlatformOpenGLContext;
class QWasmWindow;
class QWasmBackingStore;
class QWasmCompositor;
class QOpenGLContext;

class QWasmScreen : public QObject, public QPlatformScreen
{
    Q_OBJECT
public:

    QWasmScreen(QWasmCompositor *compositor);
    ~QWasmScreen();

    QRect geometry() const override;
    int depth() const override;
    QImage::Format format() const override;
    qreal devicePixelRatio() const override;
    QPlatformCursor *cursor() const override;

    void resizeMaximizedWindows();
    QWindow *topWindow() const;
    QWindow *topLevelAt(const QPoint &p) const override;

    void invalidateSize();

public slots:
    void setGeometry(const QRect &rect);
protected:

private:
    QWasmCompositor *m_compositor;

    QRect m_geometry = QRect(0, 0, 100, 100);
    int m_depth;
    QImage::Format m_format;
    QWasmCursor m_cursor;
};

QT_END_NAMESPACE
#endif // QWASMSCREEN_H
