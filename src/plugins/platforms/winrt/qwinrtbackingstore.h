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

#ifndef QWINRTBACKINGSTORE_H
#define QWINRTBACKINGSTORE_H

#define GL_GLEXT_PROTOTYPES
#include <qpa/qplatformbackingstore.h>
#include <QtCore/QScopedPointer>
#include <QtCore/QLoggingCategory>

QT_BEGIN_NAMESPACE

class QWinRTScreen;

class QWinRTBackingStorePrivate;
class QWinRTBackingStore : public QPlatformBackingStore
{
public:
    explicit QWinRTBackingStore(QWindow *window);
    ~QWinRTBackingStore() override;
    QPaintDevice *paintDevice() override;
    void beginPaint(const QRegion &) override;
    void endPaint() override;
    void flush(QWindow *window, const QRegion &region, const QPoint &offset) override;
    void resize(const QSize &size, const QRegion &staticContents) override;
    QImage toImage() const override;

private:
    bool initialize();

    QScopedPointer<QWinRTBackingStorePrivate> d_ptr;
    Q_DECLARE_PRIVATE(QWinRTBackingStore)
};

QT_END_NAMESPACE

#endif // QWINRTBACKINGSTORE_H
