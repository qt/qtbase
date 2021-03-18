/****************************************************************************
**
** Copyright (C) 2019 Samuel Gaist <samuel.gaist@idiap.ch>
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

#ifndef QCOCOASESSIONMANAGER_H
#define QCOCOASESSIONMANAGER_H

//
//  W A R N I N G
//  -------------
//
// This file is part of the QPA API and is not meant to be used
// in applications. Usage of this API may make your code
// source and binary incompatible with future versions of Qt.
//

#ifndef QT_NO_SESSIONMANAGER

#include <qpa/qplatformsessionmanager.h>

QT_BEGIN_NAMESPACE

class QCocoaSessionManager : public QPlatformSessionManager
{
public:
    QCocoaSessionManager(const QString &id, const QString &key);
    virtual ~QCocoaSessionManager();

    bool allowsInteraction() override;
    void cancel() override;
    void resetCancellation();
    bool wasCanceled() const;

    static QCocoaSessionManager *instance();

private:
    bool m_canceled;

    Q_DISABLE_COPY(QCocoaSessionManager)
};

QT_END_NAMESPACE

#endif // QT_NO_SESSIONMANAGER

#endif // QCOCOASESSIONMANAGER_H
