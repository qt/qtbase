/****************************************************************************
**
** Copyright (C) 2015 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2016 Pelagicore AG
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

#ifndef QEGLFSKMSGBMDEVICE_H
#define QEGLFSKMSGBMDEVICE_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qeglfskmsgbmcursor_p.h"
#include <private/qeglfskmsdevice_p.h>
#include <private/qeglfskmseventreader_p.h>

#include <gbm.h>

QT_BEGIN_NAMESPACE

class QEglFSKmsScreen;

class Q_EGLFS_EXPORT QEglFSKmsGbmDevice: public QEglFSKmsDevice
{
public:
    QEglFSKmsGbmDevice(QKmsScreenConfig *screenConfig, const QString &path);

    bool open() override;
    void close() override;

    void *nativeDisplay() const override;
    gbm_device *gbmDevice() const;

    QPlatformCursor *globalCursor() const;
    void destroyGlobalCursor();
    void createGlobalCursor(QEglFSKmsGbmScreen *screen);

    QPlatformScreen *createScreen(const QKmsOutput &output) override;
    QPlatformScreen *createHeadlessScreen() override;
    void registerScreenCloning(QPlatformScreen *screen,
                               QPlatformScreen *screenThisScreenClones,
                               const QList<QPlatformScreen *> &screensCloningThisScreen) override;
    void registerScreen(QPlatformScreen *screen,
                        bool isPrimary,
                        const QPoint &virtualPos,
                        const QList<QPlatformScreen *> &virtualSiblings) override;

    bool usesEventReader() const;
    QEglFSKmsEventReader *eventReader() { return &m_eventReader; }

private:
    Q_DISABLE_COPY(QEglFSKmsGbmDevice)

    gbm_device *m_gbm_device;
    QEglFSKmsEventReader m_eventReader;
    QEglFSKmsGbmCursor *m_globalCursor;
};

QT_END_NAMESPACE

#endif // QEGLFSKMSGBMDEVICE_H
