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

#ifndef QWINDOWSDIRECT2DINTEGRATION_H
#define QWINDOWSDIRECT2DINTEGRATION_H

#include "qwindowsintegration.h"

#include <QtCore/qscopedpointer.h>

QT_BEGIN_NAMESPACE

class QWindowsDirect2DContext;
class QWindowsDirect2DIntegrationPrivate;

class QWindowsDirect2DIntegration : public QWindowsIntegration
{
public:
    static QWindowsDirect2DIntegration *create(const QStringList &paramList);

    virtual ~QWindowsDirect2DIntegration();

    static QWindowsDirect2DIntegration *instance();

    QPlatformNativeInterface *nativeInterface() const override;
    QPlatformPixmap *createPlatformPixmap(QPlatformPixmap::PixelType type) const override;
    QPlatformBackingStore *createPlatformBackingStore(QWindow *window) const override;

    QWindowsDirect2DContext *direct2DContext() const;

protected:
    QWindowsWindow *createPlatformWindowHelper(QWindow *window, const QWindowsWindowData &) const override;

private:
    explicit QWindowsDirect2DIntegration(const QStringList &paramList);
    bool init();

    QScopedPointer<QWindowsDirect2DIntegrationPrivate> d;
};

QT_END_NAMESPACE

#endif // QWINDOWSDIRECT2DINTEGRATION_H
