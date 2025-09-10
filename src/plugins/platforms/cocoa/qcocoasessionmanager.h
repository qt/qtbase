// Copyright (C) 2019 Samuel Gaist <samuel.gaist@idiap.ch>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
